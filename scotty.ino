#include <SPI.h>
#include <Ethernet.h>
#include "Adafruit_WS2801.h"
#include "StripUtils.h"


#define NUM_PIXELS 50



const short dataPin  = 7;    // Yellow wire on Adafruit Pixels 
const short clockPin = 6;    // Green wire on Adafruit Pixels




byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0x88, 0x02 };

IPAddress ip(192,168,1,182);
IPAddress gateway(192,168,1, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetServer server(23);

const int inputBufferSize = 200;

char inputBuffer[inputBufferSize];
int counter = 0;
boolean receivingCommand = false;



Adafruit_WS2801 strip = Adafruit_WS2801(NUM_PIXELS, dataPin, clockPin);




void setup() {
  Serial.begin(57600);

  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  
  
  strip.begin();
  
  // Update LED contents, to start they are all 'off'
  strip.show();
  
  /*
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
  */
  
  reset();
  

}


void loop() {
  byte b = NULL;

  EthernetClient client = server.available();

  if (Serial.available() > 0) {
    b = Serial.read();
  }

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
  

}


void processBuffer(int len) {
  String strN = String(inputBuffer).substring(0, 2);
  String strC = String(inputBuffer).substring(2, 8);
  
  Serial.println("Number: " + strN);
  Serial.println("Color: " + strC);
 
  int n = convertStringNumberToInt(strN, 10);  
  uint32_t col = convertStringNumberToInt32(strC, 16);

  strip.setPixelColor(n, col);
  strip.show();
  
  
  
}



void reset() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, 0);
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


