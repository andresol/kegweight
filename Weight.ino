
/*
 *
 * Hx711.DOUT - pin #A1
 * Hx711.SCK  - pin #A2
 *
 * LCD RS pin to digital pin 13
 * LCD Enable pin to digital pin 12
 * LCD D4 pin to digital pin 8
 * LCD D5 pin to digital pin 9
 * LCD D6 pin to digital pin 10
 * LCD D7 pin to digital pin 11
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 */

// include the library code:
#include <HX711.h>
#include <LiquidCrystal.h>
#include <stdlib.h>
#include <string.h>
#include <EEPROM.h>

#define DEBUG 0
#define TEST 1

#define WEIGHT_SENSORS 1
#define DELAY 100
#define DEFAULT_WEIGHT_TIME 300
#define DEFAULT_WEIGHT_PRINT_TIME 5000
#define DEFAULT_EMPTY_VALUE 8237827
#define BAUD_RATE 9600

// Not in use. May be used in the future.
int EEPROM_ADDR = 0;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(13, 12, 8, 9, 10, 11);
HX711 scales[WEIGHT_SENSORS] = {HX711 (A1, A2)};

unsigned long last = 0;
unsigned long lastPrint = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  lcd.begin(16, 2);
  lcd.print("Vekt:");

  scales[0].set_scale(2280.f);
  scales[0].tare();
  delay(DELAY);		      
  scales[0].set_offset(DEFAULT_EMPTY_VALUE);
}


void doWeigth(int value) {
  unsigned long now = millis();

  if ((last + DEFAULT_WEIGHT_TIME) < now) {
    scales[0].power_up();
    lcd.setCursor(0, 1);
    float value = scales[0].get_units(10) * -10;
    if (value > -10 && value < 10)  {
      value = 0;
    }
    
    if ((lastPrint + DEFAULT_WEIGHT_PRINT_TIME) < now) {
       Serial.print("V1:");
       Serial.println(value, 1);
       lastPrint = now;
    }
    char weight[8];
    String grams = " g      ";
    dtostrf(value, 1, 1, weight);
    lcd.print(weight +  grams);
    delay(50);
    scales[0].power_down();	// put the ADC in sleep mode
    last = millis();
  }
}

void loop() {
  doWeigth(0);
  delay(2);
}

