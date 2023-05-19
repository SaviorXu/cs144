#include "tcp_sender.hh"

#include "tcp_config.hh"

#include "timer.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//处理接收的确认号和窗口大小
//从ByteStream读取数据，创建TCP片段，持续发送。
//追踪被发送但是没有被接收的片段。
//重新发送未被接收的片段。

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
    return _alreadySend;
 }

void TCPSender::fill_window() {
    if(next_seqno_absolute()==0)//第一次握手，发送SYN，但不携带数据
    {
        TCPSegment tcpseg;
        //处于CLOSED状态，发送SYN包
        tcpseg.header().syn=true;
        tcpseg.header().seqno=_isn;
        Timer connTimer;
        _next_seqno=_next_seqno+1;
        _segments_out.push(tcpseg);
        _alreadySend=_alreadySend+1;
    }
    if(next_seqno_absolute()>bytes_in_flight() && !_stream.eof())//接收到SYN,发送ACK
    {
        if(_stream.buffer_size()>0)//携带数据
        {
            uint16_t sendSize=_window_size<_stream.buffer_size()?_window_size:_stream.buffer_size();
            while(sendSize>0)
            {
                TCPSegment tcpseg;
                if(sendSize>TCPConfig::MAX_PAYLOAD_SIZE)
                {

                }else
                {

                }
            }
        }else//不携带数据
        {

        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // DUMMY_CODE(ackno, window_size); 
    uint64_t gap;
    if(ackno.raw_value()>_isn.raw_value())
    {
        gap=ackno.raw_value()-_isn.raw_value();
    }else
    {
        gap=(1ul<<32)-_isn.raw_value()+ackno.raw_value();
    }
    _alreadySend-=gap;
    _window_size=window_size;
}
 
//通知TCPSender时间的流逝
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    // if(ms_since_last_tick>_initial_retransmission_timeout)
    // {
    // }
    //自己写个计时器类，用于计时，每当tcp连接建立时，初始化一个计时器
}

//连续重传的次数
unsigned int TCPSender::consecutive_retransmissions() const { 
    // return {};

    }

void TCPSender::send_empty_segment() {

}
