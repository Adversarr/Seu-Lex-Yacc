#include "sly/FaModel.h"
#include "sly/RegEx.h"
#include "sly/Stream2TokenPipe.h"
#include "sly/Token.h"
#include <sly/sly.h>
#include <iostream>
#include <sstream>

using sly::core::type::AttrDict;
using sly::core::type::Production;
using sly::core::type::Token;
using sly::core::lexical::RegEx;
using sly::core::lexical::DfaModel;
using sly::runtime::Stream2TokenPipe;
using sly::core::grammar::LrParser;
using namespace std;

int main() {
  // while (true) {
  //   string str;
  //   cin >> str;
  //   cout << str << endl;
  //   sly::utils::Log::SetLogLevel(sly::utils::Log::kError);
  //   sly::core::lexical::RegEx r1(str);
  //   auto ret = sly::core::lexical::DfaModel::Merge({r1.GetDfaModel()});
  // }

  // sly::core::lexical::RegEx c_section_begin("%{");
  // auto end_token = Token::Terminator("EOF_FLAG");
  // auto any_token = Token::Terminator("any");
  // auto c_section_begin_token = Token::Terminator("c_section_begin");

  // sly::runtime::Stream2TokenPipe s2ppl(ret.first, ret.second, {
  //   any_token, c_section_begin_token
  // }, end_token);
  // istringstream iss("++++++");
  // while (true) {
  //   auto tok = s2ppl.Defer(iss);
  //   cout << tok << endl;
  //   if (tok == end_token) {
  //     sly::utils::Log::GetGlobalLogger().Info("Done.");
  //     return 0;
  //   }
  // }
  // return 0;
  sly::utils::Log::SetLogLevel(sly::utils::Log::kError);
  auto ending = Token::Terminator("EOF_FLAG");
  vector<Token> lexical_tokens = {
    Token::Terminator("any"), 
    Token::Terminator("word"), 
  };
  vector<RegEx> lexical_tokens_regex = {
    RegEx("."), 
    RegEx("[a-z]+"), 
  };
  vector<DfaModel> lexical_tokens_dfa;
  for (auto regex : lexical_tokens_regex) {
    lexical_tokens_dfa.push_back(regex.GetDfaModel());
  }
  auto [transition, state] = sly::core::lexical::DfaModel::Merge(lexical_tokens_dfa);
  auto s2ppl = Stream2TokenPipe(transition, state, lexical_tokens, ending);


  string input_string = "abc123";
  // cin >> input_string;
  stringstream input_stream(input_string);

  vector<AttrDict> attributes;
  vector<Token> tokens;
  while (true) {
    auto token = s2ppl.Defer(input_stream);
    if (token == ending)
      break;
    AttrDict ad;
    ad.Set("lval", s2ppl.buffer_); 

    cerr << ad.Get<string>("lval") << " " << token.GetTokName() << endl;

    tokens.emplace_back(token);
    attributes.emplace_back(ad);
    
  }

}