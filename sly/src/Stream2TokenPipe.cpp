#include "sly/Token.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <sly/Stream2TokenPipe.h>
#include <sly/utils.h>

namespace sly::runtime {

Stream2TokenPipe::Stream2TokenPipe(std::vector<std::vector<int>> working_table,
                                   std::vector<int> accept_states,
                                   std::vector<core::type::Token> corr_token,
                                   core::type::Token end_token)
    : table_(move(working_table)), accept_states_(move(accept_states)),
      end_token_(move(end_token)), token_list_(move(corr_token)) {
  for (const auto &v : accept_states_) {
    assert(v == -1 || v < accept_states_.size());
  }
  history_.clear();
  history_count_=0;
}

core::type::Token Stream2TokenPipe::Defer(std::istream &is) {
  int current_state = 0;
  char c;
  buffer_.clear();
  if (!(is.good() && !is.eof() && !is.fail())) {
    return end_token_;
  }
  while (is.good() && !is.eof() && !is.fail()) {
    if (history_.size() > 20) {
      history_ = history_.substr(10);
    }
    c = is.get();
    spdlog::debug("Caught ascii={} char={} From stream.", static_cast<int>(c),
                 c);
    // 1. 测试是否是可行的
    if (!(c > static_cast<int8_t>(0) && c <= static_cast<int8_t>(127))) {
      spdlog::debug("Handling the eof flag.");
      if (buffer_.length() == 0) {
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
      spdlog::debug("Push to buffer.");
      buffer_.push_back(c);
      history_.push_back(c);
      history_count_ ++;
      current_state = next_state;
    }
  }

  // 显然当前 buffer 不空
  int return_token_id = accept_states_[current_state];
  if (return_token_id < 0) {
    spdlog::error("Caught invalid lexical element({} ascii={})"
                  " current_buffer={}, history={}, position={}", 
              c, static_cast<int>(c), buffer_, history_, history_count_);
    assert(false);
  } else {
    spdlog::info("return: buffer={} corr={}", buffer_, token_list_[return_token_id].ToString());
    return token_list_[return_token_id];
  }
}

} // namespace sly::runtime