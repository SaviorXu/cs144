#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number

//convert absolute seqno -> seqno  produce the (relative) sequence number for n
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // DUMMY_CODE(n, isn);
    // return WrappingInt32{0};
    return WrappingInt32(isn.raw_value()+n);
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
//convert seqno -> absolute seqno

uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t cp=checkpoint & (0xffffffff00000000);
    uint64_t cpMod= checkpoint & (0x00000000ffffffff);
    uint32_t gap;
    uint64_t standard=(1ul << 32);
    uint64_t ret;
    if(n.raw_value()>isn.raw_value())
    {
        gap=n.raw_value()-isn.raw_value();
    }else
    {
        gap=(standard-isn.raw_value())+n.raw_value();
    }
    //printf("gap=%u cpMod=%lu\n",gap,cpMod);
    if(cpMod>gap)
    {
        uint64_t dis1=standard-cpMod+gap;
        uint64_t dis2=cpMod-gap;
        if(dis1>dis2)
        {
            // printf(">\n");
            ret=checkpoint-dis2;
        }else
        {
            // printf("< dis2=%lu\n",dis2);
            ret=checkpoint+dis1;
        }
    }else if(cpMod<gap)
    {
        if(cp==0)
        {
            ret=gap;
        }else
        {
            uint64_t dis1=standard-gap+cpMod;
            uint64_t dis2=gap-cpMod;
            if(dis1<dis2)
            {
                ret=checkpoint-dis1;
            }else
            {
                ret=checkpoint+dis2;
            }
        }
    }else
    {
        return checkpoint;
    }
    return ret;
}
// uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
//     printf("n=%u isn=%u\n",n.raw_value(),isn.raw_value());
//     int32_t offset = static_cast<uint32_t>(checkpoint) - (n - isn);
//     printf("n-isn=%d\n",(n-isn));
//     int64_t result = checkpoint - offset;
//     return result >= 0 ? result : result + (1UL << 32);
// }

