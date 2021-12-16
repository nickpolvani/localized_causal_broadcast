#ifndef VECTOR_CLOCK_H
#define VECTOR_CLOCK_H

#include <vector>
#include <cstddef>
#include <assert.h>

class VectorClock{
    private:
        std::size_t num_processes;
        std::vector<std::size_t> values;

    public:
        VectorClock(){}

        VectorClock(std::size_t i_num_processes): num_processes(i_num_processes){
            for (std::size_t i = 0; i < num_processes; i++){
                values.push_back(0);
            }
        }

        VectorClock(std::size_t i_num_processes, std::size_t * i_values): num_processes(i_num_processes){
            for (std::size_t i = 0; i < num_processes; i++){
                values.push_back(i_values[i]);
            }
        }


        // process ids go from 1 to N, indices from 0 to N-1
        void increase(std::size_t id_process){
            std::size_t idx = id_process - 1;
            assert((idx < num_processes) == true);
            values[idx] = values[idx] + 1;
        }


        void assign(std::size_t id_process, std::size_t val){
            std::size_t idx = id_process - 1;
            assert((idx < num_processes) == true);
            values[idx] = val;
        }

        // 1 <= id_process <= num_processes
        std::size_t getValue(std::size_t id_process){
            assert(((id_process >= 1) && (id_process <= num_processes))==true);
            std::size_t idx = id_process - 1;
            return values[id_process];
        }


        bool operator <(const VectorClock& v2) const{
            bool strictly_lower = false;
            assert((num_processes == v2.num_processes) == true);
            for (std::size_t i = 0; i < num_processes; i++){
                if (values[i] > v2.values[i]){
                    return false;
                }
                else if (values[i] < v2.values[i]){
                    strictly_lower = true;
                }
            }
            return strictly_lower;
        }


        bool operator <= (const VectorClock& v2) const{
            assert((num_processes == v2.num_processes) == true);
            for (std::size_t i = 0; i < num_processes; i++){
                if (values[i] > v2.values[i]){
                    return false;
                }
            }
            return true;
        }

        // writes vector clock values separated by '\0' as char representation to buffer,
        // returns number of bytes written
        std::size_t toBytes(char * buffer){
            char* cur_pointer = buffer;
            std::size_t num_bytes = 0;
            for (std::size_t i = 0; i < num_processes; i++){
                const char* cur_val = std::to_string(values[i]).c_str();
                std::size_t size = std::to_string(values[i]).size() + 1;
                std::strcpy(cur_pointer, cur_val);
                cur_pointer += size;
                num_bytes += size;
            }
            return num_bytes;
        }

        // return length of bytes representation
        std::size_t getBytesLength(){
            std::size_t vc_length = 0;
            for (std::size_t id = 1; id <= num_processes; id++){
                vc_length += std::to_string(getValue(id)).size() + 1;//NULL char 
            }
            return vc_length;
        }

        // from char* representation to VectorClock
        static VectorClock decodeData(char * data, std::size_t num_processes){
            VectorClock res = VectorClock(num_processes);
            char* cur_pointer = data;
            for (std::size_t i = 0; i < num_processes; i++){
                std::size_t size = 0;
                while (cur_pointer[size] != '\0'){
                    size++;
                }
                res.values[i] = std::stoul(std::string(cur_pointer, size));
                cur_pointer += size;
            }
            return res;
        }


};


#endif