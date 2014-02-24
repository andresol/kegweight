#!/usr/bin/env python

'''
created on May 6, 2013

@author: andresol
'''
import serial
import time
import datetime
import json
import os
import urllib2

ser = serial.Serial('/dev/ttyACM1', 9600)
'''ser = serial.Serial(3, 9600)'''
url = "URL_CHANGE_ME"
while 1:
    try:
	time.sleep(10)
        ts = time.time()
        st = datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
        line = (ser.readline()).rstrip('\r\n')
        data = 'T:' + st + "," + line + '\r\n'
	#print data
	result = urllib2.urlopen(url + line).read()
	#print url + line +'\n'
	#print result + '\n'
    except:
        print "Error", sys.exc_info()[0]
    
           
if __name__ == '__main__':
    pass

