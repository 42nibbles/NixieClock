# NixieClock

Combining Nixie tube technology with IoT

* Providing vintage style Nixie tube clock suitability for daily use.
* Building of a basis for testing and demonstrating IoT stuff using
  the [ESP8266](
  http://www.google.de/search?hl=de&source=hp&biw=&bih=&q=Mikrocontroller.net+ESP8266
  "Mikrocontroller.net ESP8266 - Google-Suche") in conjunction with
  the Arduino library.
* Open documentation allows designing of individual boxes, using of
  miscellaneous Nixie tubes, software and so forth within your own
  handicraft work.
  
## Firmware

The _firmware_ can be found in the Arduino folder. Compilation was
tested under Arduino.cc. You might use the libraries found in the
library folder. At least you will need the _NixieClock_
software. _DS1307RTC, Time-master_, and _Timezone-master_ are
available in their unimproved versions from Arduino and are assumed to
be yet part of your Arduino standard software installation. When using
the Arduino build system you can use any version you like without
problems.

When using Arduino Eclipse plugins you will get into trouble caused by
the redirection of the standard c-header file _Time.h_ that was done
in _Time-master._ This strange hijacking of the original _time.h_ is
responsible for wrecking the build process of the ESP8266 core
_arduino.ar._ So Eclipse users will likely use my modified versions of
_DS1307RTC, Time-master_, and _Timezone-master_ where I removed the
annoying redirection and detaching of standard headers.

Furthermore there are the two Arduino sketch folders _Nixie4_ and
_Nixie6._ _Nixie4_ is the firmware used by our 4 tube prototype;
_Nixie6_ is used by our 6 tube prototype.

### Prototype with 4 ИН-1 Nixie tubes

![Prototype ИН-1 Nixie clock](./pic/P1010273_s.jpg "4x ИН-1 tube
type")

This clock is an implementation with 4 ИН-1 Nixie tubes.

### Prototype with 6 ИН-12А Nixie tubes

![Prototype ИН-12А Nixie clock](./pic/P1010278_s.jpg "6x ИН-12А tube
type")

## Hardware Description

![Beware of electric shock!](./pic/High_Voltage.jpg "Beware of
electric shock!")

**Beware of electric shock!**

**Beware of electrical safety hazards!**

**The Nixie tubes are fired at approx 200 Volt! There is definitely a
danger of hazardous voltage given! Never think of working on the Nixie
hardware when you are not educated in doing work with hazardous
voltages!**

The hardware description is given in EAGLE format and a PDF-file.
