#pragma once

#include <Arduino.h>
#include <string.h>
#include <NimBLEDevice.h>

class Toio {
private:
  std::string bleAddress;

public:
  std::string name;

  uint8_t id = 0;
  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t direction = 0;

  NimBLERemoteCharacteristic *pChrMotor;
  NimBLERemoteCharacteristic *pChrButton;
  NimBLERemoteCharacteristic *pChrLight;
  NimBLERemoteCharacteristic *pChrBattery;
  NimBLERemoteCharacteristic *pChrPosition;
  NimBLERemoteCharacteristic *pChrMotion;
  NimBLERemoteCharacteristic *pChrIdInf;
  NimBLERemoteCharacteristic *pChrConfig;

  Toio();
  void commandMotor(bool forwardLeft, uint8_t speedLeft, bool forwardRight, uint8_t speedRight);

  void setBleAdress(std::string address);
  std::string getBleAdress();
  bool isMatch(std::string address);
  void light(uint8_t mode, uint8_t msec, uint8_t red, uint8_t green, uint8_t blue);
  int battrey();
  void goTo(u_int16_t x, uint16_t y, uint16_t direction);
  void wait(void *pvParameters);
  void synchronizedMotor(bool forwardLeft, uint8_t speedLeft, bool forwardRight, uint8_t speedRight, ulong now, ulong buffer);
  void update();
};

Toio::Toio() {
}

void Toio::update() {

  // Position ID を読む
  NimBLEAttValue value = pChrIdInf->readValue();
  const uint8_t *pData = value.data();
  int length = value.length();

  // 0リセット
  x = 0;
  y = 0;
  direction = 0;
  if (length == 1 && pData[0] & 0x03) {
    // 0x03(miss)なら座標を0リセット
  }
  else {
    // x,y,directionはuint16_tがuint8_tのリトルエンディアンに分割されている
    x = x | pData[2];
    x = (x << 8) | pData[1];
    y = y | pData[4];
    y = (y << 8) | pData[3];
    direction = direction | pData[6];
    direction = (direction << 8) | pData[5];
    Serial.printf("ID: %d Pos x:%d ", id, x);
    Serial.printf("y:%d ", y);
    Serial.printf("dirc:%d\n", direction);
  }
}

// バッテリー残量をintで返す
int Toio::battrey() {
  return pChrBattery->readValue<uint8_t>();
}
// アクセサメソッド
void Toio::setBleAdress(std::string address) {
  bleAddress = address;
}
// アクセサメソッド
std::string Toio::getBleAdress() {
  return bleAddress;
}
// BLEアドレスが自分の物と一致してるかを返す
bool Toio::isMatch(std::string address) {
  return bleAddress.compare(address);
}
// LEDを点灯・消灯する
// mode:0x03 msec秒点灯後消灯する
void Toio::light(uint8_t mode, uint8_t msec, uint8_t red, uint8_t green, uint8_t blue) {
  uint8_t data[14];
  data[0] = mode;  // 0x03点灯・消灯
  data[1] = msec;  // 点灯時間
  data[2] = 0x01;  // 数
  data[3] = 0x01;  // ID
  data[4] = red;   // 赤 0x00-0xFF
  data[5] = green; // 緑 0x00-0xFF
  data[6] = blue;  // 青 0x00-0xFF
  Serial.printf("%s", pChrLight->getUUID().toString().c_str());

  pChrLight->writeValue(data, 7, false);
  Serial.println("Light command sent");
}
// モーターを動かす
void Toio::commandMotor(bool forwardLeft, uint8_t speedLeft, bool forwardRight, uint8_t speedRight) {
  uint8_t data[7];
  data[0] = 0x01;                                 // モーター制御
  data[1] = 0x01;                                 // 左
  forwardLeft ? data[2] = 0x01 : data[2] = 0x02;  // 回転方向0x01:前
  data[3] = speedLeft;                            // 速度
  data[4] = 0x02;                                 // 右
  forwardRight ? data[5] = 0x01 : data[5] = 0x02; // 回転方向0x01:前
  data[6] = speedRight;                           // 速度

  Serial.printf("%s", pChrMotor->getUUID().toString().c_str());

  pChrMotor->writeValue(data, 7, false);
  Serial.println("Motor command sent");
}

// void Toio::wait(void *pvParameters) {
//   ulong lag = millis() - thisNow; // 関数呼び出しから
//   thisBuffer -= lag;
//   delay(thisBuffer);
// }

// 目標座標へ移動する
// directionは移動後にその向きに回転する
void Toio::goTo(u_int16_t toX, uint16_t toY, uint16_t toDirection) {
  uint8_t data[13];
  data[0] = 0x03; // 0x03 目標指定付きモーター制御
  data[1] = 0x00;
  data[2] = 0x05; // タイムアウト(秒)
  data[3] = 0x01; // 回転しながら移動（後退なし）
  data[4] = 0x16; // 最大速度 MAX:0xFF
  data[5] = 0x00; // 速度一定
  data[6] = 0x00;
  data[7] = 0xFF & toX; // X座標
  data[8] = toY >> 8;
  data[9] = 0xFF & toY; // Y座標
  data[10] = toY >> 8;
  data[11] = 0xFF & toDirection; // 移動後の向き
  data[12] = toDirection >> 8;
  pChrMotor->writeValue(data, 13, false);
  // Serial.printf("data %04x %04x\n", toX, toY);
  // Serial.printf("data %02x %02x %02x %02x\n", data[7], data[8], data[9], data[10]);
  Serial.printf("ID:%3d goto %3d:%3d,%3d\n", id, toX, toY, toDirection);
}