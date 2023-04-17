#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):_capacity(capacity),_total_read(0),_total_write(0),_eof(false)
{ 
    //DUMMY_CODE(capacity);
    //savior
}

size_t ByteStream::write(const string &data) {
    //注意buffer中的字节数+写的字节数需要小于总容量
    size_t index=0;
    while(index<data.size()&&_buffer.size()<_capacity)
    {
        _buffer.push_back(data[index]);
        index++;
        _total_write++;
    }
    return index;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //读的数目需要小于buffer已有的数据大小
    string ret="";
    size_t index=0;
    while(ret.size()<len&&index<_buffer.size())
    {
        ret+=_buffer[index++];
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { //DUMMY_CODE(len); 
    int left=len;
    while(left>0&&_buffer.size()>0)
    {
        _buffer.pop_front();
        left--;
        _total_read++;
    }
}

void ByteStream::end_input() {
    //用户手动设置input结束
    _eof=true;
}

bool ByteStream::input_ended() const {
     //return {};
     //返回input是否结束
     return _eof;
    }

size_t ByteStream::buffer_size() const { 
    //return {}; 
    return _buffer.size();
}

bool ByteStream::buffer_empty() const { 
    //return {};
    if(_buffer.size()==0)
    {
        return true;
    }else
    {
        return false;
    }
}

bool ByteStream::eof() const { 
    //return false; 
    //用户的input已经结束，且已经没有数据可读。
    //true if the output has reached the ending
    return _buffer.empty()&&_eof;
}

size_t ByteStream::bytes_written() const { //return {};
    return _total_write;
 }

size_t ByteStream::bytes_read() const { //return {};
    return _total_read;
 }

size_t ByteStream::remaining_capacity() const { 
    //return {};
    return _capacity-_buffer.size();
    }
