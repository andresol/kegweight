
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
 
/* Uncomment if using ethernet shield */
#define ETHERNET 1

// include the library code:
#include <HX711.h>
#include <LiquidCrystal.h>
#include <stdlib.h>
#include <string.h>
#include <EEPROMex.h>
#include <EEPROMVar.h> //https://github.com/thijse/Arduino-Libraries/tree/master/EEPROMEx

#if defined(ETHERNET)
  #include <SPI.h>
  #include <Ethernet.h>
#endif

//#define DEBUG 0
//#define TEST 0


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

// initialize the library with the numbers of the interface pins
//LiquidCrystal lcd(13, 12, 8, 9, 10, 11); //DEBUG
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); //PROD



#if defined(ETHERNET)
  HX711 scales[] = {HX711 (A3, A4), HX711 (A5, A6)}; //A3 = ANALOG1 = DT // A4=SCK
  byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  IPAddress ip(192,168,1,177);
  EthernetServer server(80);
#else
  HX711 scales[] = { HX711 (A1, A2), HX711 (A3, A4)}; //A3 = ANALOG1 = DT // A4=SCK
#endif

int eepromAddress[WEIGHT_SENSORS] = {0};

unsigned long last[WEIGHT_SENSORS];
unsigned long lastPrint[WEIGHT_SENSORS];
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

//MENU
char* EMPTY = "                ";
char* menu[] = {"Calibrate       " ,"Tare           ", "Beersize       "};
unsigned int lastItem = 1;
unsigned int menuMarker = 0;

//GUI VARIABLES
unsigned int printGUI = 0;

void setup() {
  Serial.begin(BAUD_RATE);
 
  lcd.begin(16, 2);
  lcd.print("Starting...");
  
#if defined(ETHERNET)
    Ethernet.begin(mac, ip);
    server.begin();
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
#endif

  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    //scales[i].set_scale(-228.f);
    lastValue[i] = 0;
    last[i] = 0;
    lastPrint[i] = 0;
    scales[i].set_scale(228.f);
    scales[i].tare();
    delay(DELAY);
    if (USE_EEPROM) { //Firstime use please use calibrate.
      double value = EEPROM.readDouble(eepromAddress[i]);
      scales[i].set_offset(value);//TODO: use eeprom. 
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
  } else if (millisPressedKey == 0 && lcd_key != btnNONE) {
    millisPressedKey = now;
    return btnNONE;
  } else if (((millisPressedKey + 10) < now) && (debounceTime < now)) {
    int lcd_key_confirm = read_LCD_buttons();
    debounceTime = now + 300;
    millisPressedKey = 0;
    if (lcd_key == lcd_key_confirm) {
      //Serial.println(lcd_key);
      buttonPressed = 0;
      return lcd_key;
    } else {
      millisPressedKey = now;
      return btnNONE;
    }
  } else {
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
  if ((lastPrint[i] + DEFAULT_WEIGHT_PRINT_TIME) < now || ((useFastWeight && (last[i] + DEFAULT_WEIGHT_TIME_FAST) < now))) {
    Serial.print(weightName);
    Serial.println(value, 1);
    lastPrint[i] = now;
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
      if (value < 0) {
        decimals = 2; 
      } else {
        decimals = 3; 
      }
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
  if (printGUI) {
    return;
  }
  char* weight = new char[7];
  getWeight(value, weight);
  //lcd.clear();
  if (WEIGHT_SENSORS > 2) {
    lcd.print(weight + kilo);
  } else {
    if (WEIGHT_SENSORS == 1) {
      lcd.print(weightName + weight + kilo + "   ");
      lcd.setCursor(0, 1);
      String beers = String("Beers:");
      char numberOfBeers[4];
      dtostrf(((value - KEG_WEIGHT) / (TAP_SIZE)), 1, 0, numberOfBeers);
      lcd.print(beers + numberOfBeers + " (0.33)");
    } else {
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
      if (((last[i] + DEFAULT_WEIGHT_TIME) < now) || ((useFastWeight && (last[i] + DEFAULT_WEIGHT_TIME_FAST) < now))) {
      lcd.setCursor(0, 1);

      String weightName = getName(i);
      float value = getValue(scales[i]);

      if ((value > (lastValue[i] + 50)) || (value < (lastValue[i] - 50))) {
        useFastWeight = 1;
        lastValue[i] = value;
      } else {
        useFastWeight = 0;
      }
      
      printToSerial(value, now, weightName, i);

      const int row = getRow(i);
      const int col = getCol(i);
      lcd.setCursor(col, row);
      
      printWeightOnLCD(value, weightName);

      last[i] = millis();
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
  } else {
    lcd.setCursor(0, 0);
    lcd.print(">");
    lcd.setCursor(0, 1);
    lcd.print(" ");
  }
}

void printMenu() {
  if (!printGUI){
    return;
  }
  int up = menuMarker % 2;
  printMenuMarker(up);
  if (!up) {
    lcd.setCursor(1, 0);
    lcd.print(menu[menuMarker]);
    if (menuMarker < lastItem) {
      lcd.setCursor(1, 1);
      lcd.print(menu[menuMarker + 1]);
    } else {
      lcd.setCursor(1, 1);
      lcd.print(EMPTY);
    }
  } else {
    lcd.setCursor(1, 0);
    lcd.print(menu[menuMarker - 1]);
    lcd.setCursor(1, 1);
    lcd.print(menu[menuMarker]);
  }
  
}

void doTare() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Tare all weights");
  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    scales[i].tare();
    delay(DELAY);
  }
  lcd.print("Done tare...");
  delay(DELAY * 3);
  printGUI = 0;
  lcd.clear();
}

void doCalibrate() {  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrate all weights");
  lcd.setCursor(0, 1);
  lcd.print("Please clear all.");
  delay(4000);
  for (int i = 0; i < WEIGHT_SENSORS; i++) {
    scales[i].tare();
    double sum = scales[i].read_average(10);
#if defined(DEBUG)
    Serial.print("Offset is");
    Serial.println(sum);
    //TODO: Safe into eeprom. http://playground.arduino.cc/Code/EEPROMex
#endif
    EEPROM.writeDouble(eepromAddress[i], sum);
    delay(DELAY);
    printGUI = 0;
    lcd.clear();
  }
}

void doButtonAction(int btn) {
  if (btn != btnNONE) {
#if defined(DEBUG)
    Serial.print(btn);
#endif

   switch (btn) {
    case btnRIGHT: {
      if (printGUI) {
        
      } else {
          lcd.clear();
          printGUI = 1;
       }
        break;
    }
    case btnSELECT: {
       if (printGUI) {
           if (menuMarker == 1) {
             doTare(); 
           } else if (menuMarker == 0) {
             doCalibrate();
           }
       }
        break;
      }
    case btnLEFT: {
      if (printGUI) {
         lcd.clear();
         printGUI = 0;
      } else {
         
       }
        break;
      }
    case btnUP: {
       if (menuMarker == 0) {
             menuMarker = lastItem;
       } else {
             menuMarker = menuMarker - 1;
       }
       break;
      }
    case btnDOWN: {
         if (printGUI) {
           if (menuMarker >= lastItem) {
             menuMarker = 0;
           } else {
             menuMarker = menuMarker + 1;
           }
         }
        break;
      }
    case btnNONE:
      {
        break;
      }
    }
  }
}

void handleWebReq() {
#if defined(ETHERNET)
  EthernetClient client = server.available();
  String data = String();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        data += c;
        //Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
            Serial.print(data);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");  // the connection will be closed after completion of the response
            client.println();
            client.println("{");
            client.println("\"Weights\":[");
            for (int i = 0; i < WEIGHT_SENSORS ; i++) {
              client.print("{\"V");
              client.print(i+1);
              client.print("\": ");
              client.print(lastValue[i]);
               if (i < WEIGHT_SENSORS - 1) {
                client.println("},");
            } else {
                client.println("}");
            }
          }
          client.println("]");
          client.println("}");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current lin
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
#endif
}

void loop() {
  handleWebReq();
  int btn = getButtonPressed();
  doButtonAction(btn);
  doWeigth();
  printMenu();
  delay(2);
}



