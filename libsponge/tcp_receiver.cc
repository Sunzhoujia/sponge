#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
#include <cstdio>
#include <optional>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/* 坑1: 第一条建立连接的seg是空的，仅有syn = true， 所以在收到ISN后，需要将记录的ISN + 1，便于计算ack。因为 stream index = absolute index - 1 */
void TCPReceiver::segment_received(const TCPSegment &seg) {
  if (seg.header().syn && !_syn) {
    _isnno = seg.header().seqno;
  }

  if (_isnno.has_value()) {
    _reassembler.push_substring(seg.payload().copy(), unwrap(seg.header().seqno, _isnno.value(), _reassembler.next_index()), seg.header().fin);
    _ackno = wrap(_reassembler.next_index(), _isnno.value());
  }


  // 这个判断只在接收到ISN的segment进入
  if (!_syn && seg.header().syn) {
    // 将Init Sequence Number设置为带Payload的第一个Sequence Number。
    // 如果不设置，第一个带payload的seg的seq是isnno+1, 所以reassembler计算出来的index = 1, 无法重组
    _ackno = WrappingInt32(_ackno->raw_value() + 1);
    _isnno = _ackno;
    _syn = true; // 只会设置一次
  }

  _fin = _fin ? _fin : seg.header().fin; // fin可能也会收到冗余的

  if (_fin && _reassembler.stream_out().input_ended()) {
    _ackno = WrappingInt32(_ackno->raw_value() + 1);
  }

  return;

}

optional<WrappingInt32> TCPReceiver::ackno() const {
  return _ackno;
}

size_t TCPReceiver::window_size() const {
  return _capacity - _reassembler.stream_out().buffer_size();
}
