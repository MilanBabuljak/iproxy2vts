# Technical Documentation for iproxy2vts

## Overview
iProxy2vts is a lightweight tool that bridges traffic from iProxy to VTube Studio. Yes, it could be probably some kind of shell script but whatever. This tool allows Linux users to utilize iPhone tracking over USB by forwarding the necessary ports.

## How it Works
iProxy2vts leverages `usbmuxd`, a daemon that facilitates communication between iOS devices and host computers over USB. It uses `iproxy`, a tool that forwards TCP connections from a local port to a port on the iOS device. iProxy is a part of the `libusbmuxd` package under `libimobiledevice`. 

VTube Studio runs a server on the iPhone that listens for tracking data on port `25561`. Since VTube studio already has a TCP connection that can be estabilished over wireless network, it doesn't make sense for denshisoft to implement a custom protocol for USB connection. With this realization, iProxy2vts forwards the traffic from the local port `25565` (where VTube Studio on under wine usually listens) to the iPhone's port `25561` via USB. 

Both iProxy and VTube studio listen on TCP for connections, therefor it is nescessary to handle TCP socket connections and forward data between them. iProxy2vts uses `epoll`, a scalable I/O event notification mechanism, to efficiently monitor multiple file descriptors (sockets) for incoming data.