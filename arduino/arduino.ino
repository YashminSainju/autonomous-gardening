
#include <LiquidCrystal.h>
#include "SD.h"
#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"


const int relay = 8;

LiquidCrystal lcd(7,6, 5, 4, 3, 2);
unsigned long StartTime = millis();

const int chipSelect = 10;

#define LOG_INTERVAL  1000 // mills between entries (reduce to take more/faster data)

#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()


RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line

// the logging file
File logfile;

void error(String str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates err  digitalWrite(redLEDpin, HIGH);

  while(1);
}

void setup() {
  Serial.begin(9600);

  //LCD Display
  lcd.begin(16,2);
  
  pinMode(relay, OUTPUT);

  digitalWrite(relay, HIGH);

  lcd.setCursor(0, 0);
  lcd.print("Mstr lvl:");
  lcd.setCursor(0,1);
  lcd.print("Last wtrd:");

  //SD Card and RTC 

 #if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
  if (!RTC.isrunning()){
    RTC.adjust(DateTime(__DATE__,__TIME__));
    }
  

  logfile.println("millis,stamp, datetime, moisture");    
#if ECHO_TO_SERIAL
  Serial.println("millis,stamp,datetime,moisture");
#endif //ECHO_TO_SERIAL
 
  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);
}

void turnPumpOn(){
  if (analogRead(A0) > 700){
    digitalWrite(relay, LOW);  
    StartTime = millis();   
  } else {
    digitalWrite(relay, HIGH);
  }

}
  
void lcdDisplay(){
  lcd.setCursor(9, 0);
  lcd.print(analogRead(A0));
 
  
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = (CurrentTime - StartTime)/60000;
  
  lcd.setCursor(10,1);
  lcd.print(ElapsedTime);
  lcd.print("m");
}

void sdCard(DateTime now){
   delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  

  
  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");  
#endif

  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(", ");
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(", ");
  Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print('"');
#endif //ECHO_TO_SERIAL
  
  logfile.print(", ");    
  logfile.print(analogRead(A0));
#if ECHO_TO_SERIAL
  Serial.print(", ");   
  Serial.print(analogRead(A0));
 
#endif //ECHO_TO_SERIAL


  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif // ECHO_TO_SERIAL

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  
  // blink LED to show we are syncing data to the card & updating FAT!
  logfile.flush();
}

 

void loop() {
  DateTime now = RTC.now();
  turnPumpOn();
  lcdDisplay();
  sdCard(now);
  delay(1000);
}

