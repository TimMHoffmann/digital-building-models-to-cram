#pragma once
#include <FlexLexer.h>
#include "parser.tab.hpp"

class IfcLexer : public yyFlexLexer {
public:
    explicit IfcLexer(std::istream* in = nullptr) : yyFlexLexer(in) {}

    int yylex(yy::parser::semantic_type* lval,
              yy::parser::location_type* lloc);

    using yyFlexLexer::yylex;
};

int yylex(yy::parser::semantic_type* lval,
          yy::parser::location_type* lloc,
          IfcLexer& lexer);
