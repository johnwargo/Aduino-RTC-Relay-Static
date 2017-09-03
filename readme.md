# Arduino RTC Relay - Static

<!-- toc -->

* [Hardware](#hardware)
* [Configuring the Arduino Sketch](#configuring-the-arduino-sketch)
  * [Analog Pins](#analog-pins)
  * [Time Zone](#time-zone)
  * [Slots Array](#slots-array)
* [Additional Resources](#additional-resources)
* [Update History](#update-history)

<!-- toc stop -->


An Arduino project for turning on/off a relay at a scheduled time. The project uses the Adafruit Feather and a real-time clock to ensure accuracy. This project is essentially a hand-made version of those light timers you buy to control your house lights when you're on vacation. The difference is that this version supports a virtually unlimited number of on/off timeslots plus sets its internal clock automatically using a network time server. 

This is the **Static** version of the project, it turns the relay on and off at set times (controlled via settings within the Arduino sketch). A subsequent version of this project will enable you to use Solar data (sunrise and sunset) to control when the relay turns on and/or off.

## Hardware

In the first version of this project, I used the following hardware:

+	[Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500](https://www.adafruit.com/products/3010)
+	[Adafruit Adalogger FeatherWing Real time clock + SD card](https://www.adafruit.com/products/2922)
+	[Feather Header Kit - 12-pin and 16-pin Female Header Set](https://www.adafruit.com/products/2886)
+	[CR1220 3V Lithium Coin Cell Battery](https://www.adafruit.com/products/380)
+	[Powerswitch Tail II](https://www.adafruit.com/products/268)
+	[5V 2.4A Switching Power Supply with 20AWG MicroUSB Cable](https://www.adafruit.com/products/1995)

Assembled, it looks like this:

![Assembled with Powerswitch Tail](/images/figure-01.png)

The Feather and Featherwing are connected together (using the header kit shown in the parts list) and sit inside the enclosure and receive power from a standard smartphone charger (not shown). Wires from the Featherwing Analog output connect to the Powerswitch Tail and control the power passing through the power cable. When power is supplied across those wires, a relay in the Powerswitch Tail closes and allows power to pass through the cable shown in the figure. The push button on the enclosure enables you to manually toggle the relay as needed.

Another version of the project uses the following hardware:

+	[Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500](https://www.adafruit.com/products/3010)
+	[Adafruit Adalogger FeatherWing Real time clock + SD card](https://www.adafruit.com/products/2922)
+	[Adafruit Power Relay FeatherWing](https://www.adafruit.com/products/3191)
+	[Feather Header Kit - 12-pin and 16-pin Female Header Set](https://www.adafruit.com/products/2886)
+	[Feather Stacking Headers - 12-pin and 16-pin female headers](https://www.adafruit.com/products/2830)
+	[CR1220 3V Lithium Coin Cell Battery](https://www.adafruit.com/products/380)
+	[5V 2.4A Switching Power Supply with 20AWG MicroUSB Cable](https://www.adafruit.com/products/1995)

With this version, I added a Featherwing relay board, so the Feather stack controls everything and the relay is wired directly to the power outlets. I'll add an image of the complete configuration once I complete that version of the project.

## Configuring the Arduino Sketch

The Arduino sketch exposes several configuration settings you can adjust to control the behavior of the application.

### Analog Pins

The Arduino sketch turns the relay on/off using one of the Arduino device's Analog Output pins. Set the sketch's `outputPin` constant to the Analog pin designation for your hardware configuration. In the example below, the relay is connected to the Arduino device using Analog Pin 1 (`A1`):  

	//The Analog pin the relay is connected to. This means that the
	//relay is connected to Analog Pin 1 on the Arduino device.
	const int outputPin = A1;

The Arduino sketch uses an Analog input pin to read that the push button is pressed. Set the sketch's `buttonPin` constant to the Analog pin designation for your hardware configuration. In the example below, the button is connected to the Arduino device using Analog Pin 2 (`A2`):

	//The Analog pin the button is connected to. This means that the
	//button is connected to Analog Pin 2 on the Arduino device.
	const int buttonPin = A2;

### Time Zone

In order to be able to work with the correct time for your current location (the location where you have deployed the hardware), the sketch needs to know your current time zone. Set the sketch's `timeZone` constant with the appropriate time offset for your location. To make it easier for you, the project includes a `constants.h` file that lists standard abbreviations for many common time zone. In the following example, the time zone is set to US Eastern Time (UTC -5)

	//Set the time zone using the timezone defines listes above.
	//You'll have to switch the device from standard time to daylight time
	//during the summer if you live in an area that observes daylight time.
	const int timeZone = EST;

The project's `constants.h` file contains constants for many of the common US time zones and a couple of European ones as well: 

	//Time Zones
	//Use the following to add to this list https://www.timeanddate.com/time/zones/
	//This is by no means a complete list, add to this as needed for your timezone
	#define GMT  0  // GMT – Greenwich Mean Time / Coordinated Universal Time
	#define CET  1  // Central European Time
	#define EDT -4  // Eastern Daylight Time (USA)
	#define CDT -5  // Central Daylight Time (USA)
	#define EST -5  // Eastern Standard Time (USA)
	#define CT  -6  // Central Time (USA)
	#define MDT -6  // Mountain Daylight Time (USA)
	#define MST -7  // Mountain Standard Time (USA)
	#define PDT -7  // Pacific Daylight Time (USA)
	#define PST -8  // Pacific Standard Time (USA)

If your time zone isn't listed, you can add your time zone to the list, then populate the `timeZone` constant with two or three letter time zone abbreviation. You can also just assign the time zone offset value directly to the `timeZone` constant.  For example, if your time zone is **UTC +2**, just use the following:

	const int timeZone = 2;

### Slots Array

The Arduino sketch enables you to configure an almost unlimited number of on/off times for the relay. This is controlled through the `NUMSLOTS` and `slots` options. The `slots` array contains one or more rows of on/off values. For each row, the first array element represents the time (in 24 hour format) that the relay should be turned on. The second array element represents the time (in 24 hour format) that the relay should be turned off. 

The slots array values are represented in the following format: `{ onTime, offTime }`
	
To turn the relay on at 6:00 AM, and turn it off 8:00 AM, use:

	{ 600, 800 }

To turn the relay on at 5:00 PM, and turn it off at 11:00 PM, use:

	{ 1700, 2300 }

To turn the relay on at 5:30 AM, and turn the relay off at 7:00 AM, use:

	{ 530, 700 }

Here's an example of the complete `slots` array representing turning the relay on at 6:00 AM, off at 8:00 AM; on at 5:00 PM, off at 6:30 PM, and on at 7:00 PM and off at 11:00 PM:
 
	//Use the Slots array to define when the relay(s) go on and off	
	int slots[NUMSLOTS][2] = {
	  { 600, 800 },
  	  {1700, 1830},
      {1900, 2300}
	};

> **Note**: The array rows shouldn't overlap - your on/off pairs should define discrete time periods. The sketch will check to make sure that the ** time** is before the **off time** and that each element represents a valid time (in 24 hour format), but will not validate the data across rows. 

The rows don't have to be in chronological order, the sketch will operate just fine with any order of the rows; as long as the on time is before the off time and there is no overlap across the rows.

When you run the sketch, use the **Arduino IDE Serial Monitor** to make sure your `slots` values are correct. The sketch will validate the array, display an error message when there's a problem, and stop execution until you fix the error and try again. 

`NUMSLOTS` is a constant that tells the sketch how many time slots have been configured in the sketch. Arduino sketches can't easily determine the number of elements in an array, so we'll use this constant to do it for us. Any time you add or remove a row from the `slots` array, be sure to update this constant as well.

	//Populate the following variable with the number of rows in the slots array
	//I'm doing this way because Arduino doesn't have an easy way to determine
	//the size of an array at runtime, especially multidimensional arrays.
	#define NUMSLOTS 3
 
## Additional Resources

+ [Adafruit Feather MO tutorials](https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/overview)
+ [Adafruit Adalogger FeatherWing Real time clock + SD card Tutorial](https://learn.adafruit.com/adafruit-adalogger-featherwing)
+ [RTC Library](https://github.com/adafruit/RTClib)
+ [Adafruit Power Relay FeatherWing](https://learn.adafruit.com/adafruit-power-relay-featherwing)

## Update History

Nothing yet.

***
By [John M. Wargo](http://www.johnwargo.com) - If you find this code useful, and feel like thanking me for providing it, please consider making a purchase from [my Amazon Wish List](https://amzn.com/w/1WI6AAUKPT5P9). You can find information on many different topics on my [personal blog](http://www.johnwargo.com). Learn about all of my publications at [John Wargo Books](http://www.johnwargobooks.com).