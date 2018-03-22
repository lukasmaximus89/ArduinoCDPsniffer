                  #include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>


#include "cdp_listener.h"

#define VERSION_STR "v0.1"

#define LCD_INFO_NONE 0
#define LCD_INFO_CDP_DATA 1

#define printhex(n) {if((n)<0x10){Serial.print('0');}Serial.print((n),HEX);}

//LED
#define TFT_CS     4  //10
#define TFT_RST    -1 //9  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to -1!
#define TFT_DC     8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
#define TFT_SCLK 13   // set these to be whatever pins you like!
#define TFT_MOSI 11   // set these to be whatever pins you like!

char* lcd_data_value;


void setup() {
  // Init serial
  Serial.begin(115200);

  // Let user know we're initializing
  Serial.println("Initializing");

 
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(tft.getRotation()+1);


  // tft print function!
  tftPrintTest();

  cdp_listener_init();
  cdp_packet_handler = cdp_handler;
  
  // Let user know we're done initializing
  Serial.println("Initialization done");
  
 
  tft.setTextWrap(true);
}

volatile unsigned long last_cdp_received = 0;
volatile unsigned int lcd_display_type = LCD_INFO_NONE;



void loop() {

  if (last_cdp_received ==0) {
    
  switch(cdp_listener_update()) {
    case CDP_STATUS_OK: 
    lcd_display_type = LCD_INFO_CDP_DATA;
    update_lcd();
    last_cdp_received = 1;
    break;
    //case CDP_INCOMPLETE_PACKET: Serial.println(F("Incomplete packet received")); break;
    case CDP_UNKNOWN_LLC: Serial.println(F("Unexpected LLC packet")); break;
  };
  
  Serial.println("T");
  
  } // end if Last Cdp==0;

 
  
}

void update_lcd() {
 
  //lcd.clear();
    
  switch(lcd_display_type) {
    case LCD_INFO_NONE: break;
    case LCD_INFO_CDP_DATA:
      tft.setTextSize(1);
      tft.setTextColor(ST7735_GREEN);
      tft.println("http://network.coj.go.th");     
      break;
  }
  //TODO: print current item(s)
 
}

void cdp_handler(const byte cdpData[], size_t cdpDataIndex, size_t cdpDataLength, const byte macFrom[], size_t macLength) {
    unsigned long secs = millis()/1000;
    int min = secs / 60;
    int sec = secs % 60;
    
    Serial.print(min); Serial.print(':');
    if(sec < 10) Serial.print('0');
    Serial.print(sec);
    Serial.println();

    
    Serial.print(F("CDP packet received from "));
    print_mac(macFrom, 0, macLength);
    Serial.println();

    int cdpVersion = cdpData[cdpDataIndex++];
    if(cdpVersion != 0x2) {
      Serial.print(F("Version: "));
      Serial.println(cdpVersion);
    }
    
    Serial.print(F("Version: "));
    Serial.println(cdpVersion);
    
    int cdpTtl = cdpData[cdpDataIndex++];
    Serial.print(F("TTL: "));
    Serial.println(cdpTtl);

    
    unsigned int cdpChecksum = (cdpData[cdpDataIndex] << 8) | cdpData[cdpDataIndex+1];
    cdpDataIndex += 2;
    Serial.print(F("Checksum: "));
    tft.setTextColor(ST7735_BLUE);tft.print(F("Checksum: "));tft.setTextColor(ST7735_GREEN);
    printhex(cdpChecksum >> 8);
    printhex(cdpChecksum & 0xFF);
    Serial.println();
    tft.println(cdpChecksum,HEX);


    
    while(cdpDataIndex < cdpDataLength) { // read all remaining TLV fields
      unsigned int cdpFieldType = (cdpData[cdpDataIndex] << 8) | cdpData[cdpDataIndex+1];
      cdpDataIndex+=2;
      unsigned int cdpFieldLength = (cdpData[cdpDataIndex] << 8) | cdpData[cdpDataIndex+1];
      cdpDataIndex+=2;
      cdpFieldLength -= 4;
      
      switch(cdpFieldType) {
 
        
        case 0x0001:
          handleCdpAsciiField(F("Device ID: "), cdpData, cdpDataIndex, cdpFieldLength);
          lcd_handleCdpAsciiField("Device ID: ", cdpData, cdpDataIndex, cdpFieldLength);
          break;
        case 0x0002:
          handleCdpAddresses(cdpData, cdpDataIndex, cdpFieldLength);
          break;
        case 0x0003:
          handleCdpAsciiField(F("Port ID: "), cdpData, cdpDataIndex, cdpFieldLength);
          lcd_handleCdpAsciiField("Port ID: ", cdpData, cdpDataIndex, cdpFieldLength);
          break;
    //    case 0x0004:
    //        handleCdpCapabilities(cdpData, cdpDataIndex, cdpFieldLength); //
    //        break;
        case 0x0005:
          handleCdpAsciiField(F("Software Version: "), cdpData, cdpDataIndex, cdpFieldLength);
       // lcd_handleCdpAsciiField("Software Version: ", cdpData, cdpDataIndex, cdpFieldLength);
          break;
        case 0x0006:
          handleCdpAsciiField(F("Platform: "), cdpData, cdpDataIndex, cdpFieldLength);
          lcd_handleCdpAsciiField("Platform: ", cdpData, cdpDataIndex, cdpFieldLength);
          break;
        case 0x000a:
          handleCdpNumField(F("Native VLAN: "), cdpData, cdpDataIndex, cdpFieldLength);
          lcd_handleCdpNumField(F("Native VLAN: "), cdpData, cdpDataIndex, cdpFieldLength);
          break;
        case 0x000b:
          handleCdpDuplex(cdpData, cdpDataIndex, cdpFieldLength);
          break;
        default:
          // TODO: raw field
//            Serial.print(F("Field "));
//            printhex(cdpFieldType >> 8); printhex(cdpFieldType & 0xFF);
//            Serial.print(F(", Length: "));
//            Serial.print(cdpFieldLength, DEC);
//            Serial.println();
          break;
      }
      
      cdpDataIndex += cdpFieldLength;
    }
    
    Serial.println();
  
}

void handleCdpAsciiField(const __FlashStringHelper * title, const byte a[], unsigned int offset, unsigned int length) {
  Serial.print(title);
  print_str(a, offset, length);
  Serial.println();
}

void lcd_handleCdpAsciiField(char* title, const byte a[], unsigned int offset, unsigned int length) {
  if (title=="Port ID: ") {tft.setTextColor(ST7735_BLUE);tft.print("Port: ");tft.setTextColor(ST7735_GREEN);}
  if (title=="Software Version: ") {tft.setTextColor(ST7735_BLUE);tft.print("Software Version: ");tft.setTextColor(ST7735_GREEN);}
  if (title=="Platform: ") {tft.setTextColor(ST7735_BLUE);tft.print("Model: ");tft.setTextColor(ST7735_GREEN);}
  if (title=="Device ID: ") {tft.setTextColor(ST7735_BLUE);tft.print("Device: ");tft.setTextColor(ST7735_GREEN);}

  tft_print_str(a, offset, length);
  tft.println();
  
}


void handleCdpNumField(const __FlashStringHelper * title, const byte a[], unsigned int offset, unsigned int length) {
  unsigned long num = 0;
  for(int i=0; i<length; ++i) {
    num <<= 8;
    num += a[offset + i];
  }

  Serial.print(title);
  Serial.print(num, DEC);
  Serial.println();
}

void lcd_handleCdpNumField(const __FlashStringHelper * title, const byte a[], unsigned int offset, unsigned int length) {
  unsigned long num = 0;
  for(int i=0; i<length; ++i) {
    num <<= 8;
    num += a[offset + i];
  }
  tft.setTextColor(ST7735_BLUE);tft.print(title);tft.setTextColor(ST7735_GREEN);
  tft.print(num, DEC);
  tft.println();
}


void handleCdpAddresses(const byte a[], unsigned int offset, unsigned int length) {
  Serial.println(F("Addresses: "));
 // unsigned long  numOfAddrs = (a[offset] << 24) | (a[offset+1] << 16) | (a[offset+2] << 8) | a[offset+3];
 unsigned long  numOfAddrs = 1;
 //Number of addresses:
  offset += 4;

 
  for(unsigned long i=0; i<numOfAddrs; ++i) {
    unsigned int protoType = a[offset++];
    unsigned int protoLength = a[offset++];
    byte proto[8];
    for(unsigned int j=0; j<protoLength; ++j) {
      proto[j] = a[offset++];
    }
    unsigned int addressLength = (a[offset] << 8) | a[offset+1];
    offset += 2;
    byte address[4];
    if(addressLength != 4) Serial.println(F("Expecting address length: 4"));
    for(unsigned int j=0; j<addressLength; ++j) {
      address[j] = a[offset++];
    }
    
    Serial.print("- ");
    print_ip(address, 0, 4);
  }
  Serial.println();
}


void handleCdpDuplex(const byte a[], unsigned int offset, unsigned int length) {
  Serial.print(F("Duplex: "));
  if(a[offset]) {
    Serial.println(F("Full"));
  } else {
    Serial.println(F("Half"));
  }
}

void print_str(const byte a[], unsigned int offset, unsigned int length) {
  for(int i=offset; i<offset+length; ++i) {
    Serial.write(a[i]);
  }
}

void tft_print_str(const byte a[], unsigned int offset, unsigned int length) {
  
  for(int i=offset; i<offset+length; ++i) {
    tft.write(a[i]);
  }

}


void print_ip(const byte a[], unsigned int offset, unsigned int length) {
  tft.setTextColor(ST7735_BLUE);tft.print("IP: ");tft.setTextColor(ST7735_GREEN);
  for(int i=offset; i<offset+length; ++i) {
    if(i>offset) {Serial.print('.'); tft.print('.');}
    Serial.print(a[i], DEC);
    tft.print(a[i], DEC);  
  } tft.println();
}


void print_mac(const byte a[], unsigned int offset, unsigned int length) {
  tft.setTextColor(ST7735_BLUE);tft.print("Mac: ");tft.setTextColor(ST7735_GREEN);
  for(int i=offset; i<offset+length; ++i) {
    if(i>offset) {Serial.print(':'); tft.print(':');}
    
    if(a[i] < 0x10) {Serial.print('0');tft.print('0');}
    Serial.print(a[i], HEX); tft.print(a[i], HEX);  
  } tft.println();
}

// test LCD

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void tftPrintTest() {
  tft.setTextWrap(false);
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 5);
  
  tft.setTextColor(ST7735_GREEN);
  tft.setTextSize(2);
  tft.print("COJ");
  
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(1);
  tft.setCursor(38, 5);
  tft.println("We are Network!");
  
  tft.setTextColor(ST7735_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(38, 15);
  tft.println("Court of Justice");
  tft.setTextColor(ST7735_RED);
  tft.setTextSize(1);
  tft.println("Initializing");
  tft.println();
  tft.setTextColor(ST7735_GREEN);
    
}
