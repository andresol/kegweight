
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
#define KILO 1000
#define TAP_SIZE 330
#define KEG_WEIGHT 4000
#define USE_EEPROM 0

// Not in use. May be used in the future.
//int EEPROM_ADDR = 0;

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(13, 12, 8, 9, 10, 11); //PROD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); //DEBUG

HX711 scales[WEIGHT_SENSORS] = {
  HX711 (A1, A2)};

unsigned long last = 0;
unsigned long lastPrint = 0;
const String kilo = "kg";


void setup() {
  Serial.begin(BAUD_RATE);
  lcd.begin(16, 2);
  lcd.print("Starting....");
  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    scales[i].set_scale(2280.f);
    scales[i].tare();
    delay(DELAY);
    if (USE_EEPROM) {
      scales[i].set_offset(DEFAULT_EMPTY_VALUE);//TODO: use eeprom. 
    } 
    else {
      scales[i].set_offset(DEFAULT_EMPTY_VALUE);
    }
  }
  lcd.clear();
}

String getName(const int i) {
  String weightName = String("V");
  weightName = weightName + (i + 1) + ":";
  return weightName;
}

float getValue(HX711 scale) {
  float value = scale.get_units(10) * -10;
  if (value > -10 && value < 10)  {
    value = 0;
  }
  return value;
}

void printToSerial(const float value, const unsigned long now, String& weightName) {
  if ((lastPrint + DEFAULT_WEIGHT_PRINT_TIME) < now) {
    Serial.print(weightName);
    Serial.println(value, 1);
    lastPrint = now;
  }
}

int getRow(const int i) {
  return i % 2;
} 

int getCol(const int i) {
  int col = 0;
  if (i > 1) {
    col = 9;
  }
  return col;
}

void getWeight(const float value, char* buffer) {
  int decimals = 0;
  if (value > KILO * 10 || (KILO * -10) < value) {
    if (WEIGHT_SENSORS > 2) {
      decimals = 1; 
    } else {
      decimals = 3; 
    }
  } else {
    if (WEIGHT_SENSORS > 2) {
      decimals = 0; 
    } else {
      decimals = 2;
    }
  }
  dtostrf((value / KILO), 1, decimals, buffer);
}

void printWeightOnLCD(const float value, String weightName) {
  char* weight = new char[7];
  getWeight(value, weight);

  if (WEIGHT_SENSORS > 2) {
    lcd.print(weight + kilo);
  } else {
    if (WEIGHT_SENSORS == 1) {
      lcd.print(weightName + weight + kilo);
      lcd.setCursor(0, 1);
      String beers = String("Beers:");
      char numberOfBeers[4];
      dtostrf(((value - KEG_WEIGHT) / (TAP_SIZE)), 1, 0, numberOfBeers);
      lcd.print(beers + numberOfBeers + " (0.33)");
    } else {
      char numberOfBeers[4];
      dtostrf(((value - KEG_WEIGHT) / (TAP_SIZE)), 1, 0, numberOfBeers);
      lcd.print(weightName + weight + kilo + " (" + numberOfBeers +")");
    }
  }
  
  delete weight;
}

void doWeigth(int value) {
  unsigned long now = millis();

  if ((last + DEFAULT_WEIGHT_TIME) < now) {
    for (int i = 0; i < WEIGHT_SENSORS; i++) {
      scales[i].power_up();
      lcd.setCursor(0, 1);

      String weightName = getName(i);
      float value = getValue(scales[i]);

      printToSerial(value, now, weightName);

      const int row = getRow(i);
      const int col = getCol(i);
      lcd.setCursor(col, row);
      
      printWeightOnLCD(value, weightName);

      delay(50);
      scales[i].power_down();	// put the ADC in sleep mode
      last = millis();
    }
  }
}

void loop() {
  doWeigth(0);
  delay(2);
}




