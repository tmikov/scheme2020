#include "s2020/Parser/Lexer.h"

#include "DiagContext.h"

#include <gtest/gtest.h>

using namespace s2020;
using namespace s2020::parser;

namespace {

class LexerTest : public ::testing::Test {
 protected:
  const llvm::MemoryBuffer &makeBuf(const char *str) {
    auto id = context_.sm.addNewSourceBuffer(
        llvm::MemoryBuffer::getMemBuffer(str, "input", true));
    return *context_.sm.getSourceBuffer(id);
  }

 protected:
  ASTContext context_{};
};

TEST_F(LexerTest, SmokeTest) {
  Lexer lex{context_, makeBuf(" (+\t a 10)\n ")};

  lex.advance();
  ASSERT_EQ(TokenKind::l_paren, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::identifier, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::identifier, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().exactEquals(10));
  lex.advance();
  ASSERT_EQ(TokenKind::r_paren, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());
  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());

  ASSERT_EQ(0, context_.sm.getErrorCount());
}

TEST_F(LexerTest, DecimalNumberTest) {
  Lexer lex{context_,
            makeBuf("1 100 100.5 100e2 0.314e1 314e-2 -1 +20 -50.5 +20.1")};

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().exactEquals(1));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().exactEquals(100));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(100.5));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(100e2));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(3.14));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(3.14));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().exactEquals(-1));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().exactEquals(20));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(-50.5));

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(20.1));

  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());

  ASSERT_EQ(0, context_.sm.getErrorCount());
}

TEST_F(LexerTest, BadDecimalNumberTest) {
  Lexer lex{context_, makeBuf("1a 1e 123456789123456789001234567890")};
  DiagContext diag{context_.sm};

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().exactEquals(1));
  ASSERT_EQ(1, diag.getErrCountClear());
  ASSERT_EQ("delimiter expected", diag.getMessage());

  lex.advance();
  ASSERT_EQ(TokenKind::number, lex.token.getKind());
  ASSERT_TRUE(lex.token.getNumber().inexactEquals(0));
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
  Lexer lex{context_, makeBuf("1 ; kjh\n 2 ; 3 4 \r\n  5")};

  lex.advance();
  ASSERT_TRUE(lex.token.getNumber().exactEquals(1));
  lex.advance();
  ASSERT_TRUE(lex.token.getNumber().exactEquals(2));
  lex.advance();
  ASSERT_TRUE(lex.token.getNumber().exactEquals(5));
  lex.advance();
  ASSERT_EQ(TokenKind::eof, lex.token.getKind());
}

} // anonymous namespace