//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_ANNOTATEDPARSETREE_H
#define SEULEXYACC_ANNOTATEDPARSETREE_H


#include "def.h"
#include "Token.h"
#include "Production.h"
#include "AttrDict.h"
#include <memory>
#include <vector>
#include <deque>


using namespace std;

namespace sly::core::type {
class AnnotatedParseTree {
 public:
  enum class Type{
    kNonTerminator,
    kTerminator
  };
  
  using SubNode = std::shared_ptr<AnnotatedParseTree>;
  
  const vector<AttrDict> &GetRootAttributes() const;
  
  void EmplaceBack(AnnotatedParseTree&& tree);

  void EmplaceFront(AnnotatedParseTree&& tree);
  
  void PushBack(AnnotatedParseTree tree);
  
  void Annotate(AnnotatedParseTree* p_father = nullptr);
  
  AnnotatedParseTree(Token token, AttrDict attr);
  
  explicit AnnotatedParseTree(const Production& prod);
  
  void Print(ostream& oss, int depth=0) const;
  
 private:
  Type type_;
  
  Token token_;
  
  vector<AttrDict> attrs_;
  
  vector<Action> actions_;
  
  std::deque<SubNode> sub_nodes_;
  
  bool is_annotated_; 
};
}

#endif //SEULEXYACC_ANNOTATEDPARSETREE_H
