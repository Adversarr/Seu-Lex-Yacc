# Seu - Lex - Yacc (libsly)

几个模块：

1. core
   1. type
      1. Token -- 作为 LR 的识别符号和 Lex 产生的符号
      2. Production
      3. Action
      4. ContextFreeGrammar
      5. AttrDict 作为 Union 的替代品
   2. lexical
      1. NfaModel -- NFA 的 Model
      2. DfaModel
      3. RegEx -- 正则
   3. grammar
      1. TableGenerateMethod -- virtual base class
      2. Lr1
      3. Lalr
      4. ParsingTable
      5. LrParser
2. lex
   1. LexParser -- 处理 `.l` 文件
3. yacc
   1. YaccParser -- 处理 `.y` 文件
4. runtime
   1. yylval
   2. SeuLex
   3. SeuYacc

[//]: # (TODO: 还有很多)