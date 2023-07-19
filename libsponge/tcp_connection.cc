#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::sendRst()
{
    // printf("sendRst\n");
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _isActive=false;
    TCPSegment &tcpseg=_sender.segments_out().front();
    tcpseg.header().rst=true;
    if(_receiver.ackno())
    {
        tcpseg.header().ackno=_receiver.ackno().value();
        tcpseg.header().win=_receiver.window_size();
        tcpseg.header().ack=true;
    }
    _segments_out.push(tcpseg);
    // printf("sendRst:tcpseg length()=%lu\n",tcpseg.length_in_sequence_space());
}
void TCPConnection::sendSeg()
{
    auto &segments=_sender.segments_out();
    // printf("sendSeg size=%ld\n",segments.size());
    while(!segments.empty())
    {
        auto &segment=segments.front();
        if(_receiver.ackno())
        {
            segment.header().ackno=_receiver.ackno().value();
            segment.header().win=_receiver.window_size();
            segment.header().ack=true;
        }
        _segments_out.push(segment);
        // printf("sendSeg:tcpseg seqno=%u ackno=%u syn=%d fin=%d\n",segment.header().seqno.raw_value(),segment.header().ackno.raw_value(),segment.header().syn,segment.header().fin);
        segments.pop();
    }
}

size_t TCPConnection::remaining_outbound_capacity() const { 
    // return {}; 
    return _sender.stream_in().remaining_capacity();
    }

size_t TCPConnection::bytes_in_flight() const { 
    // return {};
     return _sender.bytes_in_flight();
    }

size_t TCPConnection::unassembled_bytes() const { 
    // return {}; 
    return _receiver.unassembled_bytes();
    }

size_t TCPConnection::time_since_last_segment_received() const { 
    // return {}; 
    return _last_segment_time;
    }

//当有新报文从网络中来临，会被调用。并且设置_last_segment_time=0；
void TCPConnection::segment_received(const TCPSegment &seg) {
    // printf("segment_received seg.seqno=%u seg.ackno=%u seg.fin=%d seg.syn=%d seg.winsize=%d\n",seg.header().seqno.raw_value(),seg.header().ackno.raw_value(),seg.header().fin,seg.header().syn,seg.header().win);
    // DUMMY_CODE(seg); 
    if(!active())
    {
        return;
    }
    _last_segment_time=0;
    if(seg.header().rst) //rst is set, set both inbound and outbound streams to the error state and kills the connection permanently.
    {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _isActive=false;
        return;
    }
    _receiver.segment_received(seg);
    if(seg.header().ack)
    {
        _sender.ack_received(seg.header().ackno,seg.header().win);
    }
    // printf("tcp_connection sender=%s rece=%s\n",TCPState::state_summary(_sender).c_str(),TCPState::state_summary(_receiver).c_str());

    //三次握手的第三次可以不用携带数据，因此可以不用发送ack
    //但之后收到的确认，没有携带数据，要发送空的ack
    // if(seg.length_in_sequence_space()==0&& TCPState::state_summary(_sender)!=TCPSenderStateSummary::SYN_ACKED)
    // {
    //     // if(TCPState::state_summary(_sender)==SYN_ACKED)
    //     if(_sender.segments_out().empty())
    //     {
    //         printf("send_empty1 sender=%s rece=%s\n",TCPState::state_summary(_sender).c_str(),TCPState::state_summary(_receiver).c_str());
    //         _sender.send_empty_segment(); 
    //     }
    // }
    // if(!seg.length_in_sequence_space())
    // {
    //     if(_sender.segments_out().empty())
    //     {
    //         printf("send_empty1 sender=%s rece=%s\n",TCPState::state_summary(_sender).c_str(),TCPState::state_summary(_receiver).c_str());
    //         _sender.send_empty_segment(); 
    //     }
    // }

    //keep-alive packet
    if(_receiver.ackno().has_value() && seg.length_in_sequence_space()==0 &&seg.header().seqno==_receiver.ackno().value()-1)
    {
        _sender.send_empty_segment();
    }


    //connect是主动发送SYN,服务端被动连接，接收到SYN后，还要回复ACK+SYN
    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::SYN_RECV&&TCPState::state_summary(_sender)==TCPSenderStateSummary::CLOSED)
    {
        connect();
        return;
    }

    //?存疑
    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::FIN_RECV&&TCPState::state_summary(_sender)==TCPSenderStateSummary::SYN_ACKED)
    {
        _linger_after_streams_finish=false;
    }

    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::FIN_RECV&&TCPState::state_summary(_sender)==TCPSenderStateSummary::FIN_ACKED&&!_linger_after_streams_finish)
    {
        _isActive=false;
        return;
    }

    if(seg.length_in_sequence_space()!=0)
    {
        if(_sender.segments_out().empty())
        {
            _sender.send_empty_segment(); 
        }
    }
    //接收到数据后，调用fill_window才能继续填满发送队列
    if(TCPState::state_summary(_sender)!=TCPSenderStateSummary::CLOSED)
    {
        _sender.fill_window();
    }
    // _sender.fill_window();
    //sender发送给对方的window_size和ack是根据receiver知道的。数据流都放在receiver的缓冲区中。
    sendSeg();

}

bool TCPConnection::active() const { 
    // return {}; 
    // printf("tcp_connection:active\n");
    return _isActive;
    }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    // return {};
    if(data.size()==0)
    {
        return 0;
    }
    size_t ret=_sender.stream_in().write(data);
    _sender.fill_window();
    sendSeg();
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // printf("tcp_connection:tick _receiver=%s _sender=%s\n",TCPState::state_summary(_receiver).c_str(),TCPState::state_summary(_sender).c_str()); 
    // printf("_ms_since_last_tick=%ld\n",ms_since_last_tick);
    // DUMMY_CODE(ms_since_last_tick); 
    _sender.tick(ms_since_last_tick);
    _last_segment_time+=ms_since_last_tick;
    if(_sender.consecutive_retransmissions()>TCPConfig::MAX_RETX_ATTEMPTS)
    { 
        // _sender.stream_in().set_error();
        // _receiver.stream_out().set_error();
        // _isActive=false;
        sendRst();
        return;
    }

    if(_receiver.ackno().has_value())
    {
        _sender.fill_window();
    }
    //此处存疑
    sendSeg();
    
    // printf("tick:before if last_segment_time=%ld %d\n",_last_segment_time,_cfg.rt_timeout);
    //TIME_WAIT等到超时，关闭链接
    if(TCPState::state_summary(_receiver)==TCPReceiverStateSummary::FIN_RECV && TCPState::state_summary(_sender)==TCPSenderStateSummary::FIN_ACKED && _last_segment_time>=10*_cfg.rt_timeout)
    {
        // printf("tick:timeout\n");
        _linger_after_streams_finish=false;
        _isActive=false;
    }

}
//主动关闭，当上层没有数据要传送时，还要发送一个fin
void TCPConnection::end_input_stream() {
    // printf("tcp_connection end_input_stream\n");
    _sender.stream_in().end_input();
    _sender.fill_window();
    sendSeg();
}

//主动连接，会发送一个SYN=1的报文
void TCPConnection::connect() {
    // printf("tcp_connection:connect()\n");
    _sender.fill_window();
    sendSeg();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment();
            sendRst();

        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}



