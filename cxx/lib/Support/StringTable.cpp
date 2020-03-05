/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "s2020/Support/StringTable.h"
#include "llvm/Support/raw_ostream.h"

namespace s2020 {

llvm::raw_ostream &operator<<(llvm::raw_ostream &os, Identifier id) {
  return os << id.str();
}

} // namespace s2020
