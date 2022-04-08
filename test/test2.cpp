#include "sly/FaModel.h"
#include "sly/RegEx.h"
#include <sly/sly.h>
#include <iostream>
#include <sstream>

using namespace std;

int main() {
  sly::core::lexical::RegEx r1("123", true);
  sly::core::lexical::RegEx r2("ab*d+", true);
  sly::core::lexical::RegEx r3("[a-z]+", true);
  
  auto ret = sly::core::lexical::DfaModel::Merge({r1.GetDfaModel(), r2.GetDfaModel(), r3.GetDfaModel()});
  for (const auto& line: ret.first) {
    for (int i = 0; i < 127; ++i) {
      std:: cout << line[i] << ',';
    }
    cout << line[127] << endl;
  }
  sly::utils::Log::GetGlobalLogger().Warn("========================================");
  for (const auto& v: ret.second) {
    cout << v << ",";
  }
  cout << endl;

  istringstream iss("123abdzbdc");
  int current = 0;
  string buffer;
  while (current != -1 && !iss.eof()) {
    char c = iss.get();
    sly::utils::Log::GetGlobalLogger().Info("Caught ", c, ' ', static_cast<int>(c), " current ", current);
    if (!(c > static_cast<char>(0) && c <= static_cast<char>(127))) {
      if (ret.second[current] != -1) {
        sly::utils::Log::GetGlobalLogger().Info("End with state: ", current," buffer: ", buffer);
      } else {
        sly::utils::Log::GetGlobalLogger().Warn("End with Error.", c);
      }
      buffer = "";
      current = 0;
      break;
    }
    auto tran = ret.first[current][static_cast<int>(c)];
    if (tran == -1) {
      iss.putback(c);
      if (ret.second[current] != -1) {
        sly::utils::Log::GetGlobalLogger().Info("End with state: ", current, buffer);
      } else {
        sly::utils::Log::GetGlobalLogger().Warn("End with Error.", c);
      }
      buffer = "";
      current = 0;
    } else {
      sly::utils::Log::GetGlobalLogger().Info("Going on. ", current, "->", tran);
      buffer.push_back(c);
      current = tran;
    }
  }
  return 0;
}