#ifndef SCHEME2020_AST_NUMBER_H
#define SCHEME2020_AST_NUMBER_H

#include <cassert>
#include <cstdint>
#include <cstring>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace s2020 {
namespace ast {

class ASTContext;

enum class NumberKind : uint8_t {
  exact,
  inexact,
};

using ExactNumberT = int64_t;
using InexactNumberT = double;

class Number {
 public:
  Number() : kind_(NumberKind::exact), exact_(0) {}

  NumberKind getKind() const {
    return kind_;
  }
  bool isExact() const {
    return kind_ == NumberKind::exact;
  }
  bool isInexact() const {
    return kind_ == NumberKind::inexact;
  }

  ExactNumberT getExact() const {
    assert(kind_ == NumberKind::exact);
    return exact_;
  }
  InexactNumberT getInexact() const {
    assert(kind_ == NumberKind::inexact);
    return inexact_;
  }

  /// \return true if the two numbers are the same kind and have the same
  ///     bit pattern.
  bool equals(const Number &other) const {
    static_assert(
        sizeof(exact_) == sizeof(inexact_),
        "comparison relies on exact and inexact being the same size");
    return kind_ == other.kind_ &&
        memcmp(&exact_, &other.exact_, sizeof(exact_)) == 0;
  }

  bool operator==(const Number &other) const {
    return equals(other);
  }

  bool exactEquals(ExactNumberT o) const {
    return kind_ == NumberKind::exact && exact_ == o;
  }

  bool inexactEquals(const InexactNumberT &o) const {
    return kind_ == NumberKind::inexact &&
        memcmp(&inexact_, &o, sizeof(inexact_)) == 0;
  }

 private:
  friend class ASTContext;
  explicit Number(ExactNumberT exact)
      : kind_(NumberKind::exact), exact_(exact) {}
  explicit Number(InexactNumberT inexact)
      : kind_(NumberKind::inexact), inexact_(inexact) {}

 private:
  NumberKind kind_;
  union {
    ExactNumberT exact_;
    InexactNumberT inexact_;
  };
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, const Number &number);

} // namespace ast
} // namespace s2020

#endif // SCHEME2020_AST_NUMBER_H
