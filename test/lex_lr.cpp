/**
 * @file lex_lr.cpp
 * @author Jerry Yang (213190186@seu.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2022-04-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#include "sly/Production.h"
#include "sly/RegEx.h"
#include <iostream>
#include <sly/sly.h>
#include <vector>

using namespace sly::core::type;
using namespace sly::core::grammar;
using namespace std;



auto regex_shorthand = Token::NonTerminator("re-shorthand");
std::vector<Production> productions = {
  Production()
};
int main() {
  sly::core::lexical::RegEx any_string("[0-9]+([Ee][+-]?[0-9]+)(f|F|l|L)?");
  any_string.Compile();
  cout << any_string.CanMatch("1") << endl;
  return 0;
}