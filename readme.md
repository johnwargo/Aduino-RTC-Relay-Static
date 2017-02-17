# Arduino RTC Relay

## Introduction

An Arduino project for turning on/off a relay at a scheduled time. The project uses a real-time clock to ensure accuracy.  

This is a work in progress, so stay tuned.

## Hardware

+ [Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500](https://www.adafruit.com/products/3010)
+ [Adafruit Adalogger FeatherWing Real time clock + SD card](https://www.adafruit.com/products/2922)
+ [5V 2.4A Switching Power Supply with 20AWG MicroUSB Cable](https://www.adafruit.com/products/1995)
+ [Feather Header Kit - 12-pin and 16-pin Female Header Set](https://www.adafruit.com/products/2886)
+ [CR1220 3V Lithium Coin Cell Battery](https://www.adafruit.com/products/380)

Flank pig tail hamburger porchetta turducken beef ribs fatback shankle chuck salami venison filet mignon jerky ribeye. Shoulder jerky tongue pancetta turkey flank ball tip capicola brisket strip steak. Filet mignon andouille burgdoggen bresaola shoulder pork chop pork. T-bone pork belly pastrami pork loin bresaola leberkas turducken turkey tongue beef ribs landjaeger tri-tip pig. Shankle boudin hamburger pork loin prosciutto, turducken biltong doner t-bone pork venison leberkas shoulder.

+ [Adafruit Power Relay FeatherWing](https://www.adafruit.com/products/3191)
+ [Feather Stacking Headers - 12-pin and 16-pin female headers](https://www.adafruit.com/products/2830)

Shank burgdoggen hamburger, frankfurter turducken drumstick flank leberkas salami pork loin beef beef ribs biltong cow. Ham hock bacon tenderloin shoulder biltong meatloaf porchetta corned beef tri-tip meatball spare ribs kielbasa frankfurter boudin. Pork hamburger strip steak, beef ribs meatball porchetta picanha ham hock frankfurter fatback rump ribeye ham sausage. Cupim andouille burgdoggen, pork ball tip short ribs filet mignon chuck swine beef ribs brisket rump jerky. Hamburger filet mignon tenderloin flank.

+ [Powerswitch tail 2](https://www.adafruit.com/products/268)


Bacon ipsum dolor amet bacon shoulder frankfurter, venison boudin cupim filet mignon shank pork jerky spare ribs corned beef tail meatball ham hock. Kevin strip steak short ribs, beef burgdoggen prosciutto shoulder ground round chicken meatball pig salami andouille. Tenderloin shankle pig, tri-tip meatball venison shank beef ribs pancetta pastrami corned beef short ribs cow hamburger ribeye. Filet mignon leberkas meatloaf jowl turkey cupim, porchetta tongue brisket burgdoggen sausage picanha prosciutto corned beef hamburger. Beef pastrami landjaeger pork belly doner ham hock.

## Setting Up Your Development Environment

Download the time library from https://github.com/PaulStoffregen/Time

## Customizing the Code

///

### Configuring On/Off Times

Use the Slots array to define when and how the relay operates...

An example:

	int slots[][5] = {  
	  {SetTime, 600, Sunrise, 10, false },
	  {Sunset, 15, SetTime, 2300, false }
	};

Slots array values: `{OnTrigger, onValue, OffTrigger, OffValue, doRandom };`, described in the following table.

There's no need to provide an array bounds for the first parameter. Compiler takes care of it.

**Slots Array Values**

| Value         | Description |
| ------------- | ------------- |
| **OnTrigger**     | One of the three triggers described in the following table: `SetTime`, `Sunrise`, and `Sunset`. |
| **OnValue**       | Numeric value associated with the selected trigger. If a set time (`SetTime`) is selected, `onValue` must contain a [24 hour time](http://militarytimechart.com/) value that specifies the time the application should turn the relay on. If a trigger of `Sunrise` or `Sunset` is set, then the value provider here is added to on time. See examples below for, well, examples.|
| **OffTrigger**    | One of the three triggers described in the following table: `SetTime`, `Sunrise`, and `Sunset`. |
| **OffValue**      | Numeric value associated with the selected trigger. If a set time (`SetTime`) is selected, `onValue` must contain a [24 hour time](http://militarytimechart.com/) value that specifies the time the application should turn the relay off. If a trigger of `Sunrise` or `Sunset` is set, then the value provider here is added to on time. See examples below for, well, examples.|
| **DoRandom**      | Boolean value (`true` or `false`) controls whether the relay is turned on and off at random times between on time and off time. |


**Triggers**

| Value         | Description  |
| ------------- | ------------ |
| **SetTime**       | Toggle the relay at a **specific time**; the time is specified in `OnValue` and `OffValue`.                                 |
| **Sunrise**       | Toggle the relay at **sunrise** for the current location, +/- an optional delta value provided in `OnValue` and `OffValue`. |
| **Sunset**        | Toggle the relay at **sunset** for the current location, +/- an optional delta value provided in `OnValue` and `OffValue`.  |


An example:

	int slots[][5] = {  
	  {SetTime, 600, Sunrise, 10, false },
	  {Sunset, 15, SetTime, 2300, false }
	};


#### Examples

Turn the relay on at 6:00 AM, turn it off at 10 minutes after sunrise.

	{SetTime, 600, Sunrise, 10, false },

Turn the relay on 10 minutes after sunset, turn it off at 11:00 PM.

	Sunset, 10, SetTime, 2300, false }

At 5:30 AM, start turning the relay on and off randomly until 8:00 AM.

	Settime, 530, SetTime, 800, true }

At 15 minutes before sunset, start turning the relay on and off randomly until 11:00 PM.

	{Sunset, -15, SetTime, 2300, true }

## Additional Resources

+ [Adafruit Feather MO tutorials](https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/overview)
+ [Adafruit Adalogger FeatherWing Real time clock + SD card Tutorial](https://learn.adafruit.com/adafruit-adalogger-featherwing)
+ [RTC Library](https://github.com/adafruit/RTClib)
+ [Adafruit Power Relay FeatherWing](https://learn.adafruit.com/adafruit-power-relay-featherwing)

## Update History

Nothing yet.

***
By [John M. Wargo](http://www.johnwargo.com) - If you find this code useful, and feel like thanking me for providing it, please consider making a purchase from [my Amazon Wish List](https://amzn.com/w/1WI6AAUKPT5P9). You can find information on many different topics on my [personal blog](http://www.johnwargo.com). Learn about all of my publications at [John Wargo Books](http://www.johnwargobooks.com). 