//
// Created by Yang Jerry on 2022/3/30.
//

#include "sly/AttrDict.h"
#include "sly/FaModel.h"
#include "sly/LrParser.h"
#include "sly/RegEx.h"
#include "sly/Stream2TokenPipe.h"
#include <sly/sly.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using sly::core::type::AttrDict;
using sly::core::type::Production;
using sly::core::type::Token;
using sly::core::lexical::RegEx;
using namespace std;


/**
 * .l file analysis: 
 * 
 * delim  %%[\r\n]
 * lbrace   %{[\r\n]
 * rbrace   %}[\r\n]
 * line     .*[\r\n]
 * 
 * LexFile <- Defs delim Rules delim Sub eof
 * Defs <- DefsLine Defs | lbrace Codes rbrace | 
 * Codes <- CodesLine Codes | 
 * CodesLine <- line {}
 * DefsLine <- line {word blank RegEx "\n"}
 * Rules <- RulesLine Rules | 
 * RulesLine <- line {LexRegEx blank CodePart "\n"}
 * Sub <- SubLine Sub | 
 * SubLine <- line {}
 **/

/*
LexFile
Defs
DefsLine
Rules
RulesLine
Sub
SubLine
Codes
CodesLine
*/

/**
 * .y file analysis: 
 * 
 * delim       %%[\r\n]
 * semicolon   [ \t]*;[\r\n]
 * line        .*[\r\n]
 * YaccFile <-  Defs delim Rules delim Subroutines eof
 * Defs <- ProdLines semicolon Defs {concat ProdLines together, ...} | 
 * ProdLines <- ProdLine ProdLines | ProdLine
 * Subroutines <- SubroutinesLine Subroutines | 
 * SubroutinesLine <- line {}
 **/

void rtrim(string &str) {
  str.erase(str.find_last_not_of("\n\0") + 1); 
}

int main() {
  sly::utils::Log::SetLogLevel(sly::utils::Log::kNone);

  // 定义文法
  // 1. 定义tokens
  auto delim  = Token::Terminator("%%");
  auto lbrace = Token::Terminator("%{");
  auto rbrace = Token::Terminator("%{");
  auto line   = Token::Terminator("line");
  auto ending = Token::Terminator("EOF_FLAG");
  RegEx re_delim("%%[\r\n]+");
  RegEx re_lbrace("%{[\r\n]+");
  RegEx re_rbrace("%}[\r\n]+");
  RegEx re_line(".*[\r\n]+");
  auto LexFile   = Token::NonTerminator("LexFile");
  auto Defs      = Token::NonTerminator("Defs");
  auto DefsLine  = Token::NonTerminator("DefsLine");
  auto Rules     = Token::NonTerminator("Rules");
  auto RulesLine = Token::NonTerminator("RulesLine");
  auto Sub       = Token::NonTerminator("Sub");
  auto SubLine   = Token::NonTerminator("SubLine");
  auto Codes     = Token::NonTerminator("Codes");
  auto CodesLine = Token::NonTerminator("CodesLine");
  
  vector<Production> productions = {
    // LexFile <- Defs delim Rules delim Sub
    Production(LexFile, {[](vector<YYSTATE> &v) {
      }})(Defs)(delim)(Rules)(delim)(Sub),
    // Defs <- DefsLine Defs
    Production(Defs, {[](vector<YYSTATE> &v) {
      }})(DefsLine)(Defs),
    // Defs <- lbrace Codes rbrace | 
    Production(Defs, {[](vector<YYSTATE> &v) {
      }})(lbrace)(Codes)(rbrace),
    // Defs <- 
    Production(Defs, {[](vector<YYSTATE> &v) {
      }}),
    // Codes <- CodesLine Codes | 
    Production(Codes, {[](vector<YYSTATE> &v) {
      }})(CodesLine)(Codes),
    // Codes <- CodesLine Codes | 
    Production(Codes, {[](vector<YYSTATE> &v) {
      }}),
    // CodesLine <- line {}
    Production(CodesLine, {[](vector<YYSTATE> &v) {
        // do something
      }})(line),
    // DefsLine <- line {word blank RegEx "\n"}
    Production(DefsLine, {[](vector<YYSTATE> &v) {
        // do something
      }})(line),
    // Rules <- RulesLine Rules
    Production(Rules, {[](vector<YYSTATE> &v) {
      }})(RulesLine)(Rules),
    // Rules <- 
    Production(Rules, {[](vector<YYSTATE> &v) {
      }}),
    // RulesLine <- line {LexRegEx blank CodePart "\n"}
    Production(RulesLine, {[](vector<YYSTATE> &v) {
        // do something
      }})(line),
    // Sub <- SubLine Sub
    Production(Sub, {[](vector<YYSTATE> &v) {
      }})(SubLine)(Sub),
     // Sub <- 
    Production(Sub, {[](vector<YYSTATE> &v) {
      }}),
    // SubLine <- line {}
    Production(SubLine, {[](vector<YYSTATE> &v) {
        // do something
      }})(line),
  };
  sly::core::grammar::ContextFreeGrammar cfg(productions, LexFile, ending);
  sly::core::grammar::Lr1 lr1;
  cfg.Compile(lr1);
  auto table = cfg.GetLrTable();
  sly::core::grammar::LrParser parser(table);

  // 定义词法 transition 和 state
  auto [transition, state] = sly::core::lexical::DfaModel::Merge({
    re_delim.GetDfaModel(),
    re_lbrace.GetDfaModel(),
    re_rbrace.GetDfaModel(),
    re_line.GetDfaModel(),
  });
  auto s2ppl = sly::runtime::Stream2TokenPipe(transition, state, {
    delim, lbrace, rbrace, line, 
  }, ending);

  ifstream lexFile("../demo/1.in");

  // string input_string;
  // cout << "Input Expr: ";
  // cin >> input_string;
  // cout << "The Expr: " << input_string << endl;

  stringstream input_stream;
  input_stream << lexFile.rdbuf();
  lexFile.close();

  cout << input_stream.str();
  // return 0;

  vector<AttrDict> attributes;
  vector<Token> tokens;
  while (true) {
    auto token = s2ppl.Defer(input_stream);
    AttrDict ad;
    ad.Set("lval", s2ppl.buffer_); 
    tokens.emplace_back(token);
    attributes.emplace_back(ad);
    if (token == ending)
      break;
  }
  parser.Parse(tokens, attributes);

  auto tree = parser.GetTree();
  cout << "\n\nBefore Annotate.: " << endl;
  tree.Print(std::cout);
  // tree.Annotate();
  // cout << "\n\nAfter Annotate:" << endl;
  // tree.Print(std::cout);
  // cout << "\n\nThe Expr: " << input_string << endl;

  return 0;
}
