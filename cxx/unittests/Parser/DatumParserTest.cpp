#include "s2020/Parser/DatumParser.h"

#include "DiagContext.h"

#include <gtest/gtest.h>

using namespace s2020;
using namespace s2020::parser;
using namespace s2020::ast;

namespace {

class DatumParserTest : public ::testing::Test {
 protected:
  const llvm::MemoryBuffer &makeBuf(const char *str) {
    auto id = context_.sm.addNewSourceBuffer(
        llvm::MemoryBuffer::getMemBuffer(str, "input", true));
    return *context_.sm.getSourceBuffer(id);
  }

  NumberNode *Num(ExactNumberT n) {
    return new (context_) NumberNode(context_.makeExactNumber(n));
  }
  SymbolNode *Sym(StringRef name) {
    return new (context_) SymbolNode(context_.stringTable.getIdentifier(name));
  }

 protected:
  ASTContext context_{};
};

TEST_F(DatumParserTest, PrintTest) {
  auto res = parseDatums(
      context_,
      makeBuf("hello 10"
              " (list -10 more)"
              " (a . b)"
              " (1 2 3 . 4)"
              " (10 . (20 . (30 . ())))"
              " (if [> a 10] (display 1) (display a))"));
  ASSERT_TRUE(res.hasValue());

  std::string str;
  llvm::raw_string_ostream OS{str};
  for (const auto *node : res.getValue())
    dump(OS, node);
  OS.flush();
  ASSERT_EQ(
      "hello\n"
      "10\n"
      "(list\n"
      "    -10\n"
      "    more)\n"
      "(a . b)\n"
      "(1\n"
      "    2\n"
      "    3 . 4)\n"
      "(10\n"
      "    20\n"
      "    30)\n"
      "(if\n"
      "    (>\n"
      "        a\n"
      "        10)\n"
      "    (display\n"
      "        1)\n"
      "    (display\n"
      "        a))\n",
      str);
}

TEST_F(DatumParserTest, SmokeTest) {
  auto parsed = parseDatums(
      context_,
      makeBuf("(hello "
              "  10"
              "  (list -10 more)"
              "  (a . b)"
              "  (1 2 3 . 4)"
              "  (10 . (20 . (30 . ())))"
              "  (if [> a 10] (display 1) (display a)))"));
  ASSERT_TRUE(parsed.hasValue());
  ASSERT_EQ(1, parsed.getValue().size());

  auto *l = list(
      context_,
      Sym("hello"),
      Num(10),
      list(context_, Sym("list"), Num(-10), Sym("more")),
      cons(context_, Sym("a"), Sym("b")),
      cons(
          context_,
          Num(1),
          cons(context_, Num(2), cons(context_, Num(3), Num(4)))),
      list(context_, Num(10), Num(20), Num(30)),
      list(
          context_,
          Sym("if"),
          list(context_, Sym(">"), Sym("a"), Num(10)),
          list(context_, Sym("display"), Num(1)),
          list(context_, Sym("display"), Sym("a"))));

  ASSERT_TRUE(deepEqual(parsed.getValue().at(0), l));
}

} // anonymous namespace