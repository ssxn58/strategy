import socket  
import time
import threading
import sys
import json
import importlib
import os
import time

import logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(f'{__file__}.log', encoding='utf-8'),
    ]
)
logger = logging.getLogger(__name__)


def start():
    args = sys.argv
    json_str = args[1]
    logger.info(f"接收到的原始 JSON 字符串: {json_str}")
    dic = json.loads(json_str)
    ping_msg = dic.get('ping_msg', "123")
    sock_port = dic.get('sock_port', 53500)
    thread1 = threading.Thread(target=ping, args=(ping_msg,sock_port))
    thread2 = threading.Thread(target=start_main)
    # 启动线程
    thread1.start()
    #thread2.start()
    start_main()

def start_main():
    # app.run(host, port, debug=True)
    while(True):
        time.sleep(1)

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
    logger.info(f"{__file__} start")
    start()

