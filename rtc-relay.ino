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
#include "Wi-Fi-Config.h"
#include "Dynamic2DArray.h"

//Used to represent a blank string in the code, to make the code more easily readable
#define BLANKSTR ""

//Set the following to true to test the relay
//Causes the relay toggle every second (1000 miliseconds)
#define TESTRELAY false

//Controls how long (in milliseconds) the sketch delays between loops
#define INTERLOOPDELAY 200

//Used to define max (255) and min (0) voltage outputs for the
//Analog output pin used by the app to trigger the relay.
#define MAXOUTPUT 255
#define MINOUTPUT 0

//compiler constants used to configure how the controller acts
#define SETTIME -1
#define SUNRISE -2
#define SUNSET -3

//Timezones
//Use the following to add to this list https://www.timeanddate.com/time/zones/
//This is by no means a complete list, add to this as needed for your timezone
#define GMT  0  // GMT – Greenwich Mean Time / Coordinated Universal Time
#define CET  1  // Central European Time
#define EST -5  // Eastern Standard Time (USA)
#define EDT -4  // Eastern Daylight Time (USA)
#define PST -8  // Pacific Standard Time (USA)
#define PDT -7  // Pacific Daylight Time (USA)

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

//Sunrise and sunset times vary depending on location, so...
//If using sunrise or sunset as trigger options, populate the locLat
//and locLong values with the location's latitude and longitude values
//These values are for Charlotte, NC, to get Sunrise/Sunset values for
//your location, replace these strings with the appropriate values for
//your location.
const String locLat = "35.227085";
const String locLong = "-80.843124";

//Set the timezone using the timezone defines listes above.
//You'll have to switch the device from standard time to daylight time
//during the summer if you live in an area that observes daylight time.
const int timeZone = EST;

//Wi-Fi settings are in an external file: wifi-config.h
//Set your Wi-Fi SSID and password there.

//Populate the following variable with the number of rows in the slots array
//I'm doing this way because Arduino doesn't have an easy way to determine
//the size of an array at runtime, especially multidimensional arrays.
#define NUMSLOTS 2
//Use the Slots array to define when and how the relay operates...
//Slots array values: {OnTrigger, onValue, OffTrigger, OffValue, doRandom };
//Examples:
//turn the relay on at 6:00 AM, turn it off at 10 minutes after sunrise. No randomization.
//{SetTime, 600, Sunrise, 10, false },
//Turn the relay 15 minutes before sunset, turn it off at 11:00 PM. No randomization.
//{Sunset, -15, SetTime, 2300, false }
//At 5:30 AM, turn the relay on and off randomly until 7:00 AM.
//{Settime, 530, SetTime, 700, true }
//At 15 minutes before sunset, turn the relay on and off randomly until 11:00 PM
//{Sunset, -15, SetTime, 2300, true }
int slots[NUMSLOTS][5] = {
  {SETTIME, 600, SUNRISE, 15, false },
  {SUNSET, 15, SETTIME, 2300, false }
  //  {SETTIME, 600, SETTIME, 000, false },
  //  {SETTIME, 1700, SETTIME, 2300, false }
};

//============================================================

//Used to pretty-up the sketch's output
String hashes = "########################################";

// Realtime Clock object
RTC_PCF8523 rtc;
DateTime currentTime;

//used to store minute values the app uses
//currentMin is, well, the current minute. lastMin is the previous minute
//The two are compared to see if the minute changed and it's time to
//work again
int currentMin, lastMin;

Dynamic2DArray timeSlots(true);

//The current status of the relay (on/off)
bool relayStatus;

//Flag used to indicate whether we're still dealing
//with a button press incident. In other words, the
//loop repeats, but we want to only act upon the button
//going LOW once for as long as it's pushed.
bool buttonPushed = false;

//Are either trigger values set to sunrise or sunset?
//Then we need to know we'll need to get solar data from the Internet
bool getSolarData;

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
  Serial.println(hashes);
  Serial.println("# Arduino RTC Relay Controller         #");
  Serial.println("# By John M. Wargo (www.johnwargo.com) #");
  Serial.println(hashes);

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

  //does slots contain any solar values?
  getSolarData = checkForSolarEvents();
  Serial.print("Solar data: ");
  if (getSolarData) {
    Serial.println("ENABLED");
    //do we have the long and lat values we need ?
    if ((locLong == BLANKSTR) || (locLat == BLANKSTR)) {
      //No? then that's a problem
      Serial.println("ERROR: You must provide values for locLong and locLat");
      //loop forever
      while (true);
    }
  } else {
    Serial.println("DISABLED");
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
  // attempt to connect to Wifi network:
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

  //Build the list of active slots
  //buildSlotsList();
  //Get the current time
  currentTime = rtc.now();
  //we just turned on, are we supposed to have the relay on at this time?
  if (isInOnPeriod(currentTime.minute())) {
    toggleRelay();
  }

  timeSlots.add(1, 2);
  timeSlots.add(3, 4);
  timeSlots.add(5, 6);
}

void loop() {

  //Variable used to store button status as its read
  int buttonState;

  if (TESTRELAY) {
    //toggle the relay every second
    toggleRelay();
    delay(1000);
  } else {
    //otherwise, get down to business...
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

      //Then, lets do our time-based stuff...
      //doTimeBasedStuff();
    }

    //Wait approximately for interLoopDelay miliseconds before doing it all again
    delay(INTERLOOPDELAY);

  }
}

bool checkForSolarEvents() {
  //loop through the slots array and return true if at least
  //one of the slots uses Solar data (sunrise/sunset). This
  //lets the sketch know if it needs to call out to the
  //external service every day to get solar data
  for (int i = 0; i < NUMSLOTS; i++) {
    //Anything but setTime
    if ((slots[i][0] < SETTIME)  || (slots[i][2] < SETTIME)) {
      //Ha, we found one...
      return true;
    }
  }
  //we got here, so we must not have one
  return false;
}

bool isInOnPeriod(int timeVal) {
  //Loop through the active slots array to see if the relay is
  //supposed to be on. This assumes the device just powered on
  //and it doesn't know whether it's supposed to be on or not.
  for (int i = 0; i < timeSlots.length(); i++) {
    if ((timeVal > timeSlots.getValue(i, 0) ) && (timeVal < timeSlots.getValue(i, 1))) {
      //yep, we're supposed to be on
      return true;
    }
  }
  return false;
}

void flashxTimes(int numTimes) {
  //Cycle the relay twice numtimes times.
  //Make sure we always start with the relay OFF
  setRelay(false);
  //Do it twice for each loop, on, and off.
  int loopEnd = numTimes * 2;
  for (int i = 1; i <= loopEnd; i++) {
    toggleRelay();
    delay(250);
  }
}

//##################################################
// Rules
//##################################################
// OnTime/Sunrise, OffTime/Time - make sure offTime isn't before Sunrise.
// OnTime/Sunset, OffTime/Time - make sure offTime isn't before Sunset.
// ForEach, make sure OffTime > OnTime. Hmmm, that won't work if you are on past midnight.
//##################################################
void doTimeBasedStuff() {
  //Get the current time
  currentTime = rtc.now();
  //Get the current minute
  currentMin = currentTime.minute();
  //Has the minute changed?
  if (currentMin != lastMin) {
    Serial.println("Minute mark");
    //Yes? Then do something, or at least check to see if
    //we're supposed to do something.


    //Set the last minute pointer to the current minute.
    lastMin = currentMin;
  } else {
    // The minute hasn't changed, so skip
    //Serial.println("Skipping");
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

void getSolarTimes() {
  //Uses the Sunrise Sunset API from http://sunrise-sunset.org/api
  //Test URL to retrieve data for Charlotte, NC US
  //Usage: http://api.sunrise-sunset.org/json?lat=35.2271&lng=-80.8431&date=today

  //  WiFiClient client;
  //char charURL[255];
  //  //char server[] = "http://api.sunrise-sunset.org/json?lat=";
  //  //https://www.arduino.cc/en/Reference/StringObject
  //  String url =  "http://api.sunrise-sunset.org/json?lat=" + locLat + "&lng=" + locLong + "&date=today";
  //
  //  url.toCharArray(charURL, 255);
  //
  //  if (client.connect(charURL, 80)) {
  //    Serial.println("connected to server");
  //    // Make a HTTP request:
  //    client.println("GET /search?q=arduino HTTP/1.1");
  //    client.println("Host: www.google.com");
  //    client.println("Connection: close");
  //    client.println();
  //  }

  HttpClient client;
  // Make a HTTP request:
  client.get("http://api.sunrise-sunset.org/json?lat=" + locLat + "&lng=" + locLong + "&date=today");
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  Serial.flush();

}

