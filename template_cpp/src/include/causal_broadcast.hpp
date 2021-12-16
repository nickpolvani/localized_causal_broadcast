#ifndef FIFO_BROADCAST_H
#define FIFO_BROADCAST_H

#include "packet.hpp"
#include <map>
#include "uniform_reliable_broadcast.hpp"
#include "thread_safe_queue.hpp"
#include <thread>
#include "parser.hpp"
#include "process_controller.hpp"
#include <mutex> 
#include <condition_variable>
#include "vector_clock.hpp"

using namespace packet;

class UniformReliableBroadcast;
class ProcessController;

class CausalBroadcast{

    private:

        std::mutex mutex;
        // condition variable to wait on broadcast until packet is delivered to not flood the network
        std::condition_variable broadcast_cv;

        bool can_broadcast = true;   

        // current packet sequence number for this process
        std::size_t cur_seq_num = 0;
        
        // pending[source_id][seg_num] returns the corresponding packet (process_id does not matter in packets)
        std::map<std::size_t, std::map< std::size_t, Packet>> pending;

        // next[source_id] returns the next expected packet sequence number from process with id source_id
        std::map<long unsigned int, long unsigned int> next;

        // total number of processes in the distributed system
        std::size_t num_processes;

        std::set<std::size_t> locality;

        VectorClock vc_send;

        VectorClock vc_recv;

        // lower level abstraction
        UniformReliableBroadcast * urb = NULL;

        ThreadSafeQueue<Packet> packets_to_deliver;

        // permanent Thread that consumes packets_to_deliver
        void causalDeliver();

        //  Thread creating packets containing messages with
        // seq num from 1 to num_messages included, and broadcast them to other processes
        void broadcast();

        // number of messages to broadcast
        std::size_t num_messages;

        ProcessController * process_controller;

        VectorClock getSendVectorClock();


    public:
        // initializes next, and sets num_messags
        CausalBroadcast(ProcessController * process_controller, std::size_t i_num_messages, std::vector<Parser::Host> hosts, std::set<std::size_t> locality);


        // contains threads that execute fifoDeliver, broadcast
        std::vector<std::thread *> threads;

        // invoked by UniformReliableBroadcast when delivering
        void URBDeliver(Packet p);


        void setUrb(UniformReliableBroadcast * i_urb){
            urb = i_urb;
        }

        // begins execution of fifoDeliver, broadcast, adds thread pointers to
        // threads
        void start();
};

#endif