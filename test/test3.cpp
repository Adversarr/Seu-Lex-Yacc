#include "sly/FaModel.h"
#include "sly/RegEx.h"
#include "sly/Stream2TokenPipe.h"
#include "sly/Token.h"
#include <sly/sly.h>
#include <iostream>
#include <sstream>

using namespace std;

int main() {
  sly::utils::Log::SetLogLevel(sly::utils::Log::kError);
  sly::core::lexical::RegEx r1("[0-9]+([Ee][+-]?[0-9]+)(f|F|l|L)?");
  // sly::core::lexical::RegEx c_section_begin("%{");
  // auto end_token = Token::Terminator("EOF_FLAG");
  // auto any_token = Token::Terminator("any");
  // auto c_section_begin_token = Token::Terminator("c_section_begin");

  // auto ret = sly::core::lexical::DfaModel::Merge({r1.GetDfaModel(), c_section_begin.GetDfaModel()});
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
  return 0;
}