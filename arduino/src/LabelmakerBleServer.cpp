/*
This file is part of LabelmakerDrawBluetooth.

LabelmakerDrawBluetooth is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 or later.

LabelmakerDrawBluetooth is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with LabelmakerDrawBluetooth. If not, see <https://www.gnu.org/licenses/>.
*/
#include "LabelmakerBleServer.h"
#include "StatusHandler.h"
#include "DrawControlHandler.h"
#include "LabelmakerData.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

LabelmakerBleServer::LabelmakerBleServer(LabelMakerStatus &data)
  : m_Data(data)
{
}

void LabelmakerBleServer::Init()
{
  // Create the BLE Device
  BLEDevice::init("HackPackLabelMaker");

  // Set max MTU
  esp_err_t err = BLEDevice::setMTU(512);
  Serial.print("[LabelmakerBleServer] setMTU returned ");
  Serial.println(err);

  // Create the BLE Server
  Serial.println("[LabelmakerBleServer] Start service");
  m_pServer = BLEDevice::createServer();
  m_pServer->setCallbacks(this);

  // Create the BLE Service
  Serial.println("[LabelmakerBleServer] Create service");
  BLEService *pService = m_pServer->createService(SERVICE_UUID);

  // Create the BLE Characteristics
  m_pStatus = new StatusHandler(m_Data, *this, *pService);
  m_pDrawControl = new DrawControlHandler(m_Data, *this, *pService);

  // Start the service
  Serial.println("[LabelmakerBleServer] Start service");
  pService->start();

  // Start advertising
  Serial.println("[LabelmakerBleServer] Start advertising");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

void LabelmakerBleServer::Loop()
{
  CheckConnection();

  m_pDrawControl->Loop();
  m_pStatus->Loop();
}

void LabelmakerBleServer::CheckConnection()
{
  // disconnecting
  if (!m_DeviceConnected && m_OldDeviceConnected)
  {
    delay(500);                    // give the bluetooth stack the chance to get things ready
    m_pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    m_OldDeviceConnected = m_DeviceConnected;
  }
  // connecting
  if (m_DeviceConnected && !m_OldDeviceConnected)
  {
    // do stuff here on connecting
    m_OldDeviceConnected = m_DeviceConnected;
  }
}

bool LabelmakerBleServer::IsConnected()
{
  return m_DeviceConnected;
}

bool LabelmakerBleServer::IsReceivingImage()
{
  return m_pDrawControl->IsReceivingImage();
}

uint8_t LabelmakerBleServer::ReceivedPercent()
{
  return m_pDrawControl->ReceivedPercent();
}

size_t LabelmakerBleServer::GetImagePointCount()
{
  return m_pDrawControl->GetImagePointCount();
}

DrawPoint LabelmakerBleServer::GetPoint(size_t index)
{
  return m_pDrawControl->GetPoint(index);
}

void LabelmakerBleServer::ClearImage()
{
  m_pDrawControl->ClearImage();
}

void LabelmakerBleServer::onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  m_DeviceConnected = true;
  auto mtu = pServer->getPeerMTU(param->connect.conn_id);
  Serial.print("Device connected with MTU=");
  Serial.println(mtu);
};

void LabelmakerBleServer::onDisconnect(BLEServer *pServer)
{
  m_DeviceConnected = false;
  Serial.println("Device disconnected...");
}

void LabelmakerBleServer::onMtuChanged(BLEServer* pServer, esp_ble_gatts_cb_param_t* param)
{
  Serial.print("[LabelmakerBleServer::onMtuChanged] MTU=");
  Serial.println(param->mtu.mtu);
}