#include <nappgui.h>

static const char_t help_txt[] = {
"{\\rtf1\\ansi\\ansicpg1252\\deff0\\nouicompat\\deflang1031{\\fonttbl{\\f0\\fnil\\fcharset0 Calibri;}{\\f1\\fnil Calibri;}{\\f2\\fnil\\fcharset2 Symbol;}} \
{\\colortbl ;\\red0\\green128\\blue192;\\red0\\green0\\blue255;} \
{\\*\\generator Riched20 10.0.22621}\\viewkind4\\uc1  \
\\pard\\sa200\\sl276\\slmult1\\cf1\\b\\f0\\fs32\\lang9 BLE Scout - Bluetooth LE Tool - V0.3\\par \
\\cf0\\b0\\fs22 BLE Scout is a cross-platform desktop application leveraging the PC integrated Bluetooth controller for accessing Bluetooth LE devices. It supports scanning for BLE advertisements, displaying the exposed advert data, connecting to devices, reading their GATT table and read/write/notify/indicate charcteristics.\\par \
\\b\\fs28 Main Scan Window\\par \
\\b0\\fs22 After start the main scan window opens and provides common controls for BLE device discovery.\\par \
\\b\\fs24 Start/Stop Scanning\\par \
\\b0\\fs22 Pressing the [Start Scan] button starts scanning for BLE advertisments. Discovered devices will be shown as a panel list underneath. After scanning has started, the button turns into a [Stop Scan] button and BLE scanning stops when pressed again. The [Clear] button clears the panel list of discovered devices.\\par \
\\b\\fs24 Filters\\b0\\fs22\\par \
Several filters can be applied to the device scanning. Entering a device name or part of it into the left field will only show devices matching (not case-sensitive). Filtering for BLE MAC addresses or part of it. The mac address has to be entered as hex numbers separated by colons (aa:bb:cc:dd:ee:ff). Filtering on the RSSI value of advertisments which \\f1\\endash\\f0  to some extent \\f1\\endash\\f0  represents the distance of a device. Values have to be entered as negative numbers. Two check boxes allow filtering filtering for only connectable devices or paired devices.\\par \
\\b\\fs24 Log Output\\par \
\\b0\\fs22 A more detailed log output can be enabled by ticking the Show Log Output box. A second log window will open with more verbose logging information on device scanning and during connection with a device. It's possible to add time stamps to the logging as well as copy the log content to the clipboard or save the log output into a file. The file is automatically named with timestamp information.\\par \
\\b\\fs24 Scan Data Panel\\par \
\\b0\\fs22 For each discovered device a panel with device information is shown. It contains device name, connectable status, tx power (if available), RSSI value, mac address, advertised services, service data and manufacturer specific data.\\par \
If the UUID of a service or the company ID in the manufacturer data is in the official Bluetooth list of assigned numbers, the official name will be shown.\\par \
Manufacturer data can be displayed either as HEX numbers or ASCII string. For ASCII non printable characters are displayed as '#'.\\par \
\\b\\fs24 Connect Device\\par \
\\b0\\fs22 If a device is connectable you can engage a connection with the Connect button.\\par \
\\b\\fs28 Device Connect Window\\par \
\\b0\\fs22 On connect the GATT table of the device is being read and services, corresponding characteristics and descriptors are shown in a panel structure.\\par \
If the UUID of a service, characteristic or descriptor is in the official Bluetooth list of assigned numbers, the official name will be shown. You can hover with the mouse over the name to see the underlying UUID number.\\par \
Depending on the properties of each characteristic different buttons get added to the text field. Those are Read, WriteCommand, WriteRequest, Notify and Indicate. Data from/to a characteristic gets displayed or entered into a text field, which is either read/write or read only. You can switch the presentation between HEX numbers or ASCII string. When set to ASCII, non-printable characters are displayed as a black square.\\par \
\\b\\fs24 Virtual Serial Port (VSP)\\par \
\\b0\\fs22 Beside the official Bluetooth list of services, there exist proprietary services to expose a virtual serial port service (similar to SPP service from Bluetooth Classic). BLE-Scout knows some of these and if recognized it will show their names as well along with a button to fire up a simple VSP terminal window. Please find below a list of manufactures and proprietary GATT services / characteristics which are currently supported.\\par \
 \
\\pard{\\pntext\\f2\\'B7\\tab}{\\*\\pn\\pnlvlblt\\pnf2\\pnindent0{\\pntxtb\\'B7}}\\fi-360\\li720\\sa200\\sl276\\slmult1 Laird Connectivity with the Virtual Serial Port (VSP)\\par \
{\\pntext\\f2\\'B7\\tab}u-blox with the \\tab u-connectXpress BLE Serial Port Service\\par \
{\\pntext\\f2\\'B7\\tab}Nordic Semiconductors with the Nordic UART Service (NUS)\\par \
 \
\\pard\\sa200\\sl276\\slmult1\\b\\fs24 Length of RX Characteristic\\par \
\\b0\\fs22 The length of the peripheral RX characteristic is not exactly known to the central, hence BLE-Scout assumes MTU size minus 3 as best guess. It can be adjusted manually to another value. If the entered data is longer than the characteristic length, it will automatically split into multiple write packets.\\par \
\\b\\fs24 WriteCommand/WriteRequest\\par \
\\b0\\fs22 Depending on the write properties of the RX characteristic you can select to use write request or write command, if both are exposed.\\par \
\\b\\fs24 Pairing Devices\\par \
\\b0\\fs22 On operating systems, initiating a connection procedure will automatically run pairing if necessary. This process is entirely managed by the operating system, there isn't much that we can do from the user side.\\par \
This has the following implications:\\par \
 \
\\pard{\\pntext\\f2\\'B7\\tab}{\\*\\pn\\pnlvlblt\\pnf2\\pnindent0{\\pntxtb\\'B7}}\\fi-360\\li720\\sa200\\sl276\\slmult1 The user will need to manually confirm pairing, until that happens no successful access to protected characteristics is possible.\\par \
{\\pntext\\f2\\'B7\\tab}Removing a device can only be done from the OS Bluetooth settings page.\\par \
{\\pntext\\f2\\'B7\\tab}There is no programmatic way of controlling this process.\\par \
 \
\\pard\\sa200\\sl276\\slmult1 This applies at least for Windows OS and might be different on other OS platforms (not yet available).\\par \
\\b\\fs28 Limitations\\b0\\fs22\\par \
 \
\\pard{\\pntext\\f2\\'B7\\tab}{\\*\\pn\\pnlvlblt\\pnf2\\pnindent0{\\pntxtb\\'B7}}\\fi-360\\li720\\sa200\\sl276\\slmult1 Since BLE-Scout leverages the PC\\rquote s integrated Bluetooth Controller, this ultimately limits what\\rquote s achievable from a BT hardware perspective (e.g., number of simultaneous connections).\\par \
{\\pntext\\f2\\'B7\\tab}It is not possible to tweak BLE settings like scan interval and/or scan window or similar low-level BLE parameters.\\par \
{\\pntext\\f2\\'B7\\tab}Pairing of BLE devices is not yet implmented.\\par \
{\\pntext\\f2\\'B7\\tab}Due to a limitation of the underlying BLE library, it\\rquote s not possible to properly receive notifications or indications from devices exposing an identcal GATT table (i.e. identical service/characteristic UUIDs).\\par \
{\\pntext\\f2\\'B7\\tab}The underlying BLE library does not yet expose properties of the descriptors and hence it\\rquote s not yet possible to directly read/write them.\\par \
 \
\\pard\\sa200\\sl276\\slmult1\\b\\fs28 Libraries\\par \
\\fs24 NAppGUI\\par \
\\b0\\fs22 BLE-Scout uses NAppGUI ({{\\field{\\*\\fldinst{HYPERLINK https://nappgui.com/en/home/web/home.html }}{\\fldrslt{https://nappgui.com/en/home/web/home.html\\ul0\\cf0}}}}\\f0\\fs22 ) as GUI toolkit and cross-platform SDK.\\par \
\\b\\fs24 SimpleBLE\\par \
\\b0\\fs22 BLE-Scout BLE Tool uses the SimpleBLE library from {{\\field{\\*\\fldinst{HYPERLINK https://github.com/OpenBluetoothToolbox/SimpleBLE }}{\\fldrslt{https://github.com/OpenBluetoothToolbox/SimpleBLE\\ul0\\cf0}}}}\\f0\\fs22 .\\par \
\\b\\fs28 License\\par \
\\b0\\fs22 BLE-Scout BLE Tool is released under the MIT License.\\par \
Copyright (C) 2024 Erik Lins\\par \
\\i Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\\par \
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\\par \
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\\par \
} \
  \
"};

