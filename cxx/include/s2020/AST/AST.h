#ifndef SCHEME2020_AST_H
#define SCHEME2020_AST_H

#include "s2020/AST/ASTContext.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/SMLoc.h"

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace s2020 {
namespace ast {

using llvm::SMLoc;
using llvm::SMRange;

enum class NodeKind : uint8_t {
#define S2020_AST_NODE(name) name,
#include "s2020/AST/NodeKinds.def"
  _end,
};

llvm::StringRef nodeKindStr(NodeKind kind);

/// This corresponds to a "datum" returned by the "read" procedure, but we call
/// it an \c ast::Node to avoid confusion between compile time and runtime. Also
/// it is decorated with location information.
class Node {
 protected:
  explicit Node(NodeKind kind) : kind_(kind) {}

 public:
  NodeKind getKind() const {
    return kind_;
  }

  llvm::StringRef getNodeName() const {
    return nodeKindStr(getKind());
  }

  void setSourceRange(SMRange rng) {
    sourceRange_ = rng;
  }
  SMRange getSourceRange() const {
    return sourceRange_;
  }
  void setStartLoc(SMLoc loc) {
    sourceRange_.Start = loc;
  }
  SMLoc getStartLoc() const {
    return sourceRange_.Start;
  }
  void setEndLoc(SMLoc loc) {
    sourceRange_.End = loc;
  }
  SMLoc getEndLoc() const {
    return sourceRange_.End;
  }

  /// Copy all location data from a different node.
  void copyLocationFrom(const Node *src) {
    setSourceRange(src->getSourceRange());
  }

  // Allow allocation of AST nodes by using the Context allocator or by a
  // placement new.

  void *operator new(
      size_t size,
      ASTContext &ctx,
      size_t alignment = alignof(double)) {
    return ctx.allocateNode(size, alignment);
  }
  void *operator new(size_t, void *mem) {
    return mem;
  }

  void operator delete(void *, ASTContext &, size_t) {}
  void operator delete(void *, size_t) {}

 private:
  // Make new/delete illegal for Datum nodes.

  void *operator new(size_t) {
    llvm_unreachable("AST nodes cannot be allocated with regular new");
  }
  void operator delete(void *) {
    llvm_unreachable("AST nodes cannot be released with regular delete");
  }

 private:
  NodeKind kind_;
  SMRange sourceRange_;
};

template <NodeKind KIND>
class BaseNode : public Node {
 public:
  explicit BaseNode() : Node(KIND) {}

  static bool classof(const Node *v) {
    return v->getKind() == KIND;
  }
};

template <NodeKind KIND, typename V>
class SimpleNode : public BaseNode<KIND> {
 public:
  explicit SimpleNode(const V &value) : value_(value) {}

  const V &getValue() const {
    return value_;
  }

 private:
  V value_;
};

using BooleanNode = SimpleNode<NodeKind::Boolean, bool>;
using CharacterNode = SimpleNode<NodeKind::Character, char32_t>;
using StringNode = SimpleNode<NodeKind::String, Identifier>;
using SymbolNode = SimpleNode<NodeKind::Symbol, Identifier>;
using NumberNode = SimpleNode<NodeKind::Number, Number>;
using NullNode = BaseNode<NodeKind::Null>;

// FIXME: implement these.
using BytevectorNode = BaseNode<NodeKind::Bytevector>;
using VectorNode = BaseNode<NodeKind::Vector>;

class PairNode : public BaseNode<NodeKind::Pair> {
 public:
  explicit PairNode(Node *car, Node *cdr) : car_(car), cdr_(cdr) {}

  Node *getCar() const {
    return car_;
  }
  void setCar(Node *car) {
    car_ = car;
  }
  Node *getCdr() const {
    return cdr_;
  }
  void setCdr(Node *cdr) {
    cdr_ = cdr;
  }

 private:
  Node *car_;
  Node *cdr_;
};

/// Allocate a new PairNode.
PairNode *cons(ASTContext &ctx, Node *a, Node *b);

// Allocate a new proper list.
inline Node *list(ASTContext &ctx) {
  return new (ctx) NullNode();
}
template <typename... Args>
inline Node *list(ASTContext &ctx, Node *first, Args... args) {
  return cons(ctx, first, list(ctx, args...));
}

/// A convenience class for building a list by appending to the end.
class ListBuilder {
 public:
  ListBuilder(const ListBuilder &) = delete;
  void operator=(const ListBuilder &) = delete;

  explicit ListBuilder() = default;

  void append(ASTContext &ctx, Node *n) {
    assert(n && "Node cannot be nullptr");
    appendNewTail(new (ctx) PairNode(n, nullptr));
  }

  /// Finish the construction of the list and return it.
  Node *finishList(ASTContext &ctx);

 private:
  void appendNewTail(Node *newTail) {
    if (tail_)
      llvm::cast<PairNode>(tail_)->setCdr(newTail);
    else
      head_ = newTail;
    tail_ = newTail;
  }

 private:
  Node *head_{};
  Node *tail_{};
};

/// \return true if this is a PairNode or NullNode.
inline bool isList(Node *list) {
  return llvm::isa<PairNode>(list) || llvm::isa<NullNode>(list);
}

inline bool isListEmpty(Node *list) {
  assert(isList(list) && "argument must be a list");
  return llvm::isa<NullNode>(list);
}

/// List iterator. Note that the last element in improper lists will be ignored.
class ListIterator {
 public:
  explicit ListIterator(Node *node) : pair_(llvm::dyn_cast<PairNode>(node)) {
    assert(isList(node) && "node must be a list");
  }
  explicit ListIterator() : pair_(nullptr) {}

  bool operator==(const ListIterator &it) const {
    return pair_ == it.pair_;
  }
  bool operator!=(const ListIterator &it) const {
    return pair_ != it.pair_;
  }

  ListIterator &operator++() {
    assert(pair_ && "attempting to increment ListIterator past end");
    pair_ = llvm::dyn_cast<PairNode>(pair_->getCdr());
    return *this;
  }

  Node *operator*() const {
    assert(pair_ && "attempting to dereference end ListIterator");
    return pair_->getCar();
  }
  Node *operator->() const {
    return operator*();
  }

 private:
  PairNode *pair_;
};

/// Similar to \c ListIterator but iterate over the pairs.
class ListPairIterator {
 public:
  explicit ListPairIterator(Node *node)
      : pair_(llvm::dyn_cast<PairNode>(node)) {
    assert(isList(node) && "node must be a list");
  }
  explicit ListPairIterator() : pair_(nullptr) {}

  bool operator==(const ListPairIterator &it) const {
    return pair_ == it.pair_;
  }
  bool operator!=(const ListPairIterator &it) const {
    return pair_ != it.pair_;
  }

  ListPairIterator &operator++() {
    assert(pair_ && "attempting to increment ListIterator past end");
    pair_ = llvm::dyn_cast<PairNode>(pair_->getCdr());
    return *this;
  }

  PairNode *operator*() const {
    assert(pair_ && "attempting to dereference end ListIterator");
    return pair_;
  }
  PairNode *operator->() const {
    return operator*();
  }

 private:
  PairNode *pair_;
};

/// Convenience function for iterating lists.
inline llvm::iterator_range<ListIterator> make_range(Node *list) {
  return llvm::make_range(ListIterator(list), ListIterator());
}

/// Convenience function for iterating lists.
inline llvm::iterator_range<ListPairIterator> makeListPairRange(Node *list) {
  return llvm::make_range(ListPairIterator(list), ListPairIterator());
}

/// \return the number of elements in a list. Improper elements are ignored.
size_t listSize(Node *list);

/// \return the head of a non-empty list.
inline Node *listHead(Node *list) {
  assert(!isListEmpty(list) && "list must not be empty");
  return llvm::cast<PairNode>(list)->getCar();
}

/// \return the tail of a non-empty list.
inline Node *listTail(Node *list) {
  assert(!isListEmpty(list) && "list must not be empty");
  return llvm::cast<PairNode>(list)->getCdr();
}

/// \return true if \p list, which must be a list, is a proper list.
bool isListProper(Node *list);

/// Compare the two ASTs (ignoring source coordinates).
bool deepEqual(const Node *a, const Node *b);

/// Print the AST recursively.
void dump(llvm::raw_ostream &OS, const Node *node);

} // namespace ast
} // namespace s2020

#endif // SCHEME2020_AST_H
