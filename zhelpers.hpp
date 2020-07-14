#ifndef __ZHELPERS_HPP_INCLUDED__
#define __ZHELPERS_HPP_INCLUDED__

//  Include a bunch of headers that we will need in the examples

#include <zmq.hpp> // https://github.com/zeromq/cppzmq

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include <time.h>
#include <assert.h>
#include <stdlib.h>        // random()  RAND_MAX
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

//  Provide random number from 0..(num-1)
#define within(num) (int) ((float)((num) * random ()) / (RAND_MAX + 1.0))

//  Receive 0MQ string from socket and convert into string
inline static std::string
s_recv(zmq::socket_t& socket, int flags = 0) {

    zmq::message_t message;
    socket.recv(&message, flags);

    return std::string(static_cast<char*>(message.data()), message.size());
}

inline static bool s_recv(zmq::socket_t& socket, std::string& ostring, int flags = 0)
{
    zmq::message_t message;
    bool rc = socket.recv(&message, flags);

    if (rc) {
        ostring = std::string(static_cast<char*>(message.data()), message.size());
    }

    return (rc);
}

//  Convert string to 0MQ string and send to socket
inline static bool
s_send(zmq::socket_t& socket, const std::string& string, int flags = 0) {

    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool rc = socket.send(message, flags);
    return (rc);
}

//  Sends string as 0MQ string, as multipart non-terminal
inline static bool
s_sendmore(zmq::socket_t& socket, const std::string& string) {

    zmq::message_t message(string.size());
    memcpy(message.data(), string.data(), string.size());

    bool rc = socket.send(message, ZMQ_SNDMORE);
    return (rc);
}

inline std::string
s_set_id(zmq::socket_t& socket)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase
        << std::setw(4) << std::setfill('0') << within(0x10000) << "-"
        << std::setw(4) << std::setfill('0') << within(0x10000);
    socket.setsockopt(ZMQ_IDENTITY, ss.str().c_str(), ss.str().length());
    return ss.str();
}
//  Report 0MQ version number
//
inline static void
s_version(void)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    std::cout << "Current 0MQ version is " << major << "." << minor << "." << patch << std::endl;
}

inline static void
s_version_assert(int want_major, int want_minor)
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    if (major < want_major
        || (major == want_major && minor < want_minor)) {
        std::cout << "Current 0MQ version is " << major << "." << minor << std::endl;
        std::cout << "Application needs at least " << want_major << "." << want_minor
            << " - cannot continue" << std::endl;
        exit(EXIT_FAILURE);
    }
}

//  Return current system clock as milliseconds
inline static int64_t
s_clock(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

//  Sleep for a number of milliseconds
inline static void
s_sleep(int msecs)
{
#if (defined (WIN32))
    Sleep(msecs);
#else
    struct timespec t;
    t.tv_sec = msecs / 1000;
    t.tv_nsec = (msecs % 1000) * 1000000;
    nanosleep(&t, NULL);
#endif
}

inline static void
s_console(const char* format, ...)
{
    time_t curtime = time(NULL);
    struct tm* loctime = localtime(&curtime);
    char* formatted = new char[20];
    strftime(formatted, 20, "%y-%m-%d %H:%M:%S ", loctime);
    printf("%s", formatted);
    delete[] formatted;

    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    va_end(argptr);
    printf("\n");
}

//  ---------------------------------------------------------------------
//  Signal handling
//
//  Call s_catch_signals() in your application at startup, and then exit
//  your main loop if s_interrupted is ever 1. Works especially well with
//  zmq_poll.

static int s_interrupted = 0;
inline static void s_signal_handler(int signal_value)
{
    s_interrupted = 1;
}

inline static void s_catch_signals()
{
#if (!defined(WIN32))
    struct sigaction action;
    action.sa_handler = s_signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
#endif
}



#endif
