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

#include "helpers/zhelpers.hpp"
#include "helpers/json.hpp"

#include "helpers/cryptlite/base64.h"
#include "helpers/cryptlite/hmac.h"
#include "helpers/cryptlite/sha1.h"
#include "helpers/cryptlite/sha256.h"

// #define ENABLE_DEBUG

using json = nlohmann::json;

const std::string H_KEY = "9fj3bx8rto8475ljdslkfu8787qa";
std::vector<json> bridge;
std::mutex mute;

std::string SHA256(std::string username, std::string password){

  std::string result =  cryptlite::hmac<cryptlite::sha256>::calc_hex(username+password, H_KEY);

  // std::string result = "";
  // for (int i = 0; i<32; i++){
  //   char k = digest[i];
  //   std::cout<<k<<std::endl;
  //   result += k;
  // }
  return result;
}

void user_communicator(std::string my_name) {

  std::cout << "Who to chat with?" << std::endl;
  std::string rcvr_name;
  std::cin >> rcvr_name;
  std::cin.ignore();

  while (1) {
    std::cout << "Type..." << std::endl;
    std::string msg;
    std::getline(std::cin, msg);

    json json_message;
    json_message["method"] = "send_message";

    json params;
    params["to"] = rcvr_name;
    params["message"] = msg;

    json_message["params"] = params;

    mute.lock();
    bridge.push_back(json_message);
    mute.unlock();
  }
}

void chat(std::unique_ptr<zmq::socket_t>& client){

  client = std::move(client);
  zmq::pollitem_t item[]{{*client, 0, ZMQ_POLLIN, 0}};
  while (1) {
    int rc = zmq::poll(item, 1, 150);
    if (rc > 0) {
      if (item[0].revents & ZMQ_POLLIN) {
        s_recv(*client);
        std::string s = s_recv(*client);
        json j = json::parse(s);
        
        #ifdef ENABLE_DEBUG
        std::clog<<j<<std::endl;
        #endif
        
        if (j["method"] == "receive_message") {
          std::string from = j["params"]["from"];
          std::string message = j["params"]["message"];
          std::cout << "*New Message from " << from << ": " << message << std::endl;
        }
      }
    }

    else if (rc == 0 && !bridge.empty()) { // sending
      mute.lock();
      if (!bridge.empty()) {

        std::string string_json_message = bridge.back().dump();
        bridge.pop_back();

        s_sendmore(*client, "");
        s_send(*client, string_json_message);
        #ifdef ENABLE_DEBUG
          std::clog<<string_json_message<<std::endl;
        #endif
      }
      mute.unlock();
    }
  }
}

std::pair<std::string, std::string> get_user_pass(){
  
  std::string client_name, password;
  
  std::cout << "Username: ";
  std::cin >> client_name;

  std::cout << "Password: ";
  std::cin >> password;

  std::cin.ignore();
  
  return std::pair<std::string, std::string> (client_name, password);
}

void login_signup_handle(){//separate these

    // client = std::move(client);
    zmq::context_t context(1);
    std::unique_ptr<zmq::socket_t> client(new zmq::socket_t(context, ZMQ_DEALER));

    srandom((unsigned)time(NULL));

    std::stringstream identity;
    identity << "ID-" << rand() % 1000 + 1000;
    client->setsockopt(ZMQ_IDENTITY, identity.str().c_str(),
                              identity.str().length());

    client->connect("tcp://localhost:5672");

    std::cout << "Connected" << std::endl;

    unsigned short int command = -1;

    std::string client_name, password;
    while (!(command == 1 or command == 2)) {
      std::cout << "1- Login\n2- Sign up" << std::endl;

      std::cin >> command;
      if (command == 1 || command == 2) {

        std::pair<std::string, std::string> user_pass = get_user_pass();

        client_name = user_pass.first;
        password = user_pass.second;

        json signin_query;
        signin_query["method"] = (command == 1 ? "login" : "signup");
        
        json params;
        params["username"] = client_name;
        params["password"] = password;
        params["code"] = SHA256(client_name, password);
        signin_query["params"] = params;

        s_sendmore(*client,"");
        s_send(*client,signin_query.dump());

        s_recv(*client);
        std::string result = s_recv(*client);
        json login_query_result = json::parse(result);
        
        if (login_query_result["params"]["result"] == "unsuccessful") {
          std::cout << "Unsuccessful " << (command == 1 ? "login" : "signup")
                    << ". Try again."<<std::endl;
          command = -1;
        } 
        else if (login_query_result["params"]["result"] == "succeed") {
          std::cout << "successful " << (command == 1 ? "login." : "signup.")<<std::endl;
        }
      } 
      else if(command != 1 or command != 2){
        std::cout << "Try again." << std::endl;

      }
    }

    std::thread with_user(user_communicator, client_name);
    std::thread chat_handle(chat, std::ref(client));

    with_user.join();
    chat_handle.join();
    
}


int main(){
    
 

  login_signup_handle();
}
