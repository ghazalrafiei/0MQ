#include <pthread.h>
#include <zmq.hpp>

#include <ctime>
#include <cassert>
#include <cstdlib>        // random()  RAND_MAX
#include <cstdio>
#include <cstdarg>
#include <csignal>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <iomanip>
#include <string>
#include <sstream>
#include "zhelpers.hpp"

bool contain(std::vector<std::string> &v, std::string id){
    for(auto t : v){
        if( t==id){
            return true;
        }
    }
    return false;
}

int main() {
     
    zmq::context_t context(1);
    zmq::socket_t broker(context, ZMQ_ROUTER); 

    broker.bind("tcp://*:5672");
    std::cout<<"Connected"<<std::endl;

    std::vector<std::string> id_list;
    std::cout<<"Waiting for two clients to connect..."<<std::endl;

    for(int i = 0; i < 2; ++i){
        if(i>0) std::cout<<"Waiting for the next client..."<<std::endl;
        std::string identity = s_recv(broker);
        s_recv(broker);     //Envelope delimiter
        s_recv(broker);     //Say hello

        if( !contain(id_list, identity) ){
            id_list.push_back(identity);
        }

        std::cout<<"A new client connected"<<std::endl;
    }

    while(1){
        
            std::cout<<"\nType a messsage to send to all: "<<std::endl;
            std::string message_to_send;
            std::getline(std::cin , message_to_send);
        
        for(auto v : id_list){
            s_sendmore(broker,v);
            s_sendmore(broker, "");
            s_send(broker , message_to_send);
        }
        std::cout<<"\nSent to all"<<std::endl;
        
}
    return 0;
}