#include "stream_reassembler.hh"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), 
          _next_index(0), _eof_index(std::numeric_limits<size_t>::max()), window(std::map<size_t, std::string>()){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
  /* 遇到eof, 记录eof_index */
  if (eof) {
    _eof_flag = true;
    _eof_index = index + data.size();
  }


  if (index > _next_index) {
    insert_substring(data, index);
  } else if (index <= _next_index && data.size() + index >= _next_index) {
    insert_substring(data, index);
    write_substring();
  }

}

size_t StreamReassembler::unassembled_bytes() const {
  size_t total = 0;
  size_t last_index = 0;
  auto it = window.begin();
  for (; it != window.end(); it++) {
    auto& [start, str] = *it;
    size_t end = start + str.size() - 1;
    if (start > last_index || last_index == 0) {
      total += str.size();
      last_index = end;
    } else {
      // 存在byte重叠
      if (end > last_index) {
        total += end - last_index;
        last_index = end;
      }
    }
  }
  return total;
}

bool StreamReassembler::empty() const { 
  return window.empty();
}

void StreamReassembler::write_substring() {
  auto it = window.begin();
  for (; it != window.end(); it++) {
    auto& [start, str] = *it;
    size_t str_len = str.size();
    if (start <= _next_index) {
      if (str_len + start - 1 < _next_index) {
        continue;
      }
      size_t writted_len = start + str_len - _next_index;
      _next_index += _output.write(str.substr(_next_index - start, writted_len));
    } else {
      break;
    }
  }

  window.erase(window.begin(), it);
  // printf("eof_index %ld, next_index %ld \n", _eof_index, _next_index);
  if (_eof_index <= _next_index && _eof_flag) {
    _output.end_input();
  }
}


void StreamReassembler::insert_substring(const std::string &data, const size_t index) {
  if (window[index].size() < data.size()) {
    window[index] = data;
  }
}
