// Adafruit Feather MO tutorials
// https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/overview

// RTC board: Adafruit Adalogger FeatherWing Real time clock + SD card for data reading & writing
// https://www.adafruit.com/products/2922
// https://learn.adafruit.com/adafruit-adalogger-featherwing
// RTC Library
// https://github.com/adafruit/RTClib

// Adafruit Power Relay FeatherWing
// https://www.adafruit.com/products/3191
// https://learn.adafruit.com/adafruit-power-relay-featherwing

//Code for checking Internet time from https://www.arduino.cc/en/Tutorial/UdpNTPClient

//API for determining sunrise and sunset times
//http://sunrise-sunset.org/api

//https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>
#include <HttpClient.h>
#include "RTClib.h"
#include <TimeLib.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
//Local libraries
#include "constants.h"
#include "wi-fi-config.h"

//============================================================
// USER CONFIGURABLE OPTIONS
//============================================================
//Change the following options based on your specific needs,
//hardware configuration, and physical location.

//The Analog pin the relay is connected to. This means that the
//relay is connected to Analog Pin 1 on the Arduino device.
const int outputPin = A1;
//The Analog pin the button is connected to. This means that the
//button is connected to Analog Pin 2 on the Arduino device.
const int buttonPin = A2;

//Set the time zone using the time zones listed in constants.h.
//You'll have to manually switch the device from standard time to daylight 
//time during the summer if you live in an area that observes daylight time.
const int timeZone = EST;

//Wi-Fi settings are in an external file: wifi-config.h
//Set your Wi-Fi SSID and password there.

//Populate the following variable with the number of rows in the slots array
//I'm doing this way because Arduino doesn't have an easy way to determine
//the size of an array at runtime, especially multidimensional arrays.
#define NUMSLOTS 2
//Use the Slots array to define when the relay(s) go on and off
//Slots array values: {onTime, offTime };
// Examples:
// Turn the relay on at 6:00 AM, turn it off 8:00 AM
// { 600, 800 },
// Turn the relay on at 5:00 PM turn it off at 11:00 PM.
// {1700, 2300}
// Turn the relay on at 5:30 AM, turn the relay off at 7:00 AM.
// {530, 700}
int slots[NUMSLOTS][2] = {
  { 600, 800 },
  {2000, 2300}  
  //BE SURE TO UPDATE THE NUMSLOTS CONSTANT IF YOU ADD/REMOVE
  //ROWS FROM THIS ARRAY
};
//============================================================

//Used to pretty-up the sketch's output
String HASHES = "#########################################";
String SLOTERRORSTR = "\nInvalid SLOT value: ";

// Realtime Clock object
RTC_PCF8523 rtc;
DateTime currentTime;

//used to store minute values the app uses
//currentMin is, well, the current minute. lastMin is the previous minute
//The two are compared to see if the minute changed and it's time to
//work again
int currentMin, lastMin;

//The current status of the relay (on/off)
bool relayStatus;

//Flag used to indicate whether we're still dealing
//with a button press incident. In other words, the
//loop repeats, but we want to only act upon the button
//going LOW once for as long as it's pushed.
bool buttonPushed = false;

//============================================================
// Wi-Fi settings
//============================================================
//Pull the Wi-Fi settings from the wifi-config.h file
char ssid[] = wifi_ssid;
char pass[] = wifi_pass;
//set the initial value for the status variable
int status = WL_IDLE_STATUS;

//============================================================
// UDP/NTP settings
//============================================================
// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// local port to listen for UDP packets
unsigned int localPort = 8888;
// time.nist.gov NTP server
char timeServer[] = "time.nist.gov";
// NTP time stamp is in the first 48 bytes of the message
const int NTP_PACKET_SIZE = 48;
//buffer to hold incoming and outgoing packets
byte packetBuffer[ NTP_PACKET_SIZE];

void setup() {

  //Initialize serial
  Serial.begin(9600);
  delay(2000);
  //Then write program information to the serial port
  Serial.println(HASHES);
  Serial.println("# Arduino RTC Relay Controller (static) #");
  Serial.println("# By John M. Wargo (www.johnwargo.com)  #");
  Serial.println(HASHES);

  //Make sure we always start with the relay OFF, just in case
  setRelay(false);

  //Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8, 7, 4, 2);

  //Set the mode for the pin the pushbutton is connected to
  pinMode(buttonPin, INPUT_PULLUP);

  //==================================================
  //Check the software configuration
  //==================================================
  Serial.println("\nChecking software configuration");

  // Make sure our slot array is populated with valid times
  for (int i = 0; i < NUMSLOTS; i++) {
    //Get our time values
    int onTime = slots[i][0];
    int offTime = slots[i][1];

    // Is onTime valid?
    if (!isValidTime(onTime)) {
      //That won't work, so display an error
      Serial.print(SLOTERRORSTR);
      Serial.println(onTime);
      while (true);
    }

    // Is offTime valid?
    if (!isValidTime(offTime)) {
      //That won't work, so display an error
      Serial.print(SLOTERRORSTR);
      Serial.println(offTime);
      while (true);
    }

    //Are onTime and offTime the same?
    if (onTime == offTime) {
      //That won't work, so display an error
      Serial.print(SLOTERRORSTR);
      Serial.print("OnTime and OffTime cannot have the same value; slot #");
      Serial.println(i);
      while (true);
    }

    // Is onTime AFTER offTime?
    if ( onTime > offTime) {
      //That won't work, so display an error
      Serial.print(SLOTERRORSTR);
      Serial.print("offTime preceeds onTime in slot #");
      Serial.println(i);
      // and hop into an infinite loop
      while (true);
    }
  }

  //Do we have Wi-Fi SSID and Password values?
  //Assumes your SSID has a password
  //Change to the following if you don't have/need a SSID password
  //if (wifi_ssid == blankStr) {
  if (wifi_ssid == BLANKSTR || wifi_pass == BLANKSTR) {
    //No? then there's nothing we can do
    Serial.println("One or more required Wi-Fi settings are empty, check the wifi-config.h file.");
    //loop forever
    while (true);
  }

  //==================================================
  //Check the hardware
  //==================================================
  Serial.println("Checking hardware");
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Wi-Fi shield not found");
    //loop forever
    while (true);
  }

  if (! rtc.begin()) {
    Serial.println("RTC board not found");
    //loop forever
    while (true);
  }

  //Has the RTC board been initialized?
  if (! rtc.initialized()) {
    //RTC hasn't been initialized, so it has no time/date value
    Serial.println("RTC is not initialized");
    //The following line sets the RTC to the date & time this sketch was compiled
    //This assumes initializing the RTC immediately after compile, which you would
    //do when you flash the firmware on the device.
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println("RTC is initialized");
  }

  //==================================================
  // Connect to the network
  //==================================================
  // attempt to connect to the Wi-Fi network:
  while (status != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected");
  printWifiStatus();

  //Start the udp server
  Udp.begin(localPort);
  //Then tell the time library how to check network time
  //The getNTPtime function will execute periodically
  //to keep the clock in sync.
  setSyncProvider(getNTPTime);

  //Set the lastMin variable to one minute before now
  lastMin = rtc.now().minute() - 1;
  //if we went into negative numbers (because the minute was 0)
  if (lastMin < 0) {
    lastMin = 59;
  }

  //Blink the relay twice to indicate we're starting up
  flashxTimes(2);

  //we just started, are we supposed to have the relay on at this time?
  if (isInOnPeriod()) {
    //turn on the relay
    Serial.println("\nWhoops, we're supposed to be on!");
    setRelay(true);
  }

}

void loop() {

  //Variable used to store button status as its read
  int buttonState;

  //Read button state (pressed or not pressed?)
  buttonState = digitalRead(buttonPin);
  //If button pressed...
  if (buttonState == LOW) {
    //Serial.println("Detected button press");
    if (!buttonPushed) {
      //Serial.println("Unique button press");
      //toggle the relay
      toggleRelay();
    } else {
      //Serial.println("Skipping, button is still pushed");
    }
    //then reset our flag so it doesn't execute again
    //until the program reads that the button ISN'T pushed
    buttonPushed = true;
  } else {
    //the button isn't pressed...
    //first, reset the buttonPushed flag, indicating that the button isn't pushed
    //this time through the loop
    buttonPushed = false;

    //Get the current time
    currentTime = rtc.now();
    //Get the current minute
    currentMin = currentTime.minute();
    //Has the minute changed?
    if (currentMin != lastMin) {
      Serial.println("Minute mark");
      //Yes? Then do something, or at least check to see if
      //we're supposed to do something.
      //Get the current time in 24 hour format
      int theTime = getTime24();
      //Loop through our array
      for (int i = 0; i < NUMSLOTS; i++) {
        //Get the on and off times
        int onTime = slots[i][0];
        int offTime = slots[i][1];
        //Should we be turning the relay on?
        if (theTime == onTime) {
          //Then turn it on
          setRelay(true);
        } else {
          //Should we be turning the relay off?
          if (theTime == offTime) {
            //Then turn it off
            setRelay(false);
          }
        } //if
      } //for
      //Set the last minute pointer to the current minute.
      lastMin = currentMin;
    } else {
      // The minute hasn't changed, so skip
      //Serial.println("Skipping");
    }
  }

  //Wait approximately for interLoopDelay miliseconds before doing it all again
  delay(INTERLOOPDELAY);

}

int getTime24() {
  // Return the current time as an integer value in 24 hour format
  DateTime theTime = rtc.now();
  return (theTime.hour() * 100) + theTime.minute();
}

bool isValidTime(int timeVal) {
  //Make sure the time value is between 0 and 2359 with
  // no minute value greater than 59
  return ((timeVal > -1) && (timeVal < 2360) && (timeVal % 100 < 60));
}


bool isInOnPeriod() {
  //Loop through the active slots array to see if the relay is
  //supposed to be on. This assumes the device just powered on
  //and it doesn't know whether it's supposed to be on or not.
  int timeVal = getTime24();
  for (int i = 0; i < NUMSLOTS; i++) {
    if ((timeVal > slots[i][0] ) && (timeVal < slots[i][1])) {
      //yep, we're supposed to be on
      return true;
    }
  }
  return false;
}

void flashxTimes(int numTimes) {
  //Cycle the relay numtimes times.
  //Assumes we're starting with the relay off
  //Do it twice for each loop, on, and off.
  int loopEnd = numTimes * 2;
  for (int i = 1; i <= loopEnd; i++) {
    toggleRelay();
    delay(250);
  }
}

void toggleRelay() {
  //Flips the relay from on to off or off to on
  setRelay(!relayStatus);
}

void setRelay(bool status) {
  //Set the relay to a specific status (on=true/off=false)
  Serial.print("Relay: ");
  if (status) {
    Serial.println("ON");
    analogWrite(outputPin, MAXOUTPUT);
  } else {
    Serial.println("OFF");
    analogWrite(outputPin, MINOUTPUT);
  }
  //Store the status we set, so that toggleRelay will be able
  //to accurately toggle the relay later
  relayStatus = status;
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// send an NTP request to the time server
void sendNTPpacket(char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNTPTime() {
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Sending NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      //We got a response
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900, adjustedTime;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.print("Time: ");
      Serial.println(secsSince1900);
      Serial.println();

      //Determine the local time
      adjustedTime = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      //Set the time in the RTC
      rtc.adjust(adjustedTime);
      //Return the current time to the time library
      return adjustedTime;
    }
  }
  Serial.println("No NTP Response\n");
  return 0; // return 0 if unable to get the time
}

