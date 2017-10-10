#include "Arduino.h"

//RTC relevant stuff
#include <DS1307RTC.h>
#include <time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <Wire.h>

//(ESP) WiFi include stuff
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>

//Local class includes
#include <NixieClock.h>

//Local function prototypes
static time_t getNixieTime(void);
static time_t getNtpTime(void);
static time_t getRtcTime(void);
static bool wifi_setup(void);
static void print_wifi_mac_address(void);
static void print_wifi_status(void);

//WiFi stuff
static constexpr char _SSID[] = "your_ssid";  //network SSID (name)
static constexpr char _PASS[] = "your_password";  //network password
static WiFiUDP _udp;
static const unsigned int UDP_LOCAL_PORT = 2390; //local port to listen for UDP packets

//NTP stuff
//static constexpr char NTP_SERVER_NAME_S[] = "ntp2.ptb.de";
static constexpr char NTP_SERVER_NAME_S[] = "ntp1.t-online.de";

//We will use Central European Time (Frankfurt, Paris) as local time zone
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; //Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   //Central European Standard Time
Timezone CE(CEST, CET);

//Nixie hardware stuff
static NixieClock _clock;
//TODO: Adjust twi settings to your mcu module here
//ESP8266-07 has swapped GPIO4/GPIO5 pins!
static constexpr uint8_t MY_SDA = 5;
static constexpr uint8_t MY_SCL = 4;

//The setup function is called once at startup of the sketch
void setup()
{
  //Serial setup
  Serial.begin(9600);
  while (!Serial)
    ; //wait for serial
  Serial.println("");
  Serial.println("Nixie Clock - Ver. 1.0 on Generic ESP8266 module");

  //TWI setup
  Wire.begin(MY_SDA,MY_SCL);
  Serial.print("MCU I2C pins configured to SDA on "); Serial.print(MY_SDA);
  Serial.print(", SCL on "); Serial.println(MY_SCL);

  //TODO: Adjust settings to your port expander hardware here
  //Pay attention to the usage of NixieClock::init().
  const uint8_t PCF8574_TWI_TYPE[3] = {0x27, 0x26, 0x25};
  //const uint8_t PCF8574A_TWI_TYPE[3] = {0x3f, 0x3e, 0x3d};
  _clock.begin(PCF8574_TWI_TYPE, true, true);
  //**** STOP ****

  // Try to start wifi interface
  if ( (wifi_setup() == true) && (_udp.begin(UDP_LOCAL_PORT) == true) ) {
    Serial.print("Created UDP local port ");
    Serial.println(_udp.localPort());
  }

  // Try to start time syncing mechanisms
  Serial.println("Using localtime zone 'Berlin', CEST, CET");
  setSyncProvider( &getNixieTime ); // Arduino lib will care
}

// The loop function is called in an endless loop
void loop()
{
  static time_t old_time_utc = now();

  //Trigger this block each second
  if ( old_time_utc != now() ) {
    old_time_utc = now();
    //Local time output
    TimeChangeRule *tcr = nullptr;
    time_t local_time = CE.toLocal(old_time_utc, &tcr);
    int local_sec = second(local_time);
    if (local_sec >= 0 && local_sec < 56) {
      _clock.update(NixieClock::HMS, local_time);
    }
    else {
      _clock.update(NixieClock::dmy, local_time);
    }
  }
}

// **********************************************
// Local function declarations
// **********************************************
static time_t getNixieTime(void)
{
  static bool is_online;
  time_t utc_time = 0;

  if ( is_online ) {
    utc_time = getNtpTime();
    if ( utc_time != 0 )
      RTC.set(utc_time);
    else
      utc_time = getRtcTime();
  }
  else {
  //First round: Clock start-up
    //Accessing onboard real time clock must always be possible.
    utc_time = getRtcTime();
    if ( utc_time != 0 ) {
      is_online = true;
      setTime(utc_time);
    }
    else {
      // Bus error: Awaiting the watchdog timer biting us soon :-)
      while (1)
        ;
      // Never reached...
    }
    //Try to get ntp time over wifi
    utc_time = getNtpTime();
    if ( utc_time != 0 ) {
      setTime(utc_time);
      RTC.set(utc_time);
    }
  }
  return utc_time;
}

static time_t getNtpTime(void)
{
  //TODO: This is ugly and causes delays greater than necessary.
  if ( _udp.localPort() == 0 )
    return 0;

  while (_udp.parsePacket())
    ; //discard any previously received packets

  // Request ntp server ip address
  IPAddress time_server_ip;
  WiFi.hostByName(NTP_SERVER_NAME_S, time_server_ip);
  Serial.print("Requesting network time from ");
  Serial.print(NTP_SERVER_NAME_S);
  Serial.print(" (");
  Serial.print(time_server_ip);
  Serial.println(")");

  // Doing the NTP request
  const int NTP_PACKET_SIZE = 48;
  byte packet_buffer[NTP_PACKET_SIZE];
  memset(packet_buffer, 0, NTP_PACKET_SIZE);
  packet_buffer[0] = 0b11100011;  // LI, Version, Mode
  packet_buffer[1] = 0;     // Stratum, or type of clock
  packet_buffer[2] = 6;     // Polling Interval
  packet_buffer[3] = 0xec;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packet_buffer[12] = 49;
  packet_buffer[13] = 0x4e;
  packet_buffer[14] = 49;
  packet_buffer[15] = 52;
  // timestamp request
  _udp.beginPacket(time_server_ip, 123); // NTP server port is 123
  _udp.write(packet_buffer, NTP_PACKET_SIZE);
  _udp.endPacket();

  // Handling of NTP reply
  Serial.print("NTP reply");
  delay(1000);
  int rply_size = _udp.parsePacket();
  if ( rply_size ) {
    Serial.println("\t\t\t[OK]");
    _udp.read(packet_buffer, NTP_PACKET_SIZE);
    //the timestamp starts at byte 40 and is two words long
    unsigned long hword = word(packet_buffer[40], packet_buffer[41]);
    unsigned long lword = word(packet_buffer[42], packet_buffer[43]);
    unsigned long secs_since_1900 = hword << 16 | lword;
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800. So
    // we have to subtract seventy years.
    time_t unix_time = secs_since_1900 - 2208988800UL;
    return unix_time;
  }
  else {
    Serial.println("\t\t\t[FAILED]");
    return 0;
  }
}

static time_t getRtcTime(void)
{
  static bool rtc_connected;
  time_t utc_time = RTC.get();

  if ( rtc_connected && (utc_time != 0) ) {
    // Everything is online and fine
    return utc_time;
  }
  else {
    // First run should print some information.
    if ( utc_time != 0 ) {
      // It works.  Print boot messages.
      rtc_connected = true;
        //
      Serial.println("DS1307 real time clock found on I2C");
      Serial.print("UTC date is: ");
      Serial.print(day(utc_time));
      Serial.print(".");
      Serial.print(month(utc_time));
      Serial.print(".");
      Serial.println(year(utc_time));
      Serial.print("UTC time is: ");
      Serial.print(hour(utc_time));
      Serial.print(":");
      Serial.print(minute(utc_time));
      Serial.print(":");
      Serial.println(second(utc_time));
    }
    else {
      if ( RTC.chipPresent() ) {
        // A chip is present, but it was not initialized.
        rtc_connected = true;
        Serial.println("Found unitialized DS1307 real time clock");
        Serial.println("Setting rtc time temporary to 1 January 1970");
        RTC.set(0);
        utc_time = 1;
      }
      else {
        rtc_connected = false;
        Serial.println("");
        Serial.println("Severe hardware failure");
        Serial.println("Accessing DS1307 real time clock failed");
        Serial.println("");
      }
    }
  }
  return utc_time;
}

static void print_wifi_mac_address(void)
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[0], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.println(mac[5], HEX);
}

static void print_wifi_status(void)
{
  Serial.print("WiFi status: ");
  switch (WiFi.status()) {
  case WL_CONNECTED:
    Serial.println("WL_CONNECTED");
    break;
  case WL_NO_SHIELD:
    Serial.println("WL_NO_SHIELD");
    break;
  case WL_IDLE_STATUS:
    Serial.println("WL_IDLE_STATUS");
    break;
  case WL_NO_SSID_AVAIL:
    Serial.println("WL_NO_SSID_AVAIL");
    break;
  case WL_SCAN_COMPLETED:
    Serial.println("WL_SCAN_COMPLETED");
    break;
  case WL_CONNECT_FAILED:
    Serial.println("WL_CONNECT_FAILED");
    break;
  case WL_CONNECTION_LOST:
    Serial.println("WL_CONNECTION_LOST");
    break;
  case WL_DISCONNECTED:
    Serial.println("WL_DISCONNECTED");
    break;
  default:
    Serial.println("Unknown");
    break;
  }
}

static bool wifi_setup(void)
{
  const int WIFI_TIMEOUT_SECS = 16;
  bool wifi_available = true;

  //Connecting to WiFi
  Serial.print("WiFi connecting to SSID: ");
  Serial.println(_SSID);
  print_wifi_mac_address();
  //
  WiFi.disconnect();
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(_SSID, _PASS);
  time_t connection_moment = now();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if ( (connection_moment + WIFI_TIMEOUT_SECS) <= now() ) {
      // Timeout reached
      wifi_available = false;
      Serial.println("\t\t[FAILED]");
      break;
    }
  }
  if ( wifi_available ) {
    // Print some boot messages
    Serial.println("\t\t\t[OK]");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    print_wifi_status();
  }
  return wifi_available;
}

