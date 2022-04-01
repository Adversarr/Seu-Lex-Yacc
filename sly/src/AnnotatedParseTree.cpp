//
// Created by Yang Jerry on 2022/3/30.
//

#include <sly/AnnotatedParseTree.h>
#include <iomanip>
#include <utility>
namespace sly::core::type {

AnnotatedParseTree::AnnotatedParseTree(Token token, AttrDict attr): is_annotated_(true), type_(Type::kTerminator),
  token_(std::move(token)), attrs_({std::move(attr)})
{
}

AnnotatedParseTree::AnnotatedParseTree(Production prod): is_annotated_(false), type_(Type::kNonTerminator)
{
}


const vector<AttrDict> &sly::core::type::AnnotatedParseTree::GetRootAttributes() const {
  return attrs_;
}


void AnnotatedParseTree::Annotate(AnnotatedParseTree* p_father) {
  if (type_ == Type::kTerminator) {
    // leaf node, do nothing and return.
    return;
  }
  
  is_annotated_ = false;
  attrs_.clear();
  attrs_.emplace_back();
  for (int i = 1; i < actions_.size(); ++i) {
    auto p = sub_nodes_[i];
    if (p->is_annotated_) {
      continue;
    }
    p->Annotate(this);
    if (!(p->is_annotated_)) {
      if (p_father == nullptr) {
        throw runtime_error("Cannot annotate.");
      }
      return;
    }
    attrs_.push_back(p->attrs_.front());
    actions_[i].Modify(attrs_);
  }
  actions_[0].Modify(attrs_);
  is_annotated_ = true;
}

void AnnotatedParseTree::PushBack(AnnotatedParseTree tree) {
  EmplaceBack(move(tree));
}

void AnnotatedParseTree::EmplaceBack(AnnotatedParseTree &&tree) {
  sub_nodes_.emplace_back(make_shared<AnnotatedParseTree>(tree));
}

void AnnotatedParseTree::Print(ostream &oss, int depth) const{
  if (depth > 0) {
    oss << std::setw(depth * 2) << "* ";
  }
  if (type_ == Type::kTerminator) {
    oss << "Terminator" << token_ << std::endl;
  } else {
    oss << "NonTerminator" << token_ << std::endl;
    for (const auto &sub_node: sub_nodes_) {
      sub_node->Print(oss, depth + 1);
    }
  }
}
}