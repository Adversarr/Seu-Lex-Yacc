//
// Created by Yang Jerry on 2022/3/30.
//
#include <sly/sly.h>
#include <sly/Production.h>

namespace sly::core::type {
bool Production::CheckHealth() const {
  if (tokens_.empty())
    return false;
  
  if (tokens_.front().IsTerminator())
    return false;
  
  if (tokens_.size() != actions_.size())
    return false;
  
  return true;
}

Production::Production(std::vector<Token> tokens, std::vector<Action> actions) :
  tokens_(std::move(tokens)), actions_(std::move(actions)) {
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "performance-unnecessary-value-param"

Production Production::operator()(Token next_token) const {
  auto clone = *this;
  clone.tokens_.push_back(next_token);
  clone.actions_.emplace_back();
  return clone;
}

Production Production::operator()(Token next_token, Action next_action) const {
  auto clone = *this;
  clone.actions_.emplace_back(next_action);
  clone.tokens_.emplace_back(next_token);
  return clone;
}

bool Production::operator==(const Production &rhs) const {
  return tokens_ == rhs.tokens_ && actions_ == rhs.actions_;
}

Production::Production(Token start) :
  tokens_{start}, actions_{Action()} {
  
}

Production::Production(Token start, Action act) :
  tokens_{start}, actions_{act} {
  
}

vector<Token> &Production::GetTokens() {
  return tokens_;
}

vector<Action> &Production::GetActions() {
  return actions_;
}

const vector<Token> &Production::GetTokens() const {
  return cref(tokens_);
}

bool Production::IsEpsilon() const {
  return tokens_.size() == 1;
}

Production::Production(): tokens_({Token::NonTerminator("EMPTY")}) {
}

ostream &operator<<(ostream &os, const Production &prod)
{
  os << "Prod[" << prod.GetTokens().front() << " -> ";
  copy(prod.GetTokens().cbegin() + 1, prod.GetTokens().cend(),
       ostream_iterator<Token>(os, ", "));
  os << "]";
  return os;
}


}