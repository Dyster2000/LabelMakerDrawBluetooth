/*
This file is part of LabelMakerDrawBluetooth.

LabelMakerDrawBluetooth is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 or later.

LabelMakerDrawBluetooth is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with LabelMakerDrawBluetooth. If not, see <https://www.gnu.org/licenses/>.
*/

using Plugin.BLE;
using Plugin.BLE.Abstractions;
using Plugin.BLE.Abstractions.Contracts;

namespace LabelMakerDrawApp;

public class BleHandler
{
    public bool IsConnected { get { return _LabelMakerDevice != null; } }

    public LabelMakerDrawCommandData DrawCommandData { get; private set; }

    public LabelMakerStatusData StatusData { get; private set; }

    public event Notify OnDisconnected;
    public event Notify OnConnected;

    private readonly string DEVICE_NAME = "HackPackLabelMaker";
    private readonly Guid SERVICE_UUID = new Guid("f9f46479-4a8d-4691-886d-fba6e3c2b1f3");
    private readonly Guid STATUS_CHARACTERISTIC_UUID = new Guid("29fac492-6d43-4fa3-8929-3f902a056dfb");
    private readonly Guid DRAW_CONTROL_CHARACTERISTIC_UUID = new Guid("7e623994-f779-4997-94dd-5bc1c2a1265c");

    private ScanPopup _Popup;
    private Page _Owner;
    private CancellationTokenSource CancelControl;
    private readonly IAdapter _bluetoothAdapter;  // Class for the Bluetooth adapter
    private IDevice? _LabelMakerDevice;
    private IService? _LabelMakerService;
    private ICharacteristic? _StatusCharacteristic;
    private ICharacteristic? _DrawControlCharacteristic;

    private readonly object WriteSync = new object();

    public BleHandler(Page owner)
    {
        DrawCommandData = new LabelMakerDrawCommandData();
        StatusData = new LabelMakerStatusData();

        _Owner = owner;

        _bluetoothAdapter = CrossBluetoothLE.Current.Adapter;               // Point _bluetoothAdapter to the current adapter on the phone
        _bluetoothAdapter.DeviceDiscovered += (sender, foundBleDevice) =>   // When a BLE Device is found, run the small function below to add it to our list
        {
            if (foundBleDevice.Device != null && foundBleDevice.Device.Name != null)
            {
                var name = foundBleDevice.Device.Name;

                if (name == DEVICE_NAME)
                {
                    FoundRobot(foundBleDevice.Device);
                }
            }
        };

        _bluetoothAdapter.DeviceConnectionLost += (o, args) =>
        {
            HandleDisconnect();
        };
        _bluetoothAdapter.DeviceDisconnected += (o, args) =>
        {
            HandleDisconnect();
        };
    }

    public async Task<bool> CheckBluetoothStatus()
    {
        try
        {
            var requestStatus = await new BluetoothPermissions().CheckStatusAsync();
            return requestStatus == PermissionStatus.Granted;
        }
        catch (Exception ex)
        {
            return false;
        }
    }

    public async Task SendDrawCommand()
    {
        if (_DrawControlCharacteristic != null && _DrawControlCharacteristic.CanWrite)
        {
            var len = DrawCommandData.DrawPoints.Count;

            for (int i = 0; i < len; i += LabelMakerDrawCommandData.MaxPoints)
            {
                var data = DrawCommandData.Write(i);

                await _DrawControlCharacteristic.WriteAsync(data);
            }
        }
    }

    public async Task<bool> RequestBluetoothAccess()
    {
        try
        {
            var requestStatus = await new BluetoothPermissions().RequestAsync();
            return requestStatus == PermissionStatus.Granted;
        }
        catch (Exception ex)
        {
            // logger.LogError(ex);
            return false;
        }
    }

    public void CancelScan()
    {
        if (CancelControl != null)
            CancelControl.Cancel();
    }

    public async Task Disconnect()
    {
        if (!IsConnected)
            return;

        await _bluetoothAdapter.DisconnectDeviceAsync(_LabelMakerDevice);
        _LabelMakerDevice = null;
        _LabelMakerService = null;
        _StatusCharacteristic = null;
        _DrawControlCharacteristic = null;
    }

    public async Task Scan(ScanPopup popup)           // Function that is called when the scanButton is pressed
    {
        if (IsConnected)
            return;

        _Popup = popup;

        if (!await PermissionsGrantedAsync())
        {
            await _Owner.DisplayAlert("Permission required", "Application needs location permission", "OK");
            return;
        }

        if ((_LabelMakerDevice == null) && (!_bluetoothAdapter.IsScanning))
        {
            ScanFilterOptions scanOptions = new ScanFilterOptions();
            scanOptions.DeviceNames = [DEVICE_NAME];
            CancelControl = new CancellationTokenSource();

            await _bluetoothAdapter.StartScanningForDevicesAsync(scanOptions, null, false, CancelControl.Token);
        }
        if (_LabelMakerDevice == null)
        {
            CancelControl = null;
            return;
        }
        // Found robot
        await _bluetoothAdapter.StopScanningForDevicesAsync();
        await _bluetoothAdapter.ConnectToDeviceAsync(_LabelMakerDevice);
        await FindService();
        if (_LabelMakerService != null)
        {
            await FindCharacteristics();
        }

        CancelControl = null;
        StatusData.JustConnected = true;
        OnConnected?.Invoke();

        if (_StatusCharacteristic != null && _StatusCharacteristic.CanUpdate)
        {
            // define a callback function
            _StatusCharacteristic.ValueUpdated += (o, args) =>
            {
                var receivedBytes = args.Characteristic.Value;

                if (receivedBytes != null)
                    StatusData.Read(receivedBytes);
            };
            await _StatusCharacteristic.StartUpdatesAsync();
        }
    }

    private void FoundRobot(IDevice device)
    {
        _Popup.SetMessage("Found label maker, looking for service");
        _LabelMakerDevice = device;
        CancelControl.Cancel();
    }

    protected async Task FindService()
    {
        if (_LabelMakerDevice == null)
        {
            throw new ArgumentNullException(nameof(_LabelMakerDevice), "Parameter cannot be null");
        }
        try
        {
            var servicesListReadOnly = await _LabelMakerDevice.GetServicesAsync();

            for (int i = 0; i < servicesListReadOnly.Count; i++)
            {
                if (servicesListReadOnly[i].Id == SERVICE_UUID)
                {
                    _LabelMakerService = servicesListReadOnly[i];
                    _Popup.SetMessage("Found Service");

                    //***var mtu = await _LabelMakerDevice.RequestMtuAsync(512);
                    //***Console.WriteLine($"MTU is {mtu} <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");

                    break;
                }
            }
        }
        catch
        {
            await _Owner.DisplayAlert("Error initializing", $"Error initializing UART GATT service.", "OK");
        }
    }

    protected async Task FindCharacteristics()
    {
        if (_LabelMakerService == null)
        {
            throw new ArgumentNullException(nameof(_LabelMakerDevice), "Parameter cannot be null");
        }
        try
        {
            var charListReadOnly = await _LabelMakerService.GetCharacteristicsAsync();

            for (int i = 0; i < charListReadOnly.Count; i++)
            {
                if (charListReadOnly[i].Id == STATUS_CHARACTERISTIC_UUID)
                {
                    _Popup.SetMessage("Found Status Characteristics");
                    _StatusCharacteristic = charListReadOnly[i];
                }
                else if (charListReadOnly[i].Id == DRAW_CONTROL_CHARACTERISTIC_UUID)
                {
                    _Popup.SetMessage("Found Draw Control Characteristics");
                    _DrawControlCharacteristic = charListReadOnly[i];
                }
            }
        }
        catch
        {
            await _Owner.DisplayAlert("Error initializing", $"Error initializing UART GATT service.", "OK");
        }
    }

    private async Task<bool> PermissionsGrantedAsync()
    {
        var status = await Permissions.CheckStatusAsync<Permissions.LocationWhenInUse>();

        if (status != PermissionStatus.Granted)
        {
            status = await Permissions.RequestAsync<Permissions.LocationWhenInUse>();
        }

        return status == PermissionStatus.Granted;
    }

    private void HandleDisconnect()
    {
        if (IsConnected)
        {
            _LabelMakerDevice = null;
            _LabelMakerService = null;
            _StatusCharacteristic = null;
            _DrawControlCharacteristic = null;
            OnDisconnected?.Invoke();
        }
    }
}
