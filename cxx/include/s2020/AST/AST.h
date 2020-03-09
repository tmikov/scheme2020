#ifndef SCHEME2020_AST_H
#define SCHEME2020_AST_H

#include "s2020/AST/ASTContext.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SMLoc.h"

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

} // namespace ast
} // namespace s2020

#endif // SCHEME2020_AST_H
