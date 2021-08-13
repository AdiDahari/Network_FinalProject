import sys
import time
from socket import *

serverSocket = socket(AF_INET, SOCK_DGRAM)

serverSocket.bind(('127.0.0.1', 12000))
heartBeat = 0
while True:
    heartBeat += 1

    msg, addr = serverSocket.recvfrom(1024)
    msg = msg.upper().decode()
    message = str(msg).split(',')
    content = message[0]
    seq = message[1]
    timeStamp = float(message[2])
    rtt = time.time() - timeStamp
    if rtt > 5:
        sys.exit('Connection has lost with client')
    elif rtt > 1.5:
        res = f'Packet #{heartBeat} lost'
        print(res)
        serverSocket.sendto(res.encode(), addr)
    else:
        rcvd = f'Packet #{seq} recieved'
        print(rcvd)
        serverSocket.sendto(rcvd.encode(), addr)
    if heartBeat == 10:
        heartBeat = 0
