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

sly::core::lexical::RegEx any_string(".*");
sly::core::lexical::RegEx c_section_begin("%{");
sly::core::lexical::RegEx c_section_end("%}");
sly::core::lexical::RegEx c_section_inline_begin("{");
sly::core::lexical::RegEx c_section_inline_end("}");
sly::core::lexical::RegEx section_seperator(R"(%%)");
sly::core::lexical::RegEx blank("( |\t)");
sly::core::lexical::RegEx newline("\n");
sly::core::lexical::RegEx any_except_blank("[^ \t]*");
sly::core::lexical::RegEx any_except_newline("[^\n]*");
sly::core::lexical::RegEx letter("[a-zA-Z]");

auto any_string_token = Token::Terminator("any-str");
auto cpart = Token::Terminator("cpart");
auto c_sec_begin_token = Token::Terminator("csec-begin");
auto c_sec_end_token = Token::Terminator("csec-end");
auto c_sec_inline_begin_token = Token::Terminator("csec-begin-inline");
auto c_sec_inline_end_token = Token::Terminator("csec-end-inline");
auto sec_seperator = Token::Terminator("sec-seperator");
auto newline_token = Token::Terminator("new-line");
auto blank_token = Token::Terminator("blank");


auto regex_shorthand = Token::NonTerminator("re-shorthand");
std::vector<Production> productions = {
  Production()
};
int main() {
  return 0;
}