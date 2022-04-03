//
// Created by Yang Jerry on 2022/3/30.
//

#include <sly/Token.h>
#include <sly/utils.h>

namespace sly::core::type {


std::size_t Token::Hash::operator()(const Token &k) const
{
  return sly::utils::hash(k.GetTokName(), k.GetTokenType());
}

bool Token::operator==(const Token &another) const
{
  return type_ == another.type_ &&
         name_ == another.name_;
}

bool Token::IsTerminator() const
{
  return type_ == kTerminator;
}

Token::Token(string tok_name, Type tok_type, IdType tid) :
  name_(std::move(tok_name)), tid_(tid), type_(tok_type)
{

}

Token Token::NonTerminator(string tok_name, IdType tid)
{
  return Token(std::move(tok_name), kNonTerminator, tid);
}

Token Token::Terminator(string tok_name, IdType tid)
{
  return Token(std::move(tok_name), kTerminator, tid);
}

Token::Token()
{
  type_ = kEpsilon;
  attr_ = kNone;
}


const string &Token::GetTokName() const
{
  return name_;
}

Token::Type Token::GetTokenType() const
{
  return type_;
}

IdType Token::GetTid() const
{
  return tid_;
}

bool Token::operator<(const Token &rhs) const
{
  if (name_ < rhs.name_)
    return true;
  if (type_ < rhs.type_)
    return true;
  return false;
}

Token::Attr Token::GetAttr() const
{
  return attr_;
}

void Token::SetAttr(Token::Attr attribute)
{
  Token::attr_ = attribute;
}


ostream &operator<<(ostream &os, const Token &tok)
{
  switch (tok.GetTokenType())
  {
    case Token::kNonTerminator:
      os << "N<" + tok.GetTokName() + ">";
      break;
    case Token::kTerminator:
      os << "T<" + tok.GetTokName() + ">";
      break;
    case Token::kEpsilon:
      os << "E< >";
      break;
  }
  return os;
}

}