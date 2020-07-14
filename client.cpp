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

#include "zhelpers.hpp"

int main(){
    
    zmq::context_t context(1);
    zmq::socket_t worker(context, ZMQ_DEALER);
    
    srandom((unsigned)time(NULL));
#if (defined (WIN32))
    s_set_id(worker, (intptr_t)args);
#else
    s_set_id(worker);          //Set a printable identity
#endif

    worker.connect("tcp://localhost:5672");
    std::cout<<"Connected"<<std::endl;
    
    s_sendmore(worker,""),
    s_send(worker,"hi");

    while (1) {  
        s_recv(worker);     //  Envelope delimiter
        std::string workload = s_recv(worker);
        std::cout<<" *New message from Server: "<<workload<<std::endl;
    }

}