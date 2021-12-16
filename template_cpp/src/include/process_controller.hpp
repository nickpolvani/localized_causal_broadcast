#ifndef PROCESS_CONTROLLER_H
#define PROCESS_CONTROLLER_H


#include "parser.hpp"
#include "packet.hpp"
#include <map>

#include <iostream>
#include <fstream>
#include "perfect_link.hpp"
#include "best_effort_broadcast.hpp"
#include "uniform_reliable_broadcast.hpp"
#include "causal_broadcast.hpp"

// the process either sends o receives messages,
// so it will have only one between PLSender and PLReceiver

class PerfectLink;
class CausalBroadcast;
class BestEffortBroadcast;
class UniformReliableBroadcast;


class ProcessController{
    private:

        std::vector<Parser::Host> hosts;
        
        std::ofstream output_file;

        std::ifstream config_file;

        std::size_t num_messages; // number of messages to broadcast

        PerfectLink * perfect_link = NULL;

        BestEffortBroadcast * beb = NULL;

        UniformReliableBroadcast * urb = NULL;

        CausalBroadcast * causal_broadcast = NULL;

        std::set<std::size_t> locality;
        
    public:

        /* store addresses of hosts, key is process_id (accessed only in read after creation)
            so it is Thread safe
        */
        std::map<std::size_t, sockaddr_in> host_addresses;

        std::size_t process_id;
        
        // initializes member variables
        ProcessController(std::size_t id, Parser parser);

        // write packet delivered to output file
        void onPacketDelivered(packet::Packet p);

        // write packet sent to output file
        void onPacketBroadcast(packet::Packet p);

        void stopProcess();

        // initialize lower level abstractions and start broadcasting/delivering packets
        void start();
};

#endif