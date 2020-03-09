#include "s2020/AST/AST.h"

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

} // namespace ast
} // namespace s2020