#include <Arduino.h>
#include <M5Unified.h>
#include "Toio.h"
#include "ToioUUID.h"
// #include <NimBLEDevice.h>
#include "NimbleManager.h"

const int TOIO_UPDATE_INTERVAL = 1000; // toioのupdate()を呼ぶ間隔

int width;
int height;

// 画面の一番下にメッセージを表示
void bottomMessage(std::string msg) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, height - 20);
  M5.Display.printf("%s", msg.c_str());
}

// toioの情報を更新する
// xTaskCreateUniversalで起動されてバックグラウンドで実行される
void updateToios(void *pvParameters) {
  while (true) {
    Serial.println("update:");
    for (int i = 0; i < toios.size(); i++) {
      toios.at(i).update();
      // Serial.printf("ID: %d Battery %d％\n", i, toios.at(i).battrey());
    }
    for (int i = 0; i < toios.size(); i++) {
      // ディスプレイにpositionIDを表示する
      M5.Display.setCursor(0, i * 20);
      M5.Display.printf("ID: %2d x:%3u y:%3u d:%3u", i, toios[i].x, toios[i].y, toios[i].direction);
    }
    delay(TOIO_UPDATE_INTERVAL);
  }
}

void setup() {

  auto cfg = M5.config(); // M5Stack初期設定用の構造体を代入
  M5.begin(cfg);          // M5デバイスの初期化

  width = M5.Display.width;
  height = M5.Display.height;
  M5.Display.setTextSize(2); // テキストサイズを変更
  Serial.println("Starting NimBLE Client");

  // NimBLEスキャンを開始
  nimBLESetup();

  std::string msg = "Searching toios...";
  bottomMessage(msg);
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    if (toios.size() == 0) {
      if (advDevices.size() == 0) {
        // advertiseが見つかっていなければ中止
        return;
      }
      // toioが接続されてなければ接続開始
      if (connectToServer()) {

        // toioの情報を更新するタスクを起動
        xTaskCreateUniversal(
            updateToios,
            "updateToios",
            8192,
            NULL,
            1,
            NULL,
            APP_CPU_NUM);
        bottomMessage("Pos reset Forward  Turn R");
      }
      else {
        Serial.println("Failed to connect");
      }
      return;
    }

    // toioが接続されている場合
    for (int i = 0; i < toios.size(); i++) {
      // toios.at(i).commandMotor(false, 0x16, true, 0x16);

      // プレイマットのx120,y180を原点に4列で整列させる
      // 2台しかないので確認できないがバグってると報告アリ
      int rowX = i % 4;
      int rowY = i / 4;

      toios.at(i).goTo(120 + rowX * 40, 180 + rowY * 40, 90);
    }
  }
  if (M5.BtnA.wasReleased()) {
  }

  if (toios.size() == 0) {
    // toioがつながってなければB,Cボタン無効
    return;
  }
  // Button B
  if (M5.BtnB.wasPressed()) {

    for (int i = 0; i < toios.size(); i++) {

      toios.at(i).commandMotor(true, 0x16, true, 0x16);
    }
  }
  if (M5.BtnB.wasReleased()) {
    for (int i = 0; i < toios.size(); i++) {
      toios.at(i).commandMotor(true, 0x00, true, 0x00);
    }
  }
  // Button C
  if (M5.BtnC.wasPressed()) {
    for (int i = 0; i < toios.size(); i++) {
      toios.at(i).commandMotor(true, 0x16, false, 0x16);
    }
  }
  if (M5.BtnC.wasReleased()) {
    for (int i = 0; i < toios.size(); i++) {
      toios.at(i).commandMotor(true, 0x00, true, 0x00);
    }
  }
}