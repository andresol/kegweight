Kegweight
=========

Arduino + HX711 + Weight sensor + Kegerface for monitoring how much beer there is in the keg. 

![ScreenShot](screenshot1.jpg "Arduino")

It is now possible to use it with [Kegerface](https://github.com/andresol/kegerface) and the Weight_kegerface.py script.  
![ScreenShot2](screenshot2.jpg "Kegerface")

Library used is the [HX711](https://github.com/bogde/HX711) library.

It is important to find and define the #define DEFAULT_EMPTY_VALUE 8237827. It represent the value of the weight when it is empty.   

I find the value printing the value of scales[0].read_average(10) after  
scales[0].tare();  
delay(DELAY);  
in setup when the weight is empty. 

This step only needs to be done only one time.

GUIDE: (Needs Arduino, Arduino LCD screen, HX711, Raspberry PI/linux server and a weight).  
1. Install the Weight.ino on a arduino.  
2. Calibrate the arduino so that the #define DEFAULT_EMPTY_VALUE is correct.  
3. Install the weight init.d service. Needs to replace DAEMON=$DIR/Weight.py to Weight_Kegerface.py. (update-rc.d weight defaults)  
4. Install the Weight_Kegerface.py into a folder. E.g /opt/weight/  
5. Install the [Kegerface](https://github.com/andresol/kegerface) into e.g /usr/share/nginx/html  
6. Modify the /usr/share/nginx/html/beers.csv  
7. sudo service weight start. Cross your fingers! It will fail. Fix the bugs and ...  
8. Go to localhost:8080  
