
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
#include <EEPROMex.h>
#include <EEPROMVar.h> //https://github.com/thijse/Arduino-Libraries/tree/master/EEPROMEx

#define DEBUG 0
#define TEST 0

#define WEIGHT_SENSORS 2
#define DELAY 100
#define DEFAULT_WEIGHT_TIME 1000
#define DEFAULT_WEIGHT_TIME_FAST 50
#define DEFAULT_WEIGHT_PRINT_TIME 500
#define DEFAULT_EMPTY_VALUE 8236505
#define BAUD_RATE 9600
#define KILO 1000
#define TAP_SIZE 330
#define KEG_WEIGHT 5000
#define USE_EEPROM 1
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#define BUTTON_PIN   0

typedef struct {
  int eepromAddress;
  HX711 scale;
  unsigned long last;
  unsigned long lastPrint;
  float lastValue;
} 
Scale;

typedef struct MenuItem {
  char* name;
};

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); //PROD

Scale scale1 = {0, HX711 (A1, A2), 0, 0, 0};
Scale scale2 = {1, HX711 (A3, A4), 0, 0, 0};

Scale scales[] = { scale1, scale2};

const String kilo = "kg";

// KEY VARIABLES
int lcd_key = btnNONE;
int lcd_key_prev = btnNONE;
unsigned long lcd_key_prev_pressed = 0;
unsigned long millisPressedKey = 0;
int buttonPressed = 0;
unsigned long debounceTime = 0;

// WEIGHT VARIABLES
float lastValue[WEIGHT_SENSORS];
unsigned int useFastWeight = 0;
unsigned int useKegWeight = 0;

//MENU
char* EMPTY = "                ";
MenuItem beerSizes[] = {{"0.33"}, {"0.5"}};
MenuItem menu[] = {{"Calibrate"} , {"Tare"} , {"Beersize"} , {"Keg Weight"}};
unsigned int lastItem = 3;
unsigned int menuMarker = 0;
unsigned int level = 0;

//GUI VARIABLES
unsigned int printGUI = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  lcd.begin(16, 2);
  lcd.print("Starting...");
  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    scales[i].lastValue = 0;
    scales[i].last = 0;
    scales[i].lastPrint = 0;
    scales[i].scale.set_scale(228.f);
    scales[i].scale.tare();
    delay(DELAY);
    if (USE_EEPROM) { //Firstime use please use calibrate.
      double value = EEPROM.readDouble(scales[i].eepromAddress);
      scales[i].scale.set_offset(value);
    } 
  }
  lcd.clear();
}

// read the buttons
int read_LCD_buttons() {
  int adc_key_in = 0;
  adc_key_in = analogRead(BUTTON_PIN);
  // my buttons when read are centered at these valies: 0, 97, 254, 437, 639
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  // For V1.1 us this threshold
  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 150)  return btnUP;
  if (adc_key_in < 350)  return btnDOWN;
  if (adc_key_in < 550)  return btnLEFT;
  if (adc_key_in < 850)  return btnSELECT;
  return btnNONE;  // when all others fail, return this...
}

int getButtonPressed() {
  lcd_key = read_LCD_buttons();  // read the buttons
  unsigned long now = millis();
  if (lcd_key == btnNONE) {
    return btnNONE;
  } 
  else if (millisPressedKey == 0 && lcd_key != btnNONE) {
    millisPressedKey = now;
    return btnNONE;
  } 
  else if (((millisPressedKey + 10) < now) && (debounceTime < now)) {
    int lcd_key_confirm = read_LCD_buttons();
    debounceTime = now + 300;
    millisPressedKey = 0;
    if (lcd_key == lcd_key_confirm) {
      //Serial.println(lcd_key);
      buttonPressed = 0;
      return lcd_key;
    } 
    else {
      millisPressedKey = now;
      return btnNONE;
    }
  } 
  else {
    return btnNONE;
  }
}

String getName(const int i) {
  String weightName = String("V");
  weightName = weightName + (i + 1) + ":";
  return weightName;
}

float getValue(HX711 scale) {
  float value = scale.get_units(3) ;
  if (value > -10 && value < 10)  {
    value = 0;
  }
  return value;
}

void printToSerial(const float value, const unsigned long now, String& weightName, const int i) {
  if ((scales[i].lastPrint + DEFAULT_WEIGHT_PRINT_TIME) < now || ((useFastWeight && (scales[i].last + DEFAULT_WEIGHT_TIME_FAST) < now))) {
    Serial.print(weightName);
    Serial.println(value, 1);
    scales[i].lastPrint = now;
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
  float weight = value;
  if (useKegWeight) {
    weight -= KEG_WEIGHT;
  }
  if (weight > KILO * 10 || (KILO * -10) < weight) {
    if (WEIGHT_SENSORS > 2) {
      decimals = 1; 
    } 
    else {
      if (weight < 0) {
        decimals = 2; 
      } 
      else {
        decimals = 3; 
      }
    }
  } 
  else {
    if (WEIGHT_SENSORS > 2) {
      decimals = 0; 
    } 
    else {
      decimals = 2;
    }
  }
  dtostrf((weight / KILO), 1, decimals, buffer);

}

void printWeightOnLCD(const float value, String weightName) {
  if (printGUI) {
    return;
  }
  char* weight = new char[7];
  getWeight(value, weight);
  //lcd.clear();
  if (WEIGHT_SENSORS > 2) {
    lcd.print(weight + kilo);
  } 
  else {
    if (WEIGHT_SENSORS == 1) {
      lcd.print(weightName + weight + kilo + "   ");
      lcd.setCursor(0, 1);
      String beers = String("Beers:");
      char numberOfBeers[4];
      dtostrf(((value - KEG_WEIGHT) / (TAP_SIZE)), 1, 0, numberOfBeers);
      lcd.print(beers + numberOfBeers + " (0.33)");
    } 
    else {
      char numberOfBeers[4];
      dtostrf(((value - KEG_WEIGHT) / (TAP_SIZE)), 1, 0, numberOfBeers);
      lcd.print(weightName + weight + kilo + " (" + numberOfBeers +")    ");
    }
  }

  delete weight;
}

void doWeigth() {
  if (printGUI){
    return;
  }
  unsigned long now = millis();

  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    if (((scales[i].last + DEFAULT_WEIGHT_TIME) < now) || ((useFastWeight && (scales[i].last + DEFAULT_WEIGHT_TIME_FAST) < now))) {
      lcd.setCursor(0, 1);

      String weightName = getName(i);
      float value = getValue(scales[i].scale);

      if ((value > (scales[i].lastValue + 50)) || (value < (scales[i].lastValue - 50))) {
        useFastWeight = 1;
        scales[i].lastValue = value;
      } 
      else {
        useFastWeight = 0;
      }

      printToSerial(value, now, weightName, i);

      const int row = getRow(i);
      const int col = getCol(i);
      lcd.setCursor(col, row);

      printWeightOnLCD(value, weightName);

      scales[i].last = millis();
    }
  }
}


void printMenuMarker(int up) {
  //Menu marker
  if (up) {
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.setCursor(0, 1);
    lcd.print(">");
  } 
  else {
    lcd.setCursor(0, 0);
    lcd.print(">");
    lcd.setCursor(0, 1);
    lcd.print(" ");
  }
}

void printMenuItem(char* &item) {
  char textItem[50];
  strcpy(textItem, item);
  strcat(textItem, EMPTY);
  lcd.print(textItem);
}

void printMainMenu(int up) {
    if (!up) {
    lcd.setCursor(1, 0);
    printMenuItem(menu[menuMarker].name);
    if (menuMarker < lastItem) {
      lcd.setCursor(1, 1);
      printMenuItem(menu[menuMarker + 1].name);
    } else {
      lcd.setCursor(1, 1);
      lcd.print(EMPTY);
    }
    } else {
      lcd.setCursor(1, 0);
      printMenuItem(menu[menuMarker - 1].name);
      lcd.setCursor(1, 1);
      printMenuItem(menu[menuMarker].name);
    }
}

void printSubMenu(const int up) {

}

void printMenu() {
  if (!printGUI){
    return;
  }
  int up = menuMarker % 2;
  printMenuMarker(up);
  if (level == 0) {
    printMainMenu(up);
  } else {
    printSubMenu(up);
  }
}

void doTare() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Tare all weights");
  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    scales[i].scale.tare();
    delay(DELAY);
  }
  lcd.print("Done tare...");
  delay(DELAY * 4);
}

void kegWeight() {
  useKegWeight ^= 1 << 0;
  if (useKegWeight == 0) {
    lcd.print("Keg is not removed");
  } 
  else {
    lcd.print("Keg is removed");
  }
  delay(DELAY * 4);
}

void doCalibrate() {  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrate all weights");
  lcd.setCursor(0, 1);
  lcd.print("Please clear all.");
  delay(4000);
  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    scales[i].scale.tare();
    double sum = scales[i].scale.read_average(10);
#if defined(DEBUG)
    Serial.print("Offset is");
    Serial.println(sum);
#endif
    EEPROM.writeDouble(scales[i].eepromAddress, sum);
  }
}

void beerSize() {
  if (level > 0) {
   //TODO:
  }
}

void doSelect(const int value) {
  switch (value) {
  case 0:
    doCalibrate();
    break;
  case 1:
    doTare();
    break;
  case 2:
    beerSize();
    break;
  case 3:
    kegWeight();
    break;
  default:
    break;
  }
  delay(DELAY);
  printGUI = 0;
  lcd.clear();
}

void doRight() {
  if (printGUI) {
    level++;
  } 
  else {
    lcd.clear();
    printGUI = 1;
  }
}

void doSelect() {
  if (printGUI) {
    doSelect(menuMarker);
  }
}

void doLeft() {
  if (printGUI) {
    if (level > 0) {
      level--;
    } else {
      lcd.clear();
      printGUI = 0;
    }
   } 
}

void doUp() {
  if (menuMarker == 0) {
    menuMarker = lastItem;
   } else {
    menuMarker--;
   }
}

void doDown() {
 if (printGUI) {
   if (menuMarker >= lastItem) {
      menuMarker = 0;
    } else {
      menuMarker++;
    }
  }
}

void doButtonAction(int btn) {
  if (btn != btnNONE) {
#if defined(DEBUG)
    Serial.println(btn);
#endif
    switch (btn) {
    case btnRIGHT: {
        doRight(); break;
      } case btnSELECT: {
        doSelect(); break;
      } case btnLEFT: {
        doLeft(); break;
      } case btnUP: {
        doUp(); break;
      } case btnDOWN: { 
        doDown(); break;
      } case btnNONE: {
        break;
      }
    }
#if defined(DEBUG)
    Serial.print("MenuMarker:");
    Serial.println(menuMarker);
    Serial.print("useKegWeight:");
    Serial.println(useKegWeight);
#endif
  }
}

void loop() {
  int btn = getButtonPressed();
  doButtonAction(btn);
  doWeigth();
  printMenu();
  delay(2);
}



