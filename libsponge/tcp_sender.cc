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
    ,_retrans_timeout{retx_timeout}
    , _stream(capacity),_alreadySend(0),_window_size(1),_timer()
    ,_retrans_times(0)
     {}

//有多少序列号被发送但尚未确认的段占用
uint64_t TCPSender::bytes_in_flight() const { 
    // return {};
    return _alreadySend;
 }

void TCPSender::fill_window() {

    size_t window_size=_window_size;
    if(_state==CLOSED && next_seqno_absolute()==0)//第一次握手，发送SYN，但不携带数据
    {
        // printf("tcp_sender:send SYN\n");
        //当ack_recv处，接收到对该SYN的ack时，会将状态改变。
        TCPSegment tcpseg;
        //处于CLOSED状态，发送SYN包
        tcpseg.header().syn=true;
        tcpseg.header().seqno=WrappingInt32(_isn);
        Timer connTimer;
        _next_seqno=_next_seqno+1;
        _segments_out.push(tcpseg);
        _resend.push(tcpseg);
        _alreadySend=_alreadySend+1;
        _timer.setStart();
        _state=SYN_SENT;
    }
    if(_state==SYN_ACKED)
    {
        // printf("_stream._capacity=%ld\n",_stream.remaining_capacity());
        if(window_size==0)
        {
            window_size=1;//若在此处直接用_window_size=1.则会出现问题。当窗口设为0时，假设窗口为1.但是不能改变超时重传时间。若直接将_window_size=1。则在超时重传处会将时间加倍。
        }
        //stream.input_ended()表示应用层的数据已经读完，但buffer里面可能还有数据
        if((!_stream.eof())||(_stream.input_ended()&&_stream.buffer_size()+_alreadySend<=window_size))//还有数据要发送
        {
            uint16_t sendSize=_stream.buffer_size()>(window_size-_alreadySend)?(window_size-_alreadySend):_stream.buffer_size();
            while(sendSize>0)
            {
                uint16_t realSize=(sendSize>TCPConfig::MAX_PAYLOAD_SIZE)?TCPConfig::MAX_PAYLOAD_SIZE:sendSize;
                // _window_size-=realSize;
                sendSize-=realSize;
                TCPSegment tcpseg;
                tcpseg.payload()=std::move(_stream.read(realSize));
                tcpseg.header().seqno=wrap(_next_seqno,_isn);
                _next_seqno+=realSize;
                _alreadySend+=realSize;

                if(_stream.input_ended()&&sendSize==0&&window_size>=_alreadySend+1)
                {
                    tcpseg.header().fin=true;
                    _state=FIN_SENT;
                    _next_seqno+=1;
                    _alreadySend+=1;
                    // _retrans_timeout=_initial_retransmission_timeout;
                }
                _resend.push(tcpseg);
                _segments_out.push(tcpseg);
                if(!_timer.isStart())
                {
                    _timer.setStart();
                }
            }
        }
        //_stream.eof()表示应用层数据已经发送完，且buffer缓冲区已经没有数据
        if(_state==SYN_ACKED&&_stream.eof()&&next_seqno_absolute()<_stream.bytes_written()+2&&window_size>=1+_alreadySend)//没有数据要发送
        {
            //printf("FIN _stream.buffer_size=%ld\n",_stream.buffer_size());
            //并发送FIN
            TCPSegment tcpseg;
            tcpseg.header().fin=true;
            tcpseg.header().seqno=wrap(_next_seqno,_isn);
            _next_seqno+=1;
            _alreadySend+=1;
            _segments_out.push(tcpseg);
            _resend.push(tcpseg);
            _state=FIN_SENT;
            // _retrans_timeout=_initial_retransmission_timeout;
        }
    }
    
}


//移除已经确认过的报文
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    // DUMMY_CODE(ackno, window_size); 
    //当接收者给发送者一个信号，表示接收到最近的消息，重置RTO，
    
    // printf("ack_received retrans_timeout=%u\n",_retrans_timeout);
    if(unwrap(ackno,_isn,_next_seqno)>_next_seqno)
    {
        return;
    }

    bool has_new=false;
    _window_size=window_size;
    if(_state==SYN_SENT && ackno==wrap(1,_isn))
    {
        // printf("ack_received _state=SYN_ACKED _syn==1\n");
        _state=SYN_ACKED;
        // _resend.pop();
    }

    //此处必须判断_resend.empty()是否为空。否则会出现断错误
    if(!_resend.empty())
    {
        TCPSegment tcp=_resend.front();
        auto initial=unwrap(tcp.header().seqno,_isn,_next_seqno)+tcp.length_in_sequence_space();
        auto ack=unwrap(ackno,_isn,_next_seqno);
        while(initial<=ack)
        {
            // printf("_syn==1\n");
            // printf("initial=%ld ack=%ld\n",initial,ack);
            _alreadySend-=tcp.length_in_sequence_space();
            _resend.pop();
            has_new=true;

            if(_resend.empty())
            {
                break;
            }
            tcp=_resend.front();
            initial=unwrap(tcp.header().seqno,_isn,_next_seqno)+tcp.length_in_sequence_space();
        }
    }
    if(has_new)
    {
        _retrans_timeout=_initial_retransmission_timeout;
        _retrans_times=0;
        _timer.reset();
    }
}
 
//通知TCPSender时间的流逝
//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    //自己写个计时器类，用于计时，每当tcp连接建立时，初始化一个计时器
    // printf("call tick\n");
    if(_timer.isStart())
    {
        _timer.addDuration(ms_since_last_tick);
        // printf("getDuration=%ld _retrans_timeout=%d\n",_timer.getDuration(),_retrans_timeout);
        if(_timer.getDuration()>=_retrans_timeout)
        {
            // printf("retrans\n");
            //超时之后，需要重新传送最早的未被确认的片段。
            //若窗口大小不为0，连续重传次数+1，RTO*2
            if(!_resend.empty())
            {
                // printf("retrans send segment\n");
                _segments_out.push(_resend.front());
                //当有需要重传的报文且对方可以收时，才需要将超时重传时间设置为2倍。
                if(_window_size>0)//若初始化时，将窗口设为0。当发送SYN报文时，超时并不会加倍，因此初始时应将窗口设为1
                {
                    _retrans_times+=1;
                    _retrans_timeout<<=1;
                } 
            }
            // if(_window_size>0)//若初始化时，将窗口设为0。当发送SYN报文时，超时并不会加倍，因此初始时应将窗口设为1
            // {
            //     _retrans_times+=1;
            //     _retrans_timeout<<=1;
            // } 
            _timer.reset();       
        }
        
    }else
    {
        return;
    }
}

//连续重传的次数
unsigned int TCPSender::consecutive_retransmissions() const { 
    return _retrans_times;
    }

void TCPSender::send_empty_segment() {
    TCPSegment tcpseg;
    tcpseg.header().seqno=wrap(_next_seqno,_isn);
    _segments_out.push(tcpseg);
}
