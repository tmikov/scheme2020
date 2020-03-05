#include "s2020/Parser/Lexer.h"

#include "DiagContext.h"

#include <gtest/gtest.h>

using namespace s2020;
using namespace s2020::parser;

namespace {

class LexerTest : public ::testing::Test {
 protected:
  const llvm::MemoryBuffer &makeBuf(const char *str) {
    auto id = sm_.addNewSourceBuffer(
        llvm::MemoryBuffer::getMemBuffer(str, "input", true));
    return *sm_.getSourceBuffer(id);
  }

 protected:
  SourceErrorManager sm_{};
  Allocator allocator_{};
  StringTable stringTable_{allocator_};
};

TEST_F(LexerTest, SmokeTest) {
  Lexer lex{sm_, allocator_, stringTable_, makeBuf(" (+\t a 10)\n ")};

  lex.advance();
  ASSERT_EQ(TokenKind::l_paren, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::identifier, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::identifier, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::exact_number, lex.token.getKind());
  ASSERT_EQ(10, lex.token.getExactNumber());
  lex.advance();
  ASSERT_EQ(TokenKind::r_paren, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());

  ASSERT_EQ(0, sm_.getErrorCount());
}

TEST_F(LexerTest, DecimalNumberTest) {
  Lexer lex{sm_,
            allocator_,
            stringTable_,
            makeBuf("1 100 100.5 100e2 0.314e1 314e-2 -1 +20 -50.5 +20.1")};

  lex.advance();
  ASSERT_EQ(TokenKind::exact_number, lex.token.getKind());
  ASSERT_EQ(1, lex.token.getExactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::exact_number, lex.token.getKind());
  ASSERT_EQ(100, lex.token.getExactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(100.5, lex.token.getInexactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(100e2, lex.token.getInexactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(3.14, lex.token.getInexactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(3.14, lex.token.getInexactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::exact_number, lex.token.getKind());
  ASSERT_EQ(-1, lex.token.getExactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::exact_number, lex.token.getKind());
  ASSERT_EQ(20, lex.token.getExactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(-50.5, lex.token.getInexactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(20.1, lex.token.getInexactNumber());

  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());

  ASSERT_EQ(0, sm_.getErrorCount());
}

TEST_F(LexerTest, BadDecimalNumberTest) {
  Lexer lex{sm_,
            allocator_,
            stringTable_,
            makeBuf("1a 1e 123456789123456789001234567890")};
  DiagContext diag{sm_};

  lex.advance();
  ASSERT_EQ(TokenKind::exact_number, lex.token.getKind());
  ASSERT_EQ(1, lex.token.getExactNumber());
  ASSERT_EQ(1, diag.getErrCountClear());
  ASSERT_EQ("delimiter expected", diag.getMessage());

  lex.advance();
  ASSERT_EQ(TokenKind::inexact_number, lex.token.getKind());
  ASSERT_EQ(0, lex.token.getInexactNumber());
  ASSERT_EQ(1, diag.getErrCountClear());
  ASSERT_EQ("invalid number: missing exponent", diag.getMessage());

  lex.advance();
  ASSERT_EQ(1, diag.getErrCountClear());
  ASSERT_EQ("number overflows exact range", diag.getMessage());

  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());
  ASSERT_EQ(0, diag.getErrCount());
}

TEST_F(LexerTest, LineCommentTest) {
  Lexer lex{
      sm_, allocator_, stringTable_, makeBuf("1 ; kjh\n 2 ; 3 4 \r\n  5")};

  lex.advance();
  ASSERT_EQ(1, lex.token.getExactNumber());
  lex.advance();
  ASSERT_EQ(2, lex.token.getExactNumber());
  lex.advance();
  ASSERT_EQ(5, lex.token.getExactNumber());
  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());
}

} // anonymous namespace