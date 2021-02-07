/*************************************************************************************************/
/*  Project : Sonos remote                                                                       */
/*  Author  : PEB <pebdev@lavache.com>                                                           */
/*  Date    : 13.12.2020                                                                         */
/*************************************************************************************************/
/*  This program is free software: you can redistribute it and/or modify                         */
/*  it under the terms of the GNU General Public License as published by                         */
/*  the Free Software Foundation, either version 3 of the License, or                            */
/*  (at your option) any later version.                                                          */
/*                                                                                               */
/*  This program is distributed in the hope that it will be useful,                              */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of                               */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                                */
/*  GNU General Public License for more details.                                                 */
/*                                                                                               */
/*  You should have received a copy of the GNU General Public License                            */
/*  along with this program.  If not, see <https://www.gnu.org/licenses/>.                       */
/*************************************************************************************************/


/* I N C L U D E S *******************************************************************************/
#include <SonosUPnP.h>
#include <MicroXPath_P.h>
#include <WiFi.h>

/* File to create next to this ino file with this content :
 *  const char* ssid     = "your_wifi_SSID";
 *  const char* password = "wifi_key";
 *  const char sonos_ip_addr[] = { 192, 168, 0, 10};
 */
#include "wifi_key.h"


/* D E F I N E S *********************************************************************************/
#define PIN_IN_KY40_CLK                     (4)
#define PIN_IN_KY40_DT                      (2)
#define PIN_IN_KY40_BUTTON                  (15)

#define STEP_VOLUME                         (2)

#define LATEST_REFRESH_SONOS_TIMEOUT_MS     (60000)
#define TIMER_DATA_TRANSMIT_TIMEOUT_MS      (200)

//#define _DEBUG_MODE


/* P R O T O T Y P E S ***************************************************************************/
void ethConnectError();


/* S T R U C T U R E S ***************************************************************************/
typedef struct {
  bool antiRebound    = false;
  bool newPosition    = false;
  int16_t pinCLKLast  = LOW;
  int16_t diff        = 0;
  int16_t pos         = 0;
} ky40Data;


/* D E C L A R A T I O N S ***********************************************************************/
WiFiClient client;
SonosUPnP sonosDevice = SonosUPnP(client, ethConnectError);
IPAddress sonosDeviceIP(sonos_ip_addr[0], sonos_ip_addr[1], sonos_ip_addr[2], sonos_ip_addr[3]);
ky40Data ky40;
int16_t currentVolume = 0;
uint32_t elapsedTimeLatestRefreshSonos_ms = LATEST_REFRESH_SONOS_TIMEOUT_MS;
unsigned long timerDataTransmit_ms = 0;
bool updateMute = false;
bool updateVolume = false;


/* E T H  F U N C T I O N S **********************************************************************/
void ethConnectError (void)
{
  Serial.println ("ERROR : wifi died.");
}


/* K Y 4 0  F U N C T I O N S ********************************************************************/
void ky40_refresh (void)
{
  int16_t pinCLK = digitalRead(PIN_IN_KY40_CLK);
  
  if (pinCLK != ky40.pinCLKLast)
  {
    ky40.newPosition = true;
    
    if (digitalRead(PIN_IN_KY40_DT) != pinCLK)
    {
      ky40.diff++;
      ky40.pos++;
    }
    else
    {
      ky40.diff--;
      ky40.pos--;

      if (ky40.pos < 0)
      {
        ky40.pos = 0;
      }
    }
  }    
  
  ky40.pinCLKLast = pinCLK;
}

/*-----------------------------------------------------------------------------------------------*/
bool ky40_getButtonStatus (void)
{
  return !digitalRead(PIN_IN_KY40_BUTTON);
}

/*-----------------------------------------------------------------------------------------------*/
int32_t ky40_getRotaryDiff (void)
{
  int32_t retval = ky40.diff;
  ky40.diff = 0;
  return retval;
}

/*-----------------------------------------------------------------------------------------------*/
int32_t ky40_getRotaryPosition (void)
{
  return ky40.pos;
}

/*-----------------------------------------------------------------------------------------------*/
bool ky40_rotaryPositionChanged (void)
{
  bool retval = ky40.newPosition;
  ky40.newPosition = false;
  return retval;
}


/* M A I N  F U N C T I O N S ********************************************************************/
void setup (void)
{
  pinMode (PIN_IN_KY40_CLK,   INPUT);
  pinMode (PIN_IN_KY40_DT,    INPUT);
  pinMode (PIN_IN_KY40_BUTTON,INPUT_PULLUP);

  Serial.begin (115200);
  WiFi.begin (ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay (500);
    Serial.print (".");
  }
  Serial.println ("connected to WiFi");
}

/*-----------------------------------------------------------------------------------------------*/
void loop (void)
{
  int16_t delay_ms = 100;
  
  // Not used for the moment
  //sonosDevice.play (sonosDeviceIP);
  //sonosDevice.pause (sonosDeviceIP);
  //sonosDevice.stop (sonosDeviceIP);
  
  /* Refresh KY40 rotary data */
  ky40_refresh ();

  /* Mute button */
  if (ky40_getButtonStatus() == HIGH)
  {
    if (ky40.antiRebound == false)
    {
      /* If timer is stoped (0), start it to send data in Xms */
      if (timerDataTransmit_ms == 0)
        timerDataTransmit_ms = millis();

      /* Set flag to send data */
      updateMute = true;
      
      ky40.antiRebound = true;
    }
  }
  else
  {
    ky40.antiRebound = false;
  }

  /* Update volume according rotary position */
  if (ky40_rotaryPositionChanged() == true)
  {
    /* If timer is stoped (0), start it to send data in Xms */
    if (timerDataTransmit_ms == 0)
      timerDataTransmit_ms = millis();

    /* Set flag to send data */
    updateVolume = true;
    
    /* Set new volume level */
    currentVolume += (ky40_getRotaryDiff()*STEP_VOLUME);

    if (currentVolume < 0)
    {
      currentVolume = 0;
    }
  }

  /* No user activity, reduce Arduino activity */
  if (timerDataTransmit_ms == 0)
  {
     /* Refresh local data every Xms */
    if (elapsedTimeLatestRefreshSonos_ms >= LATEST_REFRESH_SONOS_TIMEOUT_MS)
    {
      /* Reset counter and read current volume level */
      elapsedTimeLatestRefreshSonos_ms = 0;
      currentVolume = sonosDevice.getVolume(sonosDeviceIP);
    }
    else
    {
      elapsedTimeLatestRefreshSonos_ms+=delay_ms;
    }
    
    delay(delay_ms);
  }
  /* An update was asked by user, timer to do it was reached so do it */
  else if (timerDataTransmit_ms > 0)
  {
    unsigned long ElapsedTime = millis() - timerDataTransmit_ms;

    /* Because update over TCP takes time, we do it every Xms, without blocking action for user experience */
    if (ElapsedTime >= TIMER_DATA_TRANSMIT_TIMEOUT_MS)
    {
      /* Stop the timer */
      timerDataTransmit_ms = 0;
      
      if (updateMute == true)
      {
        updateMute = false;
        
        #ifndef _DEBUG_MODE
          sonosDevice.setMute (sonosDeviceIP, !sonosDevice.getMute (sonosDeviceIP));
        #else
          Serial.println ("sonosDevice.setMute");
        #endif
      }
  
      if (updateVolume == true)
      {      
        updateVolume = false;
        
        #ifndef _DEBUG_MODE
          sonosDevice.setVolume(sonosDeviceIP, currentVolume);
        #else
          Serial.print ("sonosDevice.setVolume = ");
          Serial.println ((int)(currentVolume));
        #endif
      }
    }
  }
}
