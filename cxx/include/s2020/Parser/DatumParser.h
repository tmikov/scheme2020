#ifndef SCHEME2020_PARSER_DATUMPARSER_H
#define SCHEME2020_PARSER_DATUMPARSER_H

#include "s2020/AST/AST.h"

#include "llvm/ADT/Optional.h"

#include <vector>

namespace s2020 {
namespace parser {

/// The specified input is parsed into a sequence of datums, until an error is
/// encountered or EOF is reached. If no errors are found, a list containing the
/// datums is returned.
llvm::Optional<ast::Node *> parseDatums(
    ast::ASTContext &context,
    const llvm::MemoryBuffer &input);

} // namespace parser
} // namespace s2020

#endif // SCHEME2020_PARSER_DATUMPARSER_H
