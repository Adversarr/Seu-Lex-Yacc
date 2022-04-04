//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_STREAM2TOKENPIPE_H
#define SEULEXYACC_STREAM2TOKENPIPE_H

#include "def.h"
#include "Token.h"
#include <iostream>
#include <vector>

namespace sly::runtime {

class Stream2TokenPipe {
 public:
  explicit Stream2TokenPipe(std::vector<std::vector<int>> working_table, std::vector<int> accept_states);

  core::type::Token Defer(std::istream& is);
 
 private:

  std::vector<std::vector<int>> table_;
  
  std::vector<int> accept_states_;

  vector<core::type::Token> token_list_;
};

} // namespace sly::runtime
#endif