#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity),_nextIndex(0),_eof(false),_eof_pos(0)
{
}

//把收到的数据包中的数据转换成正确的字节流。
//需要注意若使用map<size_t,string>时，已经有cdefg未放入，后面又来了def，此时不用放入，但若来了fgh，还需要再放入h。
//需要注意已经放入了abcdef，后面又来了cde，此时已经不用再放入了。
//eof表示末尾，可以记录末尾的位置，若最后下一个要读取的位置大于末尾的位置，则设置ByteStream中的末尾标志。此时可能会存在空串且有末尾标志。

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //DUMMY_CODE(data, index, eof);
    if(index<=_nextIndex&&index+data.size()>_nextIndex)//重复接收
    {
        size_t st=_nextIndex-index;
        _nextIndex+=_output.write(data.substr(st,data.size()-st));
        while(!_notAssem.empty()&&_notAssem.begin()->first<_nextIndex)
        {
            _notAssem.erase(_notAssem.begin());
        }
        std::string str="";
        size_t tmp_next=_nextIndex;
        while(!_notAssem.empty()&&_notAssem.begin()->first==tmp_next)
        {
            str+=_notAssem.begin()->second;
            tmp_next++;
            _notAssem.erase(_notAssem.begin());
        }
        if(str.size())
        {
            _nextIndex+=_output.write(str);
        }
    }else if(index>_nextIndex)//中间还有一部分内容没有接收
    {
        for(size_t i=0;i<data.size();i++)
        {
            _notAssem.insert(pair<size_t,char>(index+i,data[i]));
        }
    }
    if(eof)
    {
        _eof=true;
        if(index+data.size()==0)
        {
            _eof_pos=0;
        }else
        {
            _eof_pos=index+data.size()-1;
        }
    }
    if(_eof&&_nextIndex>_eof_pos)
    {
        _output.end_input();
    }
    if(_eof&&_nextIndex==0&&_eof_pos==0)
    {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { //return {}; 
    return _notAssem.size();
}

bool StreamReassembler::empty() const { //return {}; 
    if(_nextIndex==_capacity)
    {
        return true;
    }else
    {
        return false;
    }
}

