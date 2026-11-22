# ESP32 NAT Router (PlatformIO, Arduino)

This project targets a classic ESP32 DevKit. It connects to an existing 2.4 GHz Wi-Fi network and shares that connection through its own Wi-Fi access point. The application uses the Arduino framework; ESP-IDF's bundled network-interface API is used for DHCP DNS and IPv4 NAPT.

## Configure

Edit `src/main.cpp` and set:

- `UPSTREAM_SSID` and `UPSTREAM_PASSWORD` to the Wi-Fi network that has internet access.
- `ACCESS_POINT_SSID` and `ACCESS_POINT_PASSWORD` to the network the ESP32 should broadcast. The password must be at least 8 characters.

The access point uses WPA2, DHCP, and `192.168.4.1` as its gateway. DHCP gives clients Cloudflare DNS (`1.1.1.1`).

## Build and flash

Open this folder in PlatformIO, select the `esp32dev` environment, then run **Build**, **Upload**, and **Monitor**. The same actions are available as `pio run`, `pio run -t upload`, and `pio device monitor` in a PlatformIO terminal.

The project pins PlatformIO Espressif32 8.0.0 (Arduino ESP32 3.3.12) so its bundled lwIP includes IPv4 forwarding and NAPT. The older Arduino 2.0.17 package disabled these features, which made NAT unavailable even after changing the source API.

## Notes

- If your board uses a different ESP32 variant, change the `board` entry in `platformio.ini`.
- ESP32 Wi-Fi is 2.4 GHz only for this setup. Its access point shares the radio/channel with the upstream connection.
- This is a basic NAT router and does not provide an Ethernet WAN port, firewall rules, IPv6 NAT, or a high-bandwidth router replacement.
- Replace the sample AP password before relying on it.
