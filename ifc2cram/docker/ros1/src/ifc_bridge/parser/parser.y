%require "3.2"
%language "c++"
%define api.namespace {yy}
%define api.parser.class {parser}
%define api.value.type variant
%define parse.error verbose
%locations

/* ---------- Forward decls, go into parser.tab.hpp ---------- */
%code requires {
  #include <string>
  #include <vector>
  #include <memory>
  #include <fstream>              
  #include "model/Entity.hpp"
  #include "model/ParamNode.hpp"

  class IfcLexer; // Forward declaration of the lexer
}

/* ---------- Only needed in parser.tab.cpp ---------- */
%code {
 #include "lexer/lexer.hpp"
  #include <cstdio>
  #include <iostream>
  #include "model/SpatialTree.hpp"
  #include "model/Entity.hpp" 

  std::vector<std::unique_ptr<Entity>> entities;
}

/* ---------- Lexer / Parser params ---------- */
%lex-param   { IfcLexer &lexer }
%parse-param { IfcLexer &lexer }

/* ---------- Tokens ---------- */
%token <std::string> ENTITY_ID IFC_TYPE STRING NUMBER IFC_ENUM
%token <std::string> ISO_FILE_KEYWORD END_ISO_FILE_KEYWORD HEADER_SEC DATA_SEC ENDSEC_KEYWORD
%token <std::string> ASTERISK EQUALS LPAREN RPAREN SEMICOLON COMMA DOT DOLLAR FILE_TYPE

/* ---------- Nonterminal types ---------- */
%type <Entity*>                        entity
%type <std::unique_ptr<ParamNode>>     param
%type <std::vector<std::unique_ptr<ParamNode>>> params param_list

%% /* =================== GRAMMAR =================== */

file:
    ISO_FILE_KEYWORD header_section data_section END_ISO_FILE_KEYWORD
;

header_section:
    HEADER_SEC statements ENDSEC_KEYWORD
;

data_section:
    DATA_SEC entities ENDSEC_KEYWORD
;

statements:
      /* empty */
    | statements statement
;

statement:
    FILE_TYPE LPAREN param_list RPAREN SEMICOLON
;

entities:
    | entities entity
;

entity:
    ENTITY_ID EQUALS IFC_TYPE LPAREN params RPAREN SEMICOLON
    {
        auto e = std::make_unique<Entity>($1, $3, std::move($5));
        entities.push_back(std::move(e));
        $$ = entities.back().get();
    }
;

params:
       { $$ = std::vector<std::unique_ptr<ParamNode>>(); }
    | param_list   { $$ = std::move($1); }
;

param_list:
    param
    {
        std::vector<std::unique_ptr<ParamNode>> list;
        list.push_back(std::move($1));
        $$ = std::move(list);
    }
  | param_list COMMA param
    {
        $1.push_back(std::move($3));
        $$ = std::move($1);
    }
;

param:
      STRING        { $$ = std::make_unique<ParamNode>($1); }
    | NUMBER        { $$ = std::make_unique<ParamNode>($1); }
    | ENTITY_ID     { $$ = std::make_unique<ParamNode>($1); }
    | IFC_TYPE      { $$ = std::make_unique<ParamNode>($1); }
    | IFC_ENUM      { $$ = std::make_unique<ParamNode>($1); }
    | ASTERISK      { $$ = std::make_unique<ParamNode>("*"); }
    | DOLLAR        { $$ = std::make_unique<ParamNode>("$"); }
    | DOT           { $$ = std::make_unique<ParamNode>("."); }
    | LPAREN RPAREN
      {
        $$ = std::make_unique<ParamNode>("()");
      }
    | LPAREN param_list RPAREN
      {
        auto node = std::make_unique<ParamNode>("()");
        for (auto& child : $2) {
            node->addChild(std::move(child));
        }
        $$ = std::move(node);
      }
    | IFC_TYPE LPAREN param_list RPAREN
      {
        auto node = std::make_unique<ParamNode>($1);
        for (auto& child : $3) {
            node->addChild(std::move(child));
        }
        $$ = std::move(node);
      }
;

%% /* =================== USER CODE =================== */

void yy::parser::error(const location_type& loc, const std::string& msg) {
    std::cerr << "Syntax error at line "
              << loc.begin.line << ": " << msg << "\n";
}


