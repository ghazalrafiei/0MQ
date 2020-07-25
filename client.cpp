#include <zmq.hpp>
#include <ctime>
#include <cassert>
#include <cstdlib> // random()  RAND_MAX
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

std::vector<json> bridge;
std::mutex mute;

void client_tasks(std::string client_name)
{
    zmq::context_t context(1);
    zmq::socket_t client(context, ZMQ_DEALER);

    srandom((unsigned)time(NULL));
#if (defined(WIN32))
    s_set_id(client, (intptr_t)args);
#else
    s_set_id(client); //Set a printable identity
#endif

    client.connect("tcp://localhost:5672");

    json login_json;
    login_json["method"] = "login";

    json params;
    params["name"] = client_name;

    login_json["params"] = params;

    s_sendmore(client, "");
    s_send(client, login_json.dump());

    zmq::pollitem_t item[]{
        {client, 0, ZMQ_POLLIN, 0}};
    while (1)
    {
        int rc = zmq::poll(item, 1, 150);
        if (rc > 0)
        {
            if (item[0].revents & ZMQ_POLLIN)
            {
                s_recv(client);
                std::string s = s_recv(client);
                json j = json::parse(s);
                if (j["method"] == "receive_message")
                {
                    std::string from = j["params"]["from"];
                    std::string message = j["params"]["message"];
                    std::cout << "*New Message from " << from << ": " << message << std::endl;
                }
            }
        }

        else if (rc == 0 && !bridge.empty())
        { //sending
            mute.lock();
            if (!bridge.empty())
            {
                std::string string_json_message = bridge.back().dump();
                bridge.pop_back();

                s_sendmore(client, "");
                s_send(client, string_json_message);
            }
            mute.unlock();
        }
    }
}
void run(std::string my_name)
{
    std::cout << "Who do you want to chat with?" << std::endl;
    std::string rcvr_name;
    std::cin >> rcvr_name;
    std::cin.ignore();

    while (1)
    {
        std::cout << "Type..." << std::endl;
        std::string msg;
        std::getline(std::cin, msg);

        json json_message;
        json_message["method"] = "send_message";

        json params;
        params["from"] = my_name;
        params["to"] = rcvr_name;
        params["message"] = msg;

        json_message["params"] = params;

        mute.lock();
        bridge.push_back(json_message);
        mute.unlock();
    }
}

int main()
{
    std::cout << "Your name is: " << std::endl;
    std::string client_name;
    std::cin >> client_name;
    std::cin.ignore();
    std::thread with_server(client_tasks, client_name);
    std::thread with_user(run, client_name);

    with_server.join();
    with_user.join();
}

