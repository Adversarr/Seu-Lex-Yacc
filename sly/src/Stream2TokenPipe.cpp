#include "sly/Token.h"
#include <cassert>
#include <sly/Stream2TokenPipe.h>
#include <sly/utils.h>

namespace sly::runtime {

Stream2TokenPipe::Stream2TokenPipe(std::vector<std::vector<int>> working_table,
                                   std::vector<int> accept_states,
                                   std::vector<core::type::Token> corr_token,
                                   core::type::Token end_token) :
  table_(move(working_table)), 
  accept_states_(move(accept_states)),
  end_token_(move(end_token)), token_list_(move(corr_token)) {
  for (const auto& v: accept_states_) {
    assert(v == -1 || v < accept_states_.size());
  }
}

core::type::Token Stream2TokenPipe::Defer(std::istream& is) {
  int current_state = 0;
  std::string buffer;
  char c;
  if (!(is.good() && !is.eof() && !is.fail())) {
    return end_token_;
  }
  while (is.good() && !is.eof() && !is.fail()) {
    c = is.get();
    utils::Log::GetGlobalLogger().Info("Caught ", static_cast<int>(c) , " From stream.");
    // 1. 测试是否是可行的
    if (!(c > static_cast<char>(0) && c <= static_cast<char>(127))) {
      utils::Log::GetGlobalLogger().Info("Handling the eof flag.");
      if (buffer.length() == 0) {
        // 如果 buffer 是空的，直接返回 eof 标志
        return end_token_;
      } else {
        // 如果 buffer 不空，则先返回之前的内容，并把 c 放回。
        is.putback(c);
        break;
      }
    }

    int next_state = table_[current_state][c];
    if (next_state == -1) {
      // flush buffer and return
      is.putback(c);
      break;
    } else {
    utils::Log::GetGlobalLogger().Info("Push to buffer.");
      buffer.push_back(c);
      current_state = next_state;
    }
  }
  // 显然当前 buffer 不空
  int return_token_id = accept_states_[current_state];
  if (return_token_id < 0) {
    utils::Log::GetGlobalLogger().Err("Caught invalid lexical element(", c , "/ascii=", static_cast<int>(c), "), current buffer = ", buffer);
    assert(false);
  } else {
    utils::Log::GetGlobalLogger().Info("buffer=", buffer);
    return token_list_[return_token_id];
  }
}

}