#include "tcp_sender.hh"

#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <cstddef>
#include <cstdint>
#include <random>
#include <algorithm>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _ackno(0)
    , _window_size(1)
    , _bytes_in_flight(0)
    , _timer(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
  /* Status: CLOSED */
  if (_next_seqno == 0) {
    TCPSegment seg;
    seg.header().syn = true;
    seg.header().seqno = next_seqno();
    send_no_empty_segments(seg);
  }
  /* Status: SYN_SENT --> stream started but nothing acknowledged */
  else if (_next_seqno == _bytes_in_flight) {
    return;
  }

  size_t window_size = _window_size == 0 ? 1 : _window_size;
  size_t remain = window_size - (_next_seqno - _ackno);

  while (remain) {
    TCPSegment seg;
    size_t len = TCPConfig::MAX_PAYLOAD_SIZE > remain ? remain : TCPConfig::MAX_PAYLOAD_SIZE;
    /* Status: SYN_ACKED --> stream ongoing */
    if (!_stream.eof()) {
      seg.payload() = Buffer(_stream.read(len));
      if (_stream.eof() && remain - seg.length_in_sequence_space() > 0) {
        seg.header().fin = true;
      }
      if (seg.length_in_sequence_space() == 0) {
        return;
      }
      send_no_empty_segments(seg);
    } 
    /* Status: SYN_ACKED -->  stream ongoing (stream has reached EOF but FIN hasn't been send yet)*/
    else if (_stream.eof()) {
      if (_next_seqno < _stream.bytes_written() + 2) {
        seg.header().fin = true;
        send_no_empty_segments(seg);
      }
      /* Status: FIN_SENT and FIN_ACKED both do nothing Just return  */
      else {
        return;
      }
    }
    remain -= len; 
  }

}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
  uint64_t abs_ackno = unwrap(ackno, _isn, _ackno);

  if (abs_ackno > _next_seqno)
    return;
  
  _window_size = static_cast<size_t>(window_size);

  // ack大的先来了
  if (abs_ackno <= _ackno) {
    return;
  }

  _ackno = abs_ackno;

  _timer.start();

  //  确认outstanding segments
  while (!_segments_unacked.empty()) {
    auto seg = _segments_unacked.front();
    if (ackno.raw_value() < seg.header().seqno.raw_value() + static_cast<uint32_t>(seg.length_in_sequence_space())) {
      break;
    }
    _bytes_in_flight -= seg.length_in_sequence_space();
    _segments_unacked.pop();
  }

  //发送segment
  fill_window();

  if (_segments_unacked.empty()) {
    _timer.close();
  }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
  if (!_timer.running() || !_timer.timeout(ms_since_last_tick)) {
    return;
  }

  if (_segments_unacked.empty()) {
    _timer.close();
    return;
  }

  _timer.restart_timer(_window_size);
  _segments_out.push(_segments_unacked.front());
}

unsigned int TCPSender::consecutive_retransmissions() const { return _timer._num_of_retransmissions; }

void TCPSender::send_no_empty_segments(TCPSegment &seg) {
  seg.header().seqno = wrap(_next_seqno, _isn);
  _next_seqno += seg.length_in_sequence_space();
  _bytes_in_flight += seg.length_in_sequence_space();
  _segments_out.push(seg);
  _segments_unacked.push(seg);
  if (!_timer.running()) {
    _timer.start();
  }
} 

void TCPSender::send_empty_segment() {
  TCPSegment seg;
  seg.header().seqno = wrap(_next_seqno, _isn);
  _segments_out.push(seg); // empty_seg 不需要加入 segmemt_unacked 队列
}
