#include <SPI.h>
#include <Ethernet.h>
#include "Adafruit_WS2801.h"
#include "StripUtils.h"


#define NUM_PIXELS 29



const short dataPin  = 7;    // Yellow wire on Adafruit Pixels 
const short clockPin = 6;    // Green wire on Adafruit Pixels




byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0x88, 0x02 };

IPAddress ip(10,1,1,230);
IPAddress gateway(10, 1, 1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(23);

const int inputBufferSize = 200;

char inputBuffer[inputBufferSize];
int counter = 0;
boolean receivingCommand = false;



Adafruit_WS2801 strip = Adafruit_WS2801(NUM_PIXELS, dataPin, clockPin);


uint32_t blinkColors[NUM_PIXELS]; 
uint8_t blinksPer10s[NUM_PIXELS];
unsigned long nextBlinkTime[NUM_PIXELS];

unsigned long m;

boolean rainbowMode = false;


void setup() {
  Serial.begin(57600);

  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  
  
  strip.begin();
  
  // Update LED contents, to start they are all 'off'
  strip.show();
  
  
  //strip self test  
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, 0x8f0000);
    strip.show();
    delay(5);
  }
  
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, 0x008f00);
    strip.show();
    delay(5);
  }
  
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, 0x00008f);
    strip.show();
    delay(5);
  }
  
  
  reset();
  

}


void loop() {
  byte b = NULL;
  m = millis();

  EthernetClient client = server.available();


  if (client) {
    b = client.read();
  }

  if (b != NULL) {
    Serial.print(counter);
    Serial.println(" " + String(b, HEX));
    if (counter > 0 && b == '[' && inputBuffer[counter-1] == '[') {
      clearBuffer();
      counter = 0;
      receivingCommand = true;
      Serial.println("receiving command");
    } 
    else if (receivingCommand && counter > 0 && b == ']' && inputBuffer[counter-1] == ']') {
      receivingCommand = false;
      counter--;
      inputBuffer[counter] = ' '; //terminator
      processBuffer(counter);

      clearBuffer();
      counter = 0;
    } 
    else {
      inputBuffer[counter++] = b;
      if (counter > inputBufferSize) {
        clearBuffer();
        counter = 0;
      }
    }
  }
  
  
  if (rainbowMode) {
    
    hmRainbowLoop();
    
  } else {
  
    
    for (int i = 0; i < NUM_PIXELS; i++) {
      if (blinkColors[i] != NULL) {
        if (nextBlinkTime[i] < m) {
          
          Serial.print("BLINK ");
          Serial.println(i);
    
          nextBlinkTime[i] = m + 10000 / blinksPer10s[i];
          uint32_t tempCol = strip.getPixelColor(i);
          strip.setPixelColor(i, blinkColors[i]);
          blinkColors[i] = tempCol;
        }
        
      }
    }
    strip.show();
  
  }

}


void processBuffer(int len) {
  if (len == 5 && String(inputBuffer).substring(0,5) == "reset") {
    reset();
    return;
  }
  
  if (len == 7 && String(inputBuffer).substring(0,7) == "rainbow") {
    rainbowMode = true;
  }
  
  if (len == 6 && String(inputBuffer).substring(0,6) == "normal") {
    rainbowMode = false; 
    reset();
  }
  
  if (len == 8 || len == 14 || len == 16) {
    String strN = String(inputBuffer).substring(0, 2);
    String strC = String(inputBuffer).substring(2, 8);
    
    Serial.println("Number: " + strN);
    Serial.println("Color: " + strC);
   
    int n = convertStringNumberToInt(strN, 10);  
  
    strip.setPixelColor(n, StripUtils().scaleColor(convertStringNumberToInt32(strC, 16)));
    strip.show();
    
    blinkColors[n] = NULL;
    
    if (len == 14 || len == 16) {
      String strBlinkC = String(inputBuffer).substring(8, 14);
            Serial.println("blink color: " + strBlinkC);
            
      blinkColors[n] = StripUtils().scaleColor(convertStringNumberToInt32(strBlinkC, 16));
      blinksPer10s[n] = 20;
      nextBlinkTime[n] = m + 10000 / blinksPer10s[n];
      Serial.println(nextBlinkTime[n]);
      
      
      if (len == 16) {
        String strBlinksPerMin = String(inputBuffer).substring(14, 16);
        blinksPer10s[n] = convertStringNumberToInt32(strBlinksPerMin, 16);
        
      }
    }
    
  } 
}





void reset() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, 0);
    blinkColors[i] = NULL;
  }
  strip.show();
 
}





void clearBuffer() {
  Serial.println("clearing buffer");
  for (int i = 0; i < inputBufferSize; i++)
    inputBuffer[i] = 0x00;
}



int convertStringNumberToInt(String str, int base) {
  char * pEnd;
  return strtol(&(str[0]), &pEnd, base);
}

uint32_t convertStringNumberToInt32(String str, int base) {
  char * pEnd;
  return strtol(&(str[0]), &pEnd, base);
}




float hmk;

void hmRainbowInit() {
  hmk = 0;
}

void hmRainbowLoop() {
   for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, StripUtils().getWheelColor(1, ((i * 128 / strip.numPixels()) + (int)hmk) % 128));
  }
  strip.show();

  hmk += 2;
  if (hmk > 128 * 5) {
    hmk = 0;
  } 
}

