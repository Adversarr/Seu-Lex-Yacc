#include "spdlog/spdlog.h"
#include <sly/LrParser.h>
#include <sly/utils.h>

namespace sly::core::grammar {

const ParsingTable &LrParser::GetPt() const {
  return pt_;
}

void LrParser::SetPt(const ParsingTable &pt) {
  pt_ = pt;
}

LrParser::LrParser(ParsingTable &parsing_table) :
  pt_(parsing_table), current_state_id_(-1), current_offset_(0) {
  
}

void LrParser::Parse(vector<Token> token_stream, vector<YYSTATE> yylval_stream) {
  if (token_stream.back() != pt_.GetEndingToken()) {
    spdlog::debug("Input token stream does not end with {} so add to it.", pt_.GetEndingToken().ToString());
    token_stream.push_back(pt_.GetEndingToken());
    yylval_stream.emplace_back();
  }
  apt_stack_.clear();
  state_stack_.clear();
  current_offset_ = 0;
  state_stack_.emplace_back(0);
  while (current_offset_ < token_stream.size()) {
    ParseOnce(token_stream, yylval_stream);
  }
}

void LrParser::ParseOnce(const vector<Token> &token_stream, const vector<YYSTATE>& yylval_stream) {
  auto &current_token = token_stream[current_offset_];
  if (current_token.GetTokenType() == Token::Type::kEpsilon) {
    current_offset_ += 1;
    return;
  } else if (current_token.GetTokenType() == Token::Type::kTerminator) {
    const auto& act= pt_.GetAction(state_stack_.back(), current_token);
    if (act.size() != 1) {
      spdlog::error("Found invalid action table.");
      assert(false);
    }
    const auto& action = act[0];
    if (action.action == ParsingTable::kShiftIn) {
      // 执行移入操作
      apt_stack_.emplace_back(current_token, yylval_stream[current_offset_]);
      // 当前位置 + 1
      current_offset_ += 1;
      // 当前状态更新
      state_stack_.emplace_back(action.id);
      spdlog::debug("Shift In [{}]], Go state {}", current_token.ToString(), state_stack_.back());
    } else if (action.action == ParsingTable::kReduce) {
      // 按照 id 进行规约
      const auto& prod = pt_.GetProductions()[action.id];
      AnnotatedParseTree apt(prod);
      for (int i = 1; i < prod.GetTokens().size(); ++i) {
        apt.EmplaceFront(move(apt_stack_.back()));
        apt_stack_.pop_back();
        state_stack_.pop_back();
      }
      apt_stack_.emplace_back(apt);
      const auto& go = pt_.GetGoto(state_stack_.back(), prod.GetTokens().front());
      if (go.size() != 1) {
        spdlog::error("Found invalid goto. from {} to {}", state_stack_.back(), prod.GetTokens().front().ToString());
        assert(false);
      }
      state_stack_.emplace_back(go[0]);
      spdlog::debug("Reduce [{}], Go state {}", current_token.ToString(), state_stack_.back());
      
    } else {
      if (action.action == ParsingTable::kAccept) {
        current_offset_ += 1;
        return;
      }
      if (action.action == ParsingTable::kError)
        spdlog::error("Found invalid action. current token={}", current_token.ToString());
      assert(false);
    }
  } else {
    spdlog::error("found non terminator in token_stream.");
    assert(false);
  }
  
}

AnnotatedParseTree LrParser::GetTree() const {
  return apt_stack_.front();
}
}