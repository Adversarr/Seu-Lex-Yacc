//
// Created by Yang Jerry on 2022/3/30.
//

#include <sly/Action.h>
namespace sly::core::type {

Action::Action() = default;

Action::Action(Action::ActionFType f) :
  opt_f_(f) {
}

void Action::Modify(vector<TokenAttr> &tokens) {
  if (opt_f_.has_value())
    opt_f_.value()(tokens);
}

bool Action::operator==(const Action &rhs) const {
  if (opt_f_.has_value() != rhs.opt_f_.has_value())
    return false;
  else
    return true;
}

}