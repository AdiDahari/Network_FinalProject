#include "Node.hpp"
#include "select.h"
#include "Message.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
using namespace std;
using namespace Network;
void Node::run()
{
    int in = 0;
    printf("fd: %d is ready. reading...\n", in);

    in = wait_for_input();

    while (1)
    {
        memset(buff, 0, 2048);
        in = wait_for_input();
        if (in == main_socket)
        {
            sockaddr_in new_friend;
            int size = sizeof(new_friend);
            int new_adj = accept(in, (sockaddr *)&new_friend, (socklen_t *)&size);

            read(new_adj, buff, 2048);
            Message m{buff};
            if (m.get_func_id() == 4)
            {
                _adj[m.get_src_id()] = new_adj;
                _socks[new_adj] = new_friend;
                _paths[m.get_src_id()].push_back(m.get_src_id());
                send_ack(m.get_src_id(), m.get_msg_id());
                add_fd_to_monitoring(new_adj);
                Message m{_id, 0, 0, 8, ""};
                discover(m);
            }
        }
        else if (in != 0) //not from keyboard
        {
            try
            {
                sockaddr_in rcvd = _socks.at(in);
                int size = sizeof(rcvd);
                int length_packet = recvfrom(in, buff, sizeof(buff), 0, (sockaddr *)&_socks[in], (socklen_t *)&size);

                if (length_packet == -1)
                    cout << "nack" << endl;

                if (length_packet == 0)
                { //disconnection
                    int key = -1;
                    for (auto p : _adj)
                    {
                        if (p.second == in)
                        {
                            key = p.first;
                        }
                    }
                    remove_fd(in);
                    _socks.erase(key);
                    _adj.erase(key);
                    _paths.erase(key);
                    Message m{_id, 0, 0, 8, ""};
                    discover(m);
                }

                Message m{buff};
                _src[m.get_msg_id()] = m.get_src_id();
                if (m.get_func_id() == 32) //send
                {
                    if (m.get_dest_id() == _id)
                    {
                        cout << m.payload;
                        int tmpSize = sizeof(_socks[m.get_src_id()]);

                        for (int i = 0; i < m.get_trail(); i++)
                        {
                            bzero(buff, 2048);
                            recvfrom(in, buff, 512, 0, (sockaddr *)&_socks[m.get_src_id()], (socklen_t *)&tmpSize);
                            Message tmp{buff};
                            cout << tmp.payload;
                        }
                        send_ack(m.get_src_id(), m.get_msg_id());
                    }
                    else
                    {
                        char ackBuff[512];
                        m.src_id = htonl(_id);
                        sendto(_adj[_paths[m.get_dest_id()][0]], &m, sizeof(m), 0, (sockaddr *)&_socks[_paths[m.get_dest_id()][0]], sizeof(_paths[m.get_dest_id()][0]));
                        int size = sizeof(_socks[_paths[m.get_dest_id()][0]]);
                        if (recvfrom(_adj[_paths[m.get_dest_id()][0]], ackBuff, sizeof(ackBuff), 0, (sockaddr *)&_socks[_paths[m.get_dest_id()][0]], (socklen_t *)&size) == -1)
                        {
                            cout << "failed to transfer msg" << endl;
                        }
                        Message validation{ackBuff};
                        uint32_t valid;
                        memcpy(&valid, validation.payload, 4);
                        if (validation.get_func_id() == 1 && valid == m.msg_id)
                        {
                            send_ack(_src[m.get_msg_id()], m.get_msg_id());
                        }
                        else
                            send_nack(_src[m.get_msg_id()], m.get_msg_id());
                    }
                }
                else if (m.get_func_id() == 8) //discover
                {
                    uint32_t tmp;
                    memcpy(&tmp, m.payload, 4);
                    int msgid = ntohl(tmp);
                    if (_src.find(ntohl(msgid)) != _src.end())
                    {
                        send_nack(_src[msgid], msgid);
                        continue;
                    }
                    discover(m);
                }
                else if (m.get_func_id() == 16) //route
                {
                    handle_route(m);
                }
                else if (m.get_func_id() == 64)
                {
                    forward_relay(m);
                }

                else if (m.get_func_id() == 1) //ack
                    cout << "ack" << endl;
                else if (m.get_func_id() == 2) //nack
                {
                    if (_cnt.find(ntohl(stoi(m.payload))) != _cnt.end())
                    {
                        _cnt[ntohl(stoi(m.payload))]--;
                        validate_nack(ntohl(stoi(m.payload)));
                        continue;
                    }
                    cout << "nack" << endl;
                }
            }
            catch (exception &e)
            {
                continue;
            }
        }
        else //command from the keyboard
        {
            read(in, buff, 2048);
            string temp = buff;
            string func;
            string args;
            int tok = temp.find(',');
            if (tok > temp.size())
            {
                temp.pop_back();
                func = temp;
            }
            else
            {
                func = temp.substr(0, tok);
                args = temp.substr(tok + 1);
            }
            if (func == "setid")
            {
                setid(stoi(args));
            }

            else if (_id == -1 && func != "setid")
            {
                printf("ID has not been set yet\n");
                continue;
            }

            else if (func == "connect")
            {
                connect(args);
            }
            else if (func == "send")
            {
                int id;
                int len;
                int sep = args.find(',');
                id = stoi(args.substr(0, sep));
                args = args.substr(sep + 1);
                sep = args.find(',');
                len = stoi(args.substr(0, sep));
                args = args.substr(sep + 1);
                send(id, len, args.c_str());
            }
            else if (func == "route")
            {
                route(stoi(args));
            }
            else if (func == "peers")
            {
                peers();
            }
        }
        in = 0;
    }
}
int Node::setid(int id)
{
    if (_id != -1)
    {
        cout << "nack" << endl;
        return -1;
    }
    _id = id;
    cout << "ack" << endl;
    return 1;
}

int Node::connect(string addr)
{
    size_t token = addr.find(':');
    string ip = addr.substr(0, token);
    string port = addr.substr(token + 1);
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        printf("socket\n");
    struct sockaddr_in socket;
    memset(&socket, '\0', sizeof(socket));
    socket.sin_family = AF_INET;
    socket.sin_port = htons(stoi(port));
    int rVal = inet_pton(AF_INET, ip.c_str(), &socket.sin_addr);
    if (rVal <= 0)
    {
        printf("inet_pton\n");
        return -1;
    }
    if (::connect(sfd, (struct sockaddr *)&socket, sizeof(socket)) == -1)
    {
        printf("connect\n");
        return -1;
    }
    Message m(_id, 0, 0, 4, "");
    if (::sendto(sfd, &m, sizeof(m), 0, (const sockaddr *)&socket, sizeof(socket)) < 0)
    {
        printf("sendto - connect\n");
        return -1;
    }
    int sent_msgid = m.get_msg_id();
    char reply[512] = {'\0'};
    struct sockaddr_in fromAddr;
    int fromAddrLen = sizeof(fromAddr);
    memset((char *)&fromAddr, 0, sizeof(fromAddr));
    if (recvfrom(sfd, reply, sizeof(reply) - 1, 0, (struct sockaddr *)&fromAddr, (socklen_t *)&fromAddrLen) == -1)
    {
        printf("recvfrom\n");
        return -1;
    }

    Message msg_from = (Message)reply;
    uint32_t pld;
    memcpy(&pld, msg_from.payload, 4);
    if (ntohl(pld) == sent_msgid)
    {
        _paths[msg_from.get_src_id()].push_back(msg_from.get_src_id());
        cout << "ack" << endl;
        cout << msg_from.get_src_id() << endl;
        _adj[msg_from.get_src_id()] = sfd;
        _socks[sfd] = socket;
        add_fd_to_monitoring(sfd);
        return 1;
    }

    else
        cout << "nack" << endl;
}

int Node::send(int dest, int len, const char *s)
{
    if (strlen(s) != len + 1)
    {
        cout << "nack" << endl;
        return -1;
    }

    saved_msgs[dest] = s;
    for (auto &p : _adj)
    {

        if (p.first == dest)
        {
            vector<Message> vec = segment_msg(dest, s);

            try
            {
                for (Message msg : vec)
                {
                    int sfd = _adj.at(dest);
                    if (sendto(sfd, &msg, 512, 0, (sockaddr *)&_socks[dest], sizeof(_socks[dest])) == -1)
                    {
                        printf("nack\n");
                        return -1;
                    }
                    usleep(1000);
                }
            }
            catch (exception &e)
            {
                printf("nack\n");
            }
            return 1;
        }
    }
    if (_paths[dest].size() == 0)
    {
        Message disc{_id, dest, 0, 8, ""};

        memcpy(disc.payload, &disc.dest_id, 4);
        discover(disc);
    }
    else
    {
        send_relay(dest, s);
        return 0;
    }
}

void Node::discover(Message m) //format: {msg_id: original discover msg, src_id: the source of current msg,..., payload: final dest}
{

    uint32_t final_dest;
    memcpy(&final_dest, m.payload, 4);
    int init_msg = m.get_msg_id(); //initial discover msg id
    int trgt = ntohl(final_dest);
    if (m.get_src_id() != _id)
    {
        _src[trgt] = ntohl(m.src_id);
    }
    _target[init_msg] = trgt;

    for (auto &p : _adj)
    {
        if (_paths[trgt].size() > 0) //already found path from this node
        {
            uint32_t path[_paths[trgt].size() + 3]; //three places before the route- msgid,length,selfid
            path[0] = m.msg_id;
            path[1] = htonl(_paths[trgt].size() + 1);
            path[2] = htonl(_id);
            for (int i = 3; i < _paths[trgt].size() + 3; i++)
            {
                path[i] = htonl(_paths[trgt][i - 3]);
            }

            Message routing{_id, _src[trgt], 0, 16, ""};
            memcpy(routing.payload, path, sizeof(path));
            sendto(_adj[m.get_src_id()], &routing, sizeof(routing), 0, (sockaddr *)&_socks[m.get_src_id()], sizeof(_socks[m.get_src_id()]));
            return;
        }
    }
    /*its a fake discover, for updating that there is a change in the graph*/
    if (m.get_dest_id() == 0)
    {
        for (auto it = _paths.cbegin(); it != _paths.cend() /* !!! */;)
        {
            if ((*it).second.size() > 1)
            {
                it = _paths.erase(it++); // Returns the new iterator to continue from.
            }
            else
            {
                ++it;
            }
        }
    }

    for (auto &p : _adj)
    {
        if (p.first == _src[trgt])
            continue;
        _cnt[init_msg] = _cnt[init_msg] + 1;
        m.src_id = htonl(_id);
        sendto(p.second, &m, sizeof(m), 0, (sockaddr *)&_socks[p.first], sizeof(_socks[p.first]));
    }
}
void Node::handle_route(Message m) //format: {msg_id: random, src_id: ..., payload: origin discover + length of path + path}
{
    uint32_t init_msg_bin;
    uint32_t len_bin;
    uint32_t temp;
    memcpy(&init_msg_bin, m.payload, 4);
    int init_msg = ntohl(init_msg_bin);
    memcpy(&len_bin, m.payload + 4, 4);
    int length = ntohl(len_bin);
    _cnt[init_msg]--; //one neighbor gave us an answer
    int step = 4;
    vector<int> route;
    char *templd = m.payload + 2 * step;
    for (int i = 0; i < length; i++)
    {
        memcpy(&temp, templd, 4);
        int num = ntohl(temp);
        route.push_back(num);
        templd += step;
    }
    /*comparing the path*/
    if (_paths[_target[init_msg]].size() == 0 || _paths[_target[init_msg]].size() > route.size())
    {
        _paths[_target[init_msg]] = route;
    }
    else if (_paths[_target[init_msg]].size() == route.size() && _paths[_target[init_msg]] > route)
    {
        _paths[_target[init_msg]] = route;
    }
    if (_cnt[init_msg] == 0) //no more neighbors to check
    {

        /*send the result to the node who asked us for discover*/
        if (_src[_target[init_msg]] != 0)
        {
            Message ans{_id, _src[init_msg], 0, 16, ""};

            char *runner = ans.payload;
            memcpy(runner, &init_msg_bin, 4);
            uint32_t updated_len_bin = htonl(length + 1);
            runner += step;
            memcpy(runner, &updated_len_bin, 4);
            runner += step;
            memcpy(runner, &ans.src_id, 4);
            runner += step;
            for (int i : _paths[_target[init_msg]])
            {
                uint32_t t = htonl(i);
                memcpy(runner, &t, 4);
                runner += step;
            }

            sendto(_adj[_src[_target[init_msg]]], &ans, 512, 0, (sockaddr *)&_socks[_src[_target[init_msg]]], sizeof(_socks[_src[_target[init_msg]]]));
            return;
        }
        if (_paths[_target[init_msg]].size() == 0)
        {
            cout << "nack" << endl;
            return;
        }
        send(_paths[_target[init_msg]].back(), saved_msgs[_target[init_msg]].size() - 1, saved_msgs[_target[init_msg]].c_str());
    }
}
int Node::route(int id)
{
    if (_paths[id].size() > 0) // theres a path
    {
        cout << _id;
        for (int node : _paths[id])
        {
            cout << "->" << node;
        }
        cout << endl;
        return 1;
    }
    cout << "No Path!" << endl;
}

void Node::peers()
{
    string s;
    if (_adj.size() < 1)
        return;
    for (auto &p : _adj)
    {
        s = to_string(p.first) + ',' + s;
    }
    s.pop_back();
    cout << s << endl;
}

void Node::send_relay(int dest, string s)
{
    vector<Message> msgs = segment_msg(dest, s);
    Message relay{_id, _paths[dest][1], msgs.size(), 64, ""};
    uint32_t tmp = ntohl(dest);
    memcpy(relay.payload, &tmp, 4);
    sendto(_adj[_paths[dest][0]], &relay, sizeof(relay), 0, (sockaddr *)&_socks[_paths[dest][0]], sizeof(_paths[dest][0]));
    char tmpBuff[512];
    int sock_size = sizeof(_socks[_paths[dest][0]]);
    recvfrom(_adj[_paths[dest][0]], tmpBuff, sizeof(tmpBuff), 0, (sockaddr *)&_socks[_paths[dest][0]], (socklen_t *)&sock_size);
    Message reply{tmpBuff};
    if (reply.get_func_id() == 2)
    {
        cout << "nack" << endl;
    }
    else
    {
        for (Message m : msgs)
        {
            sendto(_adj[_paths[dest][0]], &m, sizeof(m), 0, (sockaddr *)&_socks[_paths[dest][0]], sizeof(_paths[dest][0]));
        }
        bzero(tmpBuff, 512);
        recvfrom(_adj[_paths[dest][0]], tmpBuff, 512, 0, (sockaddr *)&_socks[_paths[dest][0]], (socklen_t *)&sock_size);
        Message answer{tmpBuff};
        if (answer.get_func_id() == 1)
        {
            cout << "ack" << endl;
        }
        if (answer.get_func_id() == 2)
        {
            cout << "nack" << endl;
        }
    }
}

void Node::forward_relay(Message m)
{
    int src = m.get_src_id();
    m.src_id = htonl(_id);
    char tempBuff[512];

    uint32_t tmp;
    memcpy(&tmp, m.payload, 4);
    int dest = ntohl(tmp);
    if (_paths[dest].size() == 1) //lateset node in the path
    {
        send_ack(src, m.get_msg_id());
    }
    else
    {
        m.dest_id = htonl(_paths[dest][1]);
        bzero(m.payload, 492);
        memcpy(m.payload, &tmp, 4);
        m.dest_id = dest;
        sendto(_adj[_paths[dest][0]], &m, sizeof(m), 0, (sockaddr *)&_socks[_paths[dest][0]], sizeof(_socks[_paths[dest][0]]));
        int len = sizeof(_socks[_paths[dest][0]]);
        if (recvfrom(_adj[_paths[dest][0]], tempBuff, 512, 0, (sockaddr *)&_socks[_paths[dest][0]], (socklen_t *)&len) == -1)
        {
            send_nack(src, m.get_msg_id());
            return;
        }
        Message ans(tempBuff);
        ans.src_id = htonl(_id);
        ans.dest_id = htonl(src);
        send_ack(src, m.get_msg_id());
    }
    /*get the message segmented*/
    for (int i = 0; i < m.get_trail(); i++)
    {
        bzero(tempBuff, 512);
        int len = sizeof(_socks[src]);
        if (recvfrom(_adj[src], tempBuff, 512, 0, (sockaddr *)&_socks[src], (socklen_t *)&len) == -1)
        {
            send_nack(src, m.get_msg_id());
            return;
        }
        Message segment{tempBuff};
        saved_msgs[src].append(string{segment.payload});
    }
    vector<Message> v = segment_msg(_paths[dest][0], saved_msgs[src]);
    /*send segments*/
    for (Message msg : v)
    {
        int chk = sendto(_adj[_paths[dest][0]], &msg, sizeof(msg), 0, (sockaddr *)&_socks[_paths[dest][0]], sizeof(_socks[_paths[dest][0]]));
        usleep(1000);
    }
    int msgToAck = v.back().get_msg_id();

    int len = sizeof(_socks[_paths[dest][0]]);
    /*ack from the destination*/
    if (recvfrom(_adj[_paths[dest][0]], tempBuff, 512, 0, (sockaddr *)&_socks[_paths[dest][0]], (socklen_t *)&len) == -1)
    {
        send_nack(src, msgToAck);
        return;
    }
    send_ack(src, msgToAck);
    saved_msgs.erase(src);
}

void Node::send_ack(int dest, int msgid)
{
    uint32_t msg = htonl(msgid);
    Message ack{_id, dest, 0, 1, ""};
    memcpy(ack.payload, &msg, sizeof(msg));
    sendto(_adj[dest], &ack, 512, 0, (sockaddr *)&_socks[dest], sizeof(_socks[dest]));
}
void Node::send_nack(int dest, int msgid)
{
    uint32_t msg = htonl(msgid);
    Message nack{_id, dest, 0, 2, ""};
    memcpy(nack.payload, &msg, sizeof(msg));
    sendto(_adj[dest], &nack, 512, 0, (sockaddr *)&_socks[dest], sizeof(_socks[dest]));
}
void Node::validate_nack(int msgid)
{
    if (_cnt[msgid] == 0)
    {
        if (_paths[_target[msgid]].size() == 0) //no path
        {
            if (msgid == _id)
            {
                cout << "nack" << endl;
                return;
            }
            send_nack(_src[msgid], msgid);
            return;
        }
    }
}
uint32_t incrementUint32(uint32_t i)
{
    int tmp = ntohl(i);
    ++tmp;
    return htonl(tmp);
}
vector<Message> Node::segment_msg(int dest, string s)
{
    vector<Message> ans;
    int numOfSeg = s.size() / 492;
    if (s.size() % 492 != 0)
    {
        numOfSeg++;
    }
    int i = 0;
    for (; i < numOfSeg - 1; i++)
    {
        string t = s.substr(i * 492, 492).c_str();
        Message m{_id, dest, (numOfSeg - i - 1), 32, t.c_str()};
        ans.push_back(m);
    }
    string t = s.substr(i * 492).c_str();
    Message m{_id, dest, 0, 32, t.c_str()};

    ans.push_back(m);
    return ans;
}