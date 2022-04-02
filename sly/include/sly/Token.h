//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_TOKEN_H
#define SEULEXYACC_TOKEN_H
#include <string>
#include "sly.h"

using namespace std;


namespace sly::core::type {
/**
 * (LR 自动机中) Token 类
 */
class Token {
 public:
  enum Type {
    kTerminator, kNonTerminator, kEpsilon
  };
  
  // Token hash
  struct Hash {
    std::size_t operator()(const Token &k) const;
  };
  
  enum Attr {
    kLeftAssociative, kRightAssociative, kNone
  };
  
  bool operator==(const Token &another) const;
  
  bool IsTerminator() const;
  
  const string &GetTokName() const;
  
  Type GetTokenType() const;
  
  Token();
  
  static Token NonTerminator(string tok_name, IdType tid = -1);
  
  static Token Terminator(string tok_name, IdType tid = -1);
 
 public:
  IdType GetTid() const;
  
  Attr GetAttr() const;
  
  void SetAttr(Attr attribute);
  
  bool operator<(const Token &rhs) const;
 
 private:
  explicit Token(string tok_name, Type tok_type, IdType tid);
 
 private:
  IdType tid_{};
  
  Attr attr_ = kNone;
  
  std::string name_;
  
  Type type_;
};

ostream &operator<<(ostream &os, const Token &tok);

}
#endif //SEULEXYACC_TOKEN_H
