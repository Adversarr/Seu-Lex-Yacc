//
// Created by Yang Jerry on 2022/3/30.
//
#include <cstddef>


#ifndef SEULEXYACC_LY_H
#define SEULEXYACC_LY_H

#ifndef EPSILON
#define EPSILON ((char) 0xFF)
#endif

#ifndef DFA_ENTRY_STATE_ID
#define DFA_ENTRY_STATE_ID 0
#endif

#ifndef YYSTATE
#define YYSTATE sly::core::type::AttrDict
#endif

#define INT 258
#define FLOAT 259
#define NAME 260
#define STRUCT 261
#define IF 262
#define ELSE 263
#define RETURN 264
#define NUMBER 265
#define LPAR 266
#define RPAR 267
#define LBRACE 268
#define RBRACE 269
#define LBRACK 270
#define RBRACK 271
#define ASSIGN 272
#define SEMICOLON 273
#define COMMA 274
#define DOT 275
#define PLUS 276
#define MINUS 277
#define TIMES 278
#define DIVIDE 279
#define EQUAL 280
#define LOW 281
#define UMINUS 282

namespace sly {
namespace core {

namespace type {

using IdType = size_t;
// 替代 Union
class AttrDict;

using TokenAttr = YYSTATE;

class Token;

class Production;

class Action;

class AnnotatedParseTree;

}

namespace lexical {

class NfaModel;

class DfaModel;

class RegEx;
}

namespace grammar {
class ContextFreeGrammar;

class ParsingTable;

class TableGenerateMethod;

class Lr1;

class Lalr;

/**
 * 给定 `ParsingTable` 和 stream<Token> 进行识别
 */
class LrParser;

}
}

namespace utils {

class File;

}

namespace lex {

class LexParser;



}
namespace yacc {

class YaccParser;


}

namespace runtime {

extern YYSTATE yylval;

class SeuLex;

class SeuYacc;

}
}

#endif //SEULEXYACC_LY_H
