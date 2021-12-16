#ifndef PACKET_PROC_ID_H
#define PACKET_PROC_ID_H

#include "packet.hpp"
// packet to be sent, needs also process id of the destination host
struct Packet_ProcId{
    packet::Packet packet;
    std::size_t dest_proc_id;
    Packet_ProcId(packet::Packet p, std::size_t dest_id): packet(p), dest_proc_id(dest_id){}
};

#endif