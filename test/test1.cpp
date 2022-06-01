//
// Created by Yang Jerry on 2022/3/30.
//

#include "sly/AttrDict.h"
#include "sly/ContextFreeGrammar.h"
#include "sly/FaModel.h"
#include "sly/LrParser.h"
#include "sly/RegEx.h"
#include "sly/Stream2TokenPipe.h"
#include <sly/sly.h>

#include <iostream>
#include <sstream>
#include <vector>

using sly::core::type::AttrDict;
using sly::core::type::Production;
using sly::core::type::Token;


using namespace std;

int main() {
  // return 0;
  sly::utils::Log::SetLogLevel(sly::utils::Log::kWarning);
  // 定义文法
  // 1. 定义tokens
  auto add =
      Token::Terminator("+", 0, Token::Attr::kLeftAssociative);
  auto sub =
      Token::Terminator("-", 0, Token::Attr::kLeftAssociative);
  auto multi =
      Token::Terminator("*", 0, Token::Attr::kLeftAssociative);
  auto div =
      Token::Terminator("/", 0, //   注意要用 R"(....)" 把每一个lambda的实现写进去（我实在是想不到怎么用宏来避免这个丑陋的东西了。
//   先用这个print出来下面的表。
  auto table = cfg.GetLrTable();
//   table.Print(cout);

  sly::core::grammar::LrParser parser(table);

  // 定义词法 transition 和 state
  auto [transition, state] = sly::core::lexical::DfaModel::Merge(
      {re_add.GetDfaModel(), re_alpha.GetDfaModel(), re_div.GetDfaModel(),
       re_lb.GetDfaModel(), re_multi.GetDfaModel(), re_rb.GetDfaModel(),
       re_sub.GetDfaModel(), re_blank.GetDfaModel()});
  auto s2ppl = sly::runtime::Stream2TokenPipe(
      transition, state, {add, alpha, div, lb, multi, rb, sub, blank}, ending);

  string input_string /* = "(1+2)*3"*/;
  cout << "Input Expr: ";
  cin >> input_string;
  cout << "The Expr: " << input_string << endl;
  stringstream input_stream(input_string);
  vector<AttrDict> attributes;
  vector<Token> tokens;
  while (true) {
    auto token = s2ppl.Defer(input_stream);
    if (token == blank)
      continue;
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
  tree.Annotate();
  cout << "\n\nAfter Annotate:" << endl;
  tree.Print(std::cout);
  cout << "\n\nThe Expr: " << input_string << endl;
  cout << "Evaluated : " << tree.GetRootAttributes()[0].Get<int>("v") << endl;
  return 0;
}
