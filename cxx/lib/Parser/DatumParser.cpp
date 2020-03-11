#include "s2020/Parser/DatumParser.h"

#include "s2020/Parser/Lexer.h"

using llvm::cast;

namespace s2020 {
namespace parser {

namespace {

class DatumParser {
 public:
  explicit DatumParser(
      ast::ASTContext &context,
      const llvm::MemoryBuffer &input)
      : context_(context), lex_(context, input) {
    lex_.advance();
  }

  llvm::Optional<std::vector<ast::Node *>> parse();

 private:
  ast::Node *parseDatum();

  /// Skip all datum comments and return true if there is a fatal error.
  bool skipDatumComments();

  template <typename N, typename V>
  ast::Node *makeSimpleNodeAndAdvance(const V &v) {
    auto *node = new (context_) N(v);
    node->setSourceRange(lex_.token.getSourceRange());
    lex_.advance();
    return node;
  }

  ast::Node *parseList(TokenKind closingKind);

 private:
  ast::ASTContext &context_;
  /// Lex lexer.
  Lexer lex_;
  /// Whether a fatal error has already been reported, so we shouldn't report
  /// any more.
  bool fatal_ = false;
  /// Maximum allowed nesting level to avoid stack overflow.
  static constexpr unsigned MAX_NESTING = 1024;
  /// Nesting level.
  unsigned nesting_ = 0;

  /// A RAII to maintain the nesting.
  class NestingRAII;
};

class DatumParser::NestingRAII {
 public:
  explicit NestingRAII(DatumParser &parser) : p_(parser) {
    ++p_.nesting_;
  }
  ~NestingRAII() {
    --p_.nesting_;
  }

 private:
  DatumParser &p_;
};

#define CHECK_NESTING()                        \
  NestingRAII nestingRAII{*this};              \
  if (nesting_ >= MAX_NESTING) {               \
    lex_.error("too many nested expressions"); \
    fatal_ = true;                             \
    return nullptr;                            \
  } else {                                     \
  }

llvm::Optional<std::vector<ast::Node *>> DatumParser::parse() {
  // Remember how many errors we started with.
  if (context_.sm.isErrorLimitReached())
    return llvm::None;

  auto numErrors = context_.sm.getErrorCount();

  std::vector<ast::Node *> res{};
  while (auto *datum = parseDatum())
    res.push_back(datum);

  // If errors occurred
  if (numErrors != context_.sm.getErrorCount())
    return llvm::None;

  return std::move(res);
}

ast::Node *DatumParser::parseDatum() {
  CHECK_NESTING();

  for (;;) {
    switch (lex_.token.getKind()) {
      case TokenKind::eof:
        return nullptr;

      case TokenKind::datum_comment:
        lex_.advance();
        // Ignore the next datum.
        if (!parseDatum())
          return nullptr;
        continue;

      case TokenKind::number:
        return makeSimpleNodeAndAdvance<ast::NumberNode>(
            lex_.token.getNumber());
      case TokenKind::identifier:
        return makeSimpleNodeAndAdvance<ast::SymbolNode>(
            lex_.token.getIdentifier());

      case TokenKind::l_paren:
        return parseList(TokenKind::r_paren);
      case TokenKind::l_square:
        return parseList(TokenKind::r_square);

      default:
        lex_.error("unexpected token");
        lex_.advance();
        continue;
    }
  }
}

bool DatumParser::skipDatumComments() {
  while (lex_.token.getKind() == TokenKind::datum_comment) {
    lex_.advance();
    if (!parseDatum())
      return fatal_;
  }

  return false;
}

ast::Node *DatumParser::parseList(s2020::parser::TokenKind closingKind) {
  CHECK_NESTING();

  auto startLoc = lex_.token.getStartLoc();
  lex_.advance();

  if (lex_.token.getKind() == closingKind) {
    auto *empty = new (context_.allocateNode<ast::NullNode>()) ast::NullNode();
    empty->setStartLoc(startLoc);
    empty->setEndLoc(lex_.token.getEndLoc());
    lex_.advance();
    return empty;
  }

  ast::PairNode *head = nullptr;
  ast::PairNode *tail = nullptr;
  bool dotted = false;

  auto *datum = parseDatum();
  if (!datum)
    goto reportUnterminated;

  head = new (context_.allocateNode<ast::PairNode>())
      ast::PairNode(datum, nullptr);
  head->setStartLoc(startLoc);
  tail = head;

  if (skipDatumComments())
    return nullptr;

  while (lex_.token.getKind() != closingKind) {
    if (lex_.token.getKind() == TokenKind::period) {
      dotted = true;

      lex_.advance();
      datum = parseDatum();
      if (!datum)
        goto reportUnterminated;

      tail->setCdr(datum);

      if (skipDatumComments())
        return nullptr;

      if (lex_.token.getKind() != closingKind) {
        lex_.error("list terminator expected");
        context_.sm.note(startLoc, "list started here");
        // Skip until the end of the list.
        while (lex_.token.getKind() != TokenKind::eof &&
               lex_.token.getKind() != closingKind) {
          if (!parseDatum())
            return nullptr;
        }
      }
      break;
    }

    datum = parseDatum();
    if (!datum)
      goto reportUnterminated;

    auto *newTail = new (context_.allocateNode<ast::PairNode>())
        ast::PairNode(datum, nullptr);
    newTail->setStartLoc(datum->getStartLoc());
    tail->setCdr(newTail);
    tail = newTail;

    if (skipDatumComments())
      return nullptr;
  }

  // If this wasn't a dotted list, we must allocate the terminating Null node.
  if (!dotted) {
    auto *empty = new (context_.allocateNode<ast::NullNode>()) ast::NullNode();
    empty->setSourceRange(lex_.token.getSourceRange());
    tail->setCdr(empty);
  }

  // Now that we have reached the end of the list, set all end locations.
  for (auto *cur = head;; cur = cast<ast::PairNode>(cur->getCdr())) {
    cur->setEndLoc(lex_.token.getEndLoc());
    if (cur == tail)
      break;
  }

  lex_.advance();
  return head;

reportUnterminated:
  if (!fatal_) {
    fatal_ = true;
    lex_.error("unterminated list");
    context_.sm.note(startLoc, "list started here");
  }
  return nullptr;
}

} // anonymous namespace

llvm::Optional<std::vector<ast::Node *>> parseDatums(
    ast::ASTContext &context,
    const llvm::MemoryBuffer &input) {
  DatumParser parser{context, input};
  return parser.parse();
}

} // namespace parser
} // namespace s2020