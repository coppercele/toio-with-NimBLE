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
};

Toio::Toio() {
}

// バッテリー残量をintで返す
int Toio::battrey() {
  return pChrBattery->readUInt8();
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
  data[0] = mode;
  data[1] = msec;
  data[2] = 0x01;
  data[3] = 0x01;
  data[4] = red;
  data[5] = green;
  data[6] = blue;
  Serial.printf("%s", pChrLight->getUUID().toString().c_str());

  pChrLight->writeValue(data, 7, false);
  Serial.println("Light command sent");
}
// モーターを動かす
void Toio::commandMotor(bool forwardLeft, uint8_t speedLeft, bool forwardRight, uint8_t speedRight) {
  uint8_t data[7];
  data[0] = 0x01;
  data[1] = 0x01;
  forwardLeft ? data[2] = 0x01 : data[2] = 0x02;
  data[3] = speedLeft;
  data[4] = 0x02;
  forwardRight ? data[5] = 0x01 : data[5] = 0x02;
  data[6] = speedRight;

  Serial.printf("%s", pChrMotor->getUUID().toString().c_str());

  pChrMotor->writeValue(data, 7, false);
  Serial.println("Motor command sent");
}

// 目標座標へ移動する
// directionは移動後にその向きに回転する
void Toio::goTo(u_int16_t toX, uint16_t toY, uint16_t toDirection) {
  uint8_t data[13];
  data[0] = 0x03;
  data[1] = 0x00;
  data[2] = 0x05;
  data[3] = 0x00;
  data[4] = 0x16;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0xFF & toX;
  data[8] = toY >> 8;
  data[9] = 0xFF & toY;
  data[10] = toY >> 8;
  data[11] = 0xFF & toDirection;
  data[12] = toDirection >> 8;
  pChrMotor->writeValue(data, 13, false);
  Serial.printf("data %04x %04x\n", toX, toY);
  Serial.printf("data %02x %02x %02x %02x\n", data[7], data[8], data[9], data[10]);
  Serial.printf("ID:%3d goto %3d:%3d,%3d\n", id, toX, toY, toDirection);
}