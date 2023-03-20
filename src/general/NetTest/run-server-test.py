import subprocess
import os
import time
import multiprocessing
import threading


def run_server():
    subprocess.run('./../../../NetTest-1')


if __name__ == '__main__':
    # os.system('g++-12 NetTest-1.cpp -o NetTest-1 -std=c++20')
    # t = threading.Thread(target=run_server, args=())
    # t.start()
    prc = multiprocessing.Process(target=run_server, args=())
    prc.start()

    time.sleep(0.1)
    subprocess.run('./../../../NetTest-2')

    prc.terminate()
