#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    if(seg.header().syn)
    {
        _syn=true;

    }
    if(_syn)//在接收到SYN报文之前的报文都是无效报文，需要丢弃不做处理
    {
        string data=seg.payload().copy();
        _reassembler.push_substring(data,unwrap(seg.header().seqno,_isn,_checkpoint),seg.header().fin);
        _checkpoint=_checkpoint+data.size()-_reassembler.unassembled_bytes();
        _isn=wrap(_checkpoint,_isn);
    }   
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    // return {}; 
    if(!_syn)
    {
        return nullopt;
    }
    return wrap(1,_isn);
}

size_t TCPReceiver::window_size() const { 
    // return {}; 
    return _capacity-_reassembler.unassembled_bytes();
    }
