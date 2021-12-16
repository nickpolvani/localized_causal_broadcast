#ifndef URB_H
#define URB_H

#include "packet.hpp"
#include <map>
#include <set>
#include "best_effort_broadcast.hpp"
#include <mutex>
#include "causal_broadcast.hpp"
#include "thread_safe_queue.hpp"
#include <thread>


using namespace packet;

class BestEffortBroadcast;
class CausalBroadcast;

class UniformReliableBroadcast{

    private:
        std::size_t process_id;

        // delivered[source_id] returns the set of sequence numbers of delivered packets
        std::map<std::size_t, std::set<std::size_t>> delivered;

        // called by BEBDeliver(), broadcast() to access pending
        std::mutex pending_mutex;

        // pending[source_id] returns set of packet_seq_num of pending packets
        std::map<std::size_t, std::set<std::size_t>> pending;
        
        // acks[source_id][seq_num] returns set of processes that have re-sent the packet with corresponding
        // source_id, packet_seq_num 
        std::map<std::size_t, std::map<std::size_t, std::set<std::size_t>>> acks;

        std::size_t num_processes;

        // lower level abstraction
        BestEffortBroadcast* beb = NULL;

        // higher level abstraction
        CausalBroadcast* causal_broadcast = NULL;

        // contains packets that need to be delivered, populated by BEBDeliver 
        // that checks if packets can be URBDelivered
        ThreadSafeQueue<Packet> packets_to_deliver;

        // checks if packet was retransmitted by a majority of processes (looking at number of acks)
        bool canDeliver(std::size_t source_id, std::size_t seq_num);

        // permanent thread consuming packets_to_deliver
        void URBDeliver();        


    public:

        UniformReliableBroadcast(std::size_t i_process_id, std::size_t i_num_processes) : 
        process_id(i_process_id), num_processes(i_num_processes){}

        
        // contains active threads (URBDeliver)
        std::vector<std::thread *> threads;

        // deliver function invoked by Best Effort Broadcast 
        // (lower level abstraction)
        void BEBDeliver(Packet p);

        void setBEB(BestEffortBroadcast* i_beb){
            beb = i_beb;
        }

        void setCausalBroadcast(CausalBroadcast* i_causal_broadcast){
            causal_broadcast = i_causal_broadcast;
        }

        void broadcast(Packet p);

        // begins execution of threads and adds them to threads
        void start();

};

#endif