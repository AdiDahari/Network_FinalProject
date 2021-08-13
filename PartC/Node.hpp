#pragma once
#include "select.h"
#include "Message.hpp"
#include <unordered_map>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <tuple>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <vector>
namespace Network
{
    class Node
    {

    private:
        int _id = -1;
        std::unordered_map<int, int> _adj;    //id : fd
        std::unordered_map<int, int> _cnt;    //msg_id : count
        std::unordered_map<int, int> _src;    // msg_id : source
        std::unordered_map<int, int> _target; // msg_id : initial source id
        std::unordered_map<int, sockaddr_in> _socks;
        std::unordered_map<int, std::vector<int>> _paths;
        char buff[2048];
        std::unordered_map<int, std::string> saved_msgs; //save msgs by target id as key
        sockaddr_in listener;
        int main_socket;
        void send_ack(int dest, int msgid);
        void send_nack(int dest, int msgid);
        void discover(Network::Message m);
        void validate_nack(int msgid);
        void handle_route(Network::Message m);
        void send_complex(int initsrc);
        std::vector<Message> segment_msg(int dest, std::string s);
        void send_relay(int dest, std::string s);
        // void handle_relay(Message m);
        void forward_relay(Message m);

    public:
        void run();
        int setid(int id);
        int connect(std::string addr);
        int connect_to();
        int send(int dest, int len, const char *s);
        int route(int id);
        void peers();
        Node(int port)
        {
            main_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (main_socket == -1)
            {
                printf("Could not create Listening socket, exiting...\n");
                exit(0);
            }

            memset(&listener, 0, sizeof(listener));

            listener.sin_family = AF_INET;
            listener.sin_addr.s_addr = INADDR_ANY;
            listener.sin_port = htons(port);
            if (bind(main_socket, (struct sockaddr *)&listener, sizeof(listener)) == -1)
            {
                printf("Bind failed with error code : %d");
                close(main_socket);
                exit(0);
            }
            if (listen(main_socket, 500) == -1)
            {
                printf("listen() failed with error code : %d");
                exit(0);
            }
            add_fd_to_monitoring(main_socket);
        }
        ~Node()
        {
            _socks.clear();
            close(main_socket);
        }
    };
}