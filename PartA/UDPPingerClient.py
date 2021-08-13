from socket import *
import time
import sys

message = b"Ping"
min = sys.float_info.max
max = 0.0
lost = 0
sum = 0.0
for i in range(10):
    clientSocket = socket(AF_INET, SOCK_DGRAM)
    clientSocket.settimeout(1.0)
    address = ("127.0.0.1", 12000)
    start = time.time()
    clientSocket.sendto(message, address)
    try:
        data, server = clientSocket.recvfrom(1024)
        size = sys.getsizeof(data)
        end = time.time()
        rtt = end - start
        if rtt > max:
            max = rtt
        if rtt < min:
            min = rtt
        sum += rtt
        seq = i + 1
        print(
            f"Reply from {address[0]}: bytes={size} time={rtt:10f}s TTL={1.0}s")
    except timeout:
        lost = lost + 1
        print("Request timed out")

avg = sum / 10.0
lossprec = lost * 10
print(f'Minimum RTT: {min:10f}s')
print(f'Maximoum RTT: {max:10f}s')
print(f'Average RTT: {avg:10f}s')
print(f'Packet Loss: {lossprec}%')
