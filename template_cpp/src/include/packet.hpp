
#ifndef PACKET_H
#define PACKET_H

#include <queue>
#include <string>
#include <cstring>
#include <stdexcept>
#include "debug.h"
#include <iostream>
#include "vector_clock.hpp"

namespace packet{

const int MAX_LENGTH = 4096; // max length of Packet in bytes

class Message{
    private:
        std::string payload;
        friend class Packet;
        
    public:
        Message(std::string i_payload) : payload(i_payload){}
        //default destructor
        //default copy constructor

        /* return length of string + 1 considering NULL char at the end*/
        int get_length(){
            return static_cast<int>(payload.length()) + 1;
        }

        std::string getContent(){
            return payload;
        }
        
        /* writes message payload into buffer */
        void toBytes(char * buffer){
            std::strcpy(buffer, payload.c_str());
        }

        /*return message with payload from data[0] to data[i], where data[i] is a NULL character*/
        static Message decodeData(char * data){
            int i = 0;
            char cur_char = data[i];
            while (cur_char != '\0'){
                i++;
                cur_char = data[i];
            }
            std::string payload = std::string(data, i);
            return Message(payload);
        }
};

/*
Packet has a sequence number, increased by the sender process every time a new packet is created
A packet contains a vector of messages numbered from first_msg_seq_num to 
first_msg_seq_num + getNumMessages() - 1. A packet is uniquely identfied by process_id and packet_seq_num 
*/
class Packet{
    private:
        std::vector<Message> messages = std::vector<Message>();
        

    public:
        static const int max_length = MAX_LENGTH; // in bytes
        std::size_t process_id;   // process id of the sender
        std::size_t source_id;    // process id of the original process that sent this packet
        std::size_t packet_seq_num;   // sequence number of the packet
        std::size_t first_msg_seq_num = 0; // sequence number of the first message
        std::size_t payload_length = 0; // number of bytes of payload
        std::size_t num_processes; // number of processes in the system
        VectorClock vector_clock;  // has length equal to num_processes
        bool is_ack = false;

        Packet(std::size_t i_process_id, std::size_t i_source_id, std::size_t i_packet_seq_num, 
                        std::size_t i_num_processes, VectorClock i_vector_clock) : 
            process_id(i_process_id), source_id(i_source_id), packet_seq_num(i_packet_seq_num), 
            num_processes(i_num_processes), vector_clock(i_vector_clock){};
        
        Packet(): process_id(0), source_id(0), packet_seq_num(0){};

        // called when re-broadcasting a received message
        void changeSenderId(std::size_t new_id){
            process_id = new_id;
            if(getLength() > MAX_LENGTH){
                throw(std::length_error("Changing sender id is not possible: size: " + std::to_string(getLength()) + " greater than MAX_SIZE\n" ));
            }
        }

        bool canAddMessage(Message m){
            if (is_ack){
                return false;
            }else{
                /*
                unsigned long int new_payload_length = std::to_string(payload_length + m.get_length()).size();
                unsigned long int old_payload_length = std::to_string(payload_length).size();
                unsigned long int new_packet_length = getLength() - old_payload_length + new_payload_length + m.get_length();*/
                return getLength() + m.get_length() <= max_length;
            }
        }


        // see if can add message keeping margin_bytes unset at the end of the buffer
        bool canAddMessage(Message m, unsigned long int margin_bytes){
            if (is_ack){
                return false;
            }else{
                /*
                unsigned long int new_payload_length = std::to_string(payload_length + m.get_length()).size();
                unsigned long int old_payload_length = std::to_string(payload_length).size();
                unsigned long int new_packet_length = getLength() - old_payload_length + new_payload_length + m.get_length();*/
                return getLength() + margin_bytes  + m.get_length() <= max_length;
            }
        }

        void addMessage(Message m){
            if (messages.size() == 0){
                first_msg_seq_num = std::stoul(m.payload);  // payload in this application 
                                                            // is the sequence number of the message
            }
            if (!canAddMessage(m)){
                std::string error_msg = "Packet full, cannot add message with length: " + std::to_string(m.get_length())
                                        + " on packet with length: " + std::to_string(getLength()) + "\n";
                throw(std::length_error(error_msg));
            }else{
                messages.push_back(m);
            }
            payload_length += m.get_length();
        }

        /*transform packet into bytes and writes them into buffer */
        void toBytes(char* buffer);

        unsigned long int getNumMessages(){
            return messages.size();
        }

        // return length of Packet in bytes
        std::size_t getLength(){
            std::size_t header_length = getHeaderLength(); //NULL characters

            std::size_t vc_length = vector_clock.getBytesLength();

            return header_length + vc_length + payload_length; 
        }


        std::size_t getHeaderLength(){
             std::size_t header_length = std::to_string(process_id).size() + 
                                std::to_string(packet_seq_num).size() + 
                                std::to_string(source_id).size() + 
                                std::to_string(first_msg_seq_num).size() +
                                std::to_string(static_cast<unsigned int>(is_ack)).size() +
                                std::to_string(payload_length).size() +
                                std::to_string(num_processes).size() + 7;
            return header_length;
        }


        //return message at position i
        Message getMessage(std::size_t i){
            return messages[i];
        }

        static Packet decodeData(char * data);

        /* i_process_id: id of the process that sent the ack (therefore received the corresponding message).
           i_progressive_number: progressive number of the message received
        */
        static Packet createAck(std::size_t i_process_id, std::size_t i_source_id, std::size_t i_packet_seq_num,
                                std::size_t num_processes, VectorClock vector_clock){
            Packet ackPacket = Packet(i_process_id, i_source_id, i_packet_seq_num, num_processes, vector_clock);
            ackPacket.is_ack = true;
            ackPacket.payload_length = 0;
            return ackPacket;
        }

};
}

#endif