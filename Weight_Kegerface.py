#!/usr/bin/env python

'''

Please change the kegerfaceBeercsv and/or arduino serial path.

It just reads a beers.csv file and replace the beer with amount from V:2014.2 kg. 
@author: andresol
'''
import serial
import time
import csv
import sys

ser = serial.Serial('/dev/ttyACM1', 9600)
kegerfaceBeercsv = '/usr/share/nginx/html/beers.csv'
kegWeight = 4
while 1:
    try:
        time.sleep(10)
        line = (ser.readline()).rstrip('\r\n')
        rowValue = -1;
        weight = -1;
        new_rows = []
        if line[0] == 'V':
            rowValue = int(line[1])
            weight = (float(line[3:]) / 1000) - kegWeight
        if rowValue > 0 and weight > 0 :
            print rowValue
            print weight
            with open(kegerfaceBeercsv, 'rb') as f:
                reader = csv.reader(f) #
                rownum = 0
                for row in reader:
                    print row
                    new_row = row      # at first, just copy the row
                    if rownum == rowValue:
                        print new_row
                        print weight
                        new_row[5] = weight
                    new_rows.append(new_row) # add the modified rows
                    rownum = rownum + 1
            f1=open(kegerfaceBeercsv, 'wb+')
            if f1 is not None:
                writer = csv.writer(f1,delimiter=',',quotechar="",quoting=csv.QUOTE_NONE)
                writer.writerows(new_rows)
                f1.close()
            else:
                print "Missing file"

    except IOError:
        print "Error", sys.exc_info()[0] 
    except ValueError:
        print "ValueError", sys.exc_info()[0]
    except:
        pass
if __name__ == '__main__':
    pass
