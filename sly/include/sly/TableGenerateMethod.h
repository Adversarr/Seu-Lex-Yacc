//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_TABLEGENERATEMETHOD_H
#define SEULEXYACC_TABLEGENERATEMETHOD_H
#include "sly.h"
#include "Token.h"
#include <vector>


using sly::core::type::IdType;

using sly::core::type::Token;


namespace sly::core::grammar {
class ParsingTable
{
 public:

  enum AutomataAction{
    kAccept, kReduce, kShiftIn, kError, kEmpty
  };
  
  struct CellTp {
    AutomataAction action = kError;
    IdType id = 0;
    vector<IdType> cause;
  };
  
  explicit ParsingTable(int n_states=0);
  
  bool PutAction(IdType lhs, const Token& tok, CellTp action);
  
  void PutActionForce(IdType lhs, const Token& tok, CellTp action);
  
  bool PutGoto(IdType lhs, const Token& tok, IdType rhs);
  
  void PutGotoForce(IdType lhs, const Token& tok, IdType rhs);
  
  vector<CellTp> GetAction(IdType lhs, const Token& tok) const;
  
  vector<IdType> GetGoto(IdType lhs, const Token& tok) const;
  
  void Reset();
  
  void Print(ostream& os) const;
 
 private:
  
  vector<unordered_map<Token, vector<CellTp>, Token::TokenHash>> action_table_;
  
  vector<unordered_map<Token, vector<IdType>, Token::TokenHash>> goto_table_;
};



class TableGenerateMethod {
 public:
  virtual void Defer(const ContextFreeGrammar& cfg) = 0;
  
  const ParsingTable &GetParsingTable() const;
 
 protected:
  
  const ContextFreeGrammar* p_grammar = nullptr;
  
 private:
  ParsingTable parsing_table_;
};
}

#endif //SEULEXYACC_TABLEGENERATEMETHOD_H
