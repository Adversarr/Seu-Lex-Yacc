//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_REGEX_H
#define SEULEXYACC_REGEX_H
#include "sly.h"
#include "FaModel.h"
namespace sly::core::lexical {

class RegEx {
 public:
  explicit RegEx(std::string expr, bool compile=false);
  
  bool CanMatch(std::string str);
  
  void Compile();
  
  
 private:
  unique_ptr<DfaModel> dfa_model_;
  
  unique_ptr<NfaModel> nfa_model_;
};

}
#endif //SEULEXYACC_REGEX_H
