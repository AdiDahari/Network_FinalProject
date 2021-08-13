import sys
import time
from socket import *
import random

clientSocket = socket(AF_INET, SOCK_DGRAM)
heartBeat = 0
for i in range(10):
    heartBeat += 1
    rand = random.randint(0, 10)
    seq = i + 1
    currTime = time.time()
    message = f'HearBeat,{seq},{currTime}'

    # clientSocket.settimeout(1.0)
    address = ("127.0.0.1", 12000)
    if rand < 4:
        time.sleep(2)
        clientSocket.sendto(message.encode(), address)
    if rand == 7:
        time.sleep(5.0)
        clientSocket.sendto(message.encode(), address)
        sys.exit('Server closed the connection')
    clientSocket.sendto(message.encode(), address)
    reply, addr = clientSocket.recvfrom(12000)
    reply = reply.upper().decode()
    print(f'Reply from server: {reply}')
clientSocket.close()
