#include "process_controller.hpp"
#include <sstream>

ProcessController::ProcessController(std::size_t id, Parser parser): 
hosts(parser.hosts()), process_id(id)
{
    //populate host_addresses
    for (Parser::Host host : parser.hosts()){
        sockaddr_in host_addr;
        host_addr.sin_family = AF_INET;
        in_addr ip;
        ip.s_addr = host.ip;
        host_addr.sin_addr = ip;
        host_addr.sin_port = host.port;
        host_addresses[host.id] = host_addr;
    }
    config_file.open(parser.configPath());
    if (config_file.bad()){
        std::cerr << "Error: could not open config file\n";
        exit(EXIT_FAILURE);
    }
    config_file >> num_messages;

    std::string line;
    while(std::getline(config_file, line)){
        std::istringstream iss(line);
        std::size_t cur_process;
        iss >> cur_process;
        if (cur_process != process_id){
            continue;
        }
        else{
            std::size_t other_process;
            while (!iss.eof()){
                iss >> other_process;
                locality.insert(other_process);
            }
        }
    }
    config_file.close();

    output_file.open(parser.outputPath());
    if (output_file.bad()){
        std::cerr << "Error: could not open output file\n";
        exit(EXIT_FAILURE);
    }

}



void ProcessController::onPacketDelivered(packet::Packet p){
    std::string out_content;
    // packet contains multiple messages
    for (std::size_t i = p.first_msg_seq_num; i < p.first_msg_seq_num + p.getNumMessages(); i++){
        out_content += "d " + std::to_string(p.source_id) + " " +  std::to_string(i) + "\n";
    }
    output_file << out_content;
    DEBUG_MSG("Process Controller writing delivered");
}


 void ProcessController::onPacketBroadcast(packet::Packet p){
    std::string out_content;
    for (std::size_t i = p.first_msg_seq_num; i < p.first_msg_seq_num + p.getNumMessages(); i++){
        out_content += "b " + std::to_string(i) + "\n";
    }
    DEBUG_MSG("Process Controller writing broadcast");
    output_file << out_content;
 }


 void ProcessController::start(){
     perfect_link = new PerfectLink(process_id, &host_addresses, host_addresses[process_id].sin_port);
     beb = new BestEffortBroadcast(perfect_link, hosts);
     perfect_link -> setBEB(beb);

     urb = new UniformReliableBroadcast(process_id, hosts.size());
     beb -> setURB(urb);
     urb -> setBEB(beb);

     causal_broadcast = new CausalBroadcast(this, num_messages, hosts, locality);
     urb -> setCausalBroadcast(causal_broadcast);
     causal_broadcast -> setUrb(urb);

     perfect_link -> start();
     beb -> start();
     urb -> start();
     causal_broadcast -> start();

     for (auto thread : perfect_link -> threads){
         thread->join();
     }
     for (auto thread : beb -> threads){
         thread->join();
     }
     for (auto thread : urb -> threads){
         thread->join();
     }
     for (auto thread : causal_broadcast -> threads){
         thread->join();
     }
 }

 
void ProcessController::stopProcess(){
    if (perfect_link != NULL){
        perfect_link->closeSocket();
        std::cout << "Closed socket\n";
    }
    
    output_file.flush();
    output_file.close();
    std::cout << "Closed output files\n";
    std::cout.flush();

} 