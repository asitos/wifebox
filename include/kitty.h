#ifndef KITTY_H
#define KITTY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#ifndef WEB_SERVER_H
#include "web_server.h"
#endif

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern Servo myServo;
extern ScreenState currentScreen;

#ifndef RESET_SERVO_DEFINED
#define RESET_SERVO_DEFINED
inline void resetServo() {
  // Disabled: servo mechanically stuck. Release PWM to stop stall current and save power.
  myServo.release();
}
#endif

#define KITTY_BUZZER_MELODY 15 // GPIO 26
#define KITTY_BUZZER_PURR   14 // GPIO 25

// ==========================================
//           BITMAP ASSETS (PROGMEM)
// ==========================================

// 8x8 Heart Icon
const unsigned char kitty_heart_icon[8] PROGMEM = {
  0x66, // .XX..XX.
  0xFF, // XXXXXXXX
  0xFF, // XXXXXXXX
  0xFF, // XXXXXXXX
  0x7E, // .XXXXXX.
  0x3C, // ..XXXX..
  0x18, // ...XX...
  0x00  // ........
};

// 8x8 Fish Icon
const unsigned char kitty_fish_icon[8] PROGMEM = {
  0x18, // ...XX...
  0x3C, // ..XXXX..
  0x7E, // .XXXXXX.
  0xDB, // XX.XX.XX
  0xFF, // XXXXXXXX
  0x7E, // .XXXXXX.
  0x3C, // ..XXXX..
  0x18  // ...XX...
};

// 32x32 Kitty Idle Frame 1 (Eyes open, neutral tail)
const unsigned char kitty_idle_frame1[128] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x01, 0xC0, 0x06, 0xC0, 0x03, 0x60,
  0x0E, 0x60, 0x06, 0x70, 0x1F, 0xF0, 0x0F, 0xF8, 0x00, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xC0,
  0x03, 0xFF, 0xFF, 0xC0, 0x03, 0xFF, 0xFF, 0xC0, 0x03, 0xCF, 0xF3, 0xC0, 0x3B, 0xEF, 0xF7, 0xDC,
  0x03, 0xFF, 0xFF, 0xC0, 0x3B, 0xFE, 0x7F, 0xDC, 0x03, 0xFD, 0xBF, 0xC0, 0x03, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00,
  0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE0, 0x00, 0xFF, 0xFF, 0x30, 0x00, 0xFF, 0xFF, 0x30,
  0x00, 0xFF, 0xFF, 0x30, 0x00, 0xFF, 0xFF, 0x60, 0x00, 0xF7, 0xEF, 0xC0, 0x00, 0x77, 0xEE, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// 32x32 Kitty Idle Frame 2 (Tail swish / breathe)
const unsigned char kitty_idle_frame2[128] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x01, 0xC0, 0x06, 0xC0, 0x03, 0x60,
  0x0E, 0x60, 0x06, 0x70, 0x1F, 0xF0, 0x0F, 0xF8, 0x00, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xC0,
  0x03, 0xFF, 0xFF, 0xC0, 0x03, 0xFF, 0xFF, 0xC0, 0x03, 0xCF, 0xF3, 0xC0, 0x3B, 0xEF, 0xF7, 0xDC,
  0x03, 0xFF, 0xFF, 0xC0, 0x3B, 0xFE, 0x7F, 0xDC, 0x03, 0xFD, 0xBF, 0xC0, 0x03, 0xFF, 0xFF, 0xC0,
  0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x60,
  0x00, 0xFF, 0xFF, 0x30, 0x00, 0xFF, 0xFF, 0x18, 0x00, 0xFF, 0xFF, 0x18, 0x00, 0xFF, 0xFF, 0x30,
  0x00, 0xFF, 0xFF, 0x60, 0x00, 0xFF, 0xFF, 0xC0, 0x00, 0xF7, 0xEF, 0x80, 0x00, 0x77, 0xEE, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// 32x32 Kitty Happy Frame (^ . ^ eyes, open smile)
const unsigned char kitty_happy[128] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x10, 0x03, 0x80, 0x01, 0xC0, 0x06, 0xC0, 0x03, 0x60,
  0x0E, 0x60, 0x06, 0x70, 0x1F, 0xF0, 0x0F, 0xF8, 0x00, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xC0,
  0x03, 0xFF, 0xFF, 0xC0, 0x03, 0xCF, 0xF3, 0xC0, 0x03, 0xB7, 0xED, 0xC0, 0x3B, 0xFF, 0xFF, 0xDC,
  0x03, 0xFF, 0xFF, 0xC0, 0x3B, 0xFE, 0x7F, 0xDC, 0x03, 0xFC, 0x3F, 0xC0, 0x03, 0xFC, 0x3F, 0xC0,
  0x00, 0xFE, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00,
  0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xE0, 0x00, 0xFF, 0xFF, 0x30, 0x00, 0xFF, 0xFF, 0x30,
  0x00, 0xFF, 0xFF, 0x30, 0x00, 0xFF, 0xFF, 0x60, 0x00, 0xF7, 0xEF, 0xC0, 0x00, 0x77, 0xEE, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// 32x32 Kitty Sleep Frame (- . - eyes, curled loaf)
const unsigned char kitty_sleep[128] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x06, 0x00, 0x03, 0xF0, 0x07, 0xC0, 0x03, 0x50, 0x05, 0xC0,
  0x03, 0x50, 0x05, 0xC0, 0x03, 0xF0, 0x07, 0xC0, 0x03, 0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xE0,
  0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x37, 0xE0, 0x07, 0xEC,
  0x07, 0xFF, 0xFF, 0xE0, 0x37, 0xFE, 0x7F, 0xEC, 0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0,
  0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x07, 0xF7, 0xEF, 0xE0, 0x07, 0xF7, 0xEF, 0xE0,
  0x07, 0xFF, 0xFF, 0xE0, 0x07, 0x00, 0x00, 0xE0, 0x07, 0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ==========================================
//           DATA STRUCTURES & STATE
// ==========================================

enum PetAction {
  ACTION_NONE,
  ACTION_EATING,
  ACTION_PETTING
};

struct VirtualPet {
  uint8_t hunger;
  uint8_t affection;
  bool isSleeping;
  
  unsigned long lastDecayTime;
  unsigned long lastAnimTime;
  uint8_t animFrame;
  
  PetAction currentAction;
  unsigned long actionStartTime;
  
  unsigned long soundEndTime;
  bool purrActive;
};

static VirtualPet kittyPet = {
  80,               // hunger
  80,               // affection
  false,            // isSleeping
  0,                // lastDecayTime
  0,                // lastAnimTime
  0,                // animFrame
  ACTION_NONE,      // currentAction
  0,                // actionStartTime
  0,                // soundEndTime
  false             // purrActive
};

// ==========================================
//          INTERACTIONS & AUDIO/SERVO
// ==========================================

inline void feedKitty() {
  kittyPet.hunger = constrain(kittyPet.hunger + 20, 0, 100);
  kittyPet.currentAction = ACTION_EATING;
  kittyPet.actionStartTime = millis();
  kittyPet.soundEndTime = millis() + 120;
  
  // High-pitch "mew" sweep on Buzzer 1 (GPIO 26)
  ledcWriteTone(KITTY_BUZZER_MELODY, 1400);
}

inline void petKitty() {
  kittyPet.affection = constrain(kittyPet.affection + 15, 0, 100);
  kittyPet.currentAction = ACTION_PETTING;
  kittyPet.actionStartTime = millis();
  kittyPet.soundEndTime = millis() + 1500;
  kittyPet.purrActive = true;
}

inline void toggleKittySleep() {
  kittyPet.isSleeping = !kittyPet.isSleeping;
  if (kittyPet.isSleeping) {
    ledcWriteTone(KITTY_BUZZER_MELODY, 440);
    kittyPet.soundEndTime = millis() + 100;
  } else {
    ledcWriteTone(KITTY_BUZZER_MELODY, 880);
    kittyPet.soundEndTime = millis() + 100;
  }
}

inline void exitKitty() {
  resetServo();
  ledcWriteTone(KITTY_BUZZER_MELODY, 0);
  ledcWriteTone(KITTY_BUZZER_PURR, 0);
  kittyPet.purrActive = false;
  kittyPet.soundEndTime = 0;
  currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
  u8g2.clearBuffer();
}

// Update routine for non-blocking timers, audio, and servo
inline void updateKitty() {
  unsigned long now = millis();

  // Passive decay: every 15 mins (slowed to 30 mins if sleeping)
  unsigned long decayInterval = kittyPet.isSleeping ? 1800000UL : 900000UL;
  if (now - kittyPet.lastDecayTime > decayInterval) {
    kittyPet.lastDecayTime = now;
    if (kittyPet.hunger > 0) kittyPet.hunger--;
    if (kittyPet.affection > 0) kittyPet.affection--;
  }

  // Animation frame toggle every 400ms
  if (now - kittyPet.lastAnimTime > 400) {
    kittyPet.lastAnimTime = now;
    kittyPet.animFrame ^= 1;
  }

  // Check action timeout (1.5s)
  if (kittyPet.currentAction != ACTION_NONE && (now - kittyPet.actionStartTime > 1500)) {
    kittyPet.currentAction = ACTION_NONE;
  }

  // Audio & Servo execution only if active on SCREEN_KITTY
  if (currentScreen == SCREEN_KITTY) {
    // Mew sweep
    if (kittyPet.currentAction == ACTION_EATING) {
      if (now < kittyPet.soundEndTime) {
        int progress = 120 - (kittyPet.soundEndTime - now);
        int freq = 1400 + (progress * 400) / 120; // 1400 -> 1800 Hz mew sweep
        ledcWriteTone(KITTY_BUZZER_MELODY, freq);
      } else if (kittyPet.soundEndTime > 0) {
        ledcWriteTone(KITTY_BUZZER_MELODY, 0);
        kittyPet.soundEndTime = 0;
      }
    }

    // Purr & micro-jitter
    if (kittyPet.purrActive) {
      if (now < kittyPet.soundEndTime) {
        unsigned long purrTime = (now - kittyPet.actionStartTime) % 160;
        if (purrTime < 90) {
          ledcWriteTone(KITTY_BUZZER_PURR, 100); // 100Hz rumble
          myServo.write((purrTime < 45) ? 92 : 88); // micro-jitter +/-2 around 90
        } else {
          ledcWriteTone(KITTY_BUZZER_PURR, 0);
        }
      } else {
        ledcWriteTone(KITTY_BUZZER_PURR, 0);
        resetServo();
        kittyPet.purrActive = false;
        kittyPet.soundEndTime = 0;
      }
    }

    // General audio cutoff
    if (!kittyPet.purrActive && kittyPet.soundEndTime > 0 && now >= kittyPet.soundEndTime) {
      ledcWriteTone(KITTY_BUZZER_MELODY, 0);
      kittyPet.soundEndTime = 0;
    }
  }
}

// ==========================================
//               DRAW ROUTINE
// ==========================================

inline void drawKittyScreen() {
  updateKitty();
  unsigned long now = millis();

  u8g2.clearBuffer();

  // --- Top Bar (y: 0-12) ---
  // Hunger Bar
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 8, "F");
  u8g2.drawFrame(8, 3, 34, 6);
  int hFill = ((int)kittyPet.hunger * 30) / 100;
  if (hFill > 0) u8g2.drawBox(10, 4, hFill, 4);

  // Affection Meter
  u8g2.drawBitmap(48, 2, 1, 8, kitty_heart_icon);
  u8g2.drawFrame(58, 3, 34, 6);
  int aFill = ((int)kittyPet.affection * 30) / 100;
  if (aFill > 0) u8g2.drawBox(60, 4, aFill, 4);

  // Sleep indicator
  if (kittyPet.isSleeping) {
    u8g2.drawStr(100, 8, "Zzz");
  }

  // --- Center Arena (y: 13-50) ---
  // Choose sprite
  const unsigned char *sprite = kitty_idle_frame1;
  if (kittyPet.isSleeping) {
    sprite = kitty_sleep;
  } else if (kittyPet.currentAction == ACTION_PETTING || kittyPet.affection >= 85) {
    sprite = kitty_happy;
  } else if (kittyPet.animFrame == 1) {
    sprite = kitty_idle_frame2;
  }

  // Draw Kitty centered at x = 48, y = 18
  u8g2.drawBitmap(48, 18, 4, 32, sprite);

  // Transient interaction animations
  if (kittyPet.currentAction == ACTION_EATING) {
    // Falling fish at x = 24
    unsigned long aElapsed = now - kittyPet.actionStartTime;
    int fishY = 16 + (int)((aElapsed * 24) / 1500);
    u8g2.drawBitmap(24, fishY, 1, 8, kitty_fish_icon);
    // Crumb dots near cat mouth
    if (aElapsed > 800) {
      u8g2.drawPixel(44, 32);
      u8g2.drawPixel(42, 34);
    }
  } else if (kittyPet.currentAction == ACTION_PETTING) {
    // Floating rising hearts
    unsigned long aElapsed = now - kittyPet.actionStartTime;
    int rise = (int)((aElapsed * 26) / 1500);
    int h1Y = 44 - rise;
    int h2Y = 40 - (rise * 4 / 5);
    if (h1Y >= 14) u8g2.drawBitmap(30, h1Y, 1, 8, kitty_heart_icon);
    if (h2Y >= 14) u8g2.drawBitmap(88, h2Y, 1, 8, kitty_heart_icon);
  }

  // --- Bottom Label Bar (y: 52-64) ---
  u8g2.drawHLine(0, 52, 128);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(6, 62, "[FEED]");
  u8g2.drawStr(52, 62, "[PET]");
  u8g2.drawStr(92, 62, "[EXIT]");

  u8g2.sendBuffer();
}

#endif // KITTY_H
