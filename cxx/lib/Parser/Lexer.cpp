#include "s2020/Parser/Lexer.h"

#include "llvm/ADT/APInt.h"

namespace s2020 {
namespace parser {

struct CC {
  enum {
    // 0,1,2
    ClassMask = 7,

    WhitespaceClass = 1,
    /// Initial identifier.
    InitialClass = 2,
    /// +, -, .
    PeculiarIdentClass = 3,
    /// 0-9
    DigitClass = 4,
    /// UTF8
    UTF8Class = 5,

    // 3
    /// Subsequent identifier.
    Subsequent = (1 << 3),
    // 4
    SignSubsequent = (2 << 3),
    // 5
    DotSubsequent = (4 << 3),
    // 6
    Delimiter = (8 << 3)
  };

  using Flags = uint8_t;

  static Flags getClass(Flags f) {
    return f & ClassMask;
  }

  static bool testSubsequent(Flags f) {
    return f & Subsequent;
  }
  static bool testSignSubsequent(Flags f) {
    return f & SignSubsequent;
  }
  static bool testDotSubsequent(Flags f) {
    return f & DotSubsequent;
  }
  static bool testDelimiter(Flags f) {
    return f & Delimiter;
  }
};

static const CC::Flags s_charTab[256] = {
#include "./CharTab.inc"
};

/// \return the character flags at the specified address.
static inline CC::Flags getCharFlags(const char *p) {
  return s_charTab[*(const unsigned char *)p];
}

const char *tokenKindStr(TokenKind kind) {
  static const char *tokenStr[] = {
#define TOK(name, str) str,
#include "s2020/Parser/TokenKinds.def"
  };

  assert(kind < TokenKind::_last_token);
  return tokenStr[ord(kind)];
}

Lexer::Lexer(ASTContext &context, const llvm::MemoryBuffer &input)
    : context_(context) {
  assert(
      context_.sm.findBufferForLoc(
          SMLoc::getFromPointer(input.getBufferStart())) &&
      "input buffer must be registered with SourceErrorManager");

  bufferStart_ = input.getBufferStart();
  bufferEnd_ = input.getBufferEnd();
  assert(*bufferEnd_ == 0 && "input buffer is not zero terminated");

  curCharPtr_ = bufferStart_;
}

Lexer::~Lexer() = default;

void Lexer::advance() {
  CC::Flags chFlags = getCharFlags(curCharPtr_);

  for (;;) {
    assert(curCharPtr_ <= bufferEnd_ && "lexing past end of input");

    switch (CC::getClass(chFlags)) {
      case CC::WhitespaceClass:
        // Whitespaces frequently come in groups, so keep keep going.
        do
          chFlags = getCharFlags(++curCharPtr_);
        while (CC::getClass(chFlags) == CC::WhitespaceClass);
        break;

      case CC::InitialClass: {
        token.setStart(curCharPtr_);
        const char *end = curCharPtr_;
        do
          ++end;
        while (CC::testSubsequent(getCharFlags(end)));

        token.setEnd(end);
        token.setIdentifier(
            getIdentifier(StringRef(curCharPtr_, end - curCharPtr_)));
        curCharPtr_ = end;

        skipUntilDelimiter();
        return;
      }

      case CC::PeculiarIdentClass: {
        token.setStart(curCharPtr_);
        const char *end = curCharPtr_;

        // "." something.
        if (*end == '.') {
          ++end;
          // Just a "."?
          if (!CC::testDotSubsequent(getCharFlags(end))) {
            // Possibly a real number?
            if (*end >= '0' && *end <= '9') {
              parseNumberDigits(curCharPtr_, false, 10, 1);
            } else {
              // No, just a ".".
              token.setEnd(end);
              token.setKind(TokenKind::period);
              curCharPtr_ = end;
            }
            return;
          }
          do
            ++end;
          while (CC::testSubsequent(getCharFlags(end)));
        } else {
          // "+"/"-" something.
          assert((*end == '+' || *end == '-') && "invalid character flags");
          ++end;
          if (*end == '.') {
            ++end;
            if (!CC::testDotSubsequent(getCharFlags(end))) {
              // Possibly a real number?
              if (*end >= '0' && *end <= '9') {
                parseNumberDigits(
                    curCharPtr_ + 1, false, 10, *curCharPtr_ == '+' ? 1 : -1);
                return;
              } else {
                // No, just a "+."
                // TODO: is this really intended to be a valid identifier?
              }
            } else {
              do
                ++end;
              while (CC::testSubsequent(getCharFlags(end)));
            }
          } else if (CC::testSignSubsequent(getCharFlags(end))) {
            do
              ++end;
            while (CC::testSubsequent(getCharFlags(end)));
          } else if (*end >= '0' && *end <= '9') {
            // A number.
            parseNumberDigits(
                end, llvm::None, 10, *curCharPtr_ == '+' ? 1 : -1);
            return;
          } else {
            // Just a sign.
          }
        }

        token.setEnd(end);
        token.setIdentifier(
            getIdentifier(StringRef(curCharPtr_, end - curCharPtr_)));
        curCharPtr_ = end;

        skipUntilDelimiter();
        return;
      }

      case CC::DigitClass:
        token.setStart(curCharPtr_);
        parseNumberDigits(curCharPtr_, llvm::None, 10, 1);
        return;

      case CC::UTF8Class:
        error(SMLoc::getFromPointer(curCharPtr_), "unsupported character");
        // Skip all UTF8 characters.
        while (CC::getClass(getCharFlags(++curCharPtr_)) == CC::UTF8Class) {
        }
        break;

      default:
        switch (*curCharPtr_) {
#define CHTOK(ch, kind)          \
  case ch:                       \
    token.setStart(curCharPtr_); \
    ++curCharPtr_;               \
    token.setEnd(curCharPtr_);   \
    token.setKind(kind);         \
    return

          CHTOK('(', TokenKind::l_paren);
          CHTOK(')', TokenKind::r_paren);
          CHTOK('[', TokenKind::l_square);
          CHTOK(']', TokenKind::r_square);
          CHTOK('{', TokenKind::l_brace);
          CHTOK('}', TokenKind::r_brace);
          CHTOK('\'', TokenKind::apostrophe);
          CHTOK('`', TokenKind::backtick);

          case ',':
            token.setStart(curCharPtr_);
            ++curCharPtr_;
            if (*curCharPtr_ == '@') {
              ++curCharPtr_;
              token.setKind(TokenKind::comma_at);
            } else {
              token.setKind(TokenKind::comma);
            }
            token.setEnd(curCharPtr_);
            return;

          // Possibly EOF.
          case '\0':
            if (curCharPtr_ == bufferEnd_) {
              token.setStart(curCharPtr_);
              token.setEnd(curCharPtr_);
              token.setKind(TokenKind::eof);
              return;
            }
            error(SMLoc::getFromPointer(curCharPtr_), "unsupported character");
            ++curCharPtr_;
            break;

          case ';': // Line comment.
            skipLineComment(curCharPtr_);
            chFlags = getCharFlags(curCharPtr_);
            break;

#undef CHTOK
        }
    }
  }
}

inline void Lexer::skipUntilDelimiter() {
  // If this character is a delimiter or we are at EOF, all is good.
  if (LLVM_LIKELY(CC::testDelimiter(getCharFlags(curCharPtr_))))
    return;
  _skipUntilDelimiterSlowPath();
}

void Lexer::_skipUntilDelimiterSlowPath() {
  // EOF?
  if (!*curCharPtr_ && curCharPtr_ == bufferEnd_)
    return;

  // Otherwise it is an error and we must skip until a delimiter.
  error(SMLoc::getFromPointer(curCharPtr_), "delimiter expected");
  ++curCharPtr_;
  while (!CC::testDelimiter(getCharFlags(curCharPtr_))) {
    if (!*curCharPtr_ && curCharPtr_ == bufferEnd_)
      break;
    ++curCharPtr_;
  }
}

void Lexer::parseNumberDigits(
    const char *start,
    llvm::Optional<bool> exact,
    int radix,
    int sign) {
  assert(sign && "sign should not be 0");

  const char *ptr = start;
  bool real = false;

  if (LLVM_LIKELY(radix == 10)) {
    while (*ptr >= '0' && *ptr <= '9')
      ++ptr;

    if (*ptr == '.') {
      ++ptr;
      real = true;
      goto fraction;
    } else if ((*ptr | 32) == 'e') {
      ++ptr;
      real = true;
      goto exponent;
    } else {
      goto end;
    }

  fraction:
    // We arrive here after we have consumed the decimal dot ".".
    //
    while (*ptr >= '0' && *ptr <= '9')
      ++ptr;

    if ((*ptr | 32) == 'e') {
      ++ptr;
      goto exponent;
    } else {
      goto end;
    }

  exponent:
    // We arrive here after we have consumed the exponent character 'e' or 'E'.
    //
    if (*ptr == '+' || *ptr == '-')
      ++ptr;
    if (*ptr >= '0' && *ptr <= '9') {
      do
        ++ptr;
      while (*ptr >= '0' && *ptr <= '9');
    } else {
      curCharPtr_ = ptr;
      token.setEnd(ptr);
      token.setNumber(context_.makeInexactNumber(0.0));
      error("invalid number: missing exponent");
      skipUntilDelimiter();
      return;
    }
  end:;
  } else {
    while ((*ptr >= '0' && *ptr <= '9' && *ptr < radix + '0') ||
           (radix == 16 && (*ptr | 32) >= 'a' && (*ptr | 32) <= 'f')) {
      ++ptr;
    }
  }

  StringRef str{start, (size_t)(ptr - start)};
  Number number;

  curCharPtr_ = ptr;
  token.setEnd(ptr);

  if (!exact.hasValue())
    exact = !real;

  if (real && exact.getValue()) {
    error("real number cannot be represented as exact");
    number = context_.makeExactNumber(0);
  } else if (!exact.getValue() && radix == 10) {
    double inexactRes;
    str.getAsDouble(inexactRes, true);
    if (sign < 0)
      inexactRes = -inexactRes;
    number = context_.makeInexactNumber(inexactRes);
  } else {
    llvm::APInt iresult{64, 0};
    str.getAsInteger(radix, iresult);

    if (exact.getValue()) {
      ExactNumberT exactRes;
      if (iresult.getActiveBits() <= 64) {
        exactRes = iresult.getZExtValue();
      } else {
        error("number overflows exact range");
        exactRes = iresult.getLoBits(64).getZExtValue();
      }
      if (sign < 0)
        exactRes = -exactRes;
      number = context_.makeExactNumber(exactRes);
    } else {
      auto aDouble = iresult.roundToDouble();
      if (sign < 0)
        aDouble = -aDouble;
      number = context_.makeInexactNumber(aDouble);
    }
  }

  token.setNumber(number);

  skipUntilDelimiter();
}

void Lexer::skipLineComment(const char *start) {
  assert(*start == ';' && "invalid line comment");
  ++start;
  for (;;) {
    switch (*start) {
      case '\r':
      case '\n':
        ++start;
        goto endLoop;

      case 0:
        if (curCharPtr_ == bufferEnd_)
          goto endLoop;
        // fallthrough.
      default:
        ++start;
    }
  }
endLoop:
  curCharPtr_ = start;
}

bool Lexer::error(llvm::SMLoc loc, const llvm::Twine &msg) {
  context_.sm.error(loc, msg);
  if (!context_.sm.isErrorLimitReached())
    return true;
  forceEOF();
  return false;
}

bool Lexer::error(llvm::SMRange range, const llvm::Twine &msg) {
  context_.sm.error(range, msg);
  if (!context_.sm.isErrorLimitReached())
    return true;
  forceEOF();
  return false;
}

bool Lexer::error(
    llvm::SMLoc loc,
    llvm::SMRange range,
    const llvm::Twine &msg) {
  context_.sm.error(loc, range, msg);
  if (!context_.sm.isErrorLimitReached())
    return true;
  forceEOF();
  return false;
}

} // namespace parser
} // namespace s2020