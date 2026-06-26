# Mica

## Description

Turn your Android phone into a wireless (or USB) microphone for your Linux system.

## System requirements (Linux Listener)

If you use a relatively mainstream Linux distribution, all requirements should be easily met out of the box. Mica was designed with the following core components in mind:

- [**Avahi**](https://avahi.org/): Required for Zeroconf DNS service resolving. Preinstalled on most common distributions.
- [**PulseAudio / PipeWire**](https://wiki.debian.org/PulseAudio): Required to manage the virtual audio devices for the microphone. Preinstalled on almost all desktop distributions.
- [**Systemd**](https://systemd.io/): Used for setting up the background autostart service for the listener. If your system relies on a different init system, you will need to configure the autostart manually.

## Installation

### Linux (Listener)

#### General instructions

To install the Mica listener on your PC two things have to be done.

1. Download the executable and put it in the desired location (for example /usr/bin/)
2. Autostart has to be setup for MicaListener (This varies between distros, so a general approach cannot be described)

#### Debian

The easiest way to install the listener is by using the provided Debian package (`.deb`) from the Releases page:

1. Download the `.deb` package.
2. Double-click to install it via your GUI package manager, or install it via terminal: `sudo apt install ./MicaListener-1.0.0-1.deb`
3. **Important:** Restart your PC to allow systemd to properly hook the background service into your user session (simply logging out and back in is usually not enough).

### 2. Android (Client)

Download and install the Android app on your phone and the listener on your PC. Open the Android app and use the connection switch to connect to your PC. It is possible to connect either per WLAN or per USB-tethering. For a WLAN connection, simply have your phone in the same network as your PC and it should be able to automatically resolve a connection. If this is not the case, then USB-tethering also is an option. Plug in your phone per USB to your PC and set your phone to USB-tethering mode. Then open the app and try to establish a connection like normal and it will automatically make the connection per USB-tethering.

### 3. Usage

The usage of the application is designed to be as simple as possible:

1. Ensure the listener is running on your PC.
2. Open the Mica app on your Android phone.
3. Use the connection switch to link your phone to your PC.

You have two options for connecting:

- **Wi-Fi (WLAN):** Ensure both your phone and PC are on the same local network. The app will automatically discover the PC and establish a connection.
- **USB-Tethering:** If your Wi-Fi network blocks device discovery or you want minimal latency, plug your phone into your PC via USB and enable "USB Tethering" in your Android settings. Open the Mica app and connect normally. It will automatically route the audio through the USB connection.

<img src="Documentation/1.png" width="200" alt="Application screenshot">

## Listener Resource usage

The architecture is designed in a way where the listener on the PC side is actively waiting for a connection. The PC listener is designed to be extremely lightweight.

- **CPU:** ~0.3% of a single core while active, ~0% while inactive (Ryzen 7 5700X)
- **RAM:** ~5MB for the listener while active, ~1MB for the process while inactive (not counting shared libraries)
- **Storage:** ~100kB of disk space

## Troubleshooting

### The listener keeps trying to connect to the old IP

When doing rapid, repeated connection attempts, the system might cache the IP associated with the service and try to reuse it, even if the phone has acquired a new IP-port combination. This can be resolved by flushing the Avahi cache on the PC by doing: `sudo systemctl restart avahi-daemon`

## Contributing

Contributions to the project are welcome and highly appreciated! If you find a bug or have a feature you would like to add, feel free to open an issue or submit a pull request.

A template has been created for creating issues. Please use that template.

## Development requirements

To set up a Debian or derivative system for compiling the C++ listener from source, the following packages are required:

`sudo apt install cmake ninja-build build-essential pkg-config libavahi-client-dev libopenal-dev`

To develop the Android app, only Android Studio and the Android SDK are required.

## License

Distributed under the MIT License. See LICENSE for more information.
