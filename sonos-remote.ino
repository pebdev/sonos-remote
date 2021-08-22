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
/* Pinout */
#define PIN_IN_KY40_CLK                     (4)
#define PIN_IN_KY40_DT                      (2)
#define PIN_IN_KY40_BUTTON                  (15)
#define PIN_IN_PLAY_BUTTON                  (13)

/* Logical */
#define BUTTON_RELEASED                     (1)
#define BUTTON_PUSHED                       (0)

/* Sonos settings */
#define STEP_VOLUME                         (2)

/* Debug mode : if uncommented, do not send command, just display information in the console */
//#define _DEBUG_MODE

/* Timers */
#define TIMER_IS_STOPED                     (0)
#define TIMER_TRANSMIT_DATA_MS              (200)
#define TIMER_REFRESH_SONOS_STATUS_MS       (60000)



/* P R O T O T Y P E S ***************************************************************************/
void ethConnectError();


/* S T R U C T U R E S ***************************************************************************/
typedef struct {
  int16_t pinButtonLast = BUTTON_RELEASED;
  int16_t pinCLKLast    = LOW;
  int16_t diff          = 0;
} ky40Data;


/* D E C L A R A T I O N S ***********************************************************************/
/* Wifi device */
WiFiClient client;

/* IO device */
int8_t pinPlayButtonLast = BUTTON_RELEASED;

/* Sonos device */
int16_t currentVolume = 0;
unsigned long timerToTransmitData_ms = TIMER_IS_STOPED;
unsigned long timerToRefreshSonos_ms = millis();
SonosUPnP sonosDevice = SonosUPnP(client, ethConnectError);
IPAddress sonosDeviceIP(sonos_ip_addr[0], sonos_ip_addr[1], sonos_ip_addr[2], sonos_ip_addr[3]);

/* Rotary internal data */
volatile ky40Data ky40;



/* E T H  F U N C T I O N S **********************************************************************/
void ethConnectError (void)
{
  Serial.println ("ERROR : wifi died.");
}



/* K Y 4 0  F U N C T I O N S ********************************************************************/
int32_t ky40_getRotaryDiff (void)
{
  int32_t retval = ky40.diff;
  ky40.diff = 0;
  return retval;
}



/* S O N O S *************************************************************************************/
void update_mute (void)
{
  int8_t muteButtonState = digitalRead(PIN_IN_KY40_BUTTON);
  
  if (muteButtonState == BUTTON_PUSHED)
  {
    /* Anti rebound check */
    if (ky40.pinButtonLast == BUTTON_RELEASED)
    {
      #ifndef _DEBUG_MODE
        sonosDevice.setMute (sonosDeviceIP, !sonosDevice.getMute (sonosDeviceIP));
      #endif
      
      Serial.println ("sonosDevice.setMute");
    }
  }

  /* Update button status for anti rebound */
  ky40.pinButtonLast = muteButtonState;
}

/*-----------------------------------------------------------------------------------------------*/
void update_volume (void)
{
  int16_t rotaryDiff = ky40_getRotaryDiff();
  
  /* Update volume according rotary position */ 
  if (rotaryDiff != 0)
  {
    /* Set new volume level */
    currentVolume += (rotaryDiff*STEP_VOLUME);
    
    if (currentVolume < 0)
    {
      currentVolume = 0;
    }

    /* We start/reset the timer to refresh volume when user will have finish to change volume */
    timerToTransmitData_ms = millis();

    /* To avoid conflict with user actions, we resset the sonos status update timer */
    timerToRefreshSonos_ms = millis();
  }

  /* If time was started and reached, we send commnand */
  if ((timerToTransmitData_ms != TIMER_IS_STOPED) && ((millis()-timerToTransmitData_ms) > TIMER_TRANSMIT_DATA_MS))
  {
    /* We stop the timer */
    timerToTransmitData_ms = TIMER_IS_STOPED;
    
    #ifndef _DEBUG_MODE
      sonosDevice.setVolume(sonosDeviceIP, currentVolume);
    #endif
    
    Serial.print ("sonosDevice.setVolume = ");
    Serial.println ((int)(currentVolume));
  }

  /* Because volume can manage by an external remote, we must update volume localy when user don't use our remote */
  if ((millis()-timerToRefreshSonos_ms) > TIMER_REFRESH_SONOS_STATUS_MS)
  {
    /* We reset the timer */
    timerToRefreshSonos_ms = millis();

    currentVolume = sonosDevice.getVolume(sonosDeviceIP);

    Serial.print ("sonosDevice.currentVolume = ");
    Serial.println ((int)(currentVolume));
  }
}

/*-----------------------------------------------------------------------------------------------*/
void update_play (void)
{
  int8_t playButtonState = digitalRead(PIN_IN_PLAY_BUTTON);

  if (playButtonState == BUTTON_PUSHED)
  {
    /* Anti rebound check */
    if (pinPlayButtonLast == BUTTON_RELEASED)
    {
      #ifndef _DEBUG_MODE
        sonosDevice.togglePause(sonosDeviceIP);
      #endif
      
      Serial.println ("sonosDevice.togglePause");
    }
  }

  /* Update button status for anti rebound */
  pinPlayButtonLast = playButtonState;
}



/* I N T E R R U P T S ***************************************************************************/
void IRAM_ATTR interrupt_ky40 (void)
{
  int8_t pinCLK = digitalRead(PIN_IN_KY40_CLK);
  int8_t pinDT  = digitalRead(PIN_IN_KY40_DT);

  if (pinCLK != ky40.pinCLKLast)
  {    
    if (pinDT != pinCLK)
    {
      ky40.diff++;
    }
    else
    {
      ky40.diff--;
    }
  }
  
  ky40.pinCLKLast = pinCLK;
}


/* M A I N  F U N C T I O N S ********************************************************************/
void setup (void)
{
  /* Pin configuration */
  pinMode (PIN_IN_KY40_CLK,   INPUT);
  pinMode (PIN_IN_KY40_DT,    INPUT);
  pinMode (PIN_IN_KY40_BUTTON,INPUT_PULLUP);
  pinMode (PIN_IN_PLAY_BUTTON,INPUT_PULLUP);

  /* Debug configuration */
  Serial.begin (115200);

  /* Wifi configuration to send command to the sonos system */
  WiFi.begin (ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay (500);
    Serial.print (".");
  }
  Serial.println ("connected to WiFi");

  /* Refresh local data */
  currentVolume = sonosDevice.getVolume(sonosDeviceIP);

  /* Interrupt configuration to monitor rotary usage */
  attachInterrupt(digitalPinToInterrupt(PIN_IN_KY40_CLK), interrupt_ky40, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN_KY40_DT),  interrupt_ky40, CHANGE);
}

/*-----------------------------------------------------------------------------------------------*/
void loop (void)
{
  /* Update different features */
  update_mute ();
  update_volume ();
  update_play ();

  delay(100);
}
