# Aqua75 Windows RGB Keepalive

This keyboard now turns RGB off after `30` seconds without activity.

A Windows app can keep the RGB alive by sending a Raw HID keepalive packet every `10` seconds.

## Raw HID details

The Aqua75 uses QMK Raw HID defaults:

* Vendor ID: `0x5434`
* Product ID: `0x0750`
* Usage Page: `0xFF60`
* Usage ID: `0x61`
* Report size: `32` bytes

Packet format from the PC to the keyboard:

* Byte `0`: command
* Byte `1`: sequence number
* Byte `2`: host OS hint
* Bytes `3..31`: set to `0`

Commands:

* `0x7F`: keepalive ping

Host OS hint values:

* `0x00`: unknown / no override
* `0x01`: Linux
* `0x02`: Windows
* `0x03`: macOS

Response from the keyboard:

* Byte `0`: `0x81` keepalive ack
* Byte `1`: echoed sequence number
* Byte `2`: `1` if RGB is currently enabled, `0` if not
* Byte `3`: keyboard effective host OS (`0` unsure, `1` Linux, `2` Windows, `3` macOS)
* Bytes `4..31`: `0`

## Windows app outline

Use a HID library that can open vendor-defined HID interfaces. `HidSharp` is one of the simplest options for C#.

Install:

```powershell
dotnet add package HidSharp
```

Minimal example:

```csharp
using HidSharp;
using System.Linq;

const int VendorId = 0x5434;
const int ProductId = 0x0750;
const int UsagePage = 0xFF60;
const int Usage = 0x61;
const int ReportLength = 32;

var device = DeviceList.Local.GetHidDevices(VendorId, ProductId)
    .FirstOrDefault(d => d.UsagePage == UsagePage && d.GetUsage() == Usage);

if (device == null)
{
    Console.WriteLine("Aqua75 raw HID interface not found.");
    return;
}

using var stream = device.Open();

byte sequence = 0;

while (true)
{
    var report = new byte[ReportLength + 1];
    report[1] = 0x7F;
    report[2] = sequence++;
    report[3] = 0x02; // Windows
    stream.Write(report);

    var response = new byte[ReportLength + 1];
    int read = stream.Read(response, 0, response.Length);

    if (read > 2 && response[1] == 0x81)
    {
        Console.WriteLine($"RGB enabled: {response[3] == 1}, effective OS: {response[4]}");
    }

    await Task.Delay(TimeSpan.FromSeconds(10));
}
```

Notes:

* On Windows, many HID libraries expose a leading report ID byte. For QMK Raw HID that report ID is usually `0`, so the `32` byte payload starts at index `1` in the app buffer.
* Send every `10` seconds to stay well under the `30` second timeout.
* If you want RGB to turn off normally, stop sending pings.
