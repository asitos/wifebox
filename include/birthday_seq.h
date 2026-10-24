#ifndef BIRTHDAY_SEQ_H
#define BIRTHDAY_SEQ_H

#include "bitmaps.h"
#include <Arduino.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern Servo myServo;
extern volatile bool birthdayMusicActive;
extern volatile unsigned long birthdayMusicStartTime;

#ifndef RESET_SERVO_DEFINED
#define RESET_SERVO_DEFINED
inline void resetServo() {
  // Disabled: servo mechanically stuck. Release PWM to stop stall current and save power.
  myServo.release();
}
#endif

static unsigned long birthdayStart = 0;
static const unsigned long phaseCurtain = 1000;
static const unsigned long phaseBombers = 2500;
static const unsigned long phaseFireworks = 4500;
static const unsigned long fadePhaseEnd = 6000;
static const unsigned long birthdayDuration = 25000;

#define NUM_CONFETTI 15
static float confX[NUM_CONFETTI];
static float confY[NUM_CONFETTI];
static float confSpeed[NUM_CONFETTI];

#define NUM_BALLOONS 4
static float ballX[NUM_BALLOONS];
static float ballY[NUM_BALLOONS];

inline void initBirthdayConfetti() {
  for (int i = 0; i < NUM_CONFETTI; i++) {
    confX[i] = random(0, 128);
    confY[i] = random(-30, 0);
    confSpeed[i] = 1.0 + random(0, 150) / 100.0;
  }
  for (int i = 0; i < NUM_BALLOONS; i++) {
    ballX[i] = 20 + i*25;
    ballY[i] = 70 + random(0,20);
  }
}

inline void startBirthday() {
  birthdayStart = millis();
  birthdayMusicStartTime = birthdayStart;
  birthdayMusicActive = true;
  initBirthdayConfetti();
  extern volatile bool stopAudioCmd;
  stopAudioCmd = true; // stop jukebox
}

inline bool renderBirthday() {
  unsigned long elapsed = millis() - birthdayStart;
  if (elapsed > birthdayDuration) {
    birthdayMusicActive = false;
    resetServo();
    u8g2.sendF("c", 0xa6); // restore normal screen
    return false;
  }

  // Rate limit display refresh to ~15 FPS (66ms) for smooth animation
  static unsigned long lastFrameTime = 0;
  if (millis() - lastFrameTime < 66) return true;
  lastFrameTime = millis();

  u8g2.clearBuffer();

  if (elapsed < phaseCurtain) {
      int pull = (elapsed * 64) / phaseCurtain; 
      u8g2.drawBox(0, 0, 64 - pull, 64);
      u8g2.drawBox(64 + pull, 0, 64 - pull, 64);
  } 
  else if (elapsed < phaseBombers) {
      unsigned long bElapsed = elapsed - phaseCurtain;
      int r = bElapsed / 25; // Slower bomber pop
      if (r < 40) {
         u8g2.drawLine(0, 64, r, 64 - r);
         u8g2.drawLine(128, 64, 128 - r, 64 - r);
         u8g2.drawCircle(r, 64 - r, 3);
         u8g2.drawCircle(128 - r, 64 - r, 3);
      }
      for (int i = 0; i < NUM_CONFETTI; i++) {
        confY[i] += confSpeed[i];
        if (confY[i] < 64) u8g2.drawPixel((int)confX[i], (int)confY[i]);
      }
  }
  else if (elapsed < phaseFireworks) {
      unsigned long fElapsed = elapsed - phaseBombers;
      for (int i = 0; i < NUM_BALLOONS; i++) {
          ballY[i] -= 1.0; // Slower balloons
          if (ballY[i] > -10) {
              u8g2.drawEllipse((int)ballX[i], (int)ballY[i], 6, 8);
              u8g2.drawLine((int)ballX[i], (int)ballY[i]+8, (int)ballX[i], (int)ballY[i]+16);
          }
      }
      if (fElapsed > 500) {
          int fwR = (fElapsed - 500) / 40; // Slower firework expansion
          if (fwR < 30) {
              for (int a = 0; a < 8; a++) {
                  float rad = a * 3.14159 / 4.0;
                  u8g2.drawPixel(64 + fwR*cos(rad), 32 + fwR*sin(rad));
                  u8g2.drawPixel(30 + fwR*0.7*cos(rad), 16 + fwR*0.7*sin(rad));
                  u8g2.drawPixel(100 + fwR*0.7*cos(rad), 20 + fwR*0.7*sin(rad));
              }
          }
      }
      if (fElapsed >= 500 && fElapsed < 800) myServo.write(180);
      else if (fElapsed >= 800) myServo.write(0);
  }
  else {
      if (elapsed < fadePhaseEnd) {
          unsigned long fadeElapsed = elapsed - phaseFireworks;
          int revealRows = fadeElapsed * 64 / (fadePhaseEnd - phaseFireworks);
          if (revealRows > 64) revealRows = 64;
          if (revealRows > 0) u8g2.drawBitmap(0, 0, 16, revealRows, epd_bitmap_wife2);
      } else {
          u8g2.drawBitmap(0, 0, 16, 64, epd_bitmap_wife2);

          // Fast, noticeable conductor bob 0 <-> 180 every 250ms
          if ((elapsed / 250) % 2 == 0) myServo.write(180);
          else myServo.write(0);

          u8g2.setFont(u8g2_font_5x8_tr);
          const char *bannerText = "happppiest birthdayy!!!        i love you humesha my pyaari wife          happy 20th!! (legally 19 </3)";
          int textWidth = u8g2.getStrWidth(bannerText);
          unsigned long finalElapsed = elapsed - fadePhaseEnd;
          int marqueeX = 128 - (int)((finalElapsed / 24) % (128 + textWidth));
          
          u8g2.setDrawColor(0);
          u8g2.drawBox(0, 54, 128, 10);
          u8g2.setDrawColor(1);
          u8g2.drawStr(marqueeX, 63, bannerText);

          u8g2.drawStr(2, 28, "[#]");
          u8g2.drawStr(115, 28, " i ");

          u8g2.setFont(u8g2_font_4x6_tr);
          u8g2.setDrawColor(2);
          u8g2.drawStr(2, 6, "<3");
          u8g2.drawStr(115, 6, ":3");
          u8g2.drawStr(2, 50, ":p");
          u8g2.drawStr(115, 50, "<3");
          u8g2.setDrawColor(1);

          unsigned long timeLeft = birthdayDuration - elapsed;
          if (timeLeft < 1800) {
             // Inverted screen flash during triumphant fanfare
             if (timeLeft > 1400) {
                 u8g2.sendF("c", 0xa7);
             } else if (timeLeft > 1000) {
                 u8g2.sendF("c", 0xa6);
             } else if (timeLeft > 600) {
                 u8g2.sendF("c", 0xa7);
             } else {
                 u8g2.sendF("c", 0xa6);
             }
             if (timeLeft > 150) myServo.write(180);
             else myServo.write(0);
          }
      }
  }

  u8g2.sendBuffer();
  return true;
}

#endif // BIRTHDAY_SEQ_H
