kegweight
=========

Arduino + HX711 + Weight sensor for monitoring how much beer there is in the keg. C

It is now possible to use it with [Kegerface](https://github.com/andresol/kegerface) and the hacky Weight_kegerface.py script.

Library used is the [HX711](https://github.com/bogde/HX711) library.


It is important to find and define det #define DEFAULT_EMPTY_VALUE 8237827. It represent the value of the weight when it is empty.   


I find the value printing the value of scales[0].read_average(10) after 
scales[0].tare();
delay(DELAY);
in setup when the weight is empty.

This step only needs to be done only one time.
