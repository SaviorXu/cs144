#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address),_duration(0){
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

void NetworkInterface::send_arp_req(const InternetDatagram &dgram, const uint32_t &next_hop_ip)
{
    EthernetFrame epack;
    epack.header().type=EthernetHeader::TYPE_ARP;
    epack.header().dst=ETHERNET_BROADCAST;
    epack.header().src=_ethernet_address;
    

    ARPMessage  arp_pack;
    arp_pack.sender_ip_address=_ip_address.ipv4_numeric();
    arp_pack.sender_ethernet_address=_ethernet_address;
    arp_pack.target_ip_address=next_hop_ip;
    arp_pack.opcode=ARPMessage::OPCODE_REQUEST;
    epack.payload()=BufferList(arp_pack.serialize());
    
    _wait_send.insert({next_hop_ip,dgram});
    _frames_out.push(epack);
    _arp_storage.insert({next_hop_ip,{epack,_duration}});
}
void NetworkInterface::send_ether_pack(const InternetDatagram &dgram, const uint32_t &next_hop_ip)
{
    EthernetFrame epack;
    epack.header().type=EthernetHeader::TYPE_IPv4;
    epack.header().dst=_storage[next_hop_ip].first;
    epack.header().src=_ethernet_address;
    epack.payload()=dgram.serialize();
    _frames_out.push(epack);
}

//发送报文时，如果没有存ip和mac的映射，需要发送arp报文。如果有，则找到mac地址发送包
//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // DUMMY_CODE(dgram, next_hop, next_hop_ip);
    if(_storage.find(next_hop_ip)==_storage.end())
    {
        auto iter=_arp_storage.find(next_hop_ip);
        if(iter!=_arp_storage.end())
        {
            if(_duration-iter->second.second>=5000)
            {
                _frames_out.push(iter->second.first);
            }
        }else
        {
            send_arp_req(dgram,next_hop_ip);
        }
    }else
    {
        send_ether_pack(dgram,next_hop_ip);
    }
}

//接收报文时，可能是ethernet报文，可能是arp回复报文（添加ip和mac的映射），可能是arp请求报文
//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);
    // return {};
    if(frame.header().type==EthernetHeader::TYPE_IPv4)
    {
        InternetDatagram ip_pack;
        ParseResult parse_res=ip_pack.parse(frame.payload());
        if(parse_res!=ParseResult::NoError||frame.header().dst!=_ethernet_address)
        {
            return {};
        }
        return ip_pack;
    }else if(frame.header().type==EthernetHeader::TYPE_ARP)
    {
        ARPMessage arp_pack;
        ParseResult arp_parse=arp_pack.parse(frame.payload());
        if(arp_parse!=ParseResult::NoError)
        {
            return {};
        }
        _storage.insert({arp_pack.sender_ip_address,{frame.header().src,30000}});
        //arp返回报文，添加ip和mac的映射,并发送报文
        if(arp_pack.opcode==ARPMessage::OPCODE_REPLY&&arp_pack.target_ethernet_address==_ethernet_address)
        {
            _storage.insert({arp_pack.sender_ip_address,{arp_pack.sender_ethernet_address,30000}});
            auto iter=_wait_send.find(arp_pack.sender_ip_address);
            if(iter!=_wait_send.end())
            {
                send_ether_pack(iter->second,iter->first);
                _wait_send.erase(iter);
            }

            auto iter2=_arp_storage.find(arp_pack.sender_ip_address);
            if(iter2!=_arp_storage.end())
            {
                _arp_storage.erase(iter2);
            }

        }else if(arp_pack.opcode==ARPMessage::OPCODE_REQUEST)
        {
            //查看缓冲中是否有该映射，若无，添加映射。
            //查看是否是请求本mac地址，若是，则发送mac地址
            if(arp_pack.target_ip_address==_ip_address.ipv4_numeric())
            {
                EthernetFrame epack;
                epack.header().type=EthernetHeader::TYPE_ARP;
                epack.header().dst=frame.header().src;
                epack.header().src=_ethernet_address;

                ARPMessage  arp_pack_send;
                arp_pack_send.sender_ip_address=_ip_address.ipv4_numeric();
                arp_pack_send.sender_ethernet_address=_ethernet_address;
                arp_pack_send.target_ip_address=arp_pack.sender_ip_address;
                arp_pack_send.target_ethernet_address=arp_pack.sender_ethernet_address;
                arp_pack_send.opcode=ARPMessage::OPCODE_REPLY;
                epack.payload()=BufferList(arp_pack_send.serialize());
                
                _frames_out.push(epack);
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    // DUMMY_CODE(ms_since_last_tick); 
    _duration+=ms_since_last_tick;
    auto iter=_storage.begin();
    for(;iter!=_storage.end();)
    {
        iter->second.second-=ms_since_last_tick;
        if(iter->second.second<=0)
        {
            iter=_storage.erase(iter);
        }else
        {
            iter++;
        }
    }
}
