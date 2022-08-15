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
          _next_index(0), _eof_index(std::numeric_limits<size_t>::max()),_unassembled_substr(std::deque<SubString>()){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
  if (eof) {
    // handle case: SubmitSegment{"", 4}.with_eof(true) 
    _eof_index = index + data.size();
    if (_eof_index == _next_index) {
      _output.end_input();
    }
  }

  // 0. 先把已经assmble的那部分去了, last_byte_index < _next_index
  if (index + data.size() - 1 < _next_index || data.size() == 0) {
    return;
  }
  // printf("_next_index %ld, index %ld, data %s \n", _next_index, index, data.c_str());

  // 1. 不考虑做合并
  if (empty()) {
    if (index == _next_index) {
      size_t written_bytes = std::min(_output.remaining_capacity(), data.size());
      string written_str = written_bytes == data.size() ? data : data.substr(0, written_bytes);
      _output.write(written_str);
      _next_index += written_bytes;

      if (_eof_index == _next_index) {
        _output.end_input();
      }
    } else if (index < _next_index){
      // 去重
      size_t last_byte_index = index + data.size() - 1;
      size_t written_bytes = std::min(_output.remaining_capacity(), last_byte_index - _next_index + 1);
      string written_str = data.substr(_next_index - index, written_bytes);
      _output.write(written_str);
      // printf("write substr %s [next_index %ld, last_byte_index %ld] \n", written_str.c_str(), _next_index, last_byte_index);

      _next_index += written_bytes;
      if (_eof_index == _next_index) {
        _output.end_input();
      }
      // if (eof && written_bytes == (last_byte_index - _next_index + 1)) {
      //   _output.end_input();
      // }
    } else {
      // 中间缺了一段，先存到unassambled数组里
      size_t written_bytes = std::min(_output.remaining_capacity(), data.size());
      string written_str = written_bytes == data.size() ? data : data.substr(0, written_bytes);
      _unassembled_substr.emplace_back(index, index + written_bytes - 1, written_str);
      // printf("store in unassembled start %ld, end %ld, content %s \n", index, index + written_bytes - 1, written_str.c_str());
    }
  } else {
    // 2. 先考虑和_unassembled_substr里的substring合并
    if (index < _next_index) {
      size_t last_byte_index = index + data.size() - 1;
      size_t written_bytes = last_byte_index - _next_index + 1;
      _unassembled_substr.emplace_back(_next_index, last_byte_index, data.substr(_next_index - index, written_bytes));
    } else {
      _unassembled_substr.emplace_back(index, index + data.size() - 1, data);
    }
    sort(_unassembled_substr.begin(), _unassembled_substr.end());
    size_t remain_space = _output.remaining_capacity();
    // 对_unassembled_substr进行byte去重
    size_t next_index = 0;
    for (size_t i = 0 ; i < _unassembled_substr.size(); i++) {
      auto& [start, end, content] = _unassembled_substr[i];
      // printf("begin fix start %ld, end %ld, content %s next_index %ld\n", start, end, content.c_str(), next_index);
      size_t len = content.size();
      if (remain_space == 0) {
        content.resize(0); // 标记下便于删除
        continue;
      }
      if (i == 0) {
        if (len <= remain_space) {
          remain_space -= len;
          next_index = start + len;
        } else {
          end =  start + remain_space - 1;
          content = content.substr(0, remain_space);
          remain_space = 0;
        }
      } else {
        // 当前substr和前面有重叠
        if (start < next_index) {
          if (end < next_index) { 
            content.resize(0); // 标记下便于删除
            continue;
          } else {
            size_t keep_len = std::min(remain_space, end - next_index + 1);
            content = content.substr(next_index - start, keep_len);
            start = next_index; 
            end = next_index + keep_len - 1;
            remain_space -= keep_len;
            next_index += keep_len;
          }
        } else {
          // substr和前面的没有重叠
          size_t keep_len = std::min(remain_space, len);
          end = start + keep_len - 1;
          if (keep_len < len) content = content.substr(0, keep_len);
          remain_space -= keep_len;
          next_index = end + 1;
        }
      }
      // printf("begin fix start %ld, end %ld, content %s next_index %ld\n", start, end, content.c_str(), next_index);
    }

    // 把 content.size() == 0 的删除
    for (auto it = _unassembled_substr.begin(); it != _unassembled_substr.end();) {
      if (it->content.size() == 0) {
        it = _unassembled_substr.erase(it);
      } else {
        ++it;
      }
    }

    // 然后往oup_put里加
    for (auto it = _unassembled_substr.begin(); it != _unassembled_substr.end();) {
      auto& [start, end ,content] = *it;
      // printf("output written %ld, end %ld, content %s\n", start, end, content.c_str());

      if (_next_index < start) break;

      if (_next_index == start) {
        _output.write(content);
        _next_index += content.size();
        // printf("write substr %s", content.c_str());
        it = _unassembled_substr.erase(it);
      } else {
        ++it;
      }
    }
    if (_eof_index == _next_index) {
      _output.end_input();
    }
    // if (eof && _unassembled_substr.empty()) {
    //   _output.end_input();
    // }
  }


}

size_t StreamReassembler::unassembled_bytes() const {
  size_t total = 0;
  for (auto& substr : _unassembled_substr) {
    total += substr.content.size();
  }
  return total;
}

bool StreamReassembler::empty() const { 
  return unassembled_bytes() == 0;  
}
