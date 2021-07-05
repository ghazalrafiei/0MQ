project(Jikjik)

set(CMAKE_CXX_STANDARD 17)


add_executable(jk-client client.cpp)

target_link_libraries(jk-client zmq pthread)

