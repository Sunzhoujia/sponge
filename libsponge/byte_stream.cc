#include "byte_stream.hh"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <assert.h>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) :_capacity(capacity), _write_pos(0), _read_pos(0), 
        _max_size(capacity + 1), _total_bytes_written(0), _total_bytes_read(0), _input_eof(false), _buffer(reinterpret_cast<char*>(malloc(_max_size))) {}

size_t ByteStream::write(const string &data) {
  // buffer is full
  if ((_write_pos + 1) % _max_size == _read_pos) {
    return 0;
  }
  size_t written_bytes = std::min(remaining_capacity(), data.size());
  for (size_t i = 0; i < written_bytes; i++) {
    _buffer[_write_pos++] = data[i];
    _write_pos %= _max_size;
  }
  _total_bytes_written += written_bytes;
  return written_bytes;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
  if (buffer_size() < len) {
    perror("read error");
  }
  string str;
  size_t pos = _read_pos;
  for (size_t i = 0; i < len; i++) {
    str.push_back(_buffer[pos++]);
    pos %= _max_size;
  }
  return str;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
  if (buffer_size() < len) {
    perror("read error");
  }
  _total_bytes_read += len;

  _read_pos = (_read_pos + len) % _max_size;  
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
  size_t read_bytes = std::min(len, buffer_size());
  string str = peek_output(read_bytes);
  pop_output(read_bytes);
  return str;
}

void ByteStream::end_input() { _input_eof = true;}

bool ByteStream::input_ended() const { return _input_eof; }

size_t ByteStream::buffer_size() const { return (_write_pos - _read_pos + _max_size) % _max_size; }

bool ByteStream::buffer_empty() const { return _read_pos == _write_pos; }

bool ByteStream::eof() const { 
  return buffer_empty() && input_ended();  
}

size_t ByteStream::bytes_written() const { return _total_bytes_written; }

size_t ByteStream::bytes_read() const { return _total_bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
