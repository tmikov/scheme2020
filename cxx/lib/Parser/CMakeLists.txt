add_s2020_library(S2020Parser STATIC
  DatumParser.cpp
  Lexer.cpp
  LINK_LIBS S2020AST S2020Support
    )
