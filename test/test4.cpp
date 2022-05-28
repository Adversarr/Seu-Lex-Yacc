//
// load from Lex File and Yacc File, and save as structural parameters
//
#include "sly/AttrDict.h"
#include "sly/FaModel.h"
#include "sly/LrParser.h"
#include "sly/RegEx.h"
#include "sly/Stream2TokenPipe.h"
#include <sly/sly.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>

using sly::core::type::AttrDict;
using sly::core::type::Production;
using sly::core::type::Token;
using sly::core::lexical::RegEx;
using sly::runtime::Stream2TokenPipe;
using sly::core::grammar::LrParser;
using sly::utils::replace_all;
using namespace std;


struct LexParms {
  struct LexDef {
    string name;
    string regex;
  };
  struct LexRule {
    string exp;
    string code;
  };

  // lexDef = {{name: "D", regex: "[0-9]"}, {name: "L", regex: "[a-zA-Z]"}, ...}
  vector<LexDef> lexDefs;
  // lexRule = {{exp: "0[0-7]*{IS}?", code: "{ count(); retur(CONSTANT) }"}, ...}
  vector<LexRule>  lexRules;
  // headCodeblock = "#include <stdio.h>\n#include "y.tab.h"\n..."
  string headCodeblock;
  // tailCodeblock = "int yywrap(void)\n{\n        return 1;\n}\n..."
  string tailCodeblock;

  void Print(ostream &oss) const;
};

struct YaccParms {
  struct YaccProd {
    string startToken;
    vector<string> nextTokens;
  };

  // yaccStartToken = "translation_unit"
  string yaccStartToken;
  // yaccTokens = {"IDENTIFIER", "CONSTANT", "SIZEOF", ...}
  vector<string> yaccTokens;
  // yaccProds = {{startToken: "and_expression", nextTokens: {"and_expression" "'&'", "equality_expression"}}}
  vector<YaccProd> yaccProds;
  // tailCodeblock = "#include <stdio.h>\n\nextern char yytext[]\n...";
  string tailCodeblock;

  void Print(ostream &oss) const;
};

struct Parms {
  struct lexToken {
    string regex;
    string action;
  };
  struct Prod {
    string startToken;
    vector<string> nextTokens;
  };
  vector<lexToken> lexTokens;
  string startToken;
  set<string> terminalTokens;
  set<string> nonTerminalTokens;
  vector<Prod> prods;

  void Print(ostream &oss) const;
};

void LexParms::Print(ostream &oss) const {
    oss << "Defs: " << endl;
    for (const auto &lexDef : lexDefs) {
      oss << "  " << lexDef.name << ": " << lexDef.regex << endl;
    }
    oss << "Rules: " << endl;
    for (const auto &lexRule : lexRules) {
      oss << "  " << lexRule.exp << ": " << endl;
      oss << "    " << lexRule.code << endl;
    }
    oss << "Head Codeblock: " << endl;
    oss << headCodeblock << endl;
    oss << "Tail Codeblock: " << endl;
    oss << tailCodeblock << endl;
  }

void YaccParms::Print(ostream &oss) const {
    oss << "Tokens: " << endl;
    for (const auto &token : yaccTokens) {
      oss << "  " << token << endl;
    }
    oss << "Start Token: " << endl;
    oss << "  " << yaccStartToken << endl;
    oss << "Production: "<< endl;
    for (const auto &prod : yaccProds) {
      oss << "  " << prod.startToken << ": " << endl;
      oss << "    ";
      for (const auto &nextToken : prod.nextTokens) {
        oss << nextToken << " ";
      }
      oss << endl;
    }
    oss << "Tail Codeblock: " << endl;
    oss << tailCodeblock << endl;
  }

void Parms::Print(ostream &oss) const {
  oss << "Lex Tokens: " << endl;
  for (const auto &[regex, action] : lexTokens) {
    oss << "  " << regex << ": " << endl;
    oss << "    " << action << endl;
  }
  oss << "Token Terminators: " << endl;
  for (const auto &token : terminalTokens) {
    oss << "  " << token << endl;
  }
  oss << "Token NonTerminators: " << endl;
  for (const auto &token : nonTerminalTokens) {
    oss << "  " << token << endl;
  }
  oss << "Start Token: " << endl;
    oss << "  " << startToken << endl;
  oss << "Productions: " << endl;
  for (const auto &[start, nexts] : prods) {
    oss << "  " << start << ": " << endl;
    oss << "    ";
    for (const auto &next : nexts) {
      oss << next << " ";
    }
    oss << endl;
  }
}

LexParms ParseLexParameters(stringstream &file_stream) {
  /**
   * .l file analysis: 
   * 
   * delim  %%[\r\n]
   * lbrace   %{[\r\n]
   * rbrace   %}[\r\n]
   * line     .*[\r\n]
   * 
   * LexFile <- Defs delim Rules delim Sub eof
   * Defs <- DefsLine Defs | lbrace Codes rbrace | 
   * Codes <- CodesLine Codes | 
   * CodesLine <- line {}
   * DefsLine <- line {word blank RegEx "\n"}
   * Rules <- RulesLine Rules | 
   * RulesLine <- line {LexRegEx blank CodePart "\n"}
   * Sub <- SubLine Sub | 
   * SubLine <- line {}
   * 
   **/
  LexParms lexParms;
  const auto ending = Token::Terminator("EOF_FLAG");

  static optional<Stream2TokenPipe> s2ppl;
  static optional<LrParser> parser;
  // initialize s2ppl and parser
  if (!s2ppl.has_value() || !parser.has_value()) {
    auto delim  = Token::Terminator("delim");
    auto lbrace = Token::Terminator("lbrace");
    auto rbrace = Token::Terminator("rbrace");
    auto line   = Token::Terminator("line");
    // auto ending = Token::Terminator("EOF_FLAG");
    RegEx re_delim("%%[\r\n]+");
    RegEx re_lbrace("%{[\r\n]+");
    RegEx re_rbrace("%}[\r\n]+");
    RegEx re_line(".*[\r\n]+");
    auto LexFile   = Token::NonTerminator("LexFile");
    auto Defs      = Token::NonTerminator("Defs");
    auto DefsLine  = Token::NonTerminator("DefsLine");
    auto Rules     = Token::NonTerminator("Rules");
    auto RulesLine = Token::NonTerminator("RulesLine");
    auto Sub       = Token::NonTerminator("Sub");
    auto SubLine   = Token::NonTerminator("SubLine");
    auto Codes     = Token::NonTerminator("Codes");
    auto CodesLine = Token::NonTerminator("CodesLine");
    vector<Production> productions = {
        // LexFile <- Defs delim Rules delim Sub
        Production(LexFile, {[](vector<YYSTATE> &v) {
          }})(Defs)(delim)(Rules)(delim)(Sub),
        // Defs <- DefsLine Defs
        Production(Defs, {[](vector<YYSTATE> &v) {
          }})(DefsLine)(Defs),
        // Defs <- lbrace Codes rbrace | 
        Production(Defs, {[](vector<YYSTATE> &v) {
          }})(lbrace)(Codes)(rbrace),
        // Defs <- 
        Production(Defs, {[](vector<YYSTATE> &v) {
          }}),
        // Codes <- CodesLine Codes | 
        Production(Codes, {[](vector<YYSTATE> &v) {
          }})(CodesLine)(Codes),
        // Codes <- CodesLine Codes | 
        Production(Codes, {[](vector<YYSTATE> &v) {
          }}),
        // CodesLine <- line {}
        Production(CodesLine, {[&lexParms](vector<YYSTATE> &v) {
            // CodesLine: copy code
            // TODO
            lexParms.headCodeblock += v[1].Get<string>("lval");
          }})(line),
        // DefsLine <- line
        Production(DefsLine, {[&lexParms](vector<YYSTATE> &v) {
            // DefsLine: word blank RegEx
            // TODO
            const string &str = v[1].Get<string>("lval");
            size_t pos = str.find_first_of(" \t\n\r\0");
            string defName = str.substr(0, pos);
            string defRegex = str.substr(pos + 1);
            sly::utils::trim(defName, " \t\n\r\0");
            sly::utils::trim(defRegex, " \t\n\r\0");
            lexParms.lexDefs.push_back({defName, defRegex});
          }})(line),
        // Rules <- RulesLine Rules
        Production(Rules, {[](vector<YYSTATE> &v) {
          }})(RulesLine)(Rules),
        // Rules <- 
        Production(Rules, {[](vector<YYSTATE> &v) {
          }}),
        // RulesLine <- line
        Production(RulesLine, {[&lexParms](vector<YYSTATE> &v) {
            // RulesLine: LexRegEx blank CodePart
            // TODO
            const string &str = v[1].Get<string>("lval");
            size_t pos = min(str.find(" {"), str.find("\t{"));
            string lexRegEx = str.substr(0, pos);
            string codePart = str.substr(pos + 1);
            sly::utils::trim(lexRegEx, " \t\n\r\0");
            sly::utils::trim(codePart, " \t\n\r\0");
            lexParms.lexRules.push_back({lexRegEx, codePart});
          }})(line),
        // Sub <- SubLine Sub
        Production(Sub, {[](vector<YYSTATE> &v) {
          }})(SubLine)(Sub),
         // Sub <- 
        Production(Sub, {[](vector<YYSTATE> &v) {
          }}),
        // SubLine <- line {}
        Production(SubLine, {[&lexParms](vector<YYSTATE> &v) {
            // Subline: copy code
            lexParms.tailCodeblock += v[1].Get<string>("lval");
          }})(line),
    };
    sly::core::grammar::ContextFreeGrammar cfg(productions, LexFile, ending);
    sly::core::grammar::Lr1 lr1;
    cfg.Compile(lr1);
    auto table = cfg.GetLrTable();
    parser = LrParser(table);
    // define transition and state
    auto [transition, state] = sly::core::lexical::DfaModel::Merge({
        re_delim.GetDfaModel(),
        re_lbrace.GetDfaModel(),
        re_rbrace.GetDfaModel(),
        re_line.GetDfaModel(),
    });
    s2ppl = sly::runtime::Stream2TokenPipe(transition, state, {
        delim, lbrace, rbrace, line, 
    }, ending);
  }

  // parse
  vector<AttrDict> attributes;
  vector<Token> tokens;
  file_stream << "\r\n";
  while (true) {
    auto token = s2ppl.value().Defer(file_stream);
    AttrDict ad;
    ad.Set("lval", s2ppl.value().buffer_); 
    tokens.emplace_back(token);
    attributes.emplace_back(ad);
    if (token == ending)
      break;
  }
  parser.value().Parse(tokens, attributes);
  auto tree = parser.value().GetTree();
  tree.Annotate();

  return lexParms;
}

YaccParms ParseYaccParameters(stringstream &file_stream) {
  /**
   * .y file analysis: 
   * 
   * delim       %%[\r\n]
   * line        .*[\r\n]
   * 
   * YaccFile <-  Defs delim ProdLines delim Sub eof
   * Defs <- DefsLine Defs | 
   * ProdLines <- ProdLine ProdLines | 
   * Sub <- SubLine Sub | 
   * SubLine <- line {}
   * 
   **/
  YaccParms yaccParms;
  const auto ending = Token::Terminator("EOF_FLAG");

  static optional<Stream2TokenPipe> s2ppl;
  static optional<LrParser> parser;
  // initialize s2ppl and parser
  if (!s2ppl.has_value() || !parser.has_value()) {
    // initialize s2ppl
    auto delim  = Token::Terminator("delim");
    auto line   = Token::Terminator("line");
    // auto ending = Token::Terminator("EOF_FLAG");
    RegEx re_delim("%%[\r\n]+");
    RegEx re_line(".*[\r\n]+");
    auto YaccFile  = Token::NonTerminator("YaccFile");
    auto Defs      = Token::NonTerminator("Defs");
    auto DefsLine  = Token::NonTerminator("DefsLine");
    auto Prods     = Token::NonTerminator("Prods");
    auto ProdLine  = Token::NonTerminator("ProdLine");
    auto Sub       = Token::NonTerminator("Sub");
    auto SubLine   = Token::NonTerminator("SubLine");
    vector<Production> productions = {
      // YaccFile <- Defs delim Prods delim Sub
      Production(YaccFile, {[&yaccParms](vector<YYSTATE> &v) {
          // parse Productions
          {
            stringstream ss;
            ss.str(v[3].Get<string>("lval"));

            int state = 0;
            string startToken;
            vector<string> nextTokens;

            string str;
            while (!ss.eof()) {
              ss >> str;
              if (str.length() == 0) {
                break;
              }
              switch (state) {
                case 0:
                  startToken = str;
                  state = 1;
                  break;
                case 1:
                  assert(str == ":");
                  state = 2;
                  break;
                case 2:
                  if (str == "|") {
                    yaccParms.yaccProds.push_back({startToken, nextTokens});
                    nextTokens.resize(0);
                    state = 2;
                  } else if (str == ";") {
                    yaccParms.yaccProds.push_back({startToken, nextTokens});
                    nextTokens.resize(0);
                    state = 0;
                  } else {
                    nextTokens.emplace_back(str);
                    state = 2;
                  }
                  break;
              }
            }
          }
        }})(Defs)(delim)(Prods)(delim)(Sub),
      // Defs <- DefsLine Defs
      Production(Defs, {[](vector<YYSTATE> &v) {
        }})(DefsLine)(Defs),
      // Defs <- 
      Production(Defs, {[](vector<YYSTATE> &v) {
        }}),
      // DefsLine <- line
      Production(DefsLine, {[&yaccParms](vector<YYSTATE> &v) {
          // parse symbol definitions
          // DefsLine: %token word word word ... / %start word
          const string &str = v[1].Get<string>("lval");
          stringstream ss;
          ss.str(str);

          string word;
          ss >> word;
          if (word == "%token") {
            while (true) {
              ss >> word;
              if (word.length() == 0 || ss.eof()) {
                break;
              }
              yaccParms.yaccTokens.emplace_back(word);
            }
          } else if (word == "%start") {
            ss >> word;
            yaccParms.yaccStartToken = word;
          }
        }})(line),
      // Prods <- ProdLine Prods
      Production(Prods, {[](vector<YYSTATE> &v) {
          string str = v[1].Get<string>("lval");
          str += v[2].Get<string>("lval");
          v[0].Set<string>("lval", str);
        }})(ProdLine)(Prods),
      // Prods <- 
      Production(Prods, {[](vector<YYSTATE> &v) {
        v[0].Set<string>("lval", "");
        }}),
      // ProdLine <- line
      Production(ProdLine, {[&yaccParms](vector<YYSTATE> &v) {
          v[0].Set<string>("lval", v[1].Get<string>("lval"));
        }})(line),
      // Sub <- SubLine Sub
      Production(Sub, {[](vector<YYSTATE> &v) {
        }})(SubLine)(Sub),
       // Sub <- 
      Production(Sub, {[](vector<YYSTATE> &v) {
        }}),
      // SubLine <- line {}
      Production(SubLine, {[&yaccParms](vector<YYSTATE> &v) {
          // Subline: copy code
          // TODO
          yaccParms.tailCodeblock += v[1].Get<string>("lval");
        }})(line),
    };
    sly::core::grammar::ContextFreeGrammar cfg(productions, YaccFile, ending);
    sly::core::grammar::Lr1 lr1;
    cfg.Compile(lr1);
    auto table = cfg.GetLrTable();
    parser = LrParser(table);

    auto [transition, state] = sly::core::lexical::DfaModel::Merge({
        re_delim.GetDfaModel(),
        re_line.GetDfaModel(),
    });
    s2ppl = sly::runtime::Stream2TokenPipe(transition, state, {
        delim, line, 
    }, ending);
  }

  // parse
  vector<AttrDict> attributes;
  vector<Token> tokens;
  file_stream << "\r\n";
  while (true) {
    auto token = s2ppl.value().Defer(file_stream);
    AttrDict ad;
    ad.Set("lval", s2ppl.value().buffer_); 
    tokens.emplace_back(token);
    attributes.emplace_back(ad);
    if (token == ending)
      break;
  }
  parser.value().Parse(tokens, attributes);
  auto tree = parser.value().GetTree();
  tree.Annotate();

  return yaccParms;
}

Parms ParseParameters(LexParms lexParms, YaccParms yaccParms) {
  Parms parms;
  auto lexRegex2regex = [](const string &exp) -> string {
    // headache, fuck lex regex
    // L?\"(\\.|[^\\"\n])*\"
    stringstream ss;
    bool isEscape = false;
    bool isLiteral = false;
    for (char c : exp) {
      if (isEscape) {
        if (c == '"') {
          ss << c;
        } else {
          ss << '\\' << c;
        }
        isEscape = false;
      } else if (c == '\\') {
        isEscape = true;
      } else if (c == '"') {
        ss << (isLiteral ? ")" : "(");
        isLiteral = !isLiteral;
      } else {
        if (isLiteral && (c == '(' || c == ')' || c == '[' || 
            c == ']' || c == '^' || c == '*' || c == '.' ||
            c == '+' || c == '|')) {
          ss << '\\' << c;
        } else {
          ss << c;
        }
      }
    }
    return ss.str();
  };
  // initialize lexTokens
  for (auto rule : lexParms.lexRules) {
    // transform lex-regex to regex
    rule.exp = lexRegex2regex(rule.exp);
    // replace {D} with its regex
    // inversely visit, to supoort recursive call
    for (auto it = lexParms.lexDefs.rbegin(); it != lexParms.lexDefs.rend(); it++) {
      auto &name = it->name;
      auto &regex = it->regex;
      replace_all(rule.exp, "{" + name + "}", regex);
    }
    parms.lexTokens.push_back({rule.exp, rule.code});
  }

  parms.startToken = yaccParms.yaccStartToken;
  // initialize terminalTokens, nonTerminalTokens
  parms.terminalTokens.insert(yaccParms.yaccStartToken);
  for (const string &tokenName : yaccParms.yaccTokens) {
    parms.terminalTokens.insert(tokenName);
  }
  for (const auto &[startToken, nextTokens] : yaccParms.yaccProds) {
    parms.terminalTokens.erase(startToken);
    parms.nonTerminalTokens.insert(startToken);
  }

  /// initialize prods
  for (const auto &[startToken, nextTokens] : yaccParms.yaccProds) {
    parms.prods.push_back({startToken, nextTokens});
  }

  return parms;
}

int main() {
  // ignore warnings
  sly::utils::Log::SetLogLevel(sly::utils::Log::kWarning);

  stringstream lex_file_stream;
  stringstream yacc_file_stream;
  {
    ifstream lexFile("./demo/c99.l");
    if (!lexFile.is_open()) {
      sly::utils::Log::GetGlobalLogger().Err("Cannot find c99.l");
      exit(-1);
    }
    lex_file_stream << lexFile.rdbuf();
    lexFile.close();

    ifstream yaccFile("./demo/c99.y");
    yacc_file_stream << yaccFile.rdbuf();
    if (!yaccFile.is_open()) {
      sly::utils::Log::GetGlobalLogger().Err("Cannot find c99.y");
      exit(-1);
    }
    yaccFile.close();
  }

  auto lexParms = ParseLexParameters(lex_file_stream);
  auto yaccParms = ParseYaccParameters(yacc_file_stream);

  // // print parameters
  // lexParms.Print(std::cout);
  // cout << endl;

  // yaccParms.Print(std::cout);
  // cout << endl;

  auto parms = ParseParameters(lexParms, yaccParms);
  parms.Print(std::cout);
  return 0;
}
