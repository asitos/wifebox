#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <U8g2lib.h>
#include <ElegantOTA.h>
#include "web_ui.h"

// External declarations for globals in main.cpp
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern String customNoteMessage;
extern volatile bool triggerWebhook;
extern volatile bool updateBrightnessFlag;
extern volatile int newBrightness;
extern volatile bool triggerBirthday;
extern volatile int newLocalSongId;
enum ScreenState { SCREEN_IDLE_WEATHER_CLOCK, SCREEN_BIRTHDAY, SCREEN_LOCAL_MUSIC, SCREEN_SPOTIFY_NOW_PLAYING, SCREEN_PIXEL_ART, SCREEN_WIFE_PHOTO, SCREEN_KITTY, SCREEN_HUG, SCREEN_WAKEUP, SCREEN_HEADPAT, SCREEN_KISS, SCREEN_HAND, SCREEN_WORKOUT, SCREEN_FREAKY, SCREEN_FOOD, SCREEN_STUDY };
extern ScreenState currentScreen;

void feedKitty();
void petKitty();
void toggleKittySleep();
void exitKitty();

AsyncWebServer server(80);

void initWebServer() {
    if (!MDNS.begin("lovebox")) {
        Serial.println("Error setting up MDNS responder!");
    } else {
        Serial.println("mDNS responder started (lovebox.local)");
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", web_html);
    });

    server.on("/brightness", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("value")) {
            int brightness = request->getParam("value")->value().toInt();
            newBrightness = constrain(brightness, 0, 255);
            updateBrightnessFlag = true;
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/send_note", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("message", true)) {
            String msg = request->getParam("message", true)->value();
            customNoteMessage = msg;
            triggerWebhook = true;
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/birthday", HTTP_POST, [](AsyncWebServerRequest *request){
        triggerBirthday = true;
        request->send(200, "text/plain", "OK");
    });

    server.on("/play", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("song", true)) {
            newLocalSongId = request->getParam("song", true)->value().toInt();
        } else if (request->hasParam("song")) {
            newLocalSongId = request->getParam("song")->value().toInt();
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/stop", HTTP_POST, [](AsyncWebServerRequest *request){
        extern volatile bool stopAudioCmd;
        stopAudioCmd = true;
        request->send(200, "text/plain", "OK");
    });

    server.on("/sleep", HTTP_POST, [](AsyncWebServerRequest *request){
        extern volatile bool triggerSleep;
        triggerSleep = true;
        request->send(200, "text/plain", "OK");
    });

    server.on("/wake", HTTP_POST, [](AsyncWebServerRequest *request){
        extern volatile bool triggerWake;
        triggerWake = true;
        request->send(200, "text/plain", "OK");
    });

    server.on("/btn", HTTP_POST, [](AsyncWebServerRequest *request){
        extern volatile bool virtualButton1;
        extern volatile bool virtualButton2_short;
        extern volatile bool virtualButton2_long;
        extern volatile bool virtualButton3;
        if(request->hasParam("id", true)){
            String id = request->getParam("id", true)->value();
            if(id == "1") virtualButton1 = true;
            else if(id == "2s") virtualButton2_short = true;
            else if(id == "2l") virtualButton2_long = true;
            else if(id == "3") virtualButton3 = true;
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/kitty", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("action", true)) {
            String act = request->getParam("action", true)->value();
            if (act == "open" || act == "show") {
                currentScreen = SCREEN_KITTY;
            } else if (act == "feed") {
                currentScreen = SCREEN_KITTY;
                feedKitty();
            } else if (act == "pet") {
                currentScreen = SCREEN_KITTY;
                petKitty();
            } else if (act == "sleep") {
                currentScreen = SCREEN_KITTY;
                toggleKittySleep();
            } else if (act == "exit") {
                exitKitty();
            }
        } else {
            currentScreen = SCREEN_KITTY;
        }
        request->send(200, "text/plain", "OK");
    });

    server.on("/screen", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("id", true)) {
            String s = request->getParam("id", true)->value();
            if (currentScreen == SCREEN_KITTY && s != "kitty") {
                exitKitty();
            }
            if (s == "clock" || s == "weather") currentScreen = SCREEN_IDLE_WEATHER_CLOCK;
            else if (s == "birthday") { triggerBirthday = true; }
            else if (s == "music") currentScreen = SCREEN_LOCAL_MUSIC;
            else if (s == "spotify") currentScreen = SCREEN_SPOTIFY_NOW_PLAYING;
            else if (s == "doodle" || s == "pixel_art") currentScreen = SCREEN_PIXEL_ART;
            else if (s == "photo" || s == "wife_photo") currentScreen = SCREEN_WIFE_PHOTO;
            else if (s == "kitty") currentScreen = SCREEN_KITTY;
            u8g2.clearBuffer();
        }
        request->send(200, "text/plain", "OK");
    });

    ElegantOTA.begin(&server);
    server.begin();
    Serial.println("HTTP server started");
}

#endif
