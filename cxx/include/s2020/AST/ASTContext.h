#ifndef SCHEME2020_AST_ASTCONTEXT_H
#define SCHEME2020_AST_ASTCONTEXT_H

#include "s2020/AST/Number.h"
#include "s2020/Support/SourceErrorManager.h"
#include "s2020/Support/StringTable.h"

#include "llvm/Support/Allocator.h"

namespace s2020 {
namespace ast {

using Allocator = llvm::BumpPtrAllocator;

class ASTContext {
 public:
  SourceErrorManager sm;
  Allocator allocator;
  StringTable stringTable{allocator};

  ASTContext();
  ~ASTContext();

  Number makeExactNumber(ExactNumberT exact) {
    return Number{exact};
  }
  Number makeInexactNumber(InexactNumberT inexact) {
    return Number{inexact};
  }

  /// Allocates AST nodes. Should not be used for non-AST data because
  /// the memory may be freed during parsing.
  template <typename T>
  T *allocateNode(size_t num = 1) {
    return allocator.template Allocate<T>(num);
  }
  void *allocateNode(size_t size, size_t alignment) {
    return allocator.Allocate(size, alignment);
  }
};

} // namespace ast
} // namespace s2020

#endif // SCHEME2020_AST_ASTCONTEXT_H
