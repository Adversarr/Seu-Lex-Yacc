#include <cassert>
#include <sly/Stream2TokenPipe.h>
#include <sly/utils.h>
namespace sly::runtime {

Stream2TokenPipe::Stream2TokenPipe(std::vector<std::vector<int>> working_table,
                                   std::vector<int> accept_states) :
  table_(move(working_table)), accept_states_(move(accept_states)) {
  assert(working_table.front().size() == 128);
  assert(accept_states.size() == 128);
}

core::type::Token Stream2TokenPipe::Defer(std::istream& is) {
  // TODO: Not tested.
  int current_state = 0;
  while (is.good() && !is.eof() && !is.fail()) {
    char c = is.get();
    utils::Log::GetGlobalLogger().Info("Caught ", c , "From stream.");
    int next_state = table_[current_state][c];
    if (next_state == -1){
      // recover.
      is.putback(c);
      break;
    }
    current_state = next_state;
  }
  // Nothing is parsed.
  if (current_state != 0)
    return token_list_[current_state];
  assert(false);
}

}