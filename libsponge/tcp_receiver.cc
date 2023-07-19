#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // printf("tcp_receiver:segment_received;seg.seqno=%u _isn=%u\n",seg.header().seqno.raw_value(),_isn.raw_value());
    // DUMMY_CODE(seg);
    if(!_syn)
    {
       if(seg.header().syn)
       {
            _syn=true;
            _isn=seg.header().seqno; 
       }else
       {
            return;
       }
    }
    string data=seg.payload().copy();
    
    // uint64_t checkpoint=_reassembler.stream_out().bytes_written()+1;//写入的数据不包括SYN序号
    // uint64_t index=unwrap(seg.header().seqno,_isn,checkpoint);//得到uint64_t，seg.header().seqno表示的是这个段的seqno，_isn表示的是这个连接开始的seqno 
    
    // index=index-1+(seg.header().syn);
    uint64_t index;
    if(_syn&&seg.header().syn)
    {
        uint64_t checkpoint=_reassembler.stream_out().bytes_written();
        index=unwrap(seg.header().seqno,_isn,checkpoint);
        //第一个序列的seg.header().seqno等于isn，因此index返回为0
    }else if(_syn&&seg.header().seqno.raw_value()<=_isn.raw_value())
    {
        return;
    }
    else
    {
        uint64_t checkpoint=_reassembler.stream_out().bytes_written();
        index=unwrap(seg.header().seqno-1,_isn,checkpoint);
        //此时，若不减1，index返回为1，因此seg.header().seqno包含了一个syn等于1，因此将syn-1。此时index仍返回0，所以字母写入_reassembler的0处
        //字节流中不包含SYN
    }
    // printf("index=%lu\n",index);
    _reassembler.push_substring(data,index,seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    // return {}; 
    if(!_syn)
    {
        return nullopt;
    }
    uint64_t n=_reassembler.stream_out().bytes_written()+1;//SYN占一个序列号
    if(_reassembler.stream_out().input_ended())//FIN占一个序列号
    {
        n++;
    }
    return wrap(n,_isn);
}

size_t TCPReceiver::window_size() const { 
    // return {}; 
    return _capacity-_reassembler.stream_out().buffer_size();
}
