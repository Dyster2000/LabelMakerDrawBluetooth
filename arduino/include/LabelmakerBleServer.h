/*
This file is part of LabelMakerDrawBluetooth.

LabelMakerDrawBluetooth is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 or later.

LabelMakerDrawBluetooth is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with LabelMakerDrawBluetooth. If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <BLEServer.h>

class BLEServer;
class BLEService;
class StatusHandler;
class DrawControlHandler;
struct LabelMakerStatus;
struct DrawPoint;

class LabelmakerBleServer : public BLEServerCallbacks
{
public:
  LabelmakerBleServer(LabelMakerStatus &data);
  virtual ~LabelmakerBleServer() = default;

  void Init();

  void Loop();

  bool IsConnected();
  bool IsReceivingImage();
  uint8_t ReceivedPercent();
  size_t GetImagePointCount();
  DrawPoint GetPoint(size_t index);
  void ClearImage();

private:
  void CheckConnection();

  void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override;
  void onDisconnect(BLEServer *pServer) override;
  void onMtuChanged(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) override;

private:
  const char *SERVICE_UUID = "f9f46479-4a8d-4691-886d-fba6e3c2b1f3";

  LabelMakerStatus &m_Data;
  BLEServer *m_pServer;
  BLEService *m_pService;
  StatusHandler *m_pStatus;
  DrawControlHandler *m_pDrawControl;

  bool m_DeviceConnected = false;
  bool m_OldDeviceConnected = false;
};
