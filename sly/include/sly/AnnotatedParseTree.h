//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_ANNOTATEDPARSETREE_H
#define SEULEXYACC_ANNOTATEDPARSETREE_H


#include "sly.h"
#include "Token.h"
#include "Production.h"
#include <memory>
#include <vector>


using namespace std;

namespace sly::core::type {
class AnnotatedParseTree {
 public:
  
  using SubNode = std::shared_ptr<AnnotatedParseTree>;
  
 private:
  
  sly::core::type::Token token_;
  
  sly::core::type::AttrDict root_attributes_;
  
  sly::core::type::Production production_;
  
  vector<SubNode> sub_nodes_;
};
}

#endif //SEULEXYACC_ANNOTATEDPARSETREE_H
