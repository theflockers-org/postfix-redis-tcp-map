#!/usr/bin/env python2.6

import sys, time
from string import *
import threading
import socket, select

def send(nada, dom_list):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect( ('127.0.0.1',6666) )
#   print s.fileno()
    for i in range(1, 10):
        for d in dom_list:
            s.send("get %s\n" % d)
            s.recv(100)
        i = i + 1
    s.close()
    #print "Finish!"

num_threads = 1000
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

for d in div:
    t = threading.Thread(target=send, args=(None, div[d]))
    t.start()

