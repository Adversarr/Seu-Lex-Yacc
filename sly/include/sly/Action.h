//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_ACTION_H
#define SEULEXYACC_ACTION_H
#include "sly.h"


namespace sly::core::type {
/**
 * LR 自动机中的 Action 类
 */
class Action {
 public:
  using ActionFType = function<void(vector<YYSTATE> &)>;
  
  void Modify(vector<TokenAttr> &tokens);
  
  Action();
  
  
  Action(ActionFType f);
  
  bool operator==(const Action &rhs) const;
 
 private:
  optional<ActionFType> opt_f_;
};

}


#endif //SEULEXYACC_ACTION_H
