#include "wrapping_integers.hh"
#include <cstdint>
#include <limits>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
  uint32_t n32 = n & UINT32_MAX;
  return WrappingInt32{n32 + isn.raw_value()};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t offset = n.raw_value() - wrap(checkpoint, isn).raw_value();
    uint64_t result = checkpoint + offset;
    /*如果新位置距离checkpoint的偏移offset大于1<<32的一半也就是1<<31,
    那么离checkpoint最近的应该是checkpoint前面的元素
    举个例子: 1---------7(checkpoint)----------------1<<32+1;
    由于是无符号数相减所以1-7 == 1<<32+1 - 7;
    所以应该是1距离7最近所以应该选1 
    */
    if (offset > (1u << 31) && result >= (1ul << 32))
        result -= (1ul << 32);
    return result;
}
