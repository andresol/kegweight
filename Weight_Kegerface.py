#!/usr/bin/env python

'''

Read a kegerface csv file and modify it.

It support data in the format V[number]:weight_in:grams! where number > 0

E.g V1:25212

beers.csv file: (https://github.com/andresol/kegerface)

SRM, NAME, TYPE, ABV, HOPPY, AMOUNT, CAPACITY, (T)ap (Bottle)
50,Rendezvous With Anis,Winter Warmer,5.5,3,16.506,19,T

@author: andresol

'''
import serial
import time
import csv
import sys

CONST_DEV_NAME = '/dev/ttyACM1' #Arduino Serial
CONST_PATH_KEGERFACE_CSV = '/usr/share/nginx/html/kegerface-master/beers.csv' 
CONST_STRIP_STR = '\r\n'
CONST_PREFIX_ARDUINO_WEIGHT = 'V'

CONST_BSD_RATE = 9600
CONST_KEG_WEIGHT = 4
CONST_KILO = 1000
DEBUG = 1

ser = serial.Serial(CONST_DEV_NAME, CONST_BSD_RATE)

def writeCsvToFile(new_rows):
    with open(CONST_PATH_KEGERFACE_CSV, 'wb+') as f1:
        writer = csv.writer(f1, delimiter=',', quotechar="", quoting=csv.QUOTE_NONE)
        writer.writerows(new_rows)

def readCsvAndModifyWeight(rowValue, weight, new_rows):
    with open(CONST_PATH_KEGERFACE_CSV, 'rb') as f:
        reader = csv.reader(f)
        rownum = 0
        for row in reader:
            new_row = row
            if rownum == rowValue:
                new_row[5] = weight
            if DEBUG:
                print new_row
            new_rows.append(new_row)
            rownum = rownum + 1

def addValueToKegerFaceFile(rowValue, weight, new_rows):
    readCsvAndModifyWeight(rowValue, weight, new_rows)
    writeCsvToFile(new_rows)

#Main code
while 1:
    try:
        time.sleep(10)
        line = (ser.readline()).rstrip(CONST_STRIP_STR)
        if DEBUG:
            print line
        rowValue = -1 
        weight = -1;
        new_rows = []
        if line[0] == CONST_PREFIX_ARDUINO_WEIGHT:
            rowValue = int(line[1])
            weight = (float(line[3:]) / CONST_KILO) - CONST_KEG_WEIGHT
            if DEBUG:
                print rowValue
                print weight
        if rowValue > 0 and weight > 0 :
            addValueToKegerFaceFile(rowValue, weight, new_rows)

    except IOError:
        print "Error", sys.exc_info()[0]
    except ValueError:
        print "ValueError", sys.exc_info()[0]
    except Exception,e :
        if DEBUG:
            raise e
        else:
            pass #Run for ever.Don't care about errors.
if __name__ == '__main__':
    pass
