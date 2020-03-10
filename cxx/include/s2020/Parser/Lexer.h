#ifndef SCHEME2020_PARSER_LEXER_H
#define SCHEME2020_PARSER_LEXER_H

#include "s2020/AST/ASTContext.h"

namespace s2020 {
namespace parser {

using ast::ASTContext;
using ast::ExactNumberT;
using ast::InexactNumberT;
using ast::Number;

enum class TokenKind : uint8_t {
#define TOK(name, str) name,
#include "TokenKinds.def"
  _last_token,
};

inline constexpr unsigned ord(TokenKind kind) {
  return static_cast<unsigned>(kind);
}

static constexpr unsigned NUM_TOKENS = ord(TokenKind::_last_token);
const char *tokenKindStr(TokenKind kind);

/// A convenient way to structurally group the data associated with the / last
/// token. More than one instance does not exist in one lexer.
class Token {
  friend class Lexer;

 public:
  Token(const Token &) = delete;
  void operator=(const Token &) = delete;

  Token() {}
  ~Token() {}

  TokenKind getKind() const {
    return kind_;
  }

  SMLoc getStartLoc() const {
    return range_.Start;
  }
  SMLoc getEndLoc() const {
    return range_.End;
  }
  SMRange getSourceRange() const {
    return range_;
  }

  StringRef inputStr() const {
    return StringRef(
        range_.Start.getPointer(),
        range_.End.getPointer() - range_.Start.getPointer());
  }

  const Number &getNumber() const {
    assert(getKind() == TokenKind::number);
    return number_;
  }

  Identifier getIdentifier() const {
    assert(getKind() == TokenKind::identifier);
    return ident_;
  }

 private:
  void setStart(const char *start) {
    range_.Start = SMLoc::getFromPointer(start);
  }
  void setEnd(const char *end) {
    range_.End = SMLoc::getFromPointer(end);
  }
  void setIdentifier(Identifier ident) {
    kind_ = TokenKind::identifier;
    ident_ = ident;
  }
  void setNumber(const Number &n) {
    kind_ = TokenKind::number;
    number_ = n;
  }
  void setKind(TokenKind kind) {
    kind_ = kind;
  }

 private:
  TokenKind kind_{TokenKind::none};
  SMRange range_{};
  union {
    Identifier ident_;
    Number number_;
  };
};

class Lexer {
 public:
  /// The last scanned token.
  Token token{};

  explicit Lexer(ASTContext &context, const llvm::MemoryBuffer &input);
  ~Lexer();

  ASTContext &getContext() const {
    return context_;
  }

  /// Force an EOF at the next token.
  void forceEOF() {
    curCharPtr_ = bufferEnd_;
  }

  /// Consume the current token and scan the next one, which becomes the new
  /// current token.
  void advance();

  Identifier getIdentifier(StringRef name) {
    return context_.stringTable.getIdentifier(name);
  }

  /// Report an error for the range from startLoc to curCharPtr.
  bool errorRange(SMLoc startLoc, const llvm::Twine &msg) {
    return error({startLoc, SMLoc::getFromPointer(curCharPtr_)}, msg);
  }

  /// Report an error using the current token's location.
  bool error(const llvm::Twine &msg) {
    return error(token.getSourceRange(), msg);
  }

  /// Emit an error at the specified source location. If the maximum number of
  /// errors has been reached, return false and move the scanning pointer to
  /// EOF.
  /// \return false if too many errors have been emitted and we need to abort.
  bool error(SMLoc loc, const llvm::Twine &msg);

  /// Emit an error at the specified source range. If the maximum number of
  /// errors has been reached, return false and move the scanning pointer to
  /// EOF.
  /// \return false if too many errors have been emitted and we need to abort.
  bool error(SMRange range, const llvm::Twine &msg);

  /// Emit an error at the specified source location and range. If the maximum
  /// number of errors has been reached, return false and move the scanning
  /// pointer to EOF.
  /// \return false if too many errors have been emitted and we need to abort.
  bool error(SMLoc loc, SMRange range, const llvm::Twine &msg);

 private:
  /// The current character is expected to be a delimiter or an EOF. If so,
  /// do nothing and return. Otherwise, report an error and skip until a
  /// delimiter or EOF.
  /// \param errorReported whether an error has already been reported and
  ///     further errors should be supressed.
  inline void skipUntilDelimiter(bool errorReported = false);

  /// The slow path of \c skipUntilDelimiter()
  /// \param errorReported whether an error has already been reported and
  ///     further errors should be supressed.
  void _skipUntilDelimiterSlowPath(bool errorReported);

  void skipLineComment(const char *start);
  void parseNumberDigits(
      const char *start,
      llvm::Optional<bool> exact,
      int radix,
      int sign);

 private:
  ASTContext &context_;

  const char *bufferStart_{};
  const char *bufferEnd_{};
  const char *curCharPtr_{};
};

} // namespace parser
} // namespace s2020

#endif // SCHEME2020_PARSER_LEXER_H
