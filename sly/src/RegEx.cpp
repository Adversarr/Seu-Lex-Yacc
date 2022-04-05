//
// Created by Yang Jerry on 2022/3/30.
//

#include "sly/FaModel.h"
#include "sly/LrParser.h"
#include <optional>
#include <sly/AttrDict.h>
#include <sly/Production.h>
#include <sly/RegEx.h>
#include <sly/TableGenerateMethod.h>
#include <sly/Token.h>
#include <sstream>
#include <vector>

namespace sly::core::lexical {

sly::core::lexical::RegEx::RegEx(std::string expr, bool compile) : expr_(expr) {
  if (compile) {
    Compile();
  }
}

using sly::core::grammar::LrParser;
using sly::core::lexical::DfaModel;
using sly::core::lexical::NfaModel;
using sly::core::type::Action;
using sly::core::type::AttrDict;
using sly::core::type::Production;
using sly::core::type::Token;

auto epsilon = Token("epsilon", Token::Type::kEpsilon, 0);
auto lbrace = Token::Terminator("(");
auto rbrace = Token::Terminator(")");
auto lbracket = Token::Terminator("[");
auto rbracket = Token::Terminator("]");
auto ch = Token::Terminator("ch");
auto star = Token::Terminator("*");
auto dot = Token::Terminator(".");
auto slash = Token::Terminator("-");
auto pl = Token::Terminator("+");
auto dash = Token::Terminator("|", 0, Token::Attr::kLeftAssociative);
auto atom = Token::NonTerminator("atom");
auto item = Token::NonTerminator("item");
auto range = Token::NonTerminator("range");
auto range_content = Token::NonTerminator("range-content");
auto closure = Token::NonTerminator("closure");
auto non_empty_closure = Token::NonTerminator("non-empty-closure");
auto seq = Token::NonTerminator("seq");
auto eof_token = Token::Terminator("eof");

std::vector<Production> productions = {
    // An regex is a sequential string.
    Production(seq, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(item),
    Production(seq, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa") +
                                     v[3].Get<NfaModel>("nfa"));
               }))(seq)(dash)(seq),
    Production(seq, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa").Cascade(
                                     v[2].Get<NfaModel>("nfa")));
               }))(item)(seq),
    // an item can be an atom
    Production(item, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(atom),
    // an item can be a closure
    Production(item, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(closure),
    // closure: $*
    Production(closure, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa").ToClosure());
               }))(atom)(star),
    // an item can be an non empty_closure
    Production(item, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(non_empty_closure),
    // non-empty-closure: $+
    Production(non_empty_closure, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa").Cascade(
                                     v[1].Get<NfaModel>("nfa").ToClosure()));
               }))(atom)(pl),

    // an atom can be another sequential with ()
    Production(atom, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[2].Get<NfaModel>("nfa"));
               }))(lbrace)(seq)(lbrace),
    // an atom can be a char
    Production(atom, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(ch),
    // an atom can be a range
    Production(atom, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(range),
    // support for grammar: [a-zA-z]
    Production(range, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[2].Get<NfaModel>("nfa"));
               }))(lbracket)(range_content)(rbracket),
    Production(range_content, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa") +
                                     v[2].Get<NfaModel>("nfa"));
               }))(ch)(range_content),
    Production(range_content, Action([](std::vector<YYSTATE> &v) {
                 auto c_end = v[3].Get<char>("lval");
                 NfaModel nfa(c_end);
                 for (char c1 = v[1].Get<char>("lval"); c1 < c_end; ++c1) {
                   nfa = nfa.Combine(NfaModel(c1));
                 }
                 v[0].Set("nfa", nfa.Combine(v[4].Get<NfaModel>("nfa")));
               }))(ch)(slash)(ch)(range_content),
    Production(range_content, Action([](std::vector<YYSTATE> &v) {
                 v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
               }))(ch),
    Production(range_content, Action([](std::vector<YYSTATE> &v) {
                 auto c_end = v[3].Get<char>("lval");
                 NfaModel nfa(c_end);
                 for (char c1 = v[1].Get<char>("lval"); c1 < c_end; ++c1) {
                   nfa = nfa.Combine(NfaModel(c1));
                 }
                 v[0].Set("nfa", nfa);
               }))(ch)(slash)(ch)};

static std::optional<std::pair<Token, AttrDict>>
stream2token(std::istream &is) {
  if (is.eof() || !is.good() || is.bad() || is.fail()) {
    return {};
  }
  char c = is.get();
  if (c == '(') {
    return {{lbrace, AttrDict()}};
  } else if (c == ')') {
    return {{rbrace, {}}};
  } else if (c == '[') {
    return {{lbracket, {}}};
  } else if (c == ']') {
    return {{rbracket, {}}};
  } else if (c == '.') {
    return {{dot, {}}};
  } else if (c == '*') {
    return {{star, {}}};
  } else if (c == '|') {
    return {{dash, {}}};
  } else if (c == '-') {
    return {{slash, {}}};
  } else if (c == '+') {
    return {{pl, {}}};
  } else if (c <= 0) {
    return {{eof_token, {}}};
  } else {
    if (c == '\\') {
      if (is.eof() || !is.good() || is.bad() || is.fail()) {
        return {};
      }
      c = is.get();
    }
    AttrDict ad;
    NfaModel nfa(c);
    ad.Set<NfaModel>("nfa", nfa);
    ad.Set<char>("lval", c);
    return {{ch, ad}};
  }
  return {};
}

grammar::ParsingTable table = sly::core::grammar::ParsingTable(
    {
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 7},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 8},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 9},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kAccept,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 10},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 1},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 1},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 7},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 8},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 9},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 12},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 13},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 20},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 21},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 22},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 24},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 7},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 8},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 9},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 3},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 3},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 26},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 27},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 1},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 1},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 21},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 22},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 5},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 7},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 29},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 30},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 4},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 11},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 20},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 21},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 22},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 10},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 24},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 33},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 35},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 24},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 15},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 2},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 2},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 20},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 21},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 22},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 3},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 3},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 6},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 8},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 26},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 37},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 38},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 13},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 39},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 2},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 2},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 9},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 12},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kShiftIn,
                     .id = 24},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 16},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
        {
            {sly::core::type::Token("eof",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("-",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token(
                 "|", sly::core::type::Token::Type::kTerminator,
                 0,
                 sly::core::type::Token::Attr::kLeftAssociative),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("*",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("+",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("(",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("ch",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("[",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
            {sly::core::type::Token("]",
                                    sly::core::type::Token::Type::kTerminator,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kReduce,
                     .id = 14},
             }},
            {sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon,
                                    0, sly::core::type::Token::Attr::kNone),
             {
                 sly::core::grammar::ParsingTable::CellTp{
                     .action = sly::core::grammar::ParsingTable::
                         AutomataAction::kError,
                     .id = 0},
             }},
        },
    },
    {
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 1,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 2,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 3,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 4,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 5,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 6,
             }},
        },
        {},
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 11,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 2,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 3,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 4,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 5,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 6,
             }},
        },
        {},
        {},
        {},
        {},
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 14,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 15,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 16,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 17,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 18,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 19,
             }},
        },
        {},
        {
            {sly::core::type::Token(
                 "range-content", sly::core::type::Token::Type::kNonTerminator,
                 0, sly::core::type::Token::Attr::kNone),
             {
                 23,
             }},
        },
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 25,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 2,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 3,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 4,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 5,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 6,
             }},
        },
        {},
        {},
        {},
        {},
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 28,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 15,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 16,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 17,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 18,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 19,
             }},
        },
        {},
        {},
        {},
        {},
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 31,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 15,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 16,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 17,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 18,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 19,
             }},
        },
        {},
        {
            {sly::core::type::Token(
                 "range-content", sly::core::type::Token::Type::kNonTerminator,
                 0, sly::core::type::Token::Attr::kNone),
             {
                 32,
             }},
        },
        {},
        {
            {sly::core::type::Token(
                 "range-content", sly::core::type::Token::Type::kNonTerminator,
                 0, sly::core::type::Token::Attr::kNone),
             {
                 34,
             }},
        },
        {},
        {
            {sly::core::type::Token(
                 "seq", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 36,
             }},
            {sly::core::type::Token(
                 "item", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 15,
             }},
            {sly::core::type::Token(
                 "closure", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 16,
             }},
            {sly::core::type::Token(
                 "non-empty-closure",
                 sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 17,
             }},
            {sly::core::type::Token(
                 "atom", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 18,
             }},
            {sly::core::type::Token(
                 "range", sly::core::type::Token::Type::kNonTerminator, 0,
                 sly::core::type::Token::Attr::kNone),
             {
                 19,
             }},
        },
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {
            {sly::core::type::Token(
                 "range-content", sly::core::type::Token::Type::kNonTerminator,
                 0, sly::core::type::Token::Attr::kNone),
             {
                 40,
             }},
        },
        {},
    },
    productions,
    sly::core::type::Token("seq", sly::core::type::Token::Type::kNonTerminator,
                           0, sly::core::type::Token::Attr::kNone),
    sly::core::type::Token("#LR augmented#",
                           sly::core::type::Token::Type::kNonTerminator, 0,
                           sly::core::type::Token::Attr::kNone),
    sly::core::type::Token("", sly::core::type::Token::Type::kEpsilon, 0,
                           sly::core::type::Token::Attr::kNone));


void RegEx::Compile() {
  std::istringstream is(expr_); 
  LrParser parser(table);
  vector<Token> token_input;
  vector<AttrDict> ad_input;
  for (;;) {
    auto token = stream2token(is);
    if (token.has_value()) {
      sly::utils::Log::GetGlobalLogger().Info("Caught ", token.value().first);
      token_input.push_back(token.value().first);
      ad_input.push_back(token.value().second);
    } else {
      sly::utils::Log::GetGlobalLogger().Info("Done");
      token_input.push_back(epsilon);
      ad_input.push_back({});
      break;
    }
  }
  parser.Parse(token_input, ad_input);
  auto tree = parser.GetTree();
  tree.Annotate();
  auto nfa = tree.GetRootAttributes()[0].Get<NfaModel>("nfa");
  *nfa_model_ = nfa;
  *dfa_model_ = DfaModel(nfa);
}

} // namespace sly::core::lexical
