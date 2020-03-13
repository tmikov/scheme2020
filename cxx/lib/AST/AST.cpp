#include "s2020/AST/AST.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;

namespace s2020 {
namespace ast {

llvm::StringRef nodeKindStr(NodeKind kind) {
  static const char *names[] = {
#define S2020_AST_NODE(name) #name,
#include "s2020/AST/NodeKinds.def"
  };

  assert(kind < NodeKind::_end && "invalid NodeKind");
  return names[(unsigned)kind];
}

PairNode *cons(ASTContext &ctx, Node *a, Node *b) {
  return new (ctx) PairNode(a, b);
}

Node *ListBuilder::finishList(ASTContext &ctx) {
  llvm::SMLoc endLoc{};
  if (tail_)
    endLoc = cast<PairNode>(tail_)->getEndLoc();

  appendNewTail(new (ctx) NullNode());

  assert(head_ && "head_ must have been initialized");

  // Initialize the source locations for consistency.
  for (auto *cur : makeListPairRange(head_)) {
    cur->setStartLoc(cur->getCar()->getStartLoc());
    cur->setEndLoc(endLoc);
  }

  auto *res = head_;
  head_ = tail_ = nullptr;
  return res;
}

size_t listSize(Node *list) {
  assert(isList(list) && "argument must be a list");
  size_t size = 0;
  for (auto *cur : makeListPairRange(list)) {
    (void)cur;
    ++size;
  }
  return size;
}

bool isProperList(Node *list) {
  assert(isList(list) && "argument must be a list");
  // An empty list is a proper list.
  if (!isa<PairNode>(list))
    return true;
  auto *cur = cast<PairNode>(list);
  while (isa<PairNode>(cur->getCdr()))
    cur = cast<PairNode>(cur->getCdr());
  return isa<NullNode>(cur->getCdr());
}

#define DECLARE_SIMPLE_AST_EQUAL(name)                                       \
  static inline bool equal##name(const name##Node *a, const name##Node *b) { \
    return a->getValue() == b->getValue();                                   \
  }
#define DECLARE_NOTIMPL_AST_EQUAL(name)                                      \
  static inline bool equal##name(const name##Node *a, const name##Node *b) { \
    llvm_unreachable("equal" #name " not implemented");                      \
    return false;                                                            \
  }

DECLARE_SIMPLE_AST_EQUAL(Boolean);
DECLARE_SIMPLE_AST_EQUAL(Character);
DECLARE_SIMPLE_AST_EQUAL(String);
DECLARE_SIMPLE_AST_EQUAL(Symbol);
DECLARE_SIMPLE_AST_EQUAL(Number);

DECLARE_NOTIMPL_AST_EQUAL(Bytevector);
DECLARE_NOTIMPL_AST_EQUAL(Vector);

static inline bool equalNull(const NullNode *a, const NullNode *b) {
  return true;
}
static inline bool equalPair(const PairNode *a, const PairNode *b) {
  return deepEqual(a->getCar(), b->getCar()) &&
      deepEqual(a->getCdr(), b->getCdr());
}

bool deepEqual(const Node *a, const Node *b) {
  if (a->getKind() != b->getKind())
    return false;

  switch (a->getKind()) {
#define S2020_AST_NODE(name) \
  case NodeKind::name:       \
    return equal##name(cast<name##Node>(a), cast<name##Node>(b));
#include "s2020/AST/NodeKinds.def"
    default:
      return true;
  }
}

static void dump(llvm::raw_ostream &OS, const Node *node, unsigned indent);

static void
dumpCharacter(llvm::raw_ostream &OS, const CharacterNode *node, unsigned) {
  char32_t ch = cast<CharacterNode>(node)->getValue();
  OS << "#\\";

  // Try the named ones first.
  switch (ch) {
#define S2020_CHARACTER(name, code) \
  case code:                        \
    OS << #name;                    \
    break;
#include "s2020/AST/Characters.def"
    default:
      if (ch > 32 && ch < 127)
        OS.write((unsigned char)ch);
      else
        OS << llvm::format("0x%x", ch);
      break;
  }
}

static void
dumpSymbol(llvm::raw_ostream &OS, const SymbolNode *node, unsigned) {
  // TODO: utf-8
  llvm::StringRef str = node->getValue().str();
  // Do we need to escape it?
  bool escaping = false;
  for (auto c : str) {
    // FIXME: we should actually enforce the rules for identifiers here.
    if (c <= 32 || c >= 127 || c == '|' || c == '\\') {
      escaping = true;
      break;
    }
  }
  if (escaping) {
    OS.write('|');
    for (auto c : str) {
      switch (c) {
        case '\a':
          OS << "\\a";
          break;
        case '\b':
          OS << "\\b";
          break;
        case '\t':
          OS << "\\t";
          break;
        case '\n':
          OS << "\\n";
          break;
        case '\r':
          OS << "\\r";
          break;
        case '|':
          OS << "\\|";
          break;
        case '\\':
          OS << "\\\\";
          break;
        default:
          if (c >= 32 && c < 127)
            OS.write((unsigned char)c);
          else
            OS << llvm::format("0x%x;", c);
          break;
      }
    }
    OS.write('|');
  } else {
    OS << str;
  }
}

static void dumpIndent(llvm::raw_ostream &OS, unsigned indent) {
  OS << llvm::left_justify("", indent * 4);
}

static void
dumpPair(llvm::raw_ostream &OS, const PairNode *node, unsigned indent) {
  OS << "(";
  dump(OS, node->getCar(), indent + 1);

  while (auto *next = dyn_cast<PairNode>(node->getCdr())) {
    node = next;
    OS << "\n";
    dumpIndent(OS, indent + 1);
    dump(OS, node->getCar(), indent + 1);
  }

  if (!isa<NullNode>(node->getCdr())) {
    OS << " . ";
    dump(OS, node->getCdr(), indent + 1);
  }

  OS << ")";
}

static void
dumpBoolean(llvm::raw_ostream &OS, const BooleanNode *node, unsigned) {
  OS << (node->getValue() ? "#t" : "#f");
}
static void
dumpNumber(llvm::raw_ostream &OS, const NumberNode *node, unsigned) {
  OS << node->getValue();
}
static void
dumpString(llvm::raw_ostream &OS, const StringNode *node, unsigned) {
  // TODO: utf-8
  OS.write('"');
  OS.write_escaped(node->getValue().str(), true);
  OS.write('"');
}
static void
dumpBytevector(llvm::raw_ostream &OS, const BytevectorNode *node, unsigned) {
  llvm_unreachable("not implemented");
}
static void
dumpVector(llvm::raw_ostream &OS, const VectorNode *node, unsigned) {
  llvm_unreachable("not implemented");
}
static void dumpNull(llvm::raw_ostream &OS, const NullNode *, unsigned) {
  OS << "()";
}

static void dump(llvm::raw_ostream &OS, const Node *node, unsigned indent) {
  switch (node->getKind()) {
#define S2020_AST_NODE(name)                        \
  case NodeKind::name:                              \
    dump##name(OS, cast<name##Node>(node), indent); \
    break;
#include "s2020/AST/NodeKinds.def"
    default:
      break;
  }
}

void dump(llvm::raw_ostream &OS, const Node *node) {
  dump(OS, node, 0);
  OS << "\n";
}

} // namespace ast
} // namespace s2020