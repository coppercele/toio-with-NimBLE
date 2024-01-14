#pragma once

#include <NimBLEDevice.h>
#include "ToioUUID.h"

std::vector<Toio> toios;

extern void bottomMessage(std::string msg);

void scanEndedCB(NimBLEScanResults results);

std::vector<NimBLEAdvertisedDevice *> advDevices;

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results) {
  Serial.println("Scan Ended");
}

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {

  void onResult(NimBLEAdvertisedDevice *advertisedDevice) {

    if (advertisedDevice->isAdvertisingService(serviceUUID)) {
      Serial.println("Found Our Service");
      // 登録済みDeviceの数チェック
      if (advDevices.size() == 0) {
        // 登録数が0ならvectorに追加
        advDevices.push_back(advertisedDevice);
        Serial.printf("onResult:myDevices=%d\n", advDevices.size());
      }
      else {
        for (int i = 0; i < advDevices.size(); i++) {
          // すでに登録されていれば重複チェック
          if (advDevices.at(i)->getAddress().equals(advertisedDevice->getAddress())) {
            Serial.printf("onResult:device already added\n");
            // 重複していたらreturn
            return;
          }
        }
        // 重複がなければvectorに登録
        // advDevice = advertisedDevice;
        advDevices.push_back(advertisedDevice);
        Serial.printf("onResult:myDevices=%d\n", advDevices.size());
      }
      std::string msg = "L button connect " + std::to_string(advDevices.size()) + " toio";
      msg += advDevices.size() == 1 ? "" : "s";
      bottomMessage(msg);
    }
  }
};

// NimBLEのイベントコールバック
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    Serial.println("Connected");
    pClient->updateConnParams(120, 120, 0, 60);
  };

  void onDisconnect(NimBLEClient *pClient) {
    Serial.print(pClient->getPeerAddress().toString().c_str());
    Serial.println(" Disconnected - Starting scan");
    NimBLEDevice::getScan()->start(0, scanEndedCB);
  };
};

/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer() {

  for (int i = 0; i < advDevices.size(); i++) {
    NimBLEClient *pClient = nullptr;
    Serial.printf("Device No. %d\n", i);

    NimBLEAdvertisedDevice *advDevice;
    advDevice = advDevices.at(i);
    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getClientListSize()) {
      pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
      if (pClient) {
        if (!pClient->connect(advDevice, false)) {
          Serial.println("Reconnect failed");
          return false;
        }
        Serial.println("Reconnected client");
      }

      else {
        pClient = NimBLEDevice::getDisconnectedClient();
      }
    }

    /** No client to reuse? Create a new one. */
    if (!pClient) {
      if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
        Serial.println("Max clients reached - no more connections available");
        return false;
      }

      pClient = NimBLEDevice::createClient();

      Serial.println("New client created");

      pClient->setClientCallbacks(&clientCB, false);
      pClient->setConnectionParams(12, 12, 0, 51);
      /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
      pClient->setConnectTimeout(5);

      if (!pClient->connect(advDevice)) {
        /** Created a client but failed to connect, don't need to keep it as it has no data */
        NimBLEDevice::deleteClient(pClient);
        Serial.println("Failed to connect, deleted client");
        return false;
      }
    }

    if (!pClient->isConnected()) {
      if (!pClient->connect(advDevice)) {
        Serial.println("Failed to connect");
        return false;
      }
    }

    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());

    /** Now we can read/write/subscribe the charateristics of the services we are interested in */
    NimBLERemoteService *pSvc = nullptr;
    pSvc = pClient->getService(serviceUUID);

    if (!pSvc) { /** make sure it's not null */
      Serial.println("service not found.");
      continue;
    }

    // pChr = pSvc->getCharacteristic();
    // toioオブジェクト作成
    Toio toio = Toio();
    // Complete Local Nameを設定
    toio.id = i;
    toio.name = advDevice->getName();
    // BLE Addressを設定
    toio.setBleAdress(advDevice->getAddress().toString());
    Serial.printf("toio name: %s\n", toio.name.c_str());
    Serial.printf("toio address: %s\n", toio.getBleAdress().c_str());
    // Characteristicsをすべて取得(trueにしないと全部取得できない)
    std::vector<NimBLERemoteCharacteristic *> *pCharacteristics = pSvc->getCharacteristics(true);
    Serial.printf("pChr->size: %d\n", pCharacteristics->size());

    for (int i = 0; i < pCharacteristics->size(); i++) {

      NimBLERemoteCharacteristic *pChr = pCharacteristics->at(i);

      if (!pChr) { /** make sure it's not null */
        continue;
      }
      if (pChr->getUUID().equals(chrUuidMotor)) {
        // モーター
        Serial.print("Motor chr\n");
        toio.pChrMotor = pChr;
      }
      else if (pChr->getUUID().equals(chrUuidBattery)) {
        // バッテリー
        Serial.print("Battery Chr\n");
        toio.pChrBattery = pChr;
      }
      else if (pChr->getUUID().equals(chrUuidLight)) {
        // ライト
        Serial.print("Light Chr\n");
        toio.pChrLight = pChr;
      }
      else if (pChr->getUUID().equals(chrUuidConfig)) {
        // 設定
        Serial.print("Config Chr\n");
        toio.pChrConfig = pChr;
        // // notifyの通知間隔を設定255(0xFF)0ms間隔
        // uint8_t data[4];
        // data[0] = 0x18;
        // data[1] = 0x00;
        // data[2] = 0x80;
        // data[3] = 0x01;
        // toio.pChrConfig->writeValue(data, 4, false);
      }
      else if (pChr->getUUID().equals(chrUuidMotion)) {
        // 6軸センサ
        Serial.print("Motion Chr\n");
        toio.pChrMotion = pChr;
      }
      else if (pChr->getUUID().equals(chrUuidIdInf)) {
        // ポジションID
        Serial.print("PositionId Chr\n");
        toio.pChrIdInf = pChr;
      }
      // else if (pChr->canNotify()) {

      //   if (!pChr->subscribe(true, notifyCB)) {
      //     /** Disconnect if subscribe failed */
      //     pClient->disconnect();
      //     return false;
      //   }
      //   Serial.print("Notify subscribed ");
      //   Serial.println(pChr->getUUID().toString().c_str());
      // }
    }
    toios.push_back(toio);

    Serial.println("Done with this device!");
  }
  Serial.println("Success!");
  // デバイススキャンを停止
  NimBLEDevice::getScan()->stop();

  return true;
}

void nimBLESetup() {

  NimBLEDevice::init("");

  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */

  /** create new scan */
  NimBLEScan *pScan = NimBLEDevice::getScan();

  /** create a callback that gets called when advertisers are found */
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

  /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(0, scanEndedCB);
}