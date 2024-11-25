/*
This file is part of LabelMakerDrawBluetooth.

LabelMakerDrawBluetooth is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 or later.

LabelMakerDrawBluetooth is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with LabelMakerDrawBluetooth. If not, see <https://www.gnu.org/licenses/>.
*/
#include "DrawControlHandler.h"
#include "LabelmakerBleServer.h"
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLE2902.h>

DrawControlHandler::DrawControlHandler(LabelMakerStatus &data, LabelmakerBleServer &server, BLEService &service)
  : m_Data(data)
  , m_Server(server)
  , m_ReceivedDataUpdated(false)
  , m_CurrentIndex(0)
  , m_ReceivingImage(false)
  , m_PercentReceived(0)
  , m_HaveImage(false)
{
  // Create a BLE Characteristic
  m_pCharacteristic = service.createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  // Create a BLE Descriptor
  m_pCharacteristic->addDescriptor(new BLE2902());

  m_pCharacteristic->setCallbacks(this);
}

void DrawControlHandler::Loop()
{
  if (m_ReceivedDataUpdated)
  {
    if (m_Points.size() > 0)
    {
      m_CurrentIndex = 0;

      m_HaveImage = true;
    }
    m_ReceivedDataUpdated = false;
  }
}

bool DrawControlHandler::IsReceivingImage()
{
  return m_ReceivingImage;
}

uint8_t DrawControlHandler::ReceivedPercent()
{
  return m_PercentReceived;
}

size_t DrawControlHandler::GetImagePointCount()
{
  if (m_HaveImage)
    return m_Points.size();
  else
    return 0;
}

DrawPoint DrawControlHandler::GetPoint(size_t index)
{
  if (index < m_Points.size())
    return m_Points[index];
  else
    return DrawPoint();
}

void DrawControlHandler::ClearImage()
{
  m_Points.clear();
  m_HaveImage = false;
  m_PercentReceived = 0;
  m_ReceivedDataUpdated = false;
}

void DrawControlHandler::onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
{
  auto recvLen = m_pCharacteristic->getLength();
  Serial.println("[onWrite] Got bytes=");
  Serial.println(recvLen);
  if ((!m_HaveImage) && (recvLen >= MinSize))// && (recvLen <= MaxSize))
  {
    auto recv = m_pCharacteristic->getData();
    memcpy(&m_ReceivedData, recv, recvLen);

    if (m_ReceivedData.Offset == 0)
    {
      m_Points.resize(m_ReceivedData.TotalSize);
      m_ReceivingImage = true;
      m_PercentReceived = 0;
    }
    else
      m_PercentReceived = m_ReceivedData.Offset * 100 / m_ReceivedData.TotalSize;

    auto cnt = (recvLen - HeaderSize) / PointSize;

    for (uint16_t i = 0; i < cnt; i++)
    {
      m_Points[m_ReceivedData.Offset + i] = m_ReceivedData.Points[i];
    }

    if (m_ReceivedData.Offset + cnt == m_ReceivedData.TotalSize)
    {
      m_ReceivedDataUpdated = true;
      m_ReceivingImage = false;
      m_PercentReceived = 100;
    }
  }
}