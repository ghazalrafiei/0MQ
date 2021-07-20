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

std::string sha256_encode(std::string str){
  return cryptlite::hmac<cryptlite::sha256>::calc_hex(str, H_KEY);
}

std::string hmac_sha256(std::string username, std::string password){
  return sha256_encode(username+password);
}

std::pair<std::string, std::string> get_user_pass() {

  std::string username, password;

  std::cout << "Username: ";
  std::cin >> username;

  std::cout << "Password: ";
  std::cin >> password;

  std::cin.ignore();

  return std::pair<std::string, std::string>(username, password);
}

json prepare_json(std::string username, std::string password, std::string method){

  json signin_query;
  signin_query["method"] = method;

  json params;
  params["username"] = username;
  params["password"] = password;
  params["code"] = hmac_sha256(username, password);
  signin_query["params"] = params;

  return signin_query;
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
    json_message["method"] = "new_message";

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

        if (j["method"] == "new_message") {

          std::cout << "*New Message from " 
                    << j["params"]["from"] << ": "
                    << j["params"]["message"] << std::endl;
        }

        else if(j["method"] == "error"){
          std::cout<<"Error: "<<j["params"]["emessage"]<<std::endl;
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

bool start_connection(std::unique_ptr<zmq::socket_t>& client,std::string username, std::string password, std::string method){

  client = std::move(client);

  std::stringstream identity;
  identity << "ID-" << sha256_encode(username).substr(0,10);
  client->setsockopt(ZMQ_IDENTITY, identity.str().c_str(),
                     identity.str().length());

  client->connect("tcp://localhost:5672");
  std::cout<<"Connected"<<std::endl;

  json json_ready = prepare_json(username, password, method);

  s_sendmore(*client, "");
  s_send(*client, json_ready.dump());

  s_recv(*client);
  std::string new_message = s_recv(*client);
  std::string result = json::parse(new_message)["params"]["result"];
  std::cout<<"reult = "<<result<<std::endl;

  if (result == "SUCCEED")
    return true;

  else if (result == "FAILED")
    return false;

  return false;

}

void run(){
  zmq::context_t context(1);
  std::unique_ptr<zmq::socket_t> client(new zmq::socket_t(context, ZMQ_DEALER));

  char command = '0';

  std::string username, password;
  
  while (!(command == '1' or command == '2')) {
    std::cout << "1- Login\n2- Sign up" << std::endl;

    std::cin >> command;
    if (command == '1' || command == '2') {

      std::pair<std::string, std::string> user_pass = get_user_pass();

      username = user_pass.first;
      password = user_pass.second;

      std::string method = (command == '1' ? "login" : "signup");
      bool connection_succeed =
          start_connection(std::ref(client), username, password, method);
      if(connection_succeed){
        std::cout << "successful " << method << std::endl;
      }
      else{
        std::cout << "Unsuccessful " << method << ". Try again." << std::endl;
        command = '0';
      }

    } 
    else{
      std::cout << "Try again." << std::endl;
    }
  }

    std::thread with_user(user_communicator, username);
    std::thread chat_handle(chat, std::ref(client));

    with_user.join();
    chat_handle.join();
}

int main(){
  run();
}
