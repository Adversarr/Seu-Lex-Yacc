//
// Created by Yang Jerry on 2022/3/30.
//

#include "sly/Production.h"
#include "spdlog/spdlog.h"
#include <sly/TableGenerateMethod.h>
#include <sly/def.h>
#include <sly/utils.h>
namespace sly::core::grammar {

ParsingTable::ParsingTable(int n_states) {
  for (int i = 0; i < n_states; ++i) {
    action_table_.emplace_back();
    goto_table_.emplace_back();
  }
}

void ParsingTable::Reset() {
  action_table_.clear();
  goto_table_.clear();
}

bool ParsingTable::PutAction(IdType lhs, const Token &tok,
                             ParsingTable::CellTp action) {
  auto f = action_table_[lhs].find(tok);
  if (f != action_table_[lhs].end()) {
    if (action.action == kAccept)
      f->second = {action};
    else
      f->second.push_back(action);
  }
  action_table_[lhs].insert({tok, {action}});

  string acts;
  if (action.action == AutomataAction::kEmpty)
    acts = "?";
  else if (action.action == AutomataAction::kShiftIn)
    acts = "Sft";
  else if (action.action == AutomataAction::kReduce)
    acts = "Red";
  else if (action.action == AutomataAction::kAccept)
    acts = "Acc";
  else
    acts = "Err";
  spdlog::debug("Putting ACTION: StateID: {} Token: {}\t Action: {} {}", lhs,
                tok.ToString(), acts, action.id);

  return true;
}

void ParsingTable::PutActionForce(IdType lhs, const Token &tok, CellTp action) {
  action_table_[lhs][tok] = {std::move(action)};
}

bool ParsingTable::PutGoto(IdType lhs, const Token &tok, IdType rhs) {
  auto f = goto_table_[lhs].find(tok);
  if (f != goto_table_[lhs].end())
    f->second.push_back(rhs);
  goto_table_[lhs].insert({tok, {rhs}});
  spdlog::debug("Putting  GOTO : From: {} To: {} \t Token{}", lhs, rhs,
                utils::ToString{}(tok));

  return true;
}

void ParsingTable::PutGotoForce(IdType lhs, const Token &tok, IdType rhs) {
  goto_table_[lhs][tok] = {rhs};
}

vector<ParsingTable::CellTp> ParsingTable::GetAction(IdType lhs,
                                                     const Token &tok) const {
  auto f = action_table_[lhs].find(tok);
  if (f != action_table_[lhs].end())
    return f->second;
  else
    return {};
}

vector<IdType> ParsingTable::GetGoto(IdType lhs, const Token &tok) const {
  auto f = goto_table_[lhs].find(tok);
  if (f != goto_table_[lhs].end())
    return f->second;
  else
    return {};
}

void ParsingTable::Print(ostream &os) const {
  os << "sly::core::grammar::ParsingTable(";
  // print out action table:
  os << "{";
  for (const auto &line : action_table_) {
    os << "{";
    for (auto [token, cell] : line) {
      os << "{";
      // print token:
      // Token(token.GetTokName(), token.GetTokenType(), token.GetTid(),
      // token.GetAttr());
      token.PrintImpl(os);
      // Print comma.
      os << ", {";
      // print cell:
      for (const auto &c : cell) {
        os << c << ",";
      }
      os << "}},";
    }
    os << "}, ";
  }
  os << "}, ";
  // print out goto
  os << "{";
  for (const auto &line : goto_table_) {
    os << "{";
    for (const auto &[token, go] : line) {
      os << "{";
      token.PrintImpl(os);
      os << ",{";
      for (auto v : go) {
        os << v << ",";
      }
      os << "}},";
    }
    os << "},";
  }
  os << "}, productions,";

  // several tokens
  entry_token_.PrintImpl(os);
  os << ",";
  augmented_token_.PrintImpl(os);
  os << ",";
  epsilon_token_.PrintImpl(os);
  os << ")";
}

const vector<unordered_map<Token, vector<ParsingTable::CellTp>, Token::Hash>> &
ParsingTable::GetActionTable() const {
  return action_table_;
}

void ParsingTable::SetActionTable(
    const vector<unordered_map<Token, vector<CellTp>, Token::Hash>>
        &action_table) {
  action_table_ = action_table;
}

const vector<unordered_map<Token, vector<IdType>, Token::Hash>> &
ParsingTable::GetGotoTable() const {
  return goto_table_;
}

void ParsingTable::SetGotoTable(
    const vector<unordered_map<Token, vector<IdType>, Token::Hash>>
        &goto_table) {
  goto_table_ = goto_table;
}

const Token &ParsingTable::GetEntryToken() const { return entry_token_; }

void ParsingTable::SetEntryToken(const Token &entry_token) {
  entry_token_ = entry_token;
}

const Token &ParsingTable::GetAugmentedToken() const {
  return augmented_token_;
}

void ParsingTable::SetAugmentedToken(const Token &augmented_token) {
  augmented_token_ = augmented_token;
}

const Token &ParsingTable::GetEndingToken() const { return ending_token_; }

void ParsingTable::SetEndingToken(const Token &ending_token) {
  ending_token_ = ending_token;
}

const Token &ParsingTable::GetEpsilonToken() const { return epsilon_token_; }

void ParsingTable::SetEpsilonToken(const Token &epsilon_token) {
  epsilon_token_ = epsilon_token;
}

const vector<type::Production> &ParsingTable::GetProductions() const {
  return productions_;
}

void ParsingTable::SetProductions(const vector<type::Production> &productions) {
  productions_ = productions;
}

const ParsingTable &TableGenerateMethod::GetParsingTable() const {
  return lr_table_;
}

ParsingTable ParsingTable::FromRaw(
    std::vector<std::unordered_map<Token, std::vector<CellTp>, Token::Hash>>
        action_table,
    std::vector<std::unordered_map<Token, std::vector<IdType>, Token::Hash>>
        goto_table,
    vector<type::Production> productions, Token entry_token,
    Token augmented_token, Token epsilon_token) {
  ParsingTable pt;
  pt.SetProductions(productions);
  pt.SetActionTable(action_table);
  pt.SetGotoTable(goto_table);
  pt.SetEntryToken(entry_token);
  pt.SetAugmentedToken(augmented_token);
  pt.SetEpsilonToken(epsilon_token);
  return pt;
}

ParsingTable::ParsingTable(
    std::vector<std::unordered_map<Token, std::vector<CellTp>, Token::Hash>>
        action_table,
    std::vector<std::unordered_map<Token, std::vector<IdType>, Token::Hash>>
        goto_table,
    vector<type::Production> productions, Token entry_token,
    Token augmented_token, Token epsilon_token)
    : productions_(productions), action_table_(action_table),
      goto_table_(goto_table), entry_token_(entry_token),
      augmented_token_(augmented_token), epsilon_token_(epsilon_token) {
  productions_.insert(productions_.begin(),
                      type::Production(augmented_token_)(entry_token));
}

ostream &operator<<(ostream &os, const ParsingTable::CellTp &cell) {
  os << "sly::core::grammar::ParsingTable::CellTp{";
  if (cell.action == ParsingTable::kReduce) {
    os << ".action = "
          "sly::core::grammar::ParsingTable::AutomataAction::kReduce,";
  } else if (cell.action == ParsingTable::kAccept) {
    os << ".action = "
          "sly::core::grammar::ParsingTable::AutomataAction::kAccept,";
  } else if (cell.action == ParsingTable::kEmpty) {
    os << ".action = sly::core::grammar::ParsingTable::AutomataAction::kEmpty,";
  } else if (cell.action == ParsingTable::kShiftIn) {
    os << ".action = "
          "sly::core::grammar::ParsingTable::AutomataAction::kShiftIn,";
  } else {
    os << ".action = sly::core::grammar::ParsingTable::AutomataAction::kError,";
  }
  os << ".id=" << cell.id << ", .cause={";
  if (!cell.cause.empty()) {
    for (int i = 0; i < cell.cause.size() - 1; ++i) {
      os << cell.cause[i] << ", ";
    }
    os << cell.cause.back();
  }
  os << "}}";
  return os;
}

bool ParsingTable::CellTp::operator==(const CellTp &rhs) const {
  return this->action == rhs.action && this->id == rhs.id &&
         this->cause == rhs.cause;
}

bool ParsingTable::operator==(const ParsingTable &rhs) const {
  bool flag;
  flag = productions_ == rhs.productions_;
  spdlog::debug("comparing productions={}", flag);
  flag = action_table_ == rhs.action_table_;
  spdlog::debug("comparing action={}", flag);
  flag = goto_table_ == rhs.goto_table_;
  spdlog::debug("comparing goto={}", flag);
  flag = entry_token_ == rhs.entry_token_;
  spdlog::debug("comparing entry={}", flag);
  flag = augmented_token_ == rhs.augmented_token_;
  spdlog::debug("comparing augmented={}", flag);
  flag = epsilon_token_ == rhs.epsilon_token_;
  spdlog::debug("comparing epsilon={}", flag);
  return productions_ == rhs.productions_ &&
         action_table_ == rhs.action_table_ && goto_table_ == rhs.goto_table_ &&
         entry_token_ == rhs.entry_token_ &&
         augmented_token_ == rhs.augmented_token_ &&
         epsilon_token_ == rhs.epsilon_token_;
}

} // namespace sly::core::grammar