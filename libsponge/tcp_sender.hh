#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <queue>
#include <map>
//! \brief The "sender" part of a TCP implementation.

class Timer {
  private:
    bool _start;

    unsigned int _init_retx_timeout;

    unsigned int _transmission_time;

    // retx_timeout may double if network is busy.
    unsigned int _cur_retx_timeout;

  public:
    unsigned int _num_of_retransmissions;

    Timer(unsigned int retx_timeout):
        _start(false),
        _init_retx_timeout(retx_timeout),
        _transmission_time(0),
        _cur_retx_timeout(retx_timeout),
        _num_of_retransmissions(0) {}

    bool running() {return _start;}

    void start() {
      _start = true;
      _cur_retx_timeout = _init_retx_timeout;
      _transmission_time = 0;
      _num_of_retransmissions = 0;
      return;
    }

    void close() {
      _start = false;
      _num_of_retransmissions = 0;
    }

    bool timeout(const size_t ms_since_last_tick) {
      if (!running()) {
        return false;
      }

      if (ms_since_last_tick + _transmission_time >= _cur_retx_timeout) {
        return true;
      }

      _transmission_time += ms_since_last_tick;
      return false;
    }

    /* 
      1. window == 0, keep RTO
      2. window != 0, RTO *= 2
     */
    void restart_timer(const size_t window) {
      if (!running()) {
        return;
      }

      if (window != 0) {
        _cur_retx_timeout *= 2;
      }

      _transmission_time = 0;
      _num_of_retransmissions++;
    }
};



//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    
    /* add private member */
    uint64_t _ackno;
    size_t _window_size;
    uint64_t _bytes_in_flight;
    Timer _timer;

    std::queue<TCPSegment> _segments_unacked{};
    bool _fin{false};

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    bool fin_sent(){
      return _fin;
    }

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    bool ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();


    void send_no_empty_segments(TCPSegment &seg);


    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    bool tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
