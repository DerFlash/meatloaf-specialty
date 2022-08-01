# meatloaf-specialty
Meatloaf for FujiNet

![meatloaf](/images/meatloaf.png)

This is the ESP32 version of [Meatloaf](https://github.com/idolpx/meatloaf) intended for the [FujiNet](https://github.com/FujiNetWIFI/) bring-up on the Commodore 64.

## Quick Instructions

Visual studio code, and the platformio addon installed from the vscode store required.

```
clone this repo
```

Be sure to execute the following command in the project folder to pull the needed submodules.
```
git submodule update --init --recursive
```

```
Copy platform.ini.sample to platform.ini
```

```
Edit platform.ini to match your system
```

```
Copy include/ssid-example.h to include/ssid.h
```

```
Edit ssid.h to match your wifi
```

```
Press the PlatformIO Upload icon arrow at the bottom in the status bar
```

```
meatloaf should now be running on the device
```

 ![platformio_upload](/images/ml-build-1.png)


