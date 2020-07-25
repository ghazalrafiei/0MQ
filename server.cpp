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

std::unordered_map<std::string, std::string> id_to_name;

bool contain(std::vector<std::string> &v, std::string id)
{
    for (auto t : v)
    {
        if (t == id)
        {
            return true;
        }
    }
    return false;
}

std::string name_to_id(std::string name)
{
    for (auto it = id_to_name.begin(); it != id_to_name.end(); ++it)
    {
        if (it->second == name)
            return it->first;
    }
    return "";
}

void handle_send(zmq::socket_t &broker, json params)
{
    std::string rcvr_name = params["to"];
    std::string rcvr_id = name_to_id(rcvr_name);

    json to_send;
    to_send["params"] = params;
    to_send["method"] = "receive_message";
    if (rcvr_id != "")
    {
        s_sendmore(broker, rcvr_id);
        s_sendmore(broker, "");
        bool s = s_send(broker, to_send.dump());
        std::cout << "A message has been sent to " << rcvr_name << "." << std::endl;
    }
    else
    {
        std::cout << "receiver does not exist";
    }
}

int main()
{

    zmq::context_t context(1);
    zmq::socket_t broker(context, ZMQ_ROUTER);

int main(){
    

    while (1)
    {
        std::string identity = s_recv(broker);
        s_recv(broker);
        std::string comming_message = s_recv(broker);

        json json_message = json::parse(comming_message);

        if (json_message["method"] == "send_message")
        {
            json params = json_message["params"];
            handle_send(broker, params);
        }
        else if (json_message["method"] == "login")
        {
            std::string name = json_message["params"]["name"];
            id_to_name[identity] = name;
            std::cout << name << " connected." << std::endl;
        }
    }

    return 0;
}