#include <pthread.h>
#include <zmq.hpp>
#include <ctime>
#include <cassert>
#include <cstdlib> // random()  RAND_MAX
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

int main()
{
    zmq::context_t context(1);
    zmq::socket_t broker(context, ZMQ_ROUTER);

    broker.bind("tcp://*:5672");
    std::cout << "Connected" << std::endl;

    std::vector<std::string> id_list;
    std::vector<std::string> name_list;
    std::unordered_map<std::string, std::string> id_to_name;

    std::cout << "How many clients?" << std::endl;
    int number_of_clients;
    std::cin >> number_of_clients;
    std::cout << "Waiting for " << number_of_clients << " clients to connect..." << std::endl;

    for (int i = 0; i < number_of_clients; ++i)
    {
        if (i > 0)
            std::cout << "Waiting for the next client..." << std::endl;
        std::string identity = s_recv(broker);

        s_recv(broker);                    //Envelope delimiter
        std::string name = s_recv(broker); //Say their name

        id_list.push_back(identity);
        name_list.push_back(name);
        id_to_name[identity] = name;

        std::cout << name << " connected" << std::endl;
    }
    std::cout << "All are connected" << std::endl;
    //**Messaging**//
    while (1)
    {
        std::cout << "Waiting for incoming message to send out... " << std::endl;

        std::string sender_id = s_recv(broker);
        s_recv(broker); //envelope delimiter
        std::string string_json_message = s_recv(broker);

        json msg = json::parse(string_json_message.c_str());
        std::string rcvr_name = msg["name"];
        std::string message = msg["message"];

        std::string rcvr_id;
        for (auto it = id_to_name.begin(); it != id_to_name.end(); ++it)
        {
            if (it->second == rcvr_name)
                rcvr_id = it->first;
        }

        json j_to_send;
        j_to_send["name"] = id_to_name[sender_id];
        j_to_send["message"] = message;

        std::string s_to_send = j_to_send.dump();

        s_sendmore(broker, rcvr_id);
        s_sendmore(broker, "");
        s_send(broker, s_to_send);
        std::cout << "sent" << std::endl;
    }
    return 0;
}
