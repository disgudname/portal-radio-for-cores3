#include <Arduino.h>
#include <M5Unified.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSD.h>
#include "AudioOutputM5Speaker.h"

static constexpr const char* kAudioPath = "/duck/audio/portal.mp3";
static constexpr const char* kBackgroundImage = "/duck/Aperture_Science.png";

static AudioGeneratorMP3* mp3 = nullptr;
static AudioFileSourceSD* file = nullptr;
static AudioOutputM5Speaker* out = nullptr;

void startPlayback() {
    if (file) {
        file->close();
        delete file;
    }
    if (mp3) {
        mp3->stop();
        delete mp3;
    }

    file = new AudioFileSourceSD(kAudioPath);
    mp3 = new AudioGeneratorMP3();

    if (!file->isOpen()) {
        Serial.println("Failed to open audio file");
        return;
    }

    mp3->begin(file, out);
    Serial.println("Playback started");
}

void setup() {
    auto cfg = M5.config();
    cfg.internal_spk = true;  // Enable internal speaker
    M5.begin(cfg);

    // Configure speaker
    auto spk_cfg = M5.Speaker.config();
    M5.Speaker.config(spk_cfg);
    M5.Speaker.begin();
    M5.Speaker.setVolume(255);  // Max volume

    // Initialize SD card first (needed for background image)
    if (!SD.begin(GPIO_NUM_4, SPI, 25000000)) {
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString("SD CARD ERROR", M5.Display.width() / 2, M5.Display.height() / 2);
        Serial.println("SD Card initialization failed");
        while (true) delay(1000);
    }

    Serial.println("SD Card initialized");

    // Display setup - draw background image
    // Image is 240x240, screen is 320x240
    M5.Display.fillScreen(TFT_BLACK);

    // Load PNG from SD card (pre-scaled to 240x240) and draw centered
    File pngFile = SD.open(kBackgroundImage, FILE_READ);
    if (pngFile) {
        size_t fileSize = pngFile.size();
        uint8_t* pngData = (uint8_t*)malloc(fileSize);
        if (pngData) {
            pngFile.read(pngData, fileSize);
            pngFile.close();

            // Center the 240x240 image on the 320x240 screen
            int xOffset = (M5.Display.width() - 240) / 2;  // 40
            M5.Display.drawPng(pngData, fileSize, xOffset, 0);
            free(pngData);
        } else {
            pngFile.close();
            Serial.println("Failed to allocate memory for PNG");
        }
    } else {
        Serial.println("Failed to open background image");
    }

    // Draw text on top of background
    M5.Display.setTextSize(4);
    M5.Display.setTextColor(TFT_WHITE);  // No background color for transparent text overlay
    M5.Display.setTextDatum(middle_center);
    M5.Display.drawString("85.2 FM", M5.Display.width() / 2, M5.Display.height() / 2);

    // Create audio output
    out = new AudioOutputM5Speaker(&M5.Speaker, 0);

    // Start playback
    startPlayback();
}

void loop() {
    M5.update();

    if (mp3 && mp3->isRunning()) {
        if (!mp3->loop()) {
            // Track ended, restart immediately for gapless loop
            mp3->stop();
            startPlayback();
        }
    } else {
        // Not running, try to start
        startPlayback();
        delay(100);
    }
}
