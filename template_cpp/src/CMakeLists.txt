# DO NAME THE SYMBOLIC VARIABLE `SOURCES`

include_directories(include)
set(SOURCES src/main.cpp src/hello.c src/packet.cpp src/udp_socket.cpp 
src/outbox.cpp src/perfect_link.cpp src/best_effort_broadcast.cpp src/uniform_reliable_broadcast.cpp
src/causal_broadcast.cpp src/process_controller.cpp) 

# DO NOT EDIT THE FOLLOWING LINE
find_package(Threads)
add_executable(da_proc ${SOURCES})
target_link_libraries(da_proc ${CMAKE_THREAD_LIBS_INIT})

