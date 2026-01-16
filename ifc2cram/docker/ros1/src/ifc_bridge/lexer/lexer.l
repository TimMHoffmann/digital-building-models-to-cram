%option c++ noyywrap nodefault yylineno
%option yyclass="IfcLexer"

%{
#include "model/Entity.hpp"
#include "model/ParamNode.hpp"
#include "utils/util.hpp"
#include "parser.tab.hpp"
#include <iostream>
#include <string>

#undef YY_DECL
#define YY_DECL int IfcLexer::yylex(yy::parser::semantic_type* lval, \
                                    yy::parser::location_type* lloc)

class IfcLexer : public yyFlexLexer {
public:
    IfcLexer(std::istream* in = nullptr) : yyFlexLexer(in) {}

    int yylex(yy::parser::semantic_type* lval,
              yy::parser::location_type* lloc);
    
    using yyFlexLexer::yylex;
};
%}

%%

"ISO-"[^;]*";"              { return yy::parser::token::ISO_FILE_KEYWORD; }
"HEADER;"                   { return yy::parser::token::HEADER_SEC; }
"DATA;"                     { return yy::parser::token::DATA_SEC; }
"ENDSEC;"                   { return yy::parser::token::ENDSEC_KEYWORD; }
"END-ISO-"[^;]*";"          { return yy::parser::token::END_ISO_FILE_KEYWORD; }

"="                         { return yy::parser::token::EQUALS; }
"("                         { return yy::parser::token::LPAREN; }
")"                         { return yy::parser::token::RPAREN; }
";"                         { return yy::parser::token::SEMICOLON; }
","                         { return yy::parser::token::COMMA; }

"*" {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::ASTERISK;
}
\'([^']|\'\')*\' {
    lval->emplace<std::string>(decode_ifc_string(yytext));
    return yy::parser::token::STRING;
}
[#][0-9]+ {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::ENTITY_ID;
}
\.[A-Z0-9_]+\. {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::IFC_ENUM;
}
FILE_[A-Za-z_][A-Za-z0-9_]* {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::FILE_TYPE;
}
IFC[A-Z0-9]* {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::IFC_TYPE;
}

[+-]?[0-9]+(\.[0-9]*)?([eE][-+]?[0-9]+)? {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::NUMBER;
}

"\." {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::DOT;
}
"$" {
    lval->emplace<std::string>(yytext);
    return yy::parser::token::DOLLAR;
}

[\r\n]+                   { /* line counting handled by yylineno */ }
[ \t]+                    { /* ignore whitespace */ }

. {
    std::cerr << "LEXER: Unknown character: " << yytext
              << " at line " << yylineno << "\n";
}
%%


int yylex(yy::parser::semantic_type* lval,
          yy::parser::location_type* lloc,
          IfcLexer& lexer)
{
    return lexer.yylex(lval, lloc);
}
