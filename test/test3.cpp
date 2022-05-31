#include "sly/FaModel.h"
#include "sly/RegEx.h"
#include "sly/Stream2TokenPipe.h"
#include "sly/Token.h"
#include <sly/sly.h>
#include <iostream>
#include <sstream>
#include <iomanip>

using sly::core::type::AttrDict;
using sly::core::type::Token;
using sly::core::lexical::RegEx;
using sly::core::lexical::DfaModel;
using sly::runtime::Stream2TokenPipe;
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
  // TODO: 原来的代码：
  // vector<Token> lexical_tokens = {
  //   Token::Terminator("any"),
  //   Token::Terminator("word"),
  // };
  // vector<RegEx> lexical_tokens_regex = {
  //   RegEx("."),
  //   RegEx("([a-zA-Z_]([a-zA-Z_]|[0-9])*)"), 
  // };
  vector<Token> lexical_tokens = {
    Token::Terminator("integer"),
    Token::Terminator("float"),
    Token::Terminator("kw-auto"),
    Token::Terminator("kw-const"),
    Token::Terminator("kw-void"),
    Token::Terminator("identifier"),
    Token::Terminator("eq"),
    Token::Terminator("minus-and-assign"),
    Token::Terminator("semicolon"),
    Token::Terminator("post-self-plus"),
    Token::Terminator("lbrace"),
    Token::Terminator("rbrace"),
    Token::Terminator("line-comment"),
    Token::Terminator("spaces"),
    Token::Terminator("any"),
  };
  vector<RegEx> lexical_tokens_regex = {
    RegEx("(([0-9])+)"),
    RegEx("([0-9]+)\\.([0-9]*)(e[+-]?[0-9]+)?"),
    RegEx("auto"),
    RegEx("const"),
    RegEx("void"),
    RegEx("([a-zA-Z_]([a-zA-Z_]|[0-9])*)"), 
    RegEx("="),
    RegEx("-="),
    RegEx(";"),
    RegEx("\\+\\+"),
    RegEx("\\)"),
    RegEx("\\("),
    RegEx("//[^\n]*"),
    RegEx("(\r|\n|\t| )+"),
    RegEx("."),
  };
  vector<DfaModel> lexical_tokens_dfa;
  for (auto& regex : lexical_tokens_regex) {
    lexical_tokens_dfa.push_back(regex.GetDfaModel());
  }
  auto [transition, state] = sly::core::lexical::DfaModel::Merge(lexical_tokens_dfa);
  auto s2ppl = Stream2TokenPipe(transition, state, lexical_tokens, ending);


  string input_string = "abc";
  // 测试样例
  // string s = R"(
  //   auto a = 1.0e2; // this is a comment.
  //   const int b = 2;
  //   b++;
  //   const int bca_123 = 4.0e-10;
  //   void abc();
  // )";
  cin >> input_string;
  stringstream input_stream(input_string);

  vector<AttrDict> attributes;
  vector<Token> tokens;
  while (true) {
    auto token = s2ppl.Defer(input_stream);
    if (token == ending)
      break;
    AttrDict ad;
    ad.Set("lval", s2ppl.buffer_); 

    cerr << setw(12) << "\"" + ad.Get<string>("lval") + "\""
         << "\t" << token.GetTokName() << endl;

    tokens.emplace_back(token);
    attributes.emplace_back(ad);
    
  }

}