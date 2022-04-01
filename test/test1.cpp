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
#include <iostream>
#include <vector>

using sly::core::type::Action;
using sly::core::type::Token;
using sly::core::type::Production;
using sly::core::type::AttrDict;


int main()
{
  auto lb = Token::Terminator("(");
  auto rb = Token::Terminator(")");
  auto alpha = Token::Terminator("a");
  auto slash = Token::Terminator("-");
  auto lbb = Token::Terminator("[");
  auto rbb = Token::Terminator("]");
  auto range = Token::NonTerminator("Range");
  auto range_prod = Production(range, Action([](std::vector<YYSTATE>& attr){
    auto c1 = attr[2].Get<char>("lval");
    auto c2 = attr[4].Get<char>("lval");
    sly::core::lexical::NfaModel model(c2);
    for (auto c = c1; c < c2; ++c) {
      model.Combine(sly::core::lexical::NfaModel(c));
    }
    attr[0].Set<sly::core::lexical::NfaModel>("model", model);
    sly::utils::Log::GetGlobalLogger().Info("Range.");
  }))(lb)(alpha)(slash)(alpha)(rb);

  sly::core::type::mark_as_printable<Token>();
  AttrDict ad;
  ad.Set<int>("iv", 1);
  ad.Set<float>("fv", 2.0);
  ad.Set<Token>("tok", lbb);
  for (auto [k, v]: ad.ToString()) {
    std::cout << k << "\t" << v << std::endl;
  }
  

  return 0;
}