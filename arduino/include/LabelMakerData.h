/*
This file is part of LabelMakerDrawBluetooth.

LabelMakerDrawBluetooth is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 or later.

LabelMakerDrawBluetooth is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with LabelMakerDrawBluetooth. If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#pragma pack(1)

struct DrawPoint
{
  uint8_t X{0};
  uint8_t Y{0};
  uint8_t Draw{0};
};

struct LabelMakerStatus
{
  uint8_t State{false};
};

#pragma pack()
