//
// Created by Yang Jerry on 2022/3/30.
//

#ifndef SEULEXYACC_FAMODEL_H
#define SEULEXYACC_FAMODEL_H
#include "sly.h"
#include <vector>
#include <set>

using namespace std;

namespace sly::core::lexical {

using AutomataState = bool;

static constexpr AutomataState Accept = true;

static constexpr AutomataState Decline = false;



class NfaModel
{
 public:
  /**
   * 利用 current_state 推导一步得到的 NFA AutomataState
   * @param current_state
   * @param opt_c
   * @return
   */
  set<int> Defer(const set<int> &current_state, optional<char> opt_c) const;
  
  const set<char> &GetCharset() const;
  
  /**
   * 返回入口节点编号
   * @return
   */
  set<int> GetEntry() const;
  
  
  /**
   * 求出给定集合的Epsilon闭包
   * @param s
   * @return
   */
  set<int> GetEpsilonClosure(const set<int> &s) const;
  
  
  /**
   * 返回 accepting state
   * @return
   */
  vector<int> GetAcceptState() const;
  
  /**
   * 闭包
   * @return
   */
  NfaModel ToClosure() const;
  
  /**
   * 串联
   * @param another
   * @return
   */
  NfaModel Cascade(const NfaModel &another) const;
  
  /**
   * 并联
   * @param another
   * @return
   */
  NfaModel Combine(const NfaModel &another) const;


  
  /**
   * 创建一个接受单个字符 opt_c 的自动机
   * @param opt_c
   */
  explicit NfaModel(optional<char> opt_c);
  
  explicit NfaModel(set<char> &&charset, set<int> &&entries, vector<AutomataState> &&states,
                    vector<map<char, set<int>>> &&transition);
  
  NfaModel(const NfaModel &) = default;
  
  NfaModel(NfaModel &&) = default;
 
 private:
  
  // charset
  set<char> charset_;
  
  // 入口编号
  set<int> entries_;
  
  // 状态描述 规定 0 为Entry
  vector<AutomataState> states_;
  
  // 状态转移矩阵
  vector<map<char, set<int>>> transition_;
  
};

class DfaModel
{
 public:
  static constexpr int entry_ = DFA_ENTRY_STATE_ID;
  
  explicit DfaModel(NfaModel &nfa);
 
 public: // Getters
  const set<char> &GetCharset() const;
  
  const vector<AutomataState> &GetStates() const;
  
  const vector<map<char, int>> &GetTransition() const;
  
  optional<int> Defer(int sid, char c) const;
 
 private:
  
  // charset
  set<char> charset_;
  
  // 状态描述 规定 0 为Entry
  vector<AutomataState> states_;
  
  // 状态转移矩阵
  vector<map<char, int>> transition_;
};


class DfaController
{
 public:
  explicit DfaController(DfaModel model);
  
  DfaController Defer(char c) const;
  
  AutomataState CanAccept() const;
 
 private:
  DfaController(shared_ptr<DfaModel> p_dfa, optional<int> current_state);
  
  shared_ptr<DfaModel> p_dfa_;
  
  optional<int> current_state_{};
};


NfaModel operator +(const NfaModel& na, const NfaModel& nb);


NfaModel operator *(const NfaModel& na, const NfaModel& nb);


NfaModel operator ++(const NfaModel& na);


}


#endif //SEULEXYACC_FAMODEL_H
