//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_SEULEX_H
#define SEULEXYACC_SEULEX_H
#include "def.h"
#include <array>
#include <istream>
#include <ostream>
#include <vector>
#include "AttrDict.h"
#include "Token.h"


namespace sly::runtime {

// Run and return id.
class SeuLex {
 public:
  using Token = core::type::Token;

  void Process();

  virtual int yylex();

  virtual int yywrap();

 private:
  std::vector<std::array<int, 128>> table_;
  
  std::vector<Token> tokens_;
  
  YYSTATE yylval;

  std::istream* yyin;

  std::ostream* yyout;

};

}

#endif //SEULEXYACC_SEULEX_H
