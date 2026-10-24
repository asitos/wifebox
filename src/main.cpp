#include "birthday_seq.h"
#include "bitmaps.h"
#include "kitty.h"
#include "songs.h"
#include "web_server.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <ElegantOTA.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <qrcode.h>

// --- PINS ---
#define SDA_PIN 21
#define SCL_PIN 22
#define BUZZER_PIN 26
#define BUZZER_CHANNEL 15
#define BUZZER2_PIN 25
#define BUZZER2_CHANNEL 14
#define SERVO_PIN 13

#define BUTTON1_PIN 4
#define BUTTON2_PIN 16
#define BUTTON3_PIN 17

// --- CONFIGURATION ---
const char *ssid = "Anvesha's M35";
const char *password = "meow1234";

const char *client_id = "60a2edbbba644233b7510aa6c214dffd";
const char *client_secret = "2b1eff5dcdf74fc1876677c290496fea";
const char *refresh_token =
    "AQD1eglb1Wp6HRuUNeU6e5o5sff6ZepFV5xpd48yoe1j_uwVRbtGJvzlDTy6WjnVnDelpc6sC_"
    "0LSq1CETzob_DpJjLceWaxEVEgcoh3N7NskpL5wPodKPEplUOCBWdlDko";

const char *ntfy_outbound_url =
    "https://ntfy.sh/rimjhim_ji_ke_liye_miss_u_button";
const char *ntfy_reply_topic = "rimjhim_ji_ke_liye_reply";

// --- STATE MACHINE & BUTTONS ---
ScreenState currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
uint8_t pixelArtBuffer[128];
unsigned long pixelArtStartTime = 0;
String customNoteMessage = "i missss you!";
// --- WEATHER & CLOCK GLOBALS ---
float weatherTemp = 0.0;
int weatherCode = -1;
bool weatherLoaded = false;
unsigned long lastWeatherFetch = 0;
const unsigned long WEATHER_INTERVAL = 900000; // 15 mins

volatile bool button1Flag = false;
volatile bool button3Flag = false;

volatile bool virtualButton1 = false;
volatile bool virtualButton2_short = false;
volatile bool virtualButton2_long = false;
volatile bool virtualButton3 = false;

unsigned long debounceTime1 = 0;
unsigned long debounceTime2 = 0;
unsigned long debounceTime3 = 0;

void IRAM_ATTR handleButton1() { button1Flag = true; }
void IRAM_ATTR handleButton3() { button3Flag = true; }
volatile bool triggerWebhook = false;
volatile bool triggerAck = false;
volatile bool isAckToggled = false;
unsigned long ackDisplayEndTime = 0;
const char *ntfy_ack_url = "https://ntfy.sh/rimjhim_ji_ke_liye_ack";

volatile bool updateBrightnessFlag = false;
volatile int newBrightness = 128;
volatile bool triggerBirthday = false;
volatile bool triggerSleep = false;
volatile bool triggerWake = false;
volatile int newLocalSongId = -1;
volatile bool stopAudioCmd = false;
volatile bool birthdayMusicActive = false;
volatile unsigned long birthdayMusicStartTime = 0;
String currentMessage = "";

String access_token = "";
unsigned long token_start_time = 0;
const unsigned long TOKEN_EXPIRE = 3500000;
unsigned long last_fetch_time = 0;
const unsigned long FETCH_DELAY = 5000;

String currentSong = "";
String currentArtist = "";
String currentAlbum = "";
bool is_playing = false;
long serverPositionMs = 0;
long durationMs = 1;
unsigned long fetchedAt = 0;

long getPositionMs() {
  if (!is_playing)
    return serverPositionMs;
  long local = serverPositionMs + (long)(millis() - fetchedAt);
  return min(local, durationMs);
}

// ════════════════════════════════════════════════════════
//  BITMAPS & UI
// ════════════════════════════════════════════════════════
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

const unsigned char spotify_10[] U8X8_PROGMEM = {
    0x3C, 0x00, 0x7E, 0x00, 0x81, 0x00, 0xFF, 0x00, 0xC3, 0x00,
    0xFF, 0x00, 0xE7, 0x00, 0xFF, 0x00, 0x7E, 0x00, 0x3C, 0x00};

const unsigned char logo_spotify_32[] U8X8_PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x1F, 0x00, 0x00, 0xFF, 0xFF, 0x00,
    0xC0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x07, 0xF0, 0xFF, 0xFF, 0x0F,
    0xF8, 0xFF, 0xFF, 0x1F, 0xFC, 0xFF, 0xFF, 0x3F, 0xFC, 0x07, 0x80, 0x3F,
    0xFE, 0x01, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1F, 0xF8, 0xFF,
    0xFF, 0x07, 0xE0, 0xFF, 0xFF, 0x01, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFE, 0xFF, 0xFF, 0x7F, 0xFE, 0x3F, 0xFC, 0x7F, 0xFC, 0x0F, 0xE0, 0x3F,
    0xFC, 0xFF, 0xFF, 0x3F, 0xF8, 0xFF, 0xFF, 0x1F, 0xF0, 0xFF, 0xFF, 0x0F,
    0xE0, 0xFF, 0xFF, 0x07, 0xC0, 0xFF, 0xFF, 0x03, 0x00, 0xFF, 0xFF, 0x00,
    0x00, 0xF8, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00};

// ════════════════════════════════════════════════════════
//  EQ BARS
// ════════════════════════════════════════════════════════
#define EQ_N 5
#define EQ_BAR_W 4
#define EQ_GAP 2
#define EQ_X 100
#define EQ_MAX_H 13

int eqH[EQ_N] = {3, 6, 10, 5, 12};
int eqTgt[EQ_N] = {3, 6, 10, 5, 12};
unsigned long lastEqTick = 0;

void eqUpdateTargets() {
  for (int i = 0; i < EQ_N; i++)
    eqTgt[i] = is_playing ? random(4, EQ_MAX_H + 1) : random(1, 3);
}

void eqSmooth() {
  for (int i = 0; i < EQ_N; i++) {
    if (eqH[i] < eqTgt[i])
      eqH[i]++;
    else if (eqH[i] > eqTgt[i])
      eqH[i]--;
  }
}

// ════════════════════════════════════════════════════════
//  SCROLL
// ════════════════════════════════════════════════════════
int titleX = 130;
int titlePause = 40;
unsigned long lastScroll = 0;

void updateScroll() {
  u8g2.setFont(u8g2_font_7x13B_tr);
  int tw = u8g2.getUTF8Width(currentSong.c_str());
  if (tw <= 128) {
    titleX = 0;
  } else {
    if (titlePause > 0)
      titlePause--;
    else {
      titleX--;
      if (titleX < -(tw + 12)) {
        titleX = 130;
        titlePause = 40;
      } else if (titleX == 0)
        titlePause = 40;
    }
  }
}

String clipStr(const String &s, int maxPx) {
  if (u8g2.getUTF8Width(s.c_str()) <= maxPx)
    return s;
  String out = s;
  while (out.length() > 0 && u8g2.getUTF8Width((out + "...").c_str()) > maxPx)
    out.remove(out.length() - 1);
  return out + "...";
}

void drawCenterText(const char *text, int y) {
  int w = u8g2.getUTF8Width(text);
  u8g2.drawStr((128 - w) / 2, y, text);
}

// ════════════════════════════════════════════════════════
//  ANIMASI MATA
// ════════════════════════════════════════════════════════
#define L_CX 35
#define R_CX 93
#define EYE_CY 29
#define BASE_RX 13
#define BASE_RY 12

float lOpenR = 1.0f, rOpenR = 1.0f;
float tLOpenR = 1.0f, tROpenR = 1.0f;
float lRx = BASE_RX, lRy = BASE_RY;
float tLRx = BASE_RX, tLRy = BASE_RY;
float rRx = BASE_RX, rRy = BASE_RY;
float tRRx = BASE_RX, tRRy = BASE_RY;
float pupX = 0.0f, pupY = 0.0f;
float tPupX = 0.0f, tPupY = 0.0f;
float lBH = 3.0f, rBH = 3.0f;
float tLBH = 3.0f, tRBH = 3.0f;
float lBT = 0.0f, rBT = 0.0f;
float tLBT = 0.0f, tRBT = 0.0f;
bool lHappy = false, rHappy = false;
unsigned long nextEyeChange = 0;

void drawEye(int cx, int cy, float rxF, float ryF, float openR, float px,
             float py, bool happy) {
  int rx = constrain((int)rxF, 3, 18);
  int ry = constrain((int)ryF, 2, 16);

  if (happy) {
    int hapRy = ry + 4;
    u8g2.setDrawColor(1);
    u8g2.drawFilledEllipse(cx, cy, rx, hapRy);
    u8g2.setDrawColor(0);
    u8g2.drawBox(cx - rx - 1, cy, rx * 2 + 2, hapRy + 2);
    u8g2.setDrawColor(1);
    u8g2.drawPixel(cx - 4, cy - (hapRy * 2 / 3));
    u8g2.drawPixel(cx + 5, cy - (hapRy / 2));
    return;
  }

  int effRy = max(1, (int)((float)ry * openR));
  if (effRy <= 2) {
    u8g2.setDrawColor(1);
    int lineW = (int)((float)rx * 1.3f);
    u8g2.drawHLine(cx - lineW / 2, cy, lineW);
    u8g2.drawHLine(cx - lineW / 2, cy + 1, lineW);
    return;
  }

  u8g2.setDrawColor(1);
  u8g2.drawFilledEllipse(cx, cy, rx, effRy);

  int pr = max(2, (rx + effRy) / 5);
  int pcx = constrain(cx + (int)px, cx - rx + pr + 2, cx + rx - pr - 2);
  int pcy = constrain(cy + (int)py, cy - effRy + pr + 2, cy + effRy - pr - 2);

  u8g2.setDrawColor(0);
  u8g2.drawDisc(pcx, pcy, pr);
  u8g2.setDrawColor(1);
  if (pr >= 3)
    u8g2.drawPixel(pcx - 1, pcy - 1);
  if (pr >= 4)
    u8g2.drawPixel(pcx - 2, pcy - 2);
  u8g2.setDrawColor(1);
}

void drawBrow(int cx, int eyeTopY, int rx, float bh, float bt, bool isLeft) {
  int baseY = constrain(eyeTopY - 2 - (int)bh, 1, 58);
  int hw = rx + 1;
  int td = constrain((int)(bt * 3.5f), -9, 9);

  int xl = cx - hw, xr = cx + hw;
  int yl = isLeft ? baseY : (baseY - td);
  int yr = isLeft ? (baseY - td) : baseY;
  yl = constrain(yl, 1, 62);
  yr = constrain(yr, 1, 62);

  u8g2.setDrawColor(1);
  u8g2.drawLine(xl, yl, xr, yr);
  u8g2.drawLine(xl + 2, yl - 1, xr - 2, yr - 1);

  int xm1 = cx - hw / 2, xm2 = cx + hw / 2;
  float tl = (float)(xm1 - xl) / (float)(xr - xl);
  float tr = (float)(xm2 - xl) / (float)(xr - xl);
  int ym1 = constrain(yl + (int)((yr - yl) * tl) - 2, 1, 62);
  int ym2 = constrain(yl + (int)((yr - yl) * tr) - 2, 1, 62);
  u8g2.drawLine(xm1, ym1, xm2, ym2);
}

void updateIdleLogic() {
  unsigned long now = millis();

  if (now > nextEyeChange) {
    lHappy = false;
    rHappy = false;
    int r = random(100);

    if (r < 8) { // KEDIP
      lRx = BASE_RX;
      lRy = BASE_RY;
      rRx = BASE_RX;
      rRy = BASE_RY;
      lOpenR = 0.0f;
      rOpenR = 0.0f;
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tPupX = 0.0f;
      tPupY = 0.0f;
      tLBH = 3.0f;
      tRBH = 3.0f;
      tLBT = 0.0f;
      tRBT = 0.0f;
      nextEyeChange = now + random(1500, 3000);
    } else if (r < 15) { // WINK
      if (random(2) == 0) {
        lRx = BASE_RX;
        lRy = BASE_RY;
        lOpenR = 0.0f;
        tLOpenR = 1.0f;
        tROpenR = 1.0f;
      } else {
        rRx = BASE_RX;
        rRy = BASE_RY;
        rOpenR = 0.0f;
        tROpenR = 1.0f;
        tLOpenR = 1.0f;
      }
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tPupX = 0.0f;
      tPupY = 0.0f;
      tLBH = 3.0f;
      tRBH = 3.0f;
      tLBT = 0.0f;
      tRBT = 0.0f;
      nextEyeChange = now + random(700, 1500);
    } else if (r < 28) { // LIRIK KIRI
      tPupX = -5.0f;
      tPupY = 0.0f;
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX - 3;
      tRRy = BASE_RY;
      tLBH = 3.0f;
      tRBH = 3.0f;
      tLBT = 0.0f;
      tRBT = 0.0f;
      nextEyeChange = now + random(900, 2000);
    } else if (r < 41) { // LIRIK KANAN
      tPupX = 5.0f;
      tPupY = 0.0f;
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX - 3;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tLBH = 3.0f;
      tRBH = 3.0f;
      tLBT = 0.0f;
      tRBT = 0.0f;
      nextEyeChange = now + random(900, 2000);
    } else if (r < 53) { // BERPIKIR
      tPupX = 0.0f;
      tPupY = -3.5f;
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tLBH = 7.0f;
      tRBH = 7.0f;
      tLBT = 1.5f;
      tRBT = 1.5f;
      nextEyeChange = now + random(1200, 2500);
    } else if (r < 63) { // NGANTUK
      tPupX = 0.0f;
      tPupY = 2.5f;
      tLOpenR = 0.35f;
      tROpenR = 0.35f;
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tLBH = 1.5f;
      tRBH = 1.5f;
      tLBT = -1.2f;
      tRBT = -1.2f;
      nextEyeChange = now + random(2000, 4000);
    } else if (r < 72) { // KAGET
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX + 3;
      tLRy = BASE_RY + 3;
      tRRx = BASE_RX + 3;
      tRRy = BASE_RY + 3;
      tPupX = 0.0f;
      tPupY = 0.0f;
      tLBH = 9.0f;
      tRBH = 9.0f;
      tLBT = 1.8f;
      tRBT = 1.8f;
      nextEyeChange = now + random(600, 1400);
    } else if (r < 81) { // SENANG
      lHappy = true;
      rHappy = true;
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tPupX = 0.0f;
      tPupY = 0.0f;
      tLBH = 5.0f;
      tRBH = 5.0f;
      tLBT = 1.5f;
      tRBT = 1.5f;
      nextEyeChange = now + random(1500, 3000);
    } else if (r < 90) { // BINGUNG
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tPupX = 0.0f;
      tPupY = 0.0f;
      if (random(2) == 0) {
        tLRx = BASE_RX + 3;
        tLRy = BASE_RY + 2;
        tRRx = BASE_RX - 4;
        tRRy = BASE_RY - 3;
        tLBH = 8.0f;
        tRBH = 2.0f;
        tLBT = 1.5f;
        tRBT = 0.0f;
      } else {
        tLRx = BASE_RX - 4;
        tLRy = BASE_RY - 3;
        tRRx = BASE_RX + 3;
        tRRy = BASE_RY + 2;
        tLBH = 2.0f;
        tRBH = 8.0f;
        tLBT = 0.0f;
        tRBT = 1.5f;
      }
      nextEyeChange = now + random(1500, 2500);
    } else { // NORMAL
      tPupX = 0.0f;
      tPupY = 0.0f;
      tLOpenR = 1.0f;
      tROpenR = 1.0f;
      tLRx = BASE_RX;
      tLRy = BASE_RY;
      tRRx = BASE_RX;
      tRRy = BASE_RY;
      tLBH = 3.0f;
      tRBH = 3.0f;
      tLBT = 0.0f;
      tRBT = 0.0f;
      nextEyeChange = now + random(1000, 2500);
    }
  }

  // Easing
  lOpenR += (tLOpenR - lOpenR) * 0.20f;
  rOpenR += (tROpenR - rOpenR) * 0.20f;
  lRx += (tLRx - lRx) * 0.22f;
  lRy += (tLRy - lRy) * 0.22f;
  rRx += (tRRx - rRx) * 0.22f;
  rRy += (tRRy - rRy) * 0.22f;
  pupX += (tPupX - pupX) * 0.20f;
  pupY += (tPupY - pupY) * 0.20f;
  lBH += (tLBH - lBH) * 0.15f;
  rBH += (tRBH - rBH) * 0.15f;
  lBT += (tLBT - lBT) * 0.15f;
  rBT += (tRBT - rBT) * 0.15f;
}

void drawIdle() {
  u8g2.clearBuffer();
  updateIdleLogic();

  int lTopY = EYE_CY - (int)lRy;
  int rTopY = EYE_CY - (int)rRy;
  drawBrow(L_CX, lTopY, (int)lRx, lBH, lBT, true);
  drawBrow(R_CX, rTopY, (int)rRx, rBH, rBT, false);
  drawEye(L_CX, EYE_CY, lRx, lRy, lOpenR, pupX, pupY, lHappy);
  drawEye(R_CX, EYE_CY, rRx, rRy, rOpenR, pupX, pupY, rHappy);

  if (lOpenR < 0.50f) {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(63, EYE_CY + 2, "z");
    u8g2.drawStr(68, EYE_CY - 5, "z");
    u8g2.drawStr(74, EYE_CY - 13, "Z");
  }

  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_4x6_tr);
  drawCenterText("wife is sleeping", 62);
  u8g2.drawHLine(4, 58, 18);
  u8g2.drawHLine(106, 58, 18);

  u8g2.sendBuffer();
}

// ════════════════════════════════════════════════════════
//  BIRTHDAY & NOTES GLOBALS
// ════════════════════════════════════════════════════════

unsigned long lastDrawTime = 0;

// ════════════════════════════════════════════════════════
//  SERVO & NTFY GLOBALS
// ════════════════════════════════════════════════════════
Servo myServo;
WiFiClientSecure ntfyStreamClient;
unsigned long lastNtfyReconnectAttempt = 0;

volatile ScreenState pendingMacroScreen = SCREEN_IDLE_WEATHER_CLOCK;
volatile bool pendingIsAlert = false;

volatile bool triggerFlagWave = false;
volatile int newServoAngle = -1; // -1 means no update
volatile bool triggerServoSweep = false;
volatile bool triggerServoHeartbeat = false;

// Servo State Machine
enum ServoMode { SERVO_IDLE, SERVO_WIGGLE, SERVO_SWEEP, SERVO_HEARTBEAT };
ServoMode currentServoMode = SERVO_IDLE;
int currentServoAngle = 0;
unsigned long lastServoMoveTime = 0;
int servoStep = 0;
int servoDir = 1;

WiFiClientSecure secureClient;
String ntfy_reply_message = "i misssss u more"; // Default fallback

// ==========================================
//                 SPOTIFY
// ==========================================
void refreshAccessToken() {
  HTTPClient http;
  http.begin(secureClient, "https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload =
      "grant_type=refresh_token&refresh_token=" + String(refresh_token) +
      "&client_id=" + String(client_id) +
      "&client_secret=" + String(client_secret);

  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument doc;
    deserializeJson(doc, response);
    access_token = doc["access_token"].as<String>();
    token_start_time = millis();
  }
  http.end();
}

void fetchCurrentlyPlaying() {
  if (access_token == "")
    return;

  HTTPClient http;
  http.begin(secureClient,
             "https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + access_token);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String response = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    if (!error && doc["is_playing"] == true) {
      is_playing = true;
      String newSong = doc["item"]["name"].as<String>();
      String newArtist = doc["item"]["artists"][0]["name"].as<String>();
      String newAlbum = doc["item"]["album"]["name"].as<String>();

      if (newSong != currentSong) {
        currentSong = newSong;
        currentArtist = newArtist;
        currentAlbum = newAlbum;
        titleX = 130;
        titlePause = 40;
      }
      durationMs = doc["item"]["duration_ms"] | 1;
      serverPositionMs = doc["progress_ms"] | 0;
      fetchedAt = millis();
    } else {
      is_playing = false;
    }
  } else if (httpCode == 204 || httpCode == 401) {
    is_playing = false;
    if (httpCode == 401)
      access_token = "";
  }
  http.end();
}

void renderSpotify() {
  if (!is_playing || currentSong.isEmpty()) {
    drawIdle();
    return;
  }

  u8g2.clearBuffer();

  // Header with EQ
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(2, 9, "ur wife is listening 2");
  for (int i = 0; i < EQ_N; i++) {
    int bh = constrain(eqH[i], 1, EQ_MAX_H);
    u8g2.drawBox(EQ_X + i * (EQ_BAR_W + EQ_GAP), 13 - bh, EQ_BAR_W, bh);
  }
  u8g2.drawHLine(0, 14, 128);

  // Judul scroll
  u8g2.setFont(u8g2_font_7x13B_tr);
  u8g2.setClipWindow(0, 15, 127, 32);
  u8g2.drawStr(titleX, 29, currentSong.c_str());
  u8g2.setMaxClipWindow();

  // Artist
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setClipWindow(0, 33, 127, 43);
  u8g2.drawStr(0, 41, clipStr(currentArtist, 128).c_str());
  u8g2.setMaxClipWindow();

  // Album
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setClipWindow(0, 44, 127, 53);
  u8g2.drawStr(0, 51, clipStr(currentAlbum, 128).c_str());
  u8g2.setMaxClipWindow();

  // Hitung posisi & durasi (Scrubber)
  long posMs = getPositionMs();
  char tPos[8], tDur[8];
  {
    int s = (int)(posMs / 1000), m = s / 60;
    s %= 60;
    sprintf(tPos, "%d:%02d", m, s);
  }
  {
    int s = (int)(durationMs / 1000), m = s / 60;
    s %= 60;
    sprintf(tDur, "%d:%02d", m, s);
  }

  int posW = strlen(tPos) * 4;
  int durW = strlen(tDur) * 4;

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(0, 62, tPos);
  u8g2.drawStr(128 - durW, 62, tDur);

  int barX = posW + 3;
  int barE = 128 - durW - 3;
  int barW = barE - barX;

  if (barW > 4) {
    int pw = (durationMs > 0) ? map(posMs, 0, durationMs, 0, barW) : 0;
    pw = constrain(pw, 0, barW);
    u8g2.drawHLine(barX, 59, barW);
    if (pw > 0)
      u8g2.drawBox(barX, 58, pw, 3);
    u8g2.drawDisc(barX + pw, 59, 2);
  }

  u8g2.sendBuffer();
}

volatile bool localMusicPlaying = false;
volatile int currentLocalSongIndex = 0;
volatile int currentNoteIndex = 0;
volatile bool isChangingTrack = false;
volatile unsigned long trackChangeEndTime = 0;
volatile bool isPlayToggled = false;
volatile unsigned long playToggleEndTime = 0;

void renderLocalMusic() {
  u8g2.clearBuffer();

  // Title
  u8g2.setFont(u8g2_font_helvB08_tr);
  const char *songName = localSongs[currentLocalSongIndex].name;
  int w = u8g2.getStrWidth(songName);
  u8g2.drawStr((128 - w) / 2, 16, songName);

  // Artist
  u8g2.setFont(u8g2_font_5x7_tr);
  const char *artistName = localSongs[currentLocalSongIndex].artist;
  drawCenterText(artistName, 26);

  // Clear toggle flag if time expired
  if (isPlayToggled && millis() > playToggleEndTime) {
    isPlayToggled = false;
  }

  // UI Controls (Generic Player)
  const char *playText = localMusicPlaying ? "[|| PAUSE]" : "[> PLAY]";
  int playW = u8g2.getStrWidth(playText);

  const char *nextText = "[>| NEXT]";
  int nextW = u8g2.getStrWidth(nextText);

  int gap = 6; // px
  int totalW = playW + gap + nextW;
  int startX = (128 - totalW) / 2;

  if (isPlayToggled) {
    u8g2.drawBox(startX - 2, 46, playW + 4, 10);
    u8g2.setDrawColor(0);
    u8g2.drawStr(startX, 54, playText);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(startX, 54, playText);
  }

  int nextX = startX + playW + gap;
  if (isChangingTrack) {
    u8g2.drawBox(nextX - 2, 46, nextW + 4, 10);
    u8g2.setDrawColor(0);
    u8g2.drawStr(nextX, 54, nextText);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(nextX, 54, nextText);
  }

  // Simple progress bar mock
  u8g2.drawFrame(10, 34, 108, 6);
  int progress =
      (currentNoteIndex * 108) / localSongs[currentLocalSongIndex].length;
  if (progress > 0) {
    u8g2.drawBox(10, 34, progress, 6);
  }

  u8g2.sendBuffer();
}

struct HbdNote {
  uint16_t f1;   // melody freq (Hz)
  uint16_t f2;   // harmony freq (Hz)
  uint16_t play; // play duration (ms)
  uint16_t rest; // silence gap (ms)
};

static const HbdNote hbd_song[25] = {
    {392, 330, 210, 40},  // G4 + E4   "Hap-"
    {392, 330, 210, 40},  // G4 + E4   "-py"
    {440, 349, 440, 60},  // A4 + F4   "Birth-"
    {392, 330, 440, 60},  // G4 + E4   "-day"
    {523, 392, 440, 60},  // C5 + G4   "to"
    {494, 392, 880, 120}, // B4 + G4   "you"

    {392, 330, 210, 40},  // G4 + E4   "Hap-"
    {392, 330, 210, 40},  // G4 + E4   "-py"
    {440, 349, 440, 60},  // A4 + F4   "Birth-"
    {392, 330, 440, 60},  // G4 + E4   "-day"
    {587, 349, 440, 60},  // D5 + F4   "to"
    {523, 330, 880, 120}, // C5 + E4   "you"

    {392, 330, 210, 40},  // G4 + E4   "Hap-"
    {392, 330, 210, 40},  // G4 + E4   "-py"
    {784, 659, 440, 60},  // G5 + E5   "Birth-"
    {659, 523, 440, 60},  // E5 + C5   "-day"
    {523, 392, 440, 60},  // C5 + G4   "dear"
    {494, 392, 440, 60},  // B4 + G4   "wife"
    {440, 349, 880, 120}, // A4 + F4   "<3"

    {698, 587, 210, 40},  // F5 + D5   "Hap-"
    {698, 587, 210, 40},  // F5 + D5   "-py"
    {659, 523, 440, 60},  // E5 + C5   "Birth-"
    {523, 392, 440, 60},  // C5 + G4   "-day"
    {587, 392, 440, 60},  // D5 + G4   "to"
    {523, 330, 1000, 150} // C5 + E4   "you!"
};

void buzzerTask(void *pvParameters) {
  unsigned long nextTime1 = 0;
  unsigned long nextTime2 = 0;
  int idx1 = 0, idx2 = 0;
  int state1 = 1, state2 = 1; // 0=playing, 1=delaying
  int lastSongIndex = -1;
  int lastHbdF1 = -1;
  int lastHbdF2 = -1;

  for (;;) {
    if (birthdayMusicActive) {
      unsigned long bElapsed = millis() - birthdayMusicStartTime;
      int targetF1 = 0;
      int targetF2 = 0;

      if (bElapsed < 120) {
        // Fast party horn slide 800 -> 1400 Hz
        targetF1 = 800 + ((int)bElapsed * 600) / 120;
        targetF2 = 0;
      } else if (bElapsed < 180) {
        targetF1 = 1400;
        targetF2 = 0;
      } else if (bElapsed < 4500) {
        // Firework pop sound at 3000ms
        if (bElapsed >= 3000 && bElapsed < 3050) {
          targetF1 = 1800;
          targetF2 = 1200;
        } else {
          targetF1 = 0;
          targetF2 = 0;
        }
      } else if (bElapsed >= 4500 && bElapsed < 23200) {
        // 2-part harmony Happy Birthday
        unsigned long songElapsed = bElapsed - 4500;
        unsigned long noteAccum = 0;
        int activeIdx = -1;
        bool inRest = false;

        for (int i = 0; i < 25; i++) {
          unsigned long noteTotal =
              (unsigned long)hbd_song[i].play + hbd_song[i].rest;
          if (songElapsed < noteAccum + noteTotal) {
            activeIdx = i;
            if (songElapsed >= noteAccum + hbd_song[i].play) {
              inRest = true;
            }
            break;
          }
          noteAccum += noteTotal;
        }

        if (activeIdx != -1 && !inRest) {
          targetF1 = hbd_song[activeIdx].f1;
          targetF2 = hbd_song[activeIdx].f2;
        } else {
          targetF1 = 0;
          targetF2 = 0;
        }
      } else if (bElapsed >= 23200 && bElapsed < 24800) {
        // Triumphant Fanfare Tag: C5 -> E5 -> G5 -> C6+G5 chord
        unsigned long fanfareElapsed = bElapsed - 23200;
        if (fanfareElapsed < 350) {
          targetF1 = 523; // C5
          targetF2 = 330; // E4
        } else if (fanfareElapsed < 700) {
          targetF1 = 659; // E5
          targetF2 = 392; // G4
        } else if (fanfareElapsed < 1050) {
          targetF1 = 784; // G5
          targetF2 = 523; // C5
        } else {
          targetF1 = 1047; // C6
          targetF2 = 784;  // G5
        }
      } else {
        targetF1 = 0;
        targetF2 = 0;
      }

      if (targetF1 != lastHbdF1) {
        ledcWriteTone(BUZZER_CHANNEL, targetF1);
        lastHbdF1 = targetF1;
      }
      if (targetF2 != lastHbdF2) {
        ledcWriteTone(BUZZER2_CHANNEL, targetF2);
        lastHbdF2 = targetF2;
      }

      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }

    lastHbdF1 = -1;
    lastHbdF2 = -1;

    if (isChangingTrack) {
      ledcWriteTone(BUZZER_CHANNEL, 0);
      ledcWriteTone(BUZZER2_CHANNEL, 0);
      idx1 = 0;
      idx2 = 0;
      state1 = 1;
      state2 = 1;
      if (millis() > trackChangeEndTime) {
        isChangingTrack = false;
        lastSongIndex = -1; // force reload
      }
      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }

    if (localMusicPlaying) {
      Melody m = localSongs[currentLocalSongIndex];

      if (lastSongIndex != currentLocalSongIndex) {
        lastSongIndex = currentLocalSongIndex;
        idx1 = 0;
        idx2 = 0;
        state1 = 1;
        state2 = 1;
        nextTime1 = millis();
        nextTime2 = millis();
      }

      unsigned long now = millis();

      // Track 1
      if (now >= nextTime1) {
        if (state1 == 0) {
          ledcWriteTone(BUZZER_CHANNEL, 0);
          state1 = 1;
          nextTime1 += m.notes[idx1].delayAfter;
          if (m.notes[idx1].delayAfter == 0)
            now = nextTime1; // force immediate eval
        }

        if (state1 == 1 && now >= nextTime1) {
          idx1++;
          if (idx1 >= m.length) {
            currentLocalSongIndex =
                (currentLocalSongIndex + 1) % NUM_LOCAL_SONGS;
            currentNoteIndex = 0;
            isChangingTrack = true;
            trackChangeEndTime = millis() + 1000; // wait 1s
          }
          currentNoteIndex = idx1;

          Note n = m.notes[idx1];
          if (m.notes2 != nullptr) {
            // Duet mode
            if (n.freq > 0)
              ledcWriteTone(BUZZER_CHANNEL, n.freq);
            else
              ledcWriteTone(BUZZER_CHANNEL, 0);
          } else {
            // Alternate mode
            if (n.freq > 0) {
              if (idx1 % 2 == 0) {
                ledcWriteTone(BUZZER_CHANNEL, n.freq);
                ledcWriteTone(BUZZER2_CHANNEL, 0);
              } else {
                ledcWriteTone(BUZZER_CHANNEL, 0);
                ledcWriteTone(BUZZER2_CHANNEL, n.freq);
              }
            } else {
              ledcWriteTone(BUZZER_CHANNEL, 0);
              ledcWriteTone(BUZZER2_CHANNEL, 0);
            }
          }
          state1 = 0;
          nextTime1 += n.duration;
        }
      }

      // Track 2
      if (m.notes2 != nullptr) {
        if (now >= nextTime2) {
          if (state2 == 0) {
            ledcWriteTone(BUZZER2_CHANNEL, 0);
            state2 = 1;
            nextTime2 += m.notes2[idx2].delayAfter;
            if (m.notes2[idx2].delayAfter == 0)
              now = nextTime2;
          }

          if (state2 == 1 && now >= nextTime2) {
            idx2++;
            if (idx2 >= m.length2) {
              idx2 = m.length2 - 1;
              nextTime2 = millis() + 10000; // sleep until track change
            }

            Note n = m.notes2[idx2];
            if (n.freq > 0)
              ledcWriteTone(BUZZER2_CHANNEL, n.freq);
            else
              ledcWriteTone(BUZZER2_CHANNEL, 0);

            state2 = 0;
            nextTime2 += n.duration;
          }
        }
      } else {
        // Handled by alternating logic
      }

      vTaskDelay(1 / portTICK_PERIOD_MS);
    } else {
      if (!birthdayMusicActive && currentScreen != SCREEN_KITTY) {
        ledcWriteTone(BUZZER_CHANNEL, 0);
        ledcWriteTone(BUZZER2_CHANNEL, 0);
      }
      lastSongIndex = -1;
      vTaskDelay(50 / portTICK_PERIOD_MS);
    }
  }
}

String getStoredWiFiSSID() {
  wifi_config_t conf;
  memset(&conf, 0, sizeof(conf));
  esp_wifi_get_config(WIFI_IF_STA, &conf);
  return String(reinterpret_cast<const char *>(conf.sta.ssid));
}

// ==========================================
//            NETWORK TASK (CORE 0)
// ==========================================
void networkTask(void *pvParameters) {
  static unsigned long lastWifiScanAttempt = 0;
  static bool isWifiScanning = false;
  static bool wasDisconnected = false;
  static String savedSSID = "";
  static String savedPass = "";

  for (;;) {
    unsigned long now = millis();

    if (WiFi.status() == WL_CONNECTED) {
      if (WiFi.SSID().length() > 0) {
        savedSSID = WiFi.SSID();
        savedPass = WiFi.psk();
      }
      if (isWifiScanning) {
        WiFi.scanDelete();
        isWifiScanning = false;
      }
      lastWifiScanAttempt = now;

      if (wasDisconnected) {
        wasDisconnected = false;
        configTime(19800, 0, "pool.ntp.org");
        weatherLoaded = false;
        Serial.println("[WiFi] Reconnected to hotspot!");
      }

      // Weather Fetch
      if (!weatherLoaded || millis() - lastWeatherFetch > WEATHER_INTERVAL) {
        HTTPClient http;
        String url =
            "http://api.open-meteo.com/v1/"
            "forecast?latitude=30.73&longitude=76.78&current_weather=true";
        http.begin(url);
        int httpCode = http.GET();
        if (httpCode == 200) {
          JsonDocument doc;
          deserializeJson(doc, http.getString());
          weatherTemp = doc["current_weather"]["temperature"];
          weatherCode = doc["current_weather"]["weathercode"];
          weatherLoaded = true;
          lastWeatherFetch = millis();
        }
        http.end();
      }

      // 1. Ntfy JSON Stream Connection
      if (!ntfyStreamClient.connected()) {
        if (millis() - lastNtfyReconnectAttempt > 5000) {
          lastNtfyReconnectAttempt = millis();
          if (ntfyStreamClient.connect("ntfy.sh", 443)) {
            String request = String("GET /") + ntfy_reply_topic +
                             "/json HTTP/1.1\r\n" + "Host: ntfy.sh\r\n" +
                             "Connection: keep-alive\r\n\r\n";
            ntfyStreamClient.print(request);
          }
        }
      } else {
        while (ntfyStreamClient.available()) {
          String line = ntfyStreamClient.readStringUntil('\n');
          line.trim();
          if (line.startsWith("{")) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, line);
            if (!error && doc["event"] == "message") {
              String msg = doc["message"].as<String>();
              if (msg.startsWith("brightness:")) {
                String valStr = msg.substring(11);
                newBrightness = valStr.toInt();
                updateBrightnessFlag = true;
              } else if (msg.startsWith("servo:")) {
                String valStr = msg.substring(6);
                newServoAngle = valStr.toInt();
              } else if (msg.startsWith("servo_cmd:sweep")) {
                triggerServoSweep = true;
              } else if (msg.startsWith("servo_cmd:heartbeat")) {
                triggerServoHeartbeat = true;
              } else if (msg.startsWith("cmd:nudge")) {
                triggerFlagWave = true;
              } else if (msg.startsWith("cmd:birthday")) {
                triggerBirthday = true;
              } else if (msg.startsWith("cmd:sleep")) {
                triggerSleep = true;
              } else if (msg.startsWith("cmd:wake")) {
                triggerWake = true;
              } else if (msg.startsWith("cmd:kitty")) {
                currentScreen = SCREEN_KITTY;
              } else if (msg.startsWith("kitty:feed")) {
                currentScreen = SCREEN_KITTY;
                feedKitty();
              } else if (msg.startsWith("kitty:pet")) {
                currentScreen = SCREEN_KITTY;
                petKitty();
              } else if (msg.startsWith("kitty:sleep")) {
                currentScreen = SCREEN_KITTY;
                toggleKittySleep();
              } else if (msg.startsWith("kitty:exit")) {
                exitKitty();
              } else if (msg.startsWith("screen:")) {
                String s = msg.substring(7);
                if (currentScreen == SCREEN_KITTY && s != "kitty") {
                  exitKitty();
                }
                if (s == "clock" || s == "weather")
                  currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
                else if (s == "birthday") {
                  triggerBirthday = true;
                } else if (s == "music")
                  currentScreen = SCREEN_LOCAL_MUSIC;
                else if (s == "spotify")
                  currentScreen = SCREEN_SPOTIFY_NOW_PLAYING;
                else if (s == "doodle" || s == "pixel_art") {
                  currentScreen = SCREEN_PIXEL_ART;
                  pixelArtStartTime = millis();
                } else if (s == "photo" || s == "wife_photo")
                  currentScreen = SCREEN_WIFE_PHOTO;
                else if (s == "kitty")
                  currentScreen = SCREEN_KITTY;
                u8g2.clearBuffer();
              } else if (msg.startsWith("cmd:btn1")) {
                virtualButton1 = true;
              } else if (msg.startsWith("cmd:btn2_short")) {
                virtualButton2_short = true;
              } else if (msg.startsWith("cmd:btn2_long")) {
                virtualButton2_long = true;
              } else if (msg.startsWith("cmd:btn3")) {
                virtualButton3 = true;
              } else if (msg.startsWith("play_song:")) {
                String idStr = msg.substring(10);
                newLocalSongId = idStr.toInt();
              } else if (msg.startsWith("cmd:stop_audio")) {
                stopAudioCmd = true;
              } else if (msg.startsWith("draw:")) {
                String hexStr = msg.substring(5);
                int idx = 0;
                int strIndex = 0;
                while (idx < 128 && strIndex < hexStr.length()) {
                  int comma = hexStr.indexOf(',', strIndex);
                  if (comma == -1)
                    comma = hexStr.length();
                  String byteStr = hexStr.substring(strIndex, comma);
                  pixelArtBuffer[idx++] =
                      (uint8_t)strtol(byteStr.c_str(), NULL, 16);
                  strIndex = comma + 1;
                }
                currentScreen = SCREEN_PIXEL_ART;
                pixelArtStartTime = millis();
                triggerFlagWave = true; // Alert notif for doodle
              } else if (msg.startsWith("macro:")) {
                String macroName = msg.substring(6);
                if (macroName == "hug")
                  pendingMacroScreen = SCREEN_HUG;
                else if (macroName == "headpat")
                  pendingMacroScreen = SCREEN_HEADPAT;
                else if (macroName == "kiss")
                  pendingMacroScreen = SCREEN_KISS;
                else if (macroName == "hand")
                  pendingMacroScreen = SCREEN_HAND;
                else if (macroName == "workout")
                  pendingMacroScreen = SCREEN_WORKOUT;
                else if (macroName == "freaky")
                  pendingMacroScreen = SCREEN_FREAKY;
                else if (macroName == "food")
                  pendingMacroScreen = SCREEN_FOOD;
                else if (macroName == "study")
                  pendingMacroScreen = SCREEN_STUDY;
                else if (macroName == "wakeup")
                  pendingMacroScreen = SCREEN_WAKEUP;
                else if (macroName == "alert")
                  pendingIsAlert = true;
              } else if (msg.startsWith("msg:")) {
                currentMessage = msg.substring(4);
                ntfy_reply_message = currentMessage;
                currentScreen =
                    SCREEN_IDLE_WEATHER_CLOCK; // Force text to show over doodle
                triggerFlagWave = true;
              } else if (msg.length() > 0) {
                currentMessage = msg;
                ntfy_reply_message = currentMessage;
                currentScreen =
                    SCREEN_IDLE_WEATHER_CLOCK; // Force text to show over doodle
                triggerFlagWave = true;
              }
            }
          }
        }
      }

      // 2. Spotify Polling
      if (access_token == "" || millis() - token_start_time > TOKEN_EXPIRE) {

        refreshAccessToken();

        configTime(19800, 0, "pool.ntp.org");
      }
      if (millis() - last_fetch_time > FETCH_DELAY) {
        fetchCurrentlyPlaying();
        last_fetch_time = millis();
      }

      // 3. Webhook Triggering (Ntfy Outbound)
      if (triggerAck) {
        triggerAck = false;
        HTTPClient http;
        http.begin(secureClient, ntfy_ack_url);
        http.addHeader("Content-Type", "text/plain");
        http.POST("she sent a hug back! :3");
        http.end();
      }

      // 3. Webhook Triggering (Ntfy Outbound)
      if (triggerWebhook) {
        triggerWebhook = false;
        HTTPClient http;
        http.begin(secureClient, ntfy_outbound_url);
        http.addHeader("Actions", "http, I misssss u more,"
                                  "https://ntfy.sh/rimjhim_ji_ke_liye_reply, "
                                  "method=POST, body=i misssss u more");
        http.addHeader("Content-Type", "text/plain");
        int httpCode = http.POST(customNoteMessage);
        http.end();
      }
    } else {
      wasDisconnected = true;
      if (ntfyStreamClient.connected()) {
        ntfyStreamClient.stop();
      }

      String storedSSID = getStoredWiFiSSID();
      String targetSSID = (storedSSID.length() > 0) ? storedSSID : ((savedSSID.length() > 0) ? savedSSID : String(ssid));
      String storedPass = WiFi.psk();
      String targetPass = (storedPass.length() > 0) ? storedPass : ((savedPass.length() > 0) ? savedPass : String(password));

      // Background rescanner: checks for wife's hotspot every 10-15s
      const unsigned long RESCAN_INTERVAL = 12000;

      if (!isWifiScanning) {
        if (now - lastWifiScanAttempt >= RESCAN_INTERVAL) {
          lastWifiScanAttempt = now;
          Serial.printf("[WiFi-Rescan] Hotspot disconnected. Scanning for '%s'...\n", targetSSID.c_str());
          WiFi.scanNetworks(true); // Non-blocking async scan
          isWifiScanning = true;
        }
      } else {
        if (now - lastWifiScanAttempt > 8000) {
          // Scan watchdog timeout
          WiFi.scanDelete();
          isWifiScanning = false;
          lastWifiScanAttempt = now;
        } else {
          int16_t n = WiFi.scanComplete();
          if (n >= 0) {
            bool hotspotFound = false;
            for (int i = 0; i < n; i++) {
              String scanned = WiFi.SSID(i);
              if (scanned == targetSSID || (storedSSID.length() > 0 && scanned == storedSSID) || scanned == String(ssid)) {
                hotspotFound = true;
                break;
              }
            }
            WiFi.scanDelete();
            isWifiScanning = false;
            lastWifiScanAttempt = now;

            if (hotspotFound) {
              Serial.printf("[WiFi-Rescan] Found '%s'! Reconnecting...\n", targetSSID.c_str());
              WiFi.begin(targetSSID.c_str(), targetPass.c_str());
            } else {
              Serial.printf("[WiFi-Rescan] '%s' not found. Retry in 12s.\n", targetSSID.c_str());
            }
          } else if (n == WIFI_SCAN_FAILED) {
            WiFi.scanDelete();
            isWifiScanning = false;
            lastWifiScanAttempt = now;
          }
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS); // Yield to watchdogs
  }
}

// ════════════════════════════════════════════════════════
//  BOOT: ANIMASI TYPEWRITER
// ════════════════════════════════════════════════════════
void typewriterLine(const char *text, int x, int y, int delayMs) {
  char buf[64] = "";
  int len = strlen(text);
  for (int i = 0; i < len; i++) {
    strncat(buf, text + i, 1);
    u8g2.setDrawColor(0);
    u8g2.drawBox(x, y - 7, 128, 9);
    u8g2.setDrawColor(1);
    u8g2.drawStr(x, y, buf);
    int cx = x + u8g2.getUTF8Width(buf);
    u8g2.drawVLine(cx, y - 6, 7);
    u8g2.sendBuffer();
    delay(delayMs);
  }
  u8g2.setDrawColor(0);
  int cx = x + u8g2.getUTF8Width(buf);
  u8g2.drawVLine(cx, y - 6, 7);
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

// ==========================================
//                 MAIN (CORE 1)
// ==========================================

struct Raindrop {
  int x;
  int y;
  int size;
  int speed;
  bool splash;
  unsigned long splashTime;
};
Raindrop raindrops[12];
bool raindropsInit = false;

void drawWeatherIcon(int code, int x, int y) {
  u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
  if (code == 0 || code == 1)
    u8g2.drawGlyph(x, y, 69); // sun
  else if (code == 2 || code == 3)
    u8g2.drawGlyph(x, y, 65); // cloud
  else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82))
    u8g2.drawGlyph(x, y, 67); // rain
  else if (code >= 95)
    u8g2.drawGlyph(x, y, 67); // thunder/rain
  else if (code == 100)
    u8g2.drawGlyph(x, y, 65); // wind (using cloud icon as base)
  else
    u8g2.drawGlyph(x, y, 65); // default cloud
}

void drawWeatherBackground(int code, bool isNight, int frame, int boxX,
                           int boxY, int boxW, int boxH) {
  if (code == 0 || code == 1) { // Clear
    if (isNight) {
      u8g2.drawCircle(105, 18, 10);
      u8g2.setDrawColor(0);
      u8g2.drawDisc(100, 15, 10);
      u8g2.setDrawColor(1);

      for (int i = 0; i < 8; i++) {
        int sx = (i * 37 + frame / 4) % 128;
        int sy = (i * 19) % 64;
        if (i % 2 == 0) {
          u8g2.drawPixel(sx, sy);
        } else {
          if ((frame / 10 + i) % 2 == 0)
            u8g2.drawPixel(sx, sy);
        }
      }
    } else {
      u8g2.drawDisc(105, 18, 8);
      for (int i = 0; i < 8; i++) {
        float angle = (i * 45 + (frame % 360) * 2) * 0.0174533;
        int rx1 = 105 + cos(angle) * 11;
        int ry1 = 18 + sin(angle) * 11;
        int rx2 = 105 + cos(angle) * 16;
        int ry2 = 18 + sin(angle) * 16;
        // Avoid negative coordinates that wrap to 255 in u8g2
        if (rx1 < 0)
          rx1 = 0;
        if (ry1 < 0)
          ry1 = 0;
        if (rx2 < 0)
          rx2 = 0;
        if (ry2 < 0)
          ry2 = 0;
        u8g2.drawLine(rx1, ry1, rx2, ry2);
      }
    }
  } else if (code == 2 || code == 3) { // Clouds
    for (int i = 0; i < 3; i++) {
      int cx = (i * 60 - frame / 3) % 180;
      if (cx < -30)
        cx += 180;
      int cy = 10 + i * 15;
      u8g2.drawCircle(cx, cy, 6);
      u8g2.drawCircle(cx + 8, cy - 4, 8);
      u8g2.drawCircle(cx + 16, cy, 6);
      u8g2.drawHLine(cx, cy + 6, 16);
    }
  } else if (code == 100) { // Windy
    for (int i = 0; i < 5; i++) {
      int y = 10 + i * 12;
      int x = (frame * (3 + i % 2) + i * 20) % 150 - 20;
      u8g2.drawHLine(x, y, 10 + (i % 3) * 5);
    }
  } else if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82) ||
             code >= 95) {
    if (!raindropsInit) {
      for (int i = 0; i < 12; i++) {
        raindrops[i].x = random(0, 128);
        raindrops[i].y = random(-64, 0);
        raindrops[i].size = random(2, 5); // Length of rain line
        raindrops[i].speed = random(2, 6);
        raindrops[i].splash = false;
        raindrops[i].splashTime = 0;
      }
      raindropsInit = true;
    }

    // Draw raindrops
    for (int i = 0; i < 12; i++) {
      raindrops[i].y += raindrops[i].speed;

      // Collision with the time box
      if (!raindrops[i].splash && raindrops[i].y + raindrops[i].size >= boxY &&
          raindrops[i].y <= boxY + boxH && raindrops[i].x >= boxX &&
          raindrops[i].x <= boxX + boxW) {
        raindrops[i].splash = true;
        raindrops[i].splashTime = millis();
        raindrops[i].y = boxY - 2; // Splash precisely on the top edge
      }

      if (raindrops[i].y > 64) {
        raindrops[i].y = random(-30, 0);
        raindrops[i].x = random(0, 128);
        raindrops[i].speed = random(2, 6);
        raindrops[i].splash = false;
      }

      if (raindrops[i].splash) {
        if (millis() - raindrops[i].splashTime > 150) {
          raindrops[i].y = random(-30, 0);
          raindrops[i].x = random(0, 128);
          raindrops[i].splash = false;
        } else {
          // Splash drawing (\_/)
          u8g2.setFont(u8g2_font_4x6_tr);
          u8g2.drawStr(raindrops[i].x - 3, raindrops[i].y, "\\_/");
        }
      } else {
        u8g2.drawLine(raindrops[i].x, raindrops[i].y, raindrops[i].x,
                      raindrops[i].y + raindrops[i].size);
      }
    }

    // Thunderstorm
    if (code >= 95) {
      if (frame % 30 < 4) {
        // Lightning bolt
        u8g2.drawLine(64, 0, 50, 30);
        u8g2.drawLine(50, 30, 70, 30);
        u8g2.drawLine(70, 30, 40, 64);
      } else if (frame % 30 == 8 || frame % 30 == 9) { // Secondary flicker
        u8g2.drawLine(60, 0, 40, 20);
        u8g2.drawLine(40, 20, 50, 20);
        u8g2.drawLine(50, 20, 30, 50);
      }
    }
  }
}

void renderClock() {
  static int clockFrame = 0;
  clockFrame++;

  u8g2.clearBuffer();
  struct tm timeinfo;
  static struct tm lastTimeinfo;
  static bool hasTime = false;

  if (getLocalTime(&timeinfo, 10)) {
    hasTime = true;
    lastTimeinfo = timeinfo;
  }

  static int pixelShiftX = 0;
  static int pixelShiftY = 0;
  if (hasTime) {
    pixelShiftX = (timeinfo.tm_min % 5) - 2;
    pixelShiftY = (timeinfo.tm_min / 12) - 2;
  }

  if (!hasTime) {
    u8g2.setFont(u8g2_font_helvB08_tr);
    int frameIdx = (millis() / 42) % WIFI_FRAME_COUNT;
    u8g2.drawBitmap(48, 10, 4, 32, epd_bitmap_wifi_frames[frameIdx]);
    drawCenterText("syncing NTP...", 54);
  } else {
    timeinfo = lastTimeinfo; // Use cached time if NTP is busy this frame
    bool isNight = (timeinfo.tm_hour >= 18 || timeinfo.tm_hour < 6);

    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    u8g2.setFont(u8g2_font_logisoso16_tr); // Even smaller font

    int tw = u8g2.getStrWidth(timeStr);
    int tx = (128 - tw) / 2 + pixelShiftX;
    int ty = 26 + pixelShiftY;

    int boxX = tx - 8;
    int boxY = ty - 18;
    int boxW = tw + 16;
    int boxH = 24;

    if (weatherLoaded) {
      drawWeatherBackground(weatherCode, isNight, clockFrame, boxX, boxY, boxW,
                            boxH);
    }

    u8g2.setDrawColor(0);
    u8g2.drawRBox(boxX, boxY, boxW, boxH, 4);
    u8g2.setDrawColor(1);
    u8g2.drawRFrame(boxX, boxY, boxW, boxH, 4);

    u8g2.setFont(u8g2_font_logisoso16_tr); // RESTORE FONT!
    u8g2.drawStr(tx, ty, timeStr);

    char dateStr[16];
    strftime(dateStr, sizeof(dateStr), "%a, %d %b", &timeinfo);
    u8g2.setFont(u8g2_font_helvB08_tr);
    int dw = u8g2.getStrWidth(dateStr);

    int dateBoxX = (128 - dw) / 2 - 2 + pixelShiftX;
    int dateBoxY = 42 - 8 + pixelShiftY;
    int dateTextX = (128 - dw) / 2 + pixelShiftX;
    int dateTextY = 42 + pixelShiftY;

    u8g2.setDrawColor(0);
    u8g2.drawBox(dateBoxX, dateBoxY, dw + 4, 10);
    u8g2.setDrawColor(1);
    u8g2.drawStr(dateTextX, dateTextY, dateStr);

    if (weatherLoaded) {
      char tempStr[15];
      sprintf(tempStr, "%.0fC", weatherTemp);
      u8g2.setFont(u8g2_font_helvB08_tr);
      int tW = u8g2.getStrWidth(tempStr);

      int wx = (128 - tW) / 2 + 10 + pixelShiftX;
      int tempBoxX = wx - 2;
      int tempBoxY = 58 - 8 + pixelShiftY;
      int tempTextY = 58 + pixelShiftY;

      u8g2.setDrawColor(0);
      u8g2.drawBox(tempBoxX, tempBoxY, tW + 4, 10);
      u8g2.setDrawColor(1);

      u8g2.drawStr(wx, tempTextY, tempStr);

      u8g2.setDrawColor(0);
      u8g2.drawBox(wx - 22, 60 - 16 + pixelShiftY, 18, 18);
      u8g2.setDrawColor(1);
      drawWeatherIcon(weatherCode, wx - 20, 60 + pixelShiftY);
    }
  }
  u8g2.sendBuffer();
}

void drawCenteredWrappedText(const String &text) {
  u8g2.setFont(u8g2_font_helvB08_tr);
  int lineHeight = u8g2.getAscent() - u8g2.getDescent() + 2;
  const int MAX_LINES = 8;
  String lines[MAX_LINES];
  int lineCount = 0;
  String currentLine = "";

  int start = 0;
  while (start < text.length() && lineCount < MAX_LINES) {
    int spaceIdx = text.indexOf(' ', start);
    if (spaceIdx == -1)
      spaceIdx = text.length();
    String word = text.substring(start, spaceIdx);

    if (u8g2.getStrWidth(word.c_str()) > 128) {
      if (currentLine.length() > 0 && lineCount < MAX_LINES) {
        lines[lineCount++] = currentLine;
        currentLine = "";
      }
      for (unsigned int i = 0; i < word.length() && lineCount < MAX_LINES;
           i++) {
        String testLine = currentLine + word[i];
        if (u8g2.getStrWidth(testLine.c_str()) > 128) {
          if (lineCount < MAX_LINES)
            lines[lineCount++] = currentLine;
          currentLine = String(word[i]);
        } else {
          currentLine = testLine;
        }
      }
    } else {
      String testLine = currentLine;
      if (testLine.length() > 0)
        testLine += " ";
      testLine += word;

      if (u8g2.getStrWidth(testLine.c_str()) > 128) {
        if (lineCount < MAX_LINES)
          lines[lineCount++] = currentLine;
        currentLine = word;
      } else {
        currentLine = testLine;
      }
    }
    start = spaceIdx + 1;
  }
  if (currentLine.length() > 0 && lineCount < MAX_LINES) {
    lines[lineCount++] = currentLine;
  }

  int totalHeight = lineCount * lineHeight;
  int startY =
      (52 - totalHeight) / 2 + u8g2.getAscent(); // Shift up for READ button

  for (int i = 0; i < lineCount; i++) {
    int w = u8g2.getStrWidth(lines[i].c_str());
    int x = (128 - w) / 2;
    u8g2.drawStr(x, startY + i * lineHeight, lines[i].c_str());
  }
}

void setupWiFiManager() {
  WiFiManager wm;
  wm.setConnectTimeout(10);
  wm.setAPCallback([](WiFiManager *myWiFiManager) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(2, 12, 126, 9);
    u8g2.setDrawColor(1);
    u8g2.drawStr(2, 20, "> AP : wifeBoxWifi");
    u8g2.sendBuffer();
  });
  wm.autoConnect("wifeBoxWifi", "iloveyou");
  WiFi.setAutoReconnect(true);
}

void setup() {
  Serial.begin(115200);

  // Set up buttons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON1_PIN), handleButton1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON3_PIN), handleButton3, FALLING);

  // Set up Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  resetServo();

  // Set up buzzers
  ledcSetup(BUZZER_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  ledcSetup(BUZZER2_CHANNEL, 2000, 8);
  ledcAttachPin(BUZZER2_PIN, BUZZER2_CHANNEL);

  // Test beeps
  ledcWriteTone(BUZZER_CHANNEL, 1000);
  delay(100);
  ledcWrite(BUZZER_CHANNEL, 0);

  ledcWriteTone(BUZZER2_CHANNEL, 1500);
  delay(100);
  ledcWrite(BUZZER2_CHANNEL, 0);

  // initBirthdayConfetti(); // removed

  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr);
  typewriterLine("> SYSTEM BOOT", 2, 10, 45);

  typewriterLine("> WiFi : CONNECTING...", 2, 20, 45);

  setupWiFiManager();

  u8g2.setDrawColor(0);
  u8g2.drawBox(2, 12, 126, 9);
  u8g2.setDrawColor(1);
  u8g2.drawStr(2, 20, "> WiFi : OK");
  u8g2.sendBuffer();
  delay(100);

  typewriterLine("> Ntfy Stream: connecting", 2, 30, 30);

  secureClient.setInsecure();
  ntfyStreamClient.setInsecure();
  ntfyStreamClient.setTimeout(5);

  u8g2.setDrawColor(0);
  u8g2.drawBox(2, 22, 126, 9);
  u8g2.setDrawColor(1);
  u8g2.drawStr(2, 30, "> Ntfy Stream: OK");
  u8g2.sendBuffer();
  delay(300);

  String ipLine = "> IP: " + WiFi.localIP().toString();
  typewriterLine(ipLine.c_str(), 2, 40, 35);
  delay(200);

  u8g2.drawHLine(0, 44, 128);
  u8g2.sendBuffer();
  delay(200);

  typewriterLine("> WebServer: READY", 2, 54, 40);
  delay(600);

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  String url = "http://" + WiFi.localIP().toString();
  qrcode_initText(&qrcode, qrcodeData, 3, 0, url.c_str());

  u8g2.clearBuffer();
  int scale = 64 / qrcode.size;
  int ox = (128 - qrcode.size * scale) / 2;
  int oy = (64 - qrcode.size * scale) / 2;

  u8g2.setDrawColor(1);
  u8g2.drawBox(ox - 2, oy - 2, qrcode.size * scale + 4,
               qrcode.size * scale + 4);

  u8g2.setDrawColor(0);
  for (int y = 0; y < qrcode.size; y++) {
    for (int x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        u8g2.drawBox(ox + x * scale, oy + y * scale, scale, scale);
      }
    }
  }
  u8g2.sendBuffer();
  u8g2.setDrawColor(1);
  delay(2000);

  refreshAccessToken();

  configTime(19800, 0, "pool.ntp.org");

  randomSeed(analogRead(0));
  eqUpdateTargets();
  initWebServer();
  xTaskCreatePinnedToCore(networkTask, "NetworkTask", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(buzzerTask, "BuzzerTask", 2048, NULL, 1, NULL, 1);
}

// Macro State Variables
unsigned long macroStartTime = 0;
int macroStep = 0;
bool macroActive = false;

void renderDisplay() {
  if (currentScreen != SCREEN_BIRTHDAY && birthdayMusicActive) {
    birthdayMusicActive = false;
    resetServo();
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWriteTone(BUZZER2_CHANNEL, 0);
  }

  switch (currentScreen) {
  case SCREEN_SPOTIFY_NOW_PLAYING:
    renderSpotify();
    break;
  case SCREEN_BIRTHDAY:
    if (!renderBirthday()) {
      birthdayMusicActive = false;
      resetServo();
      currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
    }
    break;
  case SCREEN_LOCAL_MUSIC:
    renderLocalMusic();
    break;
  case SCREEN_IDLE_WEATHER_CLOCK:
    renderClock();
    break;
  case SCREEN_HUG:
    u8g2.clearBuffer();
    if ((millis() / 500) % 2 == 0) {
      u8g2.drawDisc(64, 24, 10);
    } else {
      u8g2.drawDisc(64, 24, 15);
    }
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("ur wife sent a hug :3", 58);
    u8g2.sendBuffer();
    break;
  case SCREEN_HEADPAT:
    u8g2.clearBuffer();
    {
      static unsigned long lastHeadpatFrameTime = 0;
      static int headpatFrame = 0;
      if (millis() - lastHeadpatFrameTime > 42) {
        headpatFrame = (headpatFrame + 1) % HEADPAT_FRAME_COUNT;
        lastHeadpatFrameTime = millis();
      }
      u8g2.drawBitmap(48, 12, HEADPAT_FRAME_WIDTH / 8, HEADPAT_FRAME_HEIGHT,
                      epd_bitmap_headpat_frames[headpatFrame]);
    }
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("ur wife sent a headpat :D", 58);
    u8g2.sendBuffer();
    break;
  case SCREEN_KISS:
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    if ((millis() / 400) % 2 == 0)
      drawCenterText("<3", 32);
    else
      drawCenterText("< 3", 32);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("ur wife sent a kiss :p", 58);
    u8g2.sendBuffer();
    break;
  case SCREEN_HAND:
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB14_tr);
    if ((millis() / 300) % 2 == 0)
      drawCenterText("\\o/", 32);
    else
      drawCenterText("/o\\", 32);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("ur wife sent his hand :v", 58);
    u8g2.sendBuffer();
    break;
  case SCREEN_WORKOUT: {
    u8g2.clearBuffer();
    static unsigned long lastWorkoutFrameTime = 0;
    static int workoutFrame = 0;
    if (millis() - lastWorkoutFrameTime > 42) { // 42ms per frame
      workoutFrame = (workoutFrame + 1) % WORKOUT_FRAME_COUNT;
      lastWorkoutFrameTime = millis();
    }
    u8g2.drawBitmap(48, 12, WORKOUT_FRAME_WIDTH / 8, WORKOUT_FRAME_HEIGHT,
                    epd_bitmap_workout_frames[workoutFrame]);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("have a good workout :3", 58);
    u8g2.sendBuffer();
    break;
  }
  case SCREEN_FREAKY: {
    u8g2.clearBuffer();
    static unsigned long lastFreakyFrameTime = 0;
    static int freakyFrame = 0;
    if (millis() - lastFreakyFrameTime > 42) {
      freakyFrame = (freakyFrame + 1) % FREAKY_FRAME_COUNT;
      lastFreakyFrameTime = millis();
    }
    u8g2.drawBitmap(48, 12, FREAKY_FRAME_WIDTH / 8, FREAKY_FRAME_HEIGHT,
                    epd_bitmap_freaky_frames[freakyFrame]);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("i miss my hands over u rAWR", 58);
    u8g2.sendBuffer();
    break;
  }
  case SCREEN_FOOD: {
    u8g2.clearBuffer();
    static unsigned long lastFoodFrameTime = 0;
    static int foodFrame = 0;
    if (millis() - lastFoodFrameTime > 42) {
      foodFrame = (foodFrame + 1) % FOOD_FRAME_COUNT;
      lastFoodFrameTime = millis();
    }
    u8g2.drawBitmap(48, 12, FOOD_FRAME_WIDTH / 8, FOOD_FRAME_HEIGHT,
                    epd_bitmap_food_frames[foodFrame]);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("eat well my pretty girl :3", 58);
    u8g2.sendBuffer();
    break;
  }
  case SCREEN_STUDY: {
    u8g2.clearBuffer();
    static unsigned long lastStudyFrameTime = 0;
    static int studyFrame = 0;
    if (millis() - lastStudyFrameTime > 42) {
      studyFrame = (studyFrame + 1) % STUDY_FRAME_COUNT;
      lastStudyFrameTime = millis();
    }
    u8g2.drawBitmap(48, 12, STUDY_FRAME_WIDTH / 8, STUDY_FRAME_HEIGHT,
                    epd_bitmap_study_frames[studyFrame]);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("study well like a good kitty :3", 58);
    u8g2.sendBuffer();
    break;
  }
  case SCREEN_WAKEUP: {
    u8g2.clearBuffer();
    static unsigned long lastWakeupFrameTime = 0;
    static int wakeupFrame = 0;
    if (millis() - lastWakeupFrameTime > 42) {
      wakeupFrame = (wakeupFrame + 1) % WAKEUP_FRAME_COUNT;
      lastWakeupFrameTime = millis();
    }
    u8g2.drawBitmap(48, 12, WAKEUP_FRAME_WIDTH / 8, WAKEUP_FRAME_HEIGHT,
                    epd_bitmap_wakeup_frames[wakeupFrame]);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("good morning lovely wife :3", 58);
    u8g2.sendBuffer();
    break;
  }
  case SCREEN_PIXEL_ART:
    u8g2.clearBuffer();
    // Draw 32x32 buffer scaled 2x to 64x64, centered
    for (int y = 0; y < 32; y++) {
      for (int x = 0; x < 32; x++) {
        int byteIdx = (y * 32 + x) / 8;
        int bitIdx = x % 8;
        if (pixelArtBuffer[byteIdx] & (1 << bitIdx)) {
          u8g2.drawBox(32 + x * 2, y * 2, 2, 2);
        }
      }
    }
    // Lower caption to bottom with background
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 54, 128, 10);
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_4x6_tr);
    drawCenterText("i send u dis doodle :3", 61);
    u8g2.sendBuffer();
    break;
  case SCREEN_WIFE_PHOTO:
    u8g2.clearBuffer();
    u8g2.drawBitmap(0, 0, 16, 64, epd_bitmap_wife2);
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.setFontDirection(1);
    u8g2.drawStr(14, 20, "i <3 u");
    u8g2.drawStr(118, 2, "pyaariest billi");
    u8g2.setFontDirection(0);
    u8g2.sendBuffer();
    break;
  case SCREEN_KITTY:
    drawKittyScreen();
    break;
  }
}

void handleKittyButtons() {
  unsigned long now = millis();

  // GPIO 4 (Button 1): Feed
  if (button1Flag || virtualButton1) {
    button1Flag = false;
    virtualButton1 = false;
    if (now - debounceTime1 > 200) {
      debounceTime1 = now;
      feedKitty();
    }
  }

  // GPIO 16 (Button 2): Pet
  static bool lastBtn2StateKitty = HIGH;
  static unsigned long btn2PressTimeKitty = 0;
  bool currentBtn2 = digitalRead(BUTTON2_PIN);
  bool isBtn2Trigger = virtualButton2_short || virtualButton2_long;
  virtualButton2_short = false;
  virtualButton2_long = false;

  if (lastBtn2StateKitty == HIGH && currentBtn2 == LOW) {
    if (now - debounceTime2 > 50) {
      btn2PressTimeKitty = now;
      debounceTime2 = now;
    }
  } else if (lastBtn2StateKitty == LOW && currentBtn2 == HIGH) {
    if (now - debounceTime2 > 50) {
      if (now - btn2PressTimeKitty > 50) {
        isBtn2Trigger = true;
      }
      debounceTime2 = now;
    }
  }
  lastBtn2StateKitty = currentBtn2;

  if (isBtn2Trigger) {
    petKitty();
  }

  // GPIO 17 (Button 3): Sleep (Short) / Exit (Long > 1000ms)
  static bool lastBtn3StateKitty = HIGH;
  static unsigned long btn3PressTimeKitty = 0;
  static bool btn3LongHandled = false;
  bool currentBtn3 = digitalRead(BUTTON3_PIN);
  button3Flag = false;

  if (lastBtn3StateKitty == HIGH && currentBtn3 == LOW) {
    if (now - debounceTime3 > 50) {
      btn3PressTimeKitty = now;
      btn3LongHandled = false;
      debounceTime3 = now;
    }
  } else if (currentBtn3 == LOW) {
    if (!btn3LongHandled && (now - btn3PressTimeKitty >= 1000)) {
      btn3LongHandled = true;
      // Long press: Return to SCREEN_IDLE_WEATHER_CLOCK
      resetServo();
      ledcWriteTone(BUZZER_CHANNEL, 0);
      ledcWriteTone(BUZZER2_CHANNEL, 0);
      kittyPet.purrActive = false;
      kittyPet.soundEndTime = 0;
      debounceTime3 = now;
      currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
      u8g2.clearBuffer();
      return;
    }
  } else if (lastBtn3StateKitty == LOW && currentBtn3 == HIGH) {
    if (now - debounceTime3 > 50) {
      if (!btn3LongHandled && (now - btn3PressTimeKitty > 50)) {
        toggleKittySleep();
      }
      debounceTime3 = now;
    }
  }
  lastBtn3StateKitty = currentBtn3;

  if (virtualButton3) {
    virtualButton3 = false;
    toggleKittySleep();
  }
}

void loop() {
  ElegantOTA.loop();
  unsigned long now = millis();
  updateKitty();

  if (isAckToggled && now > ackDisplayEndTime) {
    isAckToggled = false;
    currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
    macroActive = false;
    currentServoMode = SERVO_IDLE;
    resetServo();
    u8g2.clearBuffer();
  }

  if (pendingMacroScreen != SCREEN_IDLE_WEATHER_CLOCK) {
    ScreenState pending = pendingMacroScreen;
    pendingMacroScreen = SCREEN_IDLE_WEATHER_CLOCK;
    currentScreen = pending;
    macroStartTime = now;
    macroStep = 0;
    macroActive = true;
    currentServoMode = SERVO_IDLE;
    if (pending == SCREEN_WAKEUP) {
      newBrightness = 255;
      updateBrightnessFlag = true;
    }
  }
  if (pendingIsAlert) {
    macroStartTime = now;
    macroActive = true;
    currentServoMode = SERVO_IDLE;
  }

  // Macro State Machines
  if ((currentScreen == SCREEN_HUG || currentScreen == SCREEN_HEADPAT ||
       currentScreen == SCREEN_KISS || currentScreen == SCREEN_HAND ||
       currentScreen == SCREEN_WORKOUT || currentScreen == SCREEN_FREAKY ||
       currentScreen == SCREEN_FOOD || currentScreen == SCREEN_STUDY) &&
      macroActive) {
    unsigned long elapsed = now - macroStartTime;
    if (elapsed < 300) {
      ledcWriteTone(BUZZER_CHANNEL, 523);  // C5
      ledcWriteTone(BUZZER2_CHANNEL, 659); // E5
    } else if (elapsed < 600) {
      ledcWriteTone(BUZZER_CHANNEL, 659);
      ledcWriteTone(BUZZER2_CHANNEL, 784); // G5
    } else if (elapsed < 900) {
      ledcWriteTone(BUZZER_CHANNEL, 784);
      ledcWriteTone(BUZZER2_CHANNEL, 988); // B5
    } else {
      ledcWriteTone(BUZZER_CHANNEL, 0);
      ledcWriteTone(BUZZER2_CHANNEL, 0);
    }

    // Noticeable, fast, long servo oscillation: 0 <-> 180 every 120ms for 2.4s
    if (elapsed < 2400) {
      int swing = ((elapsed / 120) % 2 == 0) ? 180 : 0;
      myServo.write(swing);
    } else {
      resetServo();
    }

    if (elapsed > 5000) {
      resetServo();
      macroActive = false;
      currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
    }
  }

  if (currentScreen == SCREEN_WAKEUP && macroActive) {
    unsigned long elapsed = now - macroStartTime;
    if (elapsed < 500) {
      ledcWriteTone(BUZZER_CHANNEL, 523);
    } else if (elapsed < 1000) {
      ledcWriteTone(BUZZER_CHANNEL, 587);
    } else if (elapsed < 1500) {
      ledcWriteTone(BUZZER_CHANNEL, 659);
    } else if (elapsed < 2500) {
      ledcWriteTone(BUZZER_CHANNEL, 784);
    } else {
      ledcWriteTone(BUZZER_CHANNEL, 0);
    }

    // Energetic wake-up rotation: 0 <-> 180 every 120ms for 3.6s
    if (elapsed < 3600) {
      int swing = ((elapsed / 120) % 2 == 0) ? 180 : 0;
      myServo.write(swing);
    } else {
      resetServo();
    }

    if (elapsed > 10000) {
      resetServo();
      macroActive = false;
      currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
    }
  }

  if (pendingIsAlert || (macroActive && macroStep < 100)) {
    static unsigned long lastAlertStep = 0;
    if (pendingIsAlert) {
      macroStep = 50;
      lastAlertStep = now;
      pendingIsAlert = false;
    }

    if (macroStep >= 50 && macroStep < 70) {
      if (now - lastAlertStep > 80) { // faster 80ms interval
        lastAlertStep = now;
        if (macroStep % 2 == 0) {
          ledcWriteTone(BUZZER_CHANNEL, 2000);
          myServo.write(180);
          u8g2.sendBuffer();
          u8g2.sendF("c", 0xa7);
        } else {
          ledcWriteTone(BUZZER_CHANNEL, 0);
          myServo.write(0);
          u8g2.sendF("c", 0xa6);
        }
        macroStep++;
      }
    } else if (macroStep == 70) {
      ledcWriteTone(BUZZER_CHANNEL, 0);
      resetServo();
      u8g2.sendF("c", 0xa6);
      macroStep = 100;
      macroActive = false;
    }
  }

  // Auto-revert Pixel Art after 60s
  if (currentScreen == SCREEN_PIXEL_ART &&
      millis() - pixelArtStartTime > 60000) {
    currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
  }

  if (updateBrightnessFlag) {
    updateBrightnessFlag = false;
    u8g2.setContrast(newBrightness);
  }
  if (triggerBirthday) {
    triggerBirthday = false;
    currentScreen = SCREEN_BIRTHDAY;
    birthdayMusicActive = true;
    startBirthday();
  }
  if (triggerSleep) {
    triggerSleep = false;
    u8g2.setPowerSave(1); // Turn off OLED
  }
  if (triggerWake) {
    triggerWake = false;
    u8g2.setPowerSave(0); // Turn on OLED
  }

  if (stopAudioCmd) {
    stopAudioCmd = false;
    localMusicPlaying = false;
    ledcWriteTone(BUZZER_CHANNEL, 0);
    ledcWriteTone(BUZZER2_CHANNEL, 0);
  }

  if (newLocalSongId != -1) {
    if (newLocalSongId >= 0 && newLocalSongId < NUM_LOCAL_SONGS) {
      currentLocalSongIndex = newLocalSongId;
      currentNoteIndex = 0;
      localMusicPlaying = true;
      isChangingTrack = true;
      trackChangeEndTime = millis() + 300;
      if (currentScreen != SCREEN_LOCAL_MUSIC) {
        currentScreen = SCREEN_LOCAL_MUSIC;
        u8g2.clearBuffer();
      }
    }
    newLocalSongId = -1;
  }

  if (now - lastEqTick > 100) {
    lastEqTick = now;
    eqSmooth();
    if (random(5) == 0)
      eqUpdateTargets();
  }
  if (now - lastScroll > 45) {
    lastScroll = now;
    updateScroll();
  }

  if (currentScreen == SCREEN_KITTY) {
    handleKittyButtons();
  } else {
    // Button 1: Switch Screens
    if (button1Flag || virtualButton1) {
      button1Flag = false;
      virtualButton1 = false;
      if (now - debounceTime1 > 250) {
        debounceTime1 = now;
        currentScreen = (ScreenState)((currentScreen + 1) % 7);
        if (currentScreen == SCREEN_PIXEL_ART) {
          pixelArtStartTime = millis();
        } else if (currentScreen == SCREEN_BIRTHDAY) {
          startBirthday();
        }
        u8g2.clearBuffer();
      }
    }

    // Button 2: Switch Local Music (Play/Pause/Next)
    static bool lastButton2State = HIGH;
    static unsigned long button2PressTime = 0;
    bool currentButton2State = digitalRead(BUTTON2_PIN);

    bool isBtn2Short = virtualButton2_short;
    bool isBtn2Long = virtualButton2_long;
    virtualButton2_short = false;
    virtualButton2_long = false;

    if (lastButton2State == HIGH && currentButton2State == LOW) {
      if (now - debounceTime2 > 50) {
        button2PressTime = now;
        debounceTime2 = now;
      }
    } else if (lastButton2State == LOW && currentButton2State == HIGH) {
      if (now - debounceTime2 > 50) {
        if (now - button2PressTime >= 1000)
          isBtn2Long = true;
        else if (now - button2PressTime > 50)
          isBtn2Short = true;
        debounceTime2 = now;
      }
    }
    lastButton2State = currentButton2State;

    if (isBtn2Short || isBtn2Long) {
      // Check if we are on a notification screen
      if (currentScreen == SCREEN_PIXEL_ART || currentScreen == SCREEN_HUG ||
          currentScreen == SCREEN_HEADPAT || currentScreen == SCREEN_KISS ||
          currentScreen == SCREEN_HAND || currentScreen == SCREEN_WAKEUP ||
          currentScreen == SCREEN_WORKOUT || currentScreen == SCREEN_FREAKY ||
          currentScreen == SCREEN_FOOD || currentScreen == SCREEN_STUDY ||
          currentServoMode == SERVO_WIGGLE) {
        triggerAck = true;
        isAckToggled = true;
        ackDisplayEndTime = now + 1500;
        currentServoMode = SERVO_IDLE;
        resetServo();
      } else {
        if (isBtn2Long) {
          // Long press: Next Song
          currentLocalSongIndex = (currentLocalSongIndex + 1) % NUM_LOCAL_SONGS;
          currentNoteIndex = 0;
          localMusicPlaying = true;
          isChangingTrack = true;
          trackChangeEndTime = millis() + 300;
        } else {
          // Short press: Play/Pause
          localMusicPlaying = !localMusicPlaying;
          isPlayToggled = true;
          playToggleEndTime = millis() + 300;
        }
        if (currentScreen != SCREEN_LOCAL_MUSIC)
          currentScreen = SCREEN_LOCAL_MUSIC;
        u8g2.clearBuffer();
      }
    }

    // Button 3: Send "Miss You" Ntfy Webhook
    if (button3Flag || virtualButton3) {
      button3Flag = false;
      virtualButton3 = false;
      if (now - debounceTime3 > 500) {
        debounceTime3 = now;
        if (currentScreen == SCREEN_IDLE_WEATHER_CLOCK) {
          // Toggle weather for testing
          static int testCodes[] = {0, 2, 100, 61, 95};
          static int cIdx = 0;
          cIdx = (cIdx + 1) % 5;
          weatherCode = testCodes[cIdx];
          weatherLoaded = true;
        } else {
          triggerWebhook = true; // Signals Network Task on Core 0

          // Flash visual confirmation on screen
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_helvB08_tr);
          drawCenterText("message sent <3", 36);
          u8g2.sendBuffer();
          delay(1500); // Block UI briefly to force her to see it
          u8g2.clearBuffer();
        }
      }
    }
  }

  // --- New Servo State Machine ---
  if (newServoAngle != -1) {
    myServo.write(newServoAngle);
    currentServoAngle = newServoAngle;
    newServoAngle = -1;
    currentServoMode = SERVO_IDLE;
  }
  if (triggerFlagWave) {
    triggerFlagWave = false;
    currentServoMode = SERVO_WIGGLE;
    lastServoMoveTime = now;
    servoStep = 0;
  }
  if (triggerServoSweep) {
    triggerServoSweep = false;
    currentServoMode = SERVO_SWEEP;
    lastServoMoveTime = now;
    currentServoAngle = 0;
    servoStep = 0;
    servoDir = 1;
  }
  if (triggerServoHeartbeat) {
    triggerServoHeartbeat = false;
    currentServoMode = SERVO_HEARTBEAT;
    lastServoMoveTime = now;
    servoStep = 0;
  }

  switch (currentServoMode) {
  case SERVO_WIGGLE: {
    // Fast, wide, noticeable flag wave: alternate 180 and 0 every 120ms
    // for 3.6s (30 swings)
    unsigned long wElapsed = now - lastServoMoveTime;
    if (wElapsed < 3600) {
      int angle = ((wElapsed / 120) % 2 == 0) ? 180 : 0;
      myServo.write(angle);
    } else {
      resetServo();
      currentServoMode = SERVO_IDLE;
    }
    break;
  }
  case SERVO_SWEEP: {
    // Fast, wide sweep: 6 deg every 15ms across 4 half-cycles (2 round trips)
    if (now - lastServoMoveTime > 15) {
      lastServoMoveTime = now;
      currentServoAngle += (servoDir * 6);
      if (currentServoAngle >= 180) {
        currentServoAngle = 180;
        servoDir = -1;
        servoStep++;
      } else if (currentServoAngle <= 0) {
        currentServoAngle = 0;
        servoDir = 1;
        servoStep++;
      }
      myServo.write(currentServoAngle);
      if (servoStep >= 4 && currentServoAngle == 0) {
        resetServo();
        currentServoMode = SERVO_IDLE;
      }
    }
    break;
  }
  case SERVO_HEARTBEAT: {
    // 4 prominent lub-dub beats over 2.5s (cycle 640ms)
    unsigned long hElapsed = now - lastServoMoveTime;
    if (hElapsed < 2560) {
      unsigned long cycleTime = hElapsed % 640;
      if (cycleTime < 100) {
        myServo.write(180);
      } else if (cycleTime < 180) {
        myServo.write(0);
      } else if (cycleTime < 280) {
        myServo.write(180);
      } else {
        myServo.write(0);
      }
    } else {
      resetServo();
      currentServoMode = SERVO_IDLE;
    }
    break;
  }
  case SERVO_IDLE:
  default:
    break;
  }

  // Render UI
  if (millis() - lastDrawTime > 33) { // ~30FPS
    if (isAckToggled) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_helvB08_tr);
      const char *confMsg = "sent him love";
      int cw = u8g2.getStrWidth(confMsg);
      u8g2.drawStr((128 - cw) / 2, 30, confMsg);

      const char *readText = "[ READ ]";
      int w = u8g2.getStrWidth(readText);
      int x = (128 - w) / 2;
      u8g2.drawBox(x - 2, 50, w + 4, 12);
      u8g2.setDrawColor(0);
      u8g2.drawStr(x, 59, readText);
      u8g2.setDrawColor(1);
      u8g2.sendBuffer();
    } else if (currentServoMode == SERVO_WIGGLE &&
               currentScreen != SCREEN_PIXEL_ART) {
      u8g2.clearBuffer();
      drawCenteredWrappedText(ntfy_reply_message);

      u8g2.setFont(u8g2_font_helvB08_tr);
      const char *readText = "[ READ ]";
      int w = u8g2.getStrWidth(readText);
      int x = (128 - w) / 2;
      u8g2.drawStr(x, 59, readText);

      u8g2.sendBuffer();
    } else {
      renderDisplay();
    }
    lastDrawTime = millis();
  }
}
