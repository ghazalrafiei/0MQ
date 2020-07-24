#include <pthread.h>
#include <thread>
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
#include <sstream>
#include <unordered_map>

#include "zhelpers.hpp"
#include "json.hpp"

using json = nlohmann::json;

std::unordered_map<std::string , std::string> id_to_name;

bool contain(std::vector<std::string> &v, std::string id){
    for(auto t : v){
        if( t==id){
            return true;
        }
    }
    return false;
}

std::string name_to_id(std::string name){
    for(auto it = id_to_name.begin(); it!= id_to_name.end(); ++it){
            if((it->second) == name);
            return (it->first);
        }
    return "";
}

int main(){
    

    zmq::context_t context(1);
    zmq::socket_t broker(context, ZMQ_ROUTER); 

    broker.bind("tcp://*:5672");
    std::cout<<"Connected"<<std::endl;

    while(1){
        std::string identity = s_recv(broker);
        s_recv(broker);
        std::string comming_message = s_recv(broker);

        json json_message = json::parse(comming_message);

        if(json_message["method"] == "send_message"){
            json params = json_message["params"];
            std::string rcvr_name = params["to"];
            std::string rcvr_id = name_to_id(rcvr_name);
    
            if(rcvr_id != ""){
                s_send(broker , rcvr_id);
                std::cout<<id_to_name[identity]<<" to "<<rcvr_id<<" "<<name_to_id(rcvr_id);
                s_send(broker , "");
                s_send(broker , params.dump());
                std::cout<<"A message sent to "<<id_to_name[rcvr_id]<<" "<<params<<std::endl;
            }
            else{
                std::cout<<"receiver does not exist";
            //reply to sender
            }
        }

        else if(json_message["method"] == "login"){
            std::string name = json_message["params"]["name"];
            id_to_name[identity] = name;
            std::cout<<name<<" connected "<<identity<<std::endl;
        }


    }

/// format:  {method = s_m, params = {message , to , from }
/// format   {method = login, params = {myname}


    return 0;
}
