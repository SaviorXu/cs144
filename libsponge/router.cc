#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    RouterElem elem(route_prefix,prefix_length,next_hop,interface_num);
    RouterVec.push_back(elem);
}

uint32_t preLen2num(uint8_t preLen)
{
    uint32_t sum=0;
    int st=32-preLen;
    for(int i=st;i<32;i++)
    {
        sum+=(1<<i);
    }
    return sum;
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // DUMMY_CODE(dgram);
    // Your code here.
    bool flag=false;
    size_t index=-1;
    uint8_t max_len=0;
    for(size_t i=0;i<RouterVec.size();i++)
    {
        std::cout<<Address::from_ipv4_numeric(preLen2num(RouterVec[i].prefix_length)).ip()<<" "<<Address::from_ipv4_numeric(dgram.header().dst).ip()<<" "<<Address::from_ipv4_numeric(RouterVec[i].route_prefix).ip()<<std::endl;
        if(((preLen2num(RouterVec[i].prefix_length)&dgram.header().dst)==RouterVec[i].route_prefix)&&max_len<=RouterVec[i].prefix_length)
        {
            max_len=RouterVec[i].prefix_length;
            index=i;
            flag=true;
        }
    }
    if(flag)
    {
        if(dgram.header().ttl<=1)
        {
            return;
        }else
        {
            dgram.header().ttl-=1;
            Address next_hop=RouterVec[index].next_hop.has_value()?RouterVec[index].next_hop.value():Address::from_ipv4_numeric(dgram.header().dst);
            interface(RouterVec[index].interface_num).send_datagram(dgram,next_hop);
        }
    }
    return;
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
