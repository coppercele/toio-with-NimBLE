#include <Arduino.h>
#include <M5Unified.h>
#include "Toio.h"
#include "ToioUUID.h"
#include <NimBLEDevice.h>

std::vector<Toio> toios;

void scanEndedCB(NimBLEScanResults results);

std::vector<NimBLEAdvertisedDevice *> advDevices;

static uint32_t scanTime = 0; /** 0 = scan forever */

// 画面の一番上にメッセージを表示
void bottomMessage(std::string msg) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setCursor(0, 220);
  M5.Display.printf("%s", msg.c_str());
}

// NimBLEのイベントコールバック
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *pClient) {
    Serial.println("Connected");
    pClient->updateConnParams(120, 120, 0, 60);
  };

  void onDisconnect(NimBLEClient *pClient) {
    Serial.print(pClient->getPeerAddress().toString().c_str());
    Serial.println(" Disconnected - Starting scan");
    NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
  };

  bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params) {
    if (params->itvl_min < 24) { /** 1.25ms units */
      return false;
    }
    else if (params->itvl_max > 40) { /** 1.25ms units */
      return false;
    }
    else if (params->latency > 2) { /** Number of intervals allowed to skip */
      return false;
    }
    else if (params->supervision_timeout > 100) { /** 10ms units */
      return false;
    }

    return true;
  };

  uint32_t onPassKeyRequest() {
    Serial.println("Client Passkey Request");
    /** return the passkey to send to the server */
    return 123456;
  };

  bool onConfirmPIN(uint32_t pass_key) {
    Serial.print("The passkey YES/NO number: ");
    Serial.println(pass_key);
    /** Return false if passkeys don't match. */
    return true;
  };

  /** Pairing process complete, we can check the results in ble_gap_conn_desc */
  void onAuthenticationComplete(ble_gap_conn_desc *desc) {
    if (!desc->sec_state.encrypted) {
      Serial.println("Encrypt connection failed - disconnecting");
      /** Find the client with the connection handle provided in desc */
      NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
      return;
    }
  };
};

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

const uint8_t BUTTON_DOWN = 0x80;
const uint8_t BUTTON_UP = 0x00;

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  std::string str = (isNotify == true) ? "Notification" : "Indication";
  str += " from ";

  for (int i = 0; i < toios.size(); i++) {
    // BLE addressからtoioのIDを特定する
    if (toios.at(i).isMatch(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress())) {
      Serial.print(str.c_str());
      Serial.printf("ID: %d Name: %s ", i, toios.at(i).name.c_str());
      break;
    }
  }

  Serial.print("data: ");
  for (int i = 0; i < length; i++) {
    // uint8_tを頭0のstringで表示する
    Serial.printf("%02X ", pData[i]);
  }
  Serial.print("\n");

  int id;
  for (id = 0; id < toios.size(); id++) {
    // BLE addressからtoioのIDを特定する
    if (toios.at(id).isMatch(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress())) {
      break;
    }
  }

  if (pRemoteCharacteristic->getUUID().equals(chrUuidButton)) {
    // Button Characteristic
    // ボタンの状態を表示
    if (pData[1] & BUTTON_DOWN) {
      M5.Display.setCursor(0, 220);
      M5.Display.printf("ID: %d Button ON ", id);
    }
    else {
      M5.Display.setCursor(0, 220);
      M5.Display.printf("ID: %d Button OFF", id);
    }
  }
  else if (pRemoteCharacteristic->getUUID().equals(chrUuidIdInf)) {
    // Position Characteristic
    // 0x03(miss)なら座標を0リセット
    if (length == 1 && pData[0] & 0x03) {
      toios[id].x = 0;
      toios[id].y = 0;
      toios[id].direction = 0;
    }
    else {
      // x,y,directionはuint16_tがuint8_tのリトルエンディアンに分割されている
      uint16_t x = 0;
      x = x | pData[2];
      x = (x << 8) | pData[1];
      uint16_t y = 0;
      y = y | pData[4];
      y = (y << 8) | pData[3];
      uint16_t direction = 0;
      direction = direction | pData[6];
      direction = (direction << 8) | pData[5];
      toios[id].x = x;
      toios[id].y = y;
      toios[id].direction = direction;
      Serial.printf("ID: %d Pos x:%d ", id, x);
      Serial.printf("y:%d ", y);
      Serial.printf("dirc:%d\n", direction);
    }

    for (int i = 0; i < toios.size(); i++) {
      // 画面に表示
      M5.Display.setCursor(0, i * 20);
      M5.Display.printf("ID: %2d x:%3u y:%3u d:%3u", i, toios[i].x, toios[i].y, toios[i].direction);
    }
  }
}

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results) {
  Serial.println("Scan Ended");
}

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
      // モーター、バッテリー、ライト、6軸センサはnotifyさせない
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
        // notifyの通知間隔を設定255(0xFF)0ms間隔
        uint8_t data[4];
        data[0] = 0x18;
        data[1] = 0x00;
        data[2] = 0x80;
        data[3] = 0x01;
        toio.pChrConfig->writeValue(data, 4, false);
      }
      else if (pChr->getUUID().equals(chrUuidMotion)) {
        // 6軸センサ
        Serial.print("Motion Chr\n");
        toio.pChrMotion = pChr;
      }
      else if (pChr->canNotify()) {

        if (!pChr->subscribe(true, notifyCB)) {
          /** Disconnect if subscribe failed */
          pClient->disconnect();
          return false;
        }
        Serial.print("Notify subscribed ");
        Serial.println(pChr->getUUID().toString().c_str());
      }
    }
    toios.push_back(toio);

    Serial.println("Done with this device!");
  }
  return true;
}

void setup() {
  // Serial.begin(115200);
  auto cfg = M5.config(); // M5Stack初期設定用の構造体を代入
  // configを設定する場合はここで設定
  // 例
  // cfg.external_spk = true;

  M5.begin(cfg); // M5デバイスの初期化

  M5.Display.setTextSize(2); // テキストサイズを変更
  Serial.println("Starting NimBLE Client");

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
  pScan->start(scanTime, scanEndedCB);
  std::string msg = "Searching toios...";
  bottomMessage(msg);
}

// １分ごとにバッテリー残量を読む
void readBattery(void *pvParameters) {
  while (true) {
    for (int i = 0; i < toios.size(); i++) {

      Serial.printf("ID: %d Battery %d％\n", i, toios.at(i).battrey());
    }

    delay(60000);
  }
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) {
    if (toios.size() == 0) {
      if (advDevices.size() == 0) {
        // advertiseが見つかっていければ中止
        return;
      }
      // toioが接続されてなければ接続開始
      if (connectToServer()) {

        Serial.println("Success! we should now be getting notifications");
        // デバイススキャンを停止
        NimBLEDevice::getScan()->stop();

        // バッテリー容量を60秒ごとにreadするタスク
        xTaskCreateUniversal(
            readBattery,
            "readBattery",
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
    // toioがつながってなければボタン無効
    return;
  }

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