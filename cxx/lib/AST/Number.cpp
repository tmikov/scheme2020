#include "s2020/AST/Number.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

namespace s2020 {
namespace ast {

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Number &number) {
  switch (number.getKind()) {
    case NumberKind::exact:
      OS << number.getExact();
      return OS;
    case NumberKind::inexact:
      OS << number.getInexact();
      return OS;
  }
  assert(false && "unsupported Number kind");
  return OS;
}

} // namespace ast
} // namespace s2020