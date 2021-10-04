// HAKR_WATCH
// V. 1.4

#include <Arduino.h>
#include <DNSServer.h>  
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <M5StickC.h>
#include <NTPClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>

#include "WORLD_IR_CODES.h"
#include "sans.h"



// Wifi setup
WiFiManager wifiManager;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
// Variables to save date and time
String formattedDate;

String dayStamp;
String timeStamp;

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Menu struct and cursor
struct MENU {
  char name[15];
  int command;
};
int cursor = 0;
int rotation = 1;
bool rstOverride = false;

/// SWITCHER ///
// Proc codes
// 0 - Clock
// 1 - Main Menu
// 2 - Settings Menu
// 3 - Wifi Clock set
// 4 - Dimmer Time adjustment
// 5 - TV B-GONE
// 6 - MEGALOVAINIA
// 7 - Battery info
// 8 - screen rotation
// 9 - timezone menu
bool isSwitching = true;
int current_proc = 0;

void switcher_button_proc() {
  if (rstOverride == false) {
    if (digitalRead(M5_BUTTON_RST) == LOW) {
      isSwitching = true;
      current_proc = 1;
    }
  }
}

/// SCREEN DIMMING ///
bool screen_dim_dimmed = false;
int screen_dim_time = 30;
int screen_dim_current = 0;

void screen_dim_proc() {
  M5.Rtc.GetBm8563Time();
  // if one of the buttons is pressed, take the current time and add screen_dim_time on to it and roll over when necessary
  if (digitalRead(M5_BUTTON_RST) == LOW || digitalRead(M5_BUTTON_HOME) == LOW) {
    if (screen_dim_dimmed) {
      screen_dim_dimmed = false;
      M5.Axp.ScreenBreath(11);
    }
    int newtime = M5.Rtc.Second + screen_dim_time;

    if (newtime >= 60) {
      newtime = newtime - 60;
    }
    screen_dim_current = newtime;
  }

  // Run the check
  // time up to 2 seconds after(for long pause operations
  if (screen_dim_dimmed == false) {
    if (M5.Rtc.Second == screen_dim_current || (M5.Rtc.Second + 1) == screen_dim_current || (M5.Rtc.Second + 2) == screen_dim_current) {
      M5.Axp.ScreenBreath(7);
      screen_dim_dimmed = true;
    }
  }
}

/// MAIN MENU ///

MENU mmenu[] = {
  { "clock", 0},
  { "TV B-GONE", 5},
  { "Megalovainia", 6},
  { "settings", 2},
};

void mmenu_drawmenu() {
  // List items
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(mmenu) / sizeof(MENU) ) ; i++ ) {
    M5.Lcd.print((cursor == i) ? ">" : " ");
    M5.Lcd.println(mmenu[i].name);
  }
}

void mmenu_setup() {
  M5.Lcd.setRotation(0);

  cursor = 0;
  rstOverride = true;
  mmenu_drawmenu();
  delay(250); // Prevent switching after menu loads up
}

void mmenu_loop() {
  // Switch in menu
  if (digitalRead(M5_BUTTON_RST) == LOW) {
    cursor++;
    cursor = cursor % ( sizeof(mmenu) / sizeof(MENU) );
    mmenu_drawmenu();
    delay(250);
  }
  // Click
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    // Unload menu
    rstOverride = false;
    isSwitching = true;
    current_proc = mmenu[cursor].command;
  }
}

/// SETTINGS MENU ///

MENU smenu[] = {
  { "set clock", 3},
  { "set dim time", 4},
  { "battery info", 7},
  { "rotation", 8},
  { "back", 1},
};

void smenu_drawmenu() {
  // List items
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(smenu) / sizeof(MENU) ) ; i++ ) {
    M5.Lcd.print((cursor == i) ? ">" : " ");
    M5.Lcd.println(smenu[i].name);
  }
}

void smenu_setup() {
  M5.Lcd.setRotation(0);

  cursor = 0;
  rstOverride = true;
  smenu_drawmenu();
  delay(250); // Prevent switching after menu loads up
}

void smenu_loop() {
  // Switch in menu
  if (digitalRead(M5_BUTTON_RST) == LOW) {
    cursor++;
    cursor = cursor % ( sizeof(smenu) / sizeof(MENU) );
    smenu_drawmenu();
    delay(250);
  }
  // Click
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    // Unload menu
    rstOverride = false;
    isSwitching = true;
    current_proc = smenu[cursor].command;
  }
}

/// Dimmer MENU ///

MENU dmenu[] = {
  { "30 seconds", 30},
  { "25 seconds", 25},
  { "20 seconds", 20},
  { "15 seconds", 15},
  { "10 seconds", 10},
  { "5 seconds", 5},
  { "back", screen_dim_time},
};

void dmenu_drawmenu() {
  // List items
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(dmenu) / sizeof(MENU) ) ; i++ ) {
    M5.Lcd.print((cursor == i) ? ">" : " ");
    M5.Lcd.println(dmenu[i].name);
  }
}

void dmenu_setup() {
  M5.Lcd.setRotation(0);

  cursor = 0;
  rstOverride = true;
  dmenu_drawmenu();
  delay(250); // Prevent switching after menu loads up
}

void dmenu_loop() {
  // Switch in menu
  if (digitalRead(M5_BUTTON_RST) == LOW) {
    cursor++;
    cursor = cursor % ( sizeof(dmenu) / sizeof(MENU) );
    dmenu_drawmenu();
    delay(250);
  }
  // Click
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    // Unload menu
    rstOverride = false;
    isSwitching = true;
    screen_dim_time = dmenu[cursor].command;
    current_proc = 2;
  }
}

/// Rotation MENU ///

MENU rmenu[] = {
  { "Right", 1},
  { "Left", 3},
  { "back", rotation},
};

void rmenu_drawmenu() {
  // List items
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 8, 1);
  for ( int i = 0 ; i < ( sizeof(rmenu) / sizeof(MENU) ) ; i++ ) {
    M5.Lcd.print((cursor == i) ? ">" : " ");
    M5.Lcd.println(rmenu[i].name);
  }
}

void rmenu_setup() {
  M5.Lcd.setRotation(0);

  cursor = 0;
  rstOverride = true;
  rmenu_drawmenu();
  delay(250); // Prevent switching after menu loads up
}

void rmenu_loop() {
  // Switch in menu
  if (digitalRead(M5_BUTTON_RST) == LOW) {
    cursor++;
    cursor = cursor % ( sizeof(rmenu) / sizeof(MENU) );
    rmenu_drawmenu();
    delay(250);
  }
  // Click
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    // Unload menu
    rstOverride = false;
    isSwitching = true;
    rotation = rmenu[cursor].command;
    // Write to EEPROM
    EEPROM.write(0, rotation);
    EEPROM.commit();
    //
    current_proc = 2;
  }
}

/// Battery MENU ///


void battery_drawmenu(int battery, int b, int c) {
  // Clear screen
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 8, 1);

  // List percents
  M5.Lcd.print("Battery: ");
  M5.Lcd.print(battery);
  M5.Lcd.println("%");

  // Deltas
  M5.Lcd.print("DeltaB: ");
  M5.Lcd.println(b);

  M5.Lcd.print("DeltaC: ");
  M5.Lcd.println(c);

  // Exit info
  M5.Lcd.println("");
  M5.Lcd.println("Press any button to exit");
}

void battery_setup() {
  M5.Lcd.setRotation(0);
  rstOverride = false;

  // Get battery levels
  float c = M5.Axp.GetVapsData() * 1.4 / 1000;
  float b = M5.Axp.GetVbatData() * 1.1 / 1000;
  //  M5.Lcd.print(b);
  int battery = ((b - 3.0) / 1.2) * 100;

  battery_drawmenu(battery, b, c);
  delay(250); // Prevent switching after menu loads up
}


void battery_loop() {
  delay(300);

  // Get battery levels
  float c = M5.Axp.GetVapsData() * 1.4 / 1000;
  float b = M5.Axp.GetVbatData() * 1.1 / 1000;
  //  M5.Lcd.print(b);
  int battery = ((b - 3.0) / 1.2) * 100;

  battery_drawmenu(battery, b, c);
  // Exit button
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    // Unload menu
    rstOverride = false;
    isSwitching = true;
    current_proc = 1;
  }
}

/// MEGALOVAINIA ///
int const TEMPO = 1200;

int melody[] = {
  N_D3, N_D3, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_C3, N_C3, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_B2, N_B2, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_AS2, N_AS2, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_D3, N_D3, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_C3, N_C3, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_B2, N_B2, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_AS2, N_AS2, N_D4, N_A3, 0, N_GS3, N_G3, N_F3, N_D3, N_F3, N_G3, N_D4, N_D4, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_C4, N_C4, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_B3, N_B3, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_AS3, N_AS3, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_D4, N_D4, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_C4, N_C4, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_B3, N_B3, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_AS3, N_AS3, N_D5, N_A4, 0, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, N_F4, N_F4, N_F4, N_F4, N_F4, N_D4, N_D4, N_D4, N_F4, N_F4, N_F4, N_G4, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, 0, N_F4, N_F4, N_F4, N_G4, N_GS4, N_A4, N_C5, N_A4, N_D5, N_D5, N_D5, N_A4, N_D5, N_C5, N_F4, N_F4, N_F4, N_F4, N_F4, N_D4, N_D4, N_D4, N_F4, N_F4, N_F4, N_F4, N_D4, N_F4, N_E4, N_D4, N_C4, 0, N_G4, N_E4, N_D4, N_D4, N_D4, N_D4, N_F3, N_G3, N_AS3, N_C4, N_D4, N_F4, N_C5, 0, N_F4, N_D4, N_F4, N_G4, N_GS4, N_G4, N_F4, N_D4, N_GS4, N_G4, N_F4, N_D4, N_F4, N_F4, N_F4, N_GS4, N_A4, N_C5, N_A4, N_GS4, N_G4, N_F4, N_D4, N_E4, N_F4, N_G4, N_A4, N_C5, N_CS5, N_GS4, N_GS4, N_G4, N_F4, N_G4, N_F3, N_G3, N_A3, N_F4, N_E4, N_D4, N_E4, N_F4, N_G4, N_E4, N_A4, N_A4, N_G4, N_F4, N_DS4, N_CS4, N_DS4, 0, N_F4, N_D4, N_F4, N_G4, N_GS4, N_G4, N_F4, N_D4, N_GS4, N_G4, N_F4, N_D4, N_F4, N_F4, N_F4, N_GS4, N_A4, N_C5, N_A4, N_GS4, N_G4, N_F4, N_D4, N_E4, N_F4, N_G4, N_A4, N_C5, N_CS5, N_GS4, N_GS4, N_G4, N_F4, N_G4, N_F3, N_G3, N_A3, N_F4, N_E4, N_D4, N_E4, N_F4, N_G4, N_E4, N_A4, N_A4, N_G4, N_F4, N_DS4, N_CS4, N_DS4,
};

int noteDurations[] = {
  16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 8, 16, 8, 8, 8, 8, 4, 16, 8, 16, 8, 8, 8, 16, 16, 16, 16, 16, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 2, 8, 16, 8, 8, 8, 8, 4, 16, 8, 16, 8, 8, 8, 8, 8, 16, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8, 15, 8, 8, 2, 3, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 2, 16, 8, 16, 8, 16, 16, 16, 16, 16, 16, 8, 8, 8, 8,  8, 8, 16, 16, 16, 2, 8, 8, 8, 8, 4, 4, 4, 4, 4, 4, 2, 8, 8, 8, 8, 2, 2, 3, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 2, 16, 8, 16, 8, 16, 16, 16, 16, 16, 16, 8, 8, 8, 8,  8, 8, 16, 16, 16, 2, 8, 8, 8, 8, 4, 4, 4, 4, 4, 4, 2, 8, 8, 8, 8, 2, 2
};

//melody = 266 notes

const int servo_pin = 26;
int freq = 50;
int ledChannel = 0;
int resolution = 10;

void megalovainia_setup() {
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawBitmap(0, 0, 160, 80, image_data_sans);
  int melody_len = sizeof(melody) / sizeof(melody[0]);
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(servo_pin, ledChannel);
  ledcWrite(ledChannel, 256);//0°

  for (int thisNote = 0; thisNote < melody_len; thisNote++) {
    switcher_button_proc(); // This is a cancel option
    if (current_proc == 6) {
      int noteDuration = TEMPO / noteDurations[thisNote];
      ledcWriteTone(0, melody[thisNote]);
      int pauseBetweenNotes = noteDuration * 1.45;
      delay(pauseBetweenNotes);
    }
    ledcWriteTone(0, 0);
  }
  isSwitching = true;
  current_proc = 1;
}

/// Timezone menu ///
bool set_tz = false;
int tz = 0;
bool skip_first_tz_loop = true;

MENU tzmenu[] = {
  { "UTC +14", 50400},
  { "UTC +13:45", 49500},
  { "UTC +13", 46800},
  { "UTC +12", 43200},
  { "UTC +11", 39600},
  { "UTC +10:30", 37800},
  { "UTC +10", 36000},
  { "UTC +9:30", 34200},
  { "UTC +9", 32400},
  { "UTC +8:45", 31500},
  { "UTC +8", 28800},
  { "UTC +7", 25200},
  { "UTC +6:30", 23400},
  { "UTC +6", 21600},
  { "UTC +5:45", 20700},
  { "UTC +5:30", 19800},
  { "UTC +5", 18000},
  { "UTC +4:30", 16200},
  { "UTC +4", 14400},
  { "UTC +3:30", 12600},
  { "UTC +3", 10800},
  { "UTC +2", 7200},
  { "UTC +1", 3600},
  { "UTC +0", 0},
  { "UTC -1", -3600},
  { "UTC -2", -7200},
  { "UTC -3", -10800},
  { "UTC -3:30", -12600},
  { "UTC -4", -14400},
  { "UTC -5", -18000},
  { "UTC -6", -21000},
  { "UTC -7", -25200},
  { "UTC -8", -28800},
  { "UTC -9", -32400},
  { "UTC -9:30", -34200},
  { "UTC -10", -36000},
  { "UTC -11", -39600},
  { "UTC -12", -43200},
};

void tzmenu_drawmenu() {
  // List items
  M5.Lcd.setRotation(0);
  M5.Lcd.setTextSize(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 8, 1);
  // FUTURE REF: This code is a scrolling menu. Might use later for expansion
  if (cursor > 18) {
    for ( int i = 0 + (cursor - 18) ; i < ( sizeof(tzmenu) / sizeof(MENU) ) ; i++ ) {
      M5.Lcd.print((cursor == i) ? ">" : " ");
      M5.Lcd.println(tzmenu[i].name);
    }
  } else {
    for (
      int i = 0 ; i < ( sizeof(tzmenu) / sizeof(MENU) ) ; i++ ) {
      M5.Lcd.print((cursor == i) ? ">" : " ");
      M5.Lcd.println(tzmenu[i].name);
    }
  }
}

void tzmenu_setup() {
  M5.Lcd.setRotation(0);

  cursor = 0;
  rstOverride = true;
  tzmenu_drawmenu();
  delay(250); // Prevent switching after menu loads up
}

void tzmenu_loop() {
  if (!skip_first_tz_loop) {
    // Switch in menu
    if (digitalRead(M5_BUTTON_RST) == LOW) {
      cursor++;
      cursor = cursor % ( sizeof(tzmenu) / sizeof(MENU) );
      tzmenu_drawmenu();
      delay(250);
    }
    // Click
    if (digitalRead(M5_BUTTON_HOME) == LOW) {
      // Unload menu
      rstOverride = false;
      isSwitching = true;
      set_tz = true;
      tz = tzmenu[cursor].command;

      current_proc = 3;
    }
  } else {
    skip_first_tz_loop = false;
  }
}


/// SET CLOCK ///
bool set_clock_readyForLoop = false;

void set_clock_setup() {
  // First, check/set the timezone
  if (set_tz == false) {
    isSwitching = true;
    current_proc = 9;
  } else {
    // Set the screen
    M5.Lcd.setTextSize(1);
    M5.Lcd.setRotation(rotation);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 8, 2);

    // Password Gen
    int len = 8;
    char *letters = "abcdefghijklmnopqrstuvwxyz0123456789";
    String pw;
    int i;
    for (i = 0; i < len; i++) {
      pw = pw + letters[random(0, 35)];
    }

    M5.Lcd.println("Please connect WIFI");
    M5.Lcd.println("SSID: HAKRWATCH");
    M5.Lcd.print("PW: ");
    M5.Lcd.println(pw);
    // Lock the device
    rstOverride = true;
    // Set up WifiManager
    wifiManager.autoConnect("HAKRWATCH", pw.c_str());
    // Clear screen
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20, 30, 1);
    M5.Lcd.print("PLS WAIT");
    // Set ntp client
    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    // GMT +8 = 28800
    // GMT -1 = -3600
    // GMT 0 = 0
    timeClient.setTimeOffset(tz);
    // Now make a loop to wait for the ntp time
    set_clock_readyForLoop = true;
  }
}

void set_clock_loop() {
  if (set_clock_readyForLoop == true) {
    Serial.println("waiting for clock");
    while (!timeClient.update()) {
      Serial.println("waiting for clock update");
      timeClient.forceUpdate();
    }
    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    formattedDate = timeClient.getFormattedDate();
    Serial.println(formattedDate);

    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    Serial.print("DATE: ");
    Serial.println(dayStamp);
    // Extract time
    timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
    Serial.print("HOUR: ");
    Serial.println(timeStamp);

    // TODO: Add date to RTC!

    // Set the RTC
    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours   = timeStamp.substring(0, 2).toInt();
    TimeStruct.Seconds = timeStamp.substring(6, 8).toInt();
    TimeStruct.Minutes = timeStamp.substring(3, 5).toInt();
    M5.Rtc.SetTime(&TimeStruct);
    delay(1000);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(20, 30, 1);
    M5.Lcd.print("TIME SET");
    WiFi.disconnect(true);
    delay(2000);
    rstOverride = false;
    isSwitching = true;
    current_proc = 0;
  }
}

/// TV-B-GONE ///
void xmitCodeElement(uint16_t ontime, uint16_t offtime, uint8_t PWM_code );
void quickflashLEDx( uint8_t x );
void delay_ten_us(uint16_t us);
void quickflashLED( void );
uint8_t read_bits(uint8_t count);
uint16_t rawData[300];


#define putstring_nl(s) Serial.println(s)
#define putstring(s) Serial.print(s)
#define putnum_ud(n) Serial.print(n, DEC)
#define putnum_uh(n) Serial.print(n, HEX)

#define MAX_WAIT_TIME 65535 //tens of us (ie: 655.350ms)

IRsend irsend(IRLED);  // Set the GPIO to be used to sending the message.

extern const IrCode* const NApowerCodes[];
extern const IrCode* const EUpowerCodes[];
extern uint8_t num_NAcodes, num_EUcodes;
uint8_t bitsleft_r = 0;
uint8_t bits_r = 0;
uint8_t code_ptr;
volatile const IrCode * powerCode;

// we cant read more than 8 bits at a time so dont try!
uint8_t read_bits(uint8_t count)
{
  uint8_t i;
  uint8_t tmp = 0;

  // we need to read back count bytes
  for (i = 0; i < count; i++) {
    // check if the 8-bit buffer we have has run out
    if (bitsleft_r == 0) {
      // in which case we read a new byte in
      bits_r = powerCode->codes[code_ptr++];
      DEBUGP(putstring("\n\rGet byte: ");
             putnum_uh(bits_r);
            );
      // and reset the buffer size (8 bites in a byte)
      bitsleft_r = 8;
    }
    // remove one bit
    bitsleft_r--;
    // and shift it off of the end of 'bits_r'
    tmp |= (((bits_r >> (bitsleft_r)) & 1) << (count - 1 - i));
  }
  // return the selected bits in the LSB part of tmp
  return tmp;
}

#define BUTTON_PRESSED LOW
#define BUTTON_RELEASED HIGH

uint16_t ontime, offtime;
uint8_t i, num_codes;
uint8_t region;

void tvbgone_setup() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(5, 1);
  M5.Lcd.println("M5StickC-IR");
  irsend.begin();
  delay_ten_us(5000);
  // determine region
  //M5.Lcd.setCursor(8,1);
  if (digitalRead(REGIONSWITCH)) {
    region = NA;
    DEBUGP(putstring_nl("NA"));
    M5.Lcd.println("Region: NA");
  }
  else {
    region = EU;
    DEBUGP(putstring_nl("EU"));
    M5.Lcd.println("Region: EU");
  }
  // Tell the user what region we're in  - 3 flashes is NA, 6 is EU
  if (region == NA)
    quickflashLEDx(3);
  else //region == EU
    quickflashLEDx(6);
  delay(1000); // Give time after loading
}

void sendAllCodes()
{
  bool endingEarly = false; //will be set to true if the user presses the button during code-sending

  // determine region from REGIONSWITCH: 1 = NA, 0 = EU (defined in main.h)
  if (digitalRead(REGIONSWITCH)) {
    region = NA;
    num_codes = num_NAcodes;
  }
  else {
    region = EU;
    num_codes = num_EUcodes;
  }

  // for every POWER code in our collection
  for (i = 0 ; i < num_codes; i++)
  {

    // print out the code # we are about to transmit
    DEBUGP(putstring("\n\r\n\rCode #: ");
           putnum_ud(i));

    // point to next POWER code, from the right database
    if (region == NA) {
      powerCode = NApowerCodes[i];
    }
    else {
      powerCode = EUpowerCodes[i];
    }

    // Read the carrier frequency from the first byte of code structure
    const uint8_t freq = powerCode->timer_val;
    // set OCR for Timer1 to output this POWER code's carrier frequency

    // Print out the frequency of the carrier and the PWM settings
    DEBUGP(putstring("\n\rFrequency: ");
           putnum_ud(freq);
          );

    DEBUGP(uint16_t x = (freq + 1) * 2;
           putstring("\n\rFreq: ");
           putnum_ud(F_CPU / x);
          );

    // Get the number of pairs, the second byte from the code struct
    const uint8_t numpairs = powerCode->numpairs;
    DEBUGP(putstring("\n\rOn/off pairs: ");
           putnum_ud(numpairs));
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(5, 1);
    M5.Lcd.println("M5StickC-IR");
    M5.Lcd.println(numpairs);
    // Get the number of bits we use to index into the timer table
    // This is the third byte of the structure
    const uint8_t bitcompression = powerCode->bitcompression;
    DEBUGP(putstring("\n\rCompression: ");
           putnum_ud(bitcompression);
           putstring("\n\r"));

    // For EACH pair in this code....
    code_ptr = 0;
    for (uint8_t k = 0; k < numpairs; k++) {
      uint16_t ti;

      // Read the next 'n' bits as indicated by the compression variable
      // The multiply by 4 because there are 2 timing numbers per pair
      // and each timing number is one word long, so 4 bytes total!
      ti = (read_bits(bitcompression)) * 2;

      // read the onTime and offTime from the program memory
      ontime = powerCode->times[ti];  // read word 1 - ontime
      offtime = powerCode->times[ti + 1]; // read word 2 - offtime

      DEBUGP(putstring("\n\rti = ");
             putnum_ud(ti >> 1);
             putstring("\tPair = ");
             putnum_ud(ontime));
      DEBUGP(putstring("\t");
             putnum_ud(offtime));

      rawData[k * 2] = ontime * 10;
      rawData[(k * 2) + 1] = offtime * 10;
      yield();
    }

    // Send Code with library
    irsend.sendRaw(rawData, (numpairs * 2) , freq);
    Serial.print("\n");
    yield();
    //Flush remaining bits, so that next code starts
    //with a fresh set of 8 bits.
    bitsleft_r = 0;

    // visible indication that a code has been output.
    //quickflashLED();

    // delay 205 milliseconds before transmitting next POWER code
    delay_ten_us(20500);

    // if user is pushing (holding down) TRIGGER button, stop transmission early
    if (digitalRead(TRIGGER) == BUTTON_PRESSED)
    {
      while (digitalRead(TRIGGER) == BUTTON_PRESSED) {
        yield();
      }
      endingEarly = true;
      delay_ten_us(50000); //500ms delay
      quickflashLEDx(4);
      //pause for ~1.3 sec to give the user time to release the button so that the code sequence won't immediately start again.
      delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
      delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
      break; //exit the POWER code "for" loop
    }

  } //end of POWER code for loop

  if (endingEarly == false)
  {
    //pause for ~1.3 sec, then flash the visible LED 8 times to indicate that we're done
    delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
    delay_ten_us(MAX_WAIT_TIME); // wait 655.350ms
    quickflashLEDx(8);
  }

} //end of sendAllCodes

void tvbgone_loop()
{
  //Super "ghetto" (but decent enough for this application) button debouncing:
  //-if the user pushes the Trigger button, then wait a while to let the button stop bouncing, then start transmission of all POWER codes
  if (digitalRead(TRIGGER) == BUTTON_PRESSED)
  {
    delay_ten_us(40000);
    while (digitalRead(TRIGGER) == BUTTON_PRESSED) {
      delay_ten_us(500);
      yield();
    }
    sendAllCodes();
  }
  yield();
}

void delay_ten_us(uint16_t us) {
  uint8_t timer;
  while (us != 0) {
    // for 8MHz we want to delay 80 cycles per 10 microseconds
    // this code is tweaked to give about that amount.
    for (timer = 0; timer <= DELAY_CNT; timer++) {
      NOP;
      NOP;
    }
    NOP;
    us--;
  }
}

void quickflashLED( void ) {
  digitalWrite(LED, LOW);
  delay_ten_us(3000);   // 30 ms ON-time delay
  digitalWrite(LED, HIGH);
}

void quickflashLEDx( uint8_t x ) {
  quickflashLED();
  while (--x) {
    delay_ten_us(25000);     // 250 ms OFF-time delay between flashes
    quickflashLED();
  }
}

/// CLOCK ///
void clock_setup() {
  M5.Lcd.setRotation(rotation);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
}

void clock_loop() {
  M5.Rtc.GetBm8563Time();
  M5.Lcd.setCursor(40, 30, 2);
  M5.Lcd.printf("%02d : %02d : %02d\n", M5.Rtc.Hour, M5.Rtc.Minute, M5.Rtc.Second);
  delay(250);
}

///

/// ENTRY ///
void setup() {
  // M5 Initalization Code
  M5.begin();
  M5.Axp.ScreenBreath(11); // Brightness
  M5.Lcd.setRotation(rotation);
  M5.Lcd.setTextColor(GREEN, BLACK);

  // EEPROM
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  rotation = EEPROM.read(0);

  // Boot Screen
  digitalWrite(M5_LED, HIGH); //LEDOFF
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 30, 1);
  M5.Lcd.setRotation(rotation);
  M5.Lcd.print("HAKR_WATCH");


  // Pin setup
  pinMode(M5_LED, OUTPUT);
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);

  // Random seed
  randomSeed(analogRead(0));

  // Finish with time to show logo
  delay(3000);


}

void loop() {
  // This is the code to handle running the main loops
  // Background processes
  switcher_button_proc();
  screen_dim_proc();

  // Switcher
  if (isSwitching) {
    isSwitching = false;
    switch (current_proc) {
      case 0:
        clock_setup();
        break;
      case 1:
        mmenu_setup();
        break;
      case 2:
        smenu_setup();
        break;
      case 3:
        set_clock_setup();
        break;
      case 4:
        dmenu_setup();
        break;
      case 5:
        tvbgone_setup();
        break;
      case 6:
        megalovainia_setup();
        break;
      case 7:
        battery_setup();
        break;
      case 8:
        rmenu_setup();
        break;
      case 9:
        tzmenu_setup();
        break;
    }
  }

  switch (current_proc) {
    case 0:
      clock_loop();
      break;
    case 1:
      mmenu_loop();
      break;
    case 2:
      smenu_loop();
      break;
    case 3:
      set_clock_loop();
      break;
    case 4:
      dmenu_loop();
      break;
    case 5:
      tvbgone_loop();
      break;
    case 7:
      battery_loop();
      break;
    case 8:
      rmenu_loop();
      break;
    case 9:
      tzmenu_loop();
      break;
  }
}
