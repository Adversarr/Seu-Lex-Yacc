//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_ACTION_H
#define SEULEXYACC_ACTION_H

#include "def.h"
#include <vector>
#include <functional>
#include <optional>

namespace sly::core::type {
/**
 * LR 自动机中的 Action 类
 */
class Action {
 public:
  using ActionFType = std::function<void(std::vector<YYSTATE> &)>;
  
  void Modify(std::vector<TokenAttr> &tokens);
  
  Action();
  
  
  Action(ActionFType f);
  
  bool operator==(const Action &rhs) const;
 
 private:
  std::optional<ActionFType> opt_f_;
};

}


#endif //SEULEXYACC_ACTION_H
