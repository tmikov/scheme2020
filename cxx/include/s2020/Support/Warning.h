/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef S2020_SUPPORT_WARNINGS_H
#define S2020_SUPPORT_WARNINGS_H

#include "llvm/ADT/DenseMapInfo.h"

#include <type_traits>

namespace s2020 {

/// Categories of warnings the compiler might emit, that can be turned on or
/// off.
enum class Warning {
  NoWarning,
  UndefinedVariable,
  DirectEval,
  Misc,

  _NumWarnings,
};

} // namespace s2020

namespace llvm {

using s2020::Warning;

/// Information to store a \p Warning as the key of a \p DenseMap or \p
/// DensetSet
template <>
struct DenseMapInfo<Warning> {
  using Underlying = std::underlying_type<Warning>::type;
  using UnderlyingInfo = DenseMapInfo<Underlying>;

  static inline Warning getEmptyKey() {
    return static_cast<Warning>(UnderlyingInfo::getEmptyKey());
  }

  static inline Warning getTombstoneKey() {
    return static_cast<Warning>(UnderlyingInfo::getTombstoneKey());
  }

  static unsigned getHashValue(Warning warning) {
    return UnderlyingInfo::getHashValue(static_cast<Underlying>(warning));
  }

  static bool isEqual(Warning lhs, Warning rhs) {
    return lhs == rhs;
  }
};

} // namespace llvm

#endif // S2020_SUPPORT_SOURCEERRORMANAGER_H
