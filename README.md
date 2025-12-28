# iproxy2vts

> [!WARNING]
> This is proof of concept software provided as-is without any warranties. Use at your own risk.
> The code quality is horrible due to it being written in less then 2 hours just to test the idea.

iProxy2vts is a lightweight tool that bridges traffic from iProxy to VTube Studio, allowing Linux users to utilize iPhone tracking over USB.

## Dependencies
  - libimobiledevice-dev 
  - libusbmuxd-dev 
  - iproxy

on Ubuntu/Debian, install them via:
  
  ```bash
  sudo apt-get install libimobiledevice-dev libusbmuxd-dev iproxy
  ```

## Building iProxy2vts
1. Make sure you have all dependencies installed.
2. `make`
3. ???
4. Profit?

The resulting binary will be named `iproxy2vts`.

## Running iProxy2vts
1. Connect your iPhone to your Linux machine via USB.

> [!CAUTION]
> Make sure to trust the computer on your iPhone when prompted. If not prompted, use `idevicepair pair` from the command line. After that, check the connection with `idevicepair validate`.

2. Start VTube Studio on your iPhone and make sure USB is enabled under USB Settings.
3. Start VTube Studio on your Linux machine, make sure it's set to port 25565.
4. Run iProxy2vts
5. ???
6. Profit?