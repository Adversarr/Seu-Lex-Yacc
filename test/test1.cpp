//
// Created by Yang Jerry on 2022/3/30.
//

#include <sly/sly.h>
#include <sly/Production.h>
#include <sly/ContextFreeGrammar.h>
#include <sly/TableGenerateMethod.h>
#include <sly/TableGenerateMethodImpl.h>
#include <iostream>


using namespace sly;
using sly::core::type::Token;
using sly::core::type::Production;

int main()
{
  utils::Log::SetLogLevel(sly::utils::Log::kWarning);
  
  auto Expr = Token::NonTerminator("Expr");
  auto id = Token::Terminator("id");
  auto multi = Token::Terminator("*");
  auto plus = Token::Terminator("+");
  auto lb = Token::Terminator("(");
  auto rb = Token::Terminator(")");
  
  multi.SetAttr(Token::kLeftAssociative);
  plus.SetAttr(Token::kLeftAssociative);
  sly::core::grammar::ContextFreeGrammar cfg({
            Production(Expr)(lb)(rb),
            Production(Expr)(Expr)(plus)(Expr),
            Production(Expr)(Expr)(multi)(Expr),
            Production(Expr)(id)
          }, Expr, Token::Terminator("[[Exit]]"));
  
  try {
    sly::core::grammar::Lr1 method;
    cfg.Compile(method);
    auto table = cfg.GetLrTable();
  } catch (exception &e) {
    cerr << e.what();
  }
  return 0;
}