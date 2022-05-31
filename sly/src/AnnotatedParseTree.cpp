//
// Created by Yang Jerry on 2022/3/30.
//

#include <sly/AnnotatedParseTree.h>

#include <iomanip>
#include <sstream>
#include <utility>

namespace sly::core::type {

AnnotatedParseTree::AnnotatedParseTree(Token token, AttrDict attr): 
  is_annotated_(true), type_(Type::kTerminator),
  token_(std::move(token)), attrs_({std::move(attr)})
{
}

AnnotatedParseTree::AnnotatedParseTree(const Production& prod):
  is_annotated_(false), type_(Type::kNonTerminator), actions_(prod.GetActions()), token_(prod.GetTokens().front())
{
}


const vector<AttrDict> &sly::core::type::AnnotatedParseTree::GetRootAttributes() const {
  return attrs_;
}


void AnnotatedParseTree::Annotate(AnnotatedParseTree* p_father) {
  if (type_ == Type::kTerminator) { // leaf node, do nothing and return.
    return;
  }
  
  is_annotated_ = false;
  attrs_.clear();
  attrs_.emplace_back();
  for (int i = 1; i < actions_.size(); ++i) {
    auto p = sub_nodes_[i - 1];
    if (!(p->is_annotated_)) {
      p->Annotate(this);
    }
    if (!(p->is_annotated_)) {
      if (p_father == nullptr) {
        throw runtime_error("Cannot annotate.");
      }
      return;
    }
    attrs_.emplace_back(p->attrs_.front());
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


void AnnotatedParseTree::EmplaceFront(AnnotatedParseTree &&tree) {
  sub_nodes_.emplace_front(make_shared<AnnotatedParseTree>(tree));
}

string AnnotatedParseTree::ToString() const {
  stringstream ss;
  Print(ss);
  return "AnnontatedParseTree{" + ss.str() + "}";
}

void AnnotatedParseTree::Print(ostream &oss, int depth) const{
  if (depth > 0) {
    oss << std::setw(depth * 2) << "* ";
  }
  oss << token_ << "<";
  if (!attrs_.empty()) {
    auto dict = attrs_[0].ToStrDict();
    for (const auto&[k, v]: dict) {
      if (typeid(v) == typeid(std::string)) {
        oss << k << ":" << sly::utils::escape(v) << "/";
      } else {
        oss << k << ":" << v << "/";
      }
    }
  }
  oss << ">";
  oss << std::endl;
  if (type_ == Type::kNonTerminator) {
    for (const auto &sub_node: sub_nodes_) {
      sub_node->Print(oss, depth + 1);
    }
  }
}
}
