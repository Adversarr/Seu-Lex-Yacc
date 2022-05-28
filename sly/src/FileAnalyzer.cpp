
#include "sly/FaModel.h"
#include "sly/LrParser.h"
#include <array>
#include <cstddef>
#include <optional>
#include <sly/AttrDict.h>
#include <sly/Production.h>
#include <sly/RegEx.h>
#include <sly/FileAnalyzer.h>
#include <sly/TableGenerateMethod.h>
#include <sly/TableGenerateMethodImpl.h>

#include <sly/Token.h>
#include <sstream>
#include <vector>

namespace sly::core::lexical {

const DfaModel& RegEx::GetDfaModel(){
  Compile();
  return dfa_model_;
}

DfaModel re2dfa(string expr_);

sly::core::lexical::RegEx::RegEx(std::string expr, bool compile)
    : expr_(expr), compiled(false) {}

bool RegEx::CanMatch(std::string str) {
  Compile();
  DfaController dc(dfa_model_);
  for (auto c : str) {
    dc.Defer(c);
  }
  return dc.CanAccept();
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
auto anti = Token::Terminator("^");
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
    Production(seq,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
                   },
                   "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\"));"))(item),
    Production(seq, Action(
                        [](std::vector<YYSTATE> &v) {
                          v[0].Set("nfa", v[1].Get<NfaModel>("nfa") +
                                              v[3].Get<NfaModel>("nfa"));
                        },
                        "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\") + "
                        "v[3].Get<NfaModel>(\"nfa\"));"))(seq)(dash)(seq),
    Production(seq, Action(
                        [](std::vector<YYSTATE> &v) {
                          v[0].Set("nfa", v[1].Get<NfaModel>("nfa").Cascade(
                                              v[2].Get<NfaModel>("nfa")));
                        },
                        "v[0].Set(\"nfa\", "
                        "v[1].Get<NfaModel>(\"nfa\").Cascade(v[2].Get<NfaModel>"
                        "(\"nfa\")));"))(item)(seq),
    // an item can be an atom
    Production(item, Action(
                         [](std::vector<YYSTATE> &v) {
                           v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
                         },
                         "v[0].Set(\"nfa\", "
                         "v[1].Get<NfaModel>(\"nfa\").Cascade(v[2].Get<"
                         "NfaModel>(\"nfa\")));"))(atom),
    // an item can be a closure
    Production(item,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
                   },
                   "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\"));"))(closure),
    // closure: $*
    Production(
        closure,
        Action(
            [](std::vector<YYSTATE> &v) {
              v[0].Set("nfa", v[1].Get<NfaModel>("nfa").ToClosure());
            },
            "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\").ToClosure());"))(
        atom)(star),
    // an item can be an non empty_closure
    Production(item, Action(
                         [](std::vector<YYSTATE> &v) {
                           v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
                         },
                         "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\"));"))(
        non_empty_closure),
    // non-empty-closure: $+
    Production(non_empty_closure,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     v[0].Set("nfa", v[1].Get<NfaModel>("nfa").Cascade(v[1].Get<NfaModel>("nfa").ToClosure()));
                   },
                   "v[0].Set(\"nfa\", "
                   "v[1].Get<NfaModel>(\"nfa\").Cascade(v[1].Get<NfaModel>("
                   "\"nfa\").ToClosure()));"))(atom)(pl),

    // an atom can be another sequential with ()
    Production(atom, Action(
                         [](std::vector<YYSTATE> &v) {
                           v[0].Set("nfa", v[2].Get<NfaModel>("nfa"));
                         },
                         "v[0].Set(\"nfa\", v[2].Get<NfaModel>(\"nfa\"));"))(
        lbrace)(seq)(rbrace),
    // an atom can be a char
    Production(atom,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
                   },
                   "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\"));"))(ch),
    // an atom can be a range
    Production(atom,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     v[0].Set("nfa", v[1].Get<NfaModel>("nfa"));
                   },
                   "v[0].Set(\"nfa\", v[1].Get<NfaModel>(\"nfa\"));"))(range),
    // support for grammar: [a-zA-z]
    Production(range, Action(
                          [](std::vector<YYSTATE> &v) {
                            auto e = v[2].Get<set<char>>("cs");
                            NfaModel nfa(e);
                            v[0].Set("nfa", nfa);
                          },
                          R"(auto e = v[2].Get<set<char>>("cs");
                            NfaModel nfa(e);
                            v[0].Set("nfa", nfa);)"))(
        lbracket)(range_content)(rbracket),
    // support for grammar: [^a-zA-z]
    Production(range, Action(
                          [](std::vector<YYSTATE> &v) {
                            set<char> cs;
                            auto e = v[3].Get<set<char>>("cs");
                            for (char c = 0; c <= 127 && c >= 0; ++c) {
                              if (e.find(c) == e.end())
                                cs.emplace(c);
                            }
                            NfaModel nfa(cs);
                            v[0].Set("nfa", nfa);
                          },
                         R"(set<char> cs;
                            auto e = v[3].Get<set<char>>("cs");
                            for (char c = 0; c <= 127 && c >= 0; ++c) {
                              if (e.find(c) == e.end())
                                cs.emplace(c);
                            }
                            NfaModel nfa(cs);
                            v[0].Set("nfa", nfa);)"))(
        lbracket)(anti)(range_content)(rbracket),
    Production(range_content,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     set<char> cs{v[1].Get<char>("lval")};
                     for (const auto &c : v[2].Get<set<char>>("cs")) {
                       cs.insert(c);
                     }
                     v[0].Set("cs", cs);
                   },
                   R"(set<char> cs{v[1].Get<char>("lval")};
                     for (const auto &c : v[2].Get<set<char>>("cs")) {
                       cs.insert(c);
                     }
                     v[0].Set("cs", cs);)"))(ch)(range_content),
    Production(
        range_content,
        Action(
            [](std::vector<YYSTATE> &v) {
              auto c_end = v[3].Get<char>("lval");
              auto cs = v[4].Get<set<char>>("cs");
              for (char c = v[1].Get<char>("lval"); c <= c_end; ++c) {
                cs.insert(c);
              }
              v[0].Set("cs", cs);
            },
            R"(auto c_end = v[3].Get<char>("lval");
              auto cs = v[4].Get<set<char>>("cs");
              for (char c = v[1].Get<char>("lval"); c <= c_end; ++c) {
                cs.insert(c);
              }
              v[0].Set("cs", cs);)"))
      (ch)(slash)(ch)(range_content),
    Production(range_content,
               Action(
                   [](std::vector<YYSTATE> &v) {
                     v[0].Set("cs", set<char>{v[1].Get<char>("lval")});
                   },
                   R"(v[0].Set("cs", set<char>{v[1].Get<char>("lval")});)"))(ch),
    Production(
        range_content,
        Action(
            [](std::vector<YYSTATE> &v) {
              auto c_end = v[3].Get<char>("lval");
              set<char> cs;
              for (char c = v[1].Get<char>("lval"); c <= c_end; ++c) {
                cs.insert(c);
              }
              v[0].Set("cs", cs);
            },
            "auto c_end = v[3].Get<char>(\"lval\");"
            "NfaModel nfa(c_end);"
            "for (char c1 = v[1].Get<char>(\"lval\"); c1 < c_end; ++c1) {"
            "nfa = nfa.Combine(NfaModel(c1));"
            "}"
            "v[0].Set(\"nfa\", nfa);"))(ch)(slash)(ch),
    Production(atom, Action{[](std::vector<YYSTATE>& v){
      set<char> cs;
      for (char c = 1; c < static_cast<char>(127); ++c) {
        if (c == '\n' || c == '\r') continue;
        cs.emplace(c);
      }
      v[0].Set("nfa", NfaModel(cs));
    }, R"([](std::vector<YYSTATE>& v){
      set<char> cs;
      for (char c = 1; c < static_cast<char>(127); ++c) {
        cs.emplace(c);
      }
      v[0].Set("nfa", NfaModel(cs));)"})(dot)};

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
  } else if (c == '^') {
    return {{anti, {}}};
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
      // special escape character: \t, \v, \n, \f, \r
      if (c == 't') {
        c = '\t';
      } else if (c == 'v') {
        c = '\v';
      } else if (c == 'n') {
        c = '\n';
      } else if (c == 'f') {
        c = '\f';
      } else if (c == 'r') {
        c = '\r';
      }
    }
    AttrDict ad;
    NfaModel nfa(c);
    ad.Set<NfaModel>("nfa", nfa);
    ad.Set<char>("lval", c);
    return {{ch, ad}};
  }
  return {};
}


void RegEx::Compile() {
  if (!compiled){
    dfa_model_ = re2dfa(expr_);
    compiled = true;
  }
}

DfaModel re2dfa(string expr_) {
  std::istringstream is(expr_);
  static optional<grammar::ParsingTable> opt;
  if (!opt.has_value()) {
    auto level = sly::utils::Log::GetGlobalLogger().level;
    sly::utils::Log::SetLogLevel(utils::Log::kNone);
    sly::core::grammar::ContextFreeGrammar grammar{productions, seq, eof_token};
    sly::core::grammar::Lr1 lr1;
    grammar.Compile(lr1);
    opt = make_optional(move(lr1.GetParsingTable()));
    sly::utils::Log::SetLogLevel(level);
  }
  LrParser parser(opt.value());
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
  return DfaModel(nfa);
}

} // namespace sly::core::lexical