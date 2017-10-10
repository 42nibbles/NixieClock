/*
    NixieClock.hpp
 
    Created on: 24.02.2017
    Copyright 2017 Michael Hoffmann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBRARIES_NIXIECLOCK_NIXIECLOCK_H_
#define LIBRARIES_NIXIECLOCK_NIXIECLOCK_H_

#include <cstdbool>
#include <cstdint>
#include <time.h>

/**
   The NixieClock class offers software access to the display part of
   nixie tube clocks from the <a
   href="http://42nibbles.de/">42nibbles</a> project.  Both 4 and 6
   tube types are supported as well.  The power off feature of the
   enhanced clock and the blinking dot feature are supported, too.
*/
class NixieClock {
public:
	/**
	   \brief Output format enum

	   This enum defines the output format.  The qualifiers are
	   taken from the Unix command <a
	   href="http://www.google.de/search?q=manual+page+date(1)&btnG=Suche/">
	   date(1)</a> to benefit in future from its extensibility.
	   You have to use 6 digit qualifiers for 6 tube displays and
	   4 digit qualifiers for 4 tube displays.

	   \sa update
	 */
	enum format_t {
		HMS, ///< 6 digits: HH:MM:SS (Hour, Minute, Second)
		dmy, ///< 6 digits: DD:MM:YY (Day, Month, Year)
		HM, ///< 4 digits: HH:MM (Hour, Minute)
		dm, ///< 4 digits: DD:MM (Day, Month)
		Y ///< 4 digits: YYYY (Year)
	};

	//virtual ~NixieClock(); //No destructor will ever get called
	/**
	   \brief Initialize library instance
	   @param twi_address  A three element array for storing the
	   I2C bus addresses of the three port expander ICs. Index
	   0 contains the address of the highest digit I2C bus
	   expander.
	   @param has_power_control  Set this 'true' for all newer
	   hardware versions.
	   @param has_dotcontrol  Set this 'true' for all newer
	   hardware versions.

	   This has to be called for making hardware initialization
	   before doing anything with the display.  Normally, you will
	   have to set twi_address {0x27, 0x26, 0x25} when using
	   <a href="http://search.datasheetcatalog.net/key/PCF8574">
	   PCF8574</a> port expanders and {0x3f, 0x3e, 0x3d} when
	   using <a
	   href="http://search.datasheetcatalog.net/key/PCF8574A">
	   PCF8574A</a> port expanders.  The values of
	   _has_power_control_ and _has_dotcontrol_ are set true by
	   default, because except some old prototypes all newer
	   boards are able to switch on/off display power by using
	   power_on() and power_off(), just as using the blinking
	   colon feature when displaying time or the dotted date
	   representation.

	   If the hardware has power control the nixie tubes will get
	   powered on by default.

	   \sa update
	 */
	void begin(const uint8_t twi_address[3], bool has_power_control=true,
		   bool has_dotcontrol=true);
	/**
	   \brief Turns power on.

	   Turns on the nixie tubes when power control is
	   available-otherwise will be ignored.
	 */
	void power_on();
	/**
	   \brief Turns power off.

	   Turns off the nixie tubes when power control is
	   available-otherwise will be ignored.
	 */
	void power_off();
	/**
	   \brief Updates the display
	   @param format The desired output format.
	   @param time The Unix time as it will be displayed,
	   i.e. not concerning local time zones.

	   This function updates the display.  The output
	   representation is defined by _format_. You have to respect
	   the number of nixie tubes available in your clock.  At
	   present _format_ must be NixieClock::HMS or NixieClock::deg
	   for 6 tube displays and NixieClock::HM, NixieClock::de or
	   NixieClock::g for 4 tube displays.

	   The parameter _time_ is the time to be displayed in Unix
	   format.  Localization is not concerned, so you have to use
	   your own functions for time zone calculations. 
\code{.cpp}
#include <NixieClock.h>
static NixieClock _clock;

void setup(void)
{
  // Initialization of a default clock hardware using PCF8574.
  const uint8_t PCF8574_TWI_TYPE[3] = { 0x27, 0x26, 0x25 };
  _clock.init( PCF8574_TWI_TYPE, true, true );
}

void loop(void)
{
  // Assume 4 tube display clock.  Dot logic is done automatically.
  static time_t local_time = get_local_time_func();
  if ( local_time != get_local_time_func() )
  {
    // Enter this section at the begin of each new second.
    local_time = func_get_local_time();
    if ( ( to_second(local_time) >= 0 ) && ( to_second(local_time) < 56 ) )
    {
      // Show time in range from 0 to 55 seconds.
      _clock.update( NixieClock::HM, local_time );
    }
    else
    {
      // Show date in range from 56 to 59 seconds.
      _clock.update( NixieClock::dm, local_time );
    }
  }
}
\endcode
	 */
	void update(enum format_t format, time_t time);
private:
	static constexpr unsigned char MASK_DOT = 0x80;
	static constexpr unsigned char MASK_COLON = 0xC0;
	static constexpr unsigned char HIGH_VOLTAGE_SWITCH = 13;

	uint8_t _twi_address[3] = {0,0,0};
	bool _has_power_control = false;
	bool _has_dotcontrol = false;
};

#endif /* LIBRARIES_NIXIECLOCK_NIXIECLOCK_H_ */
