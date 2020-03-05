/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef S2020_TEST_PARSER_DIAGCONTEXT_H
#define S2020_TEST_PARSER_DIAGCONTEXT_H

#include "s2020/Parser/Lexer.h"

namespace s2020 {
namespace parser {

class DiagContext {
  int errCount_{0};
  int warnCount_{0};
  std::string message_{};

 public:
  explicit DiagContext(SourceErrorManager &mgr) {
    mgr.setDiagHandler(handler, this);
  }

  void clear() {
    errCount_ = warnCount_ = 0;
  }
  int getErrCount() const {
    return errCount_;
  }
  int getWarnCount() const {
    return warnCount_;
  }

  int getErrCountClear() {
    unsigned res = errCount_;
    clear();
    return res;
  }
  int getWarnCountClear() {
    unsigned res = warnCount_;
    clear();
    return res;
  }

  const std::string &getMessage() {
    return message_;
  }

 private:
  static void handler(const llvm::SMDiagnostic &msg, void *ctx) {
    DiagContext *diag = static_cast<DiagContext *>(ctx);
    if (msg.getKind() == llvm::SourceMgr::DK_Error) {
      ++diag->errCount_;
      diag->message_ = msg.getMessage();
    } else if (msg.getKind() == llvm::SourceMgr::DK_Warning) {
      ++diag->warnCount_;
      diag->message_ = msg.getMessage();
    } else {
      diag->message_.clear();
    }
  }
};

} // namespace parser
} // namespace s2020

#endif // S2020_TEST_PARSER_DIAGCONTEXT_H
