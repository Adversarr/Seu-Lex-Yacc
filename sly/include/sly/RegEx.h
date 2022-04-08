//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_REGEX_H
#define SEULEXYACC_REGEX_H
#include "def.h"
#include "FaModel.h"
#include <memory>
#include <string>
#include <iostream>
namespace sly::core::lexical {

class RegEx {
 public:

  explicit RegEx(std::string expr, bool compile=false);
  
  bool CanMatch(std::string str);
  
  void Compile();
  
  inline const DfaModel& GetDfaModel() const ;

 private:

  std::string expr_;

  DfaModel dfa_model_;

};

const DfaModel& RegEx::GetDfaModel() const {
  return dfa_model_;
}


}
#endif //SEULEXYACC_REGEX_H
