#!/usr/bin/env python2.6

import sys, time
from string import *
import threading
import socket, select

times = 1

def send(nada, dom_list):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect( ('127.0.0.1', 25000) )
    for i in range(0, times):
        for d in dom_list:
            s.send("get %s\r\n" % d)
            s.recv(100)
        i = i + 1
    s.close()

num_threads = 1
domain = []
for a in ascii_lowercase:
    for b in ascii_lowercase:
        for c in ascii_lowercase:
            domain.append("%s%s%s.com.br" % (a,b,c))

size = len(domain) / num_threads

div = {}
i = 0

for d in domain:
    if i not in div:
        div[i] = []

    if i == num_threads -1:
        i = 0

    div[i].append(d)
    i = i + 1


print "Starting test...\n"
init_time = time.time()
i = 0
t = {}
for i in len(div):
    t[i] = threading.Thread(target=send, args=(None, div[i]))
    t[i].start()
    i = i +1

i = 0
while i < len(t):
    t[i].join()
    i = i + 1

delay = time.time() - init_time
rps   = len(domain) / delay

print "Requests:             %s" % len(domain)
print "Run delay:            %4.3f ms" % (delay)
print "Theads:               %i" % num_threads
print "AVG Req by thread:    %i requests per thread" % ( (rps / num_threads) )
print "AVG Requests:         ~ %i requests per second\n" % ( int(rps) * times )
