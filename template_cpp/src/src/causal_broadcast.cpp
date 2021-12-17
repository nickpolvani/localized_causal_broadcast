#include "causal_broadcast.hpp"



CausalBroadcast::CausalBroadcast(ProcessController * i_process_controller, std::size_t i_num_messages,
                                 std::vector<Parser::Host> hosts, std::set<std::size_t> i_locality){
    num_processes = hosts.size();
    num_messages = i_num_messages; 
    process_controller = i_process_controller; 
    locality = i_locality;    
    vc_send = VectorClock(hosts.size());
    vc_recv = VectorClock(hosts.size());
    for (auto host : hosts){
        next[host.id] = 0;
    }
}


VectorClock CausalBroadcast::getSendVectorClock(){
    std::size_t process_id = process_controller->process_id;
    VectorClock vector_clock_send(vc_send);
    vector_clock_send.assign(process_id, cur_seq_num);
    return vector_clock_send;
        
}

void CausalBroadcast::URBDeliver(Packet p){
    std::unique_lock<std::mutex> lock(mutex);
    while(can_broadcast){
        broadcast_cv.wait(lock);
    }
    DEBUG_MSG("About to deliver packet " << p.packet_seq_num << " from process: " << p.source_id);
    pending[p.source_id][p.packet_seq_num] = p;
    while(pending[p.source_id].count(next[p.source_id]) == 1){

        Packet cur_packet = pending[p.source_id][next[p.source_id]];
        if (cur_packet.vector_clock <= vc_recv){
            next[cur_packet.source_id] = next[cur_packet.source_id] + 1;
            pending[cur_packet.source_id].erase(cur_packet.packet_seq_num);
            vc_recv.increase(p.source_id);
            if (locality.count(p.source_id) == 1){
                vc_send.increase(p.source_id);
            }
            causalDeliver(cur_packet);
        }
        else{
            break;
        }
    }
   
}

void CausalBroadcast::causalDeliver(Packet p){
    DEBUG_MSG("CAUSAL: about to deliver packet: " <<  p.source_id << " " << p.packet_seq_num);
    // if I deliver the packet I broadcasted I can broadcast the next one
    process_controller -> onPacketDelivered(p);
    if (p.source_id == process_controller ->process_id && (!end_broadcast)){
        can_broadcast = true;
        broadcast_cv.notify_all();
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

            broadcast_cv.notify_all();  //broadcasted, so I wake up deliver thread
            while(!can_broadcast){
                broadcast_cv.wait(lock);
            }

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
            end_broadcast = true;
            urb -> broadcast(curr_packet);
            cur_seq_num++;
            broadcast_cv.notify_all();
        }
    }
    DEBUG_MSG("Finished broadcasting");
}



void CausalBroadcast::start(){
    std::thread * broadcast_thread = new std::thread([this] {this -> broadcast();});

    threads.push_back(broadcast_thread);
}