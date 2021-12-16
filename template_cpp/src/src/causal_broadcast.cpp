#include "causal_broadcast.hpp"



CausalBroadcast::CausalBroadcast(ProcessController * i_process_controller, std::size_t i_num_messages,
                                 std::vector<Parser::Host> hosts, std::set<std::size_t> i_locality){
    num_processes = hosts.size();
    num_messages = i_num_messages; 
    process_controller = i_process_controller; 
    locality = i_locality;    
    vc_send = VectorClock(hosts.size());
    vc_recv = VectorClock(hosts.size());
}


VectorClock CausalBroadcast::getSendVectorClock(){
    std::size_t process_id = process_controller->process_id;
    VectorClock vector_clock_send(vc_send);
    vector_clock_send.assign(process_id, cur_seq_num);
    return vector_clock_send;
        
}

void CausalBroadcast::URBDeliver(Packet p){
    pending[p.vector_clock][p.source_id] = p;
    for (auto it_vc = pending.begin(); it_vc != pending.end(); /* no increment */){
        VectorClock cur_vc = it_vc->first;

        // deliver packets with older vector clocks
        if (cur_vc <= p.vector_clock){  
            // iterate for all source processes
            for (auto it_s = it_vc->second.begin(); it_s != it_vc->second.end(); it_s++){
                std::size_t source_id = it_s -> first;
                Packet cur_packet = it_s -> second;
                vc_recv.increase(source_id);
                if (locality.count(source_id) == 1){
                    vc_send.increase(source_id);
                }
                packets_to_deliver.push(cur_packet);
            }
            // eliminate all those vector clocks from pending and increment iterator
            it_vc = pending.erase(it_vc);
        }
        else{ // since pending is ordered in terms of vector clocks, we don't have lower vector clocks in pending
            break;
        }
    }
}

void CausalBroadcast::causalDeliver(){
    while(true){
        Packet p = packets_to_deliver.pop();
        DEBUG_MSG("FIFO: about to deliver packet: " <<  p.source_id << " " << p.packet_seq_num);
        // if I deliver the packet I broadcasted I can broadcast the next one
        if (p.source_id == process_controller ->process_id){
            can_broadcast = true;
            broadcast_cv.notify_all();
        }
        process_controller -> onPacketDelivered(p);
    }
}


void CausalBroadcast::broadcast(){
    assert ((urb != NULL) == true);
    DEBUG_MSG("number of packets to send: " << num_messages << "\n");
    unsigned long int process_id = process_controller -> process_id;

    std::unique_lock lock(mutex);


    // create and send Packets
    VectorClock vector_clock_send = getSendVectorClock();

    Packet curr_packet(process_id, process_id, cur_seq_num, num_processes, vector_clock_send);
    for (std::size_t i = 1; i <= num_messages; i++){
        Message curr_message(std::to_string(i));

        while(!can_broadcast){
            broadcast_cv.wait(lock);
        }

        // packet is full (leave a margin of 5 bytes to change the process_id when re-broadcasting messages)
        if (!curr_packet.canAddMessage(curr_message, 5)){  
            DEBUG_MSG("FIFO: trying to urb broadcast packet, seq_num: " << curr_packet.packet_seq_num << "\n");
            process_controller -> onPacketBroadcast(curr_packet);
            // set before because otherwise I could execute the delivery and then set this variable to false again
            // having a deadlock
            can_broadcast = false;   
            urb -> broadcast(curr_packet);
            cur_seq_num++;
            vector_clock_send = getSendVectorClock();
            curr_packet = Packet(process_id, process_id, cur_seq_num, num_processes, vector_clock_send);
        }
        curr_packet.addMessage(curr_message);

        // last message, so send packet even if not full
        if (i == num_messages){ 
            DEBUG_MSG("FIFO: trying to urb broadcast packet, seq_num: " << curr_packet.packet_seq_num << "\n");
            process_controller -> onPacketBroadcast(curr_packet);
            // set before because otherwise I could execute the delivery and then set this variable to false again
            // having a deadlock
            can_broadcast = false;
            urb -> broadcast(curr_packet);
            cur_seq_num++;
        }
    }
}



void CausalBroadcast::start(){
    std::thread * deliver_thread = new std::thread([this] {this -> causalDeliver();});
    std::thread * broadcast_thread = new std::thread([this] {this -> broadcast();});

    threads.push_back(deliver_thread);
    threads.push_back(broadcast_thread);
}