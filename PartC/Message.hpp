#pragma once
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
using namespace std;
namespace Network
{
    static int cnt = 0;

    struct Message
    {
        uint32_t msg_id;
        uint32_t src_id;
        uint32_t dest_id;
        uint32_t trail;
        uint32_t func_id;
        char payload[492] = {'\0'};
        Message(int src, int dest, int tra, int fun_id, const char *msg) : src_id(htonl(src)), dest_id(htonl(dest)), trail(htonl(tra)), func_id(htonl(fun_id))
        {
            if (cnt % 10 == 0)
            {
                cnt++;
            }
            string tmp = to_string(src) + '0' + to_string(cnt++);
            msg_id = htonl(stoi(tmp));
            strcpy(payload, msg);
        }
        Message(const Message &m)
        {
            msg_id = m.msg_id;
            src_id = m.src_id;
            dest_id = m.dest_id;
            trail = m.trail;
            func_id = m.func_id;
            memcpy(payload, m.payload, 492);
        }
        Message(char *m)
        {
            memcpy(&msg_id, m, 4);
            memcpy(&src_id, m + 4, 4);
            memcpy(&dest_id, m + 8, 4);
            memcpy(&trail, m + 12, 4);
            memcpy(&func_id, m + 16, 4);
            memcpy(payload, m + 20, 492);
        }
        const char *stage_msg()
        {
            char msg[512];
            sprintf(msg, (const char *)this);
            // memcpy(msg, (const void *)msg_id, sizeof(msg_id));
            // memcpy(msg + sizeof(msg_id), (const void *)src_id, sizeof(src_id));
            // memcpy(msg + sizeof(msg_id) + sizeof(src_id), (const void *)dest_id, sizeof(dest_id));
            // memcpy(msg + sizeof(msg_id) + sizeof(src_id) + sizeof(dest_id), (const void *)trail, sizeof(trail));
            // memcpy(msg + sizeof(msg_id) + sizeof(src_id) + sizeof(dest_id) + sizeof(trail), (const void *)func_id, sizeof(func_id));
            // strcpy((msg + 20), payload);
            return msg;
        }
        friend ostream &operator<<(ostream &os, Message &m)
        {
            os << "MsgID: " << ntohl(m.msg_id) << ", SrcID: " << ntohl(m.src_id) << ", DestID: " << ntohl(m.dest_id) << ", Trail#: " << ntohl(m.trail) << ", FuncID: " << ntohl(m.func_id) << ", Payload: " << m.payload << endl;
            return os;
        }
        int get_msg_id()
        {
            return ntohl(msg_id);
        }
        int get_src_id()
        {
            return ntohl(src_id);
        }
        int get_dest_id()
        {
            return ntohl(dest_id);
        }
        int get_trail()
        {
            return ntohl(trail);
        }
        int get_func_id()
        {
            return ntohl(func_id);
        }
    };

}