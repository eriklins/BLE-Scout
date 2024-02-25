# BLE Scout - Bluetooth LE Tool

BLE Scout is a cross-platform desktop application leveraging the PC integrated Bluetooth controller for accessing Bluetooth LE devices. It supports scanning for BLE advertisements, displaying the exposed advert data, connecting to devices, reading their GATT table and read/write/notify/indicate charcteristics.

## Main Scan Window
After start the main scan window opens and provides common controls for BLE device discovery.
Start/Stop Scanning
Pressing the [Start Scan] button starts scanning for BLE advertisments. Discovered devices will be shown as a panel list underneath. After scanning has started, the button turns into a [Stop Scan] button and BLE scanning stops when pressed again. The [Clear] button clears the panel list of discovered devices.

### Filters
Several filters can be applied to the device scanning. Entering a device name or part of it into the left field will only show devices matching (not case-sensitive). Filtering for BLE MAC addresses or part of it. The mac address has to be entered as hex numbers separated by colons (aa:bb:cc:dd:ee:ff). Filtering on the RSSI value of advertisments which – to some extent – represents the distance of a device. Values have to be entered as negative numbers. Two check boxes allow filtering filtering for only connectable devices or paired devices.

### Log Output
A more detailed log output can be enabled by ticking the Show Log Output box. A second log window will open with more verbose logging information on device scanning and during connection with a device. It's possible to add time stamps to the logging as well as copy the log content to the clipboard or save the log output into a file. The file is automatically named with timestamp information.

### Scan Data Panel
For each discovered device a panel with device information is shown. It contains device name, connectable status, tx power (if available), RSSI value, mac address, advertised services, service data and manufacturer specific data.
If the UUID of a service or the company ID in the manufacturer data is in the official Bluetooth list of assigned numbers, the official name will be shown.
Manufacturer data can be displayed either as HEX numbers or ASCII string. For ASCII non printable characters are displayed as '#'.

### Connect Device
If a device is connectable you can engage a connection with the Connect button.

## Device Connect Window
On connect the GATT table of the device is being read and services, corresponding characteristics and descriptors are shown in a panel structure.
If the UUID of a service, characteristic or descriptor is in the official Bluetooth list of assigned numbers, the official name will be shown.

Depending on the properties of each characteristic different buttons get added to the text field. Those are Read, WriteCommand, WriteRequest, Notify and Indicate. Data from/to a characteristic gets displayed or entered into a text field, which is either read/write or read only. You can switch the presentation between HEX numbers or ASCII string. When set to ASCII, non-printable characters are displayed as a black square.

### Virtual Serial Port (VSP)
Beside the official Bluetooth list of services, there exist proprietary services to expose a virtual serial port service (similar to SPP service from Bluetooth Classic). BLE-Scout knows some of these and if recognized it will show their names as well along with a button to fire up a simple VSP terminal window. Please find below a list of manufactures and proprietary GATT services / characteristics which are currently supported.

* Laird Connectivity with the Virtual Serial Port (VSP)
* Nordic Semiconductors with the Nordic UART Service (NUS)
* u-blox with the u-connectXpress BLE Serial Port Service
 
### Length of RX Characteristic
The length of the peripheral RX characteristic is not exactly known to the central, hence BLE-Scout assumes MTU size minus 3 as best guess. It can be adjusted manually to another value. If the entered data is longer than the characteristic length, it will automatically split into multiple write packets.

### WriteCommand/WriteRequest
Depending on the write properties of the RX characteristic you can select to use write request or write command, if both are exposed.
Pairing Devices

On operating systems, initiating a connection procedure will automatically run pairing if necessary. This process is entirely managed by the operating system, there isn't much that we can do from the user side.

This has the following implications:
* The user will need to manually confirm pairing, until that happens no successful access to protected characteristics is possible.
* Removing a device can only be done from the OS Bluetooth settings page.
* There is no programmatic way of controlling this process.

This applies at least for Windows OS and might be different on other OS platforms (not yet available).

## Limitations
* Since BLE-Scout leverages the PC’s integrated Bluetooth Controller, this ultimately limits what’s achievable from a BT hardware perspective (e.g., number of simultaneous connections).
* It is not possible to tweak BLE settings like scan interval and/or scan window or similar low-level BLE parameters.
* Pairing of BLE devices is not yet implmented.
* Due to a limitation of the underlying BLE library, it’s not possible to properly receive notifications or indications from devices exposing an identcal GATT table (i.e. identical service/characteristic UUIDs).
* The underlying BLE library does not yet expose properties of the descriptors and hence it’s not yet possible to directly read/write them.

## Libraries

### NAppGUI
BLE-Scout uses NAppGUI (https://nappgui.com/en/home/web/home.html) as GUI toolkit and cross-platform SDK.

### SimpleBLE
BLE-Scout BLE Tool uses the SimpleBLE library from https://github.com/OpenBluetoothToolbox/SimpleBLE.

## License
BLE-Scout BLE Tool is released under the MIT License.
Copyright (C) 2024 Erik Lins

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.