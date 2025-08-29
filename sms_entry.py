import socket  
import time
import threading
import sys
import json
import importlib
import os

from flask import Flask
app = Flask(__name__)
host = "127.0.0.1"
port = 5003

@app.route('/sms', methods=['GET'])
def sms():
    return 'sms'

def start():
    args = sys.argv
    dic = json.loads(args[1])
    ping_msg = dic.get('ping_msg', "123")
    sock_port = dic.get('sock_port', 53500)
    thread1 = threading.Thread(target=ping, args=(ping_msg,sock_port))
    thread2 = threading.Thread(target=start_main)
    # 启动线程
    thread1.start()
    #thread2.start()
    start_main()

def start_main():
    app.run(host, port, debug=True)


def ping(ping_msg : str, sock_port):
    sender = UdpSender(sock_port)
    while True:
        sender.udpSend(ping_msg)
        # print("send msg")
        time.sleep(3)

class UdpSender:
    def __init__(self, port: int):
        self.__udpSender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udpSender.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        self.__remoteAddr = ('127.0.0.1', port)

    def udpSend(self, msg : str):
        self.__udpSender.sendto(msg.encode(), self.__remoteAddr)
        # self.__udpSender.close()
    
if __name__ == "__main__":
    start()

