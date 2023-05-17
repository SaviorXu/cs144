#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

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
    , _stream(capacity) {}

//有多少序列号被发送但尚未确认的段占用
uint64_t TCPSender::bytes_in_flight() const { 
    // return {};
 
 }

void TCPSender::fill_window() {
    // string str="";
    uint64_t sendsize=_stream.buffer_size()<_windowsize?_stream.buffer_size():_windowsize;
    while(sendsize>0)
    {
        string str="";
        int segpayload;
        if(sendsize>TCPConfig::MAX_PAYLOAD_SIZE)
        {
            segpayload=TCPConfig::MAX_PAYLOAD_SIZE;
        }else
        {
            segpayload=sendsize;
        }
        str+=_stream.peek_output(segpayload);
        _stream.pop_output(segpayload);
        sendsize-=segpayload;
        _windowsize-=segpayload;
        TCPSegment tcpseg;
        tcpseg.parse(Buffer(std::move(str)));
        if(_isn==next_seqno())
        {
            tcpseg.header().syn=true;
        }
        if(_stream.input_ended()&&sendsize==0)
        {
            tcpseg.header().fin=true;
        }
    }
    // if(bsize<=_windowsize)
    // {
    //     str+=_stream.peek_output(bsize);
    //     _stream.pop_output(bsize);
    //     _windowsize-=bsize;
    // }else
    // {
    //     str+=_stream.peek_output(_windowsize);
    //     _stream.pop_output(_windowsize);
    //     _windowsize=0;
    // }
    // TCPSegment tcpseg;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // DUMMY_CODE(ackno, window_size); 
    _ackno=ackno;
    _windowsize=window_size;
    }

//通知TCPSender时间的流逝
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    // if(ms_since_last_tick>_initial_retransmission_timeout)
    // {

    // }
}

//连续重传的次数
unsigned int TCPSender::consecutive_retransmissions() const { 
    // return {};

    }

void TCPSender::send_empty_segment() {}
