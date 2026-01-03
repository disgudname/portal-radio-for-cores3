#include <Arduino.h>
#include <M5Unified.h>
#include <Audio.h>

namespace {
constexpr const char *kAudioPath = "/duck/audio/portal.mp3";
constexpr int kI2sBclkPin = 41;
constexpr int kI2sLrckPin = 42;
constexpr int kI2sDataPin = 2;
constexpr int kVolume = 12;

Audio audio;
}

void audio_eof_mp3(const char *info) {
  (void)info;
  audio.connecttoFS(M5.Sd, kAudioPath);
}

void setup() {
  auto config = M5.config();
  M5.begin(config);

  M5.Display.setTextSize(4);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextDatum(middle_center);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.drawString("85.2 FM", M5.Display.width() / 2,
                        M5.Display.height() / 2);

  if (!M5.Sd.begin()) {
    M5.Display.setTextSize(2);
    M5.Display.setCursor(0, M5.Display.height() - 30);
    M5.Display.setTextDatum(top_left);
    M5.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5.Display.print("SD init failed");
    while (true) {
      delay(1000);
    }
  }

  audio.setPinout(kI2sBclkPin, kI2sLrckPin, kI2sDataPin);
  audio.setVolume(kVolume);
  audio.connecttoFS(M5.Sd, kAudioPath);
}

void loop() {
  audio.loop();
  M5.update();
}
