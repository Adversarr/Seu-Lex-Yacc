//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_REGEX_H
#define SEULEXYACC_REGEX_H
#include "def.h"
#include "FaModel.h"
#include <memory>
#include <string>
namespace sly::core::lexical {

class RegEx {
 public:
  explicit RegEx(std::string expr, bool compile=false);
  
  bool CanMatch(std::string str);
  
  void Compile();
  
  
 private:
  std::unique_ptr<DfaModel> dfa_model_;
  
  std::unique_ptr<NfaModel> nfa_model_;
};

}
#endif //SEULEXYACC_REGEX_H
