#include <zmq.hpp>
#include <ctime>
#include <cassert>
#include <cstdlib>        // random()  RAND_MAX
#include <cstdio>
#include <cstdarg>
#include <csignal>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <iomanip>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>

#include "zhelpers.hpp"
#include "json.hpp"

using json = nlohmann::json;

std::vector<std::string> bridge;
std::mutex mute;

void client_tasks(std::string client_name){
    zmq::context_t context(1);
    zmq::socket_t client(context , ZMQ_DEALER);

    srandom((unsigned)time(NULL));
    #if (defined (WIN32))
        s_set_id(client, (intptr_t)args);
    #else
        s_set_id(client);          //Set a printable identity
    #endif

    client.connect("tcp://localhost:5672");
    
    s_sendmore(client , "");
    s_send(client , client_name);
    
    zmq::pollitem_t item[]{
        {client , 0 , ZMQ_POLLIN,0}
    };
    while(1){
        
        int rc = zmq::poll(item , 1 , 10);
        if(rc>0){//receiving
            if(item[0].revents & ZMQ_POLLIN){
                s_recv(client); 
                std::cout<<"received"<<std::endl;
                std::string s ;
                s = s_recv(client);
                json j = json::parse(s);
                std::string from = j["name"];
                std::string message = j["message"];
                std::cout<<"*New Message from "<<from<<": "<<message<<std::endl;
            }
        }
        
        else if(rc == 0 &&  !bridge.empty()){//sending
            mute.lock();
            if(!bridge.empty()){
                std::string string_json_message = bridge.back();
                bridge.pop_back();
        
                s_sendmore(client  ,"");
                s_send(client , string_json_message);
            }
            mute.unlock();
        }
        
    }   
}
void run(){
    while (1){
        std::cout<<"who do you want to message?"<<std::endl;
        std::string receiver;
        std::cin>>receiver;
        
        std::cin.ignore();
        std::cout<<"type the message: "<<std::endl;
        std::string msg;
        std::getline(std::cin , msg);
        
        json json_message;
        json_message["name"] = receiver;
        json_message["message"] = msg;
        std::string js = json_message.dump();
        
        mute.lock();
        bridge.push_back(js);
        mute.unlock();
    }
    
}

int main(){
    std::cout<<"Your name is: "<<std::endl;
    std::string client_name;
    std::cin>>client_name;

    std::thread with_server(client_tasks,client_name);
    std::thread with_user(run);
    with_server.join();
    with_user.join();
    
    
    
    

}