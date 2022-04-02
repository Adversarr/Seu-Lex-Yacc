//
// Created by Yang Jerry on 2022/3/30.
//

#include <sly/sly.h>
#include <sly/Production.h>
#include <sly/ContextFreeGrammar.h>
#include <sly/TableGenerateMethod.h>
#include <sly/TableGenerateMethodImpl.h>
#include <sly/AttrDict.h>
#include <sly/FaModel.h>
#include <sly/LrParser.h>

#include <iostream>

#include <vector>

using sly::core::type::Action;
using sly::core::type::Token;
using sly::core::type::Production;
using sly::core::type::AttrDict;


int main()
{
  auto alpha = Token::Terminator("a");
  
  auto add = Token::Terminator("+");
  auto sub = Token::Terminator("-");
  auto multi = Token::Terminator("*");
  add.SetAttr(sly::core::type::Token::kLeftAssociative);
  sub.SetAttr(sly::core::type::Token::kLeftAssociative);
  multi.SetAttr(sly::core::type::Token::kLeftAssociative);
  
  auto lb = Token::Terminator("(");
  auto rb = Token::Terminator(")");
  auto ending = Token::Terminator("EndingTok");
  
  auto expr = Token::NonTerminator("Expr");
  auto fact = Token::NonTerminator("Fact");
  sly::core::grammar::ContextFreeGrammar cfg({
    Production(expr, {[](vector<YYSTATE>& v){
      v[0].Set<int>("v", v[1].Get<int>("v") + v[3].Get<int>("v"));
    }})(expr)(add)(expr),
    Production(expr, {[](vector<YYSTATE>& v){
      v[0].Set<int>("v", v[1].Get<int>("v") - v[3].Get<int>("v"));
    }})(expr)(sub)(expr),
    Production(expr, {[](vector<YYSTATE>& v){
      v[0].Set<int>("v", v[1].Get<int>("v"));
    }})(fact),
    Production(fact, {[](vector<YYSTATE>& v){
      v[0].Set<int>("v", v[1].Get<int>("v") * v[3].Get<int>("v"));
    }})(fact)(multi)(fact),
    Production(fact, {[](vector<YYSTATE>& v){
      v[0].Set<int>("v", v[2].Get<int>("v"));
    }})(lb)(expr)(rb),
    Production(fact, {[](vector<YYSTATE>& v){
      v[0].Set<int>("v", v[1].Get<int>("v"));
    }})(alpha),
  }, expr, ending);
  sly::core::grammar::Lr1 lr1;
  cfg.Compile(lr1);
  auto table = cfg.GetLrTable();
  auto one = AttrDict(); one.Set<int>("v", 1);
  auto two = AttrDict(); two.Set<int>("v", 2);
  sly::core::grammar::LrParser parser(table);
  parser.Parse({alpha, multi, lb, alpha, add, alpha, rb},
               {  two,    {}, {},   two,  {},   one, {}});
  auto tree = parser.GetTree();
  tree.Print(std::cout);
  tree.Annotate();
  tree.Print(std::cout);
  return 0;
}
