# iproxy2vts

> [!WARNING]
> This is proof of concept software provided as-is without any warranties. Use at your own risk.

iProxy2vts is a lightweight systemd service that bridges VTube Studio tracking data from your iPhone to VTS on Linux over USB.

## Dependencies
  - libimobiledevice-dev 
  - libusbmuxd-dev 
  - iproxy (part of libusbmuxd-tools)
  - libnotify-bin (for notifications)

On Ubuntu/Debian, install them via:
  
```bash
sudo apt-get install libimobiledevice-dev libusbmuxd-dev libusbmuxd-tools libnotify-bin
```

On Arch Linux:
```bash
sudo pacman -S libimobiledevice usbmuxd libnotify
```

## Building & Installing

### Build
```bash
make
```

### Install as systemd user service
```bash
make install
```

This installs:
- Binary to `~/.local/bin/iproxy2vts`
- Systemd service to `~/.config/systemd/user/iproxy2vts.service`

### Enable and start the service
```bash
systemctl --user daemon-reload
systemctl --user enable iproxy2vts
systemctl --user start iproxy2vts
```

### Uninstall
```bash
make uninstall
```