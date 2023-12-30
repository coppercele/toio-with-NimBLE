#pragma once

#include <NimBLEDevice.h>

// BLE UUIDを列挙
const NimBLEUUID serviceUUID = NimBLEUUID("10b20100-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidIdInf = NimBLEUUID("10b20101-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidMotor = NimBLEUUID("10b20102-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidLight = NimBLEUUID("10b20103-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidSound = NimBLEUUID("10b20104-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidMotion = NimBLEUUID("10b20106-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidButton = NimBLEUUID("10b20107-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidBattery = NimBLEUUID("10b20108-5b3b-4571-9508-cf3efcd7bbae");
const NimBLEUUID chrUuidConfig = NimBLEUUID("10b201FF-5b3b-4571-9508-cf3efcd7bbae");