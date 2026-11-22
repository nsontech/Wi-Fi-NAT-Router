#include <Arduino.h>
#include <WiFi.h>

#include "esp_err.h"
#include "esp_netif.h"
#include "dhcpserver/dhcpserver.h"
#include "lwip/ip4_addr.h"

// Replace these with the credentials for the Wi-Fi network that has internet.
static constexpr char UPSTREAM_SSID[] = "YOUR_ROUTER_SSID";
static constexpr char UPSTREAM_PASSWORD[] = "YOUR_ROUTER_PASSWORD";

// Wi-Fi network broadcast by the ESP32.
static constexpr char ACCESS_POINT_SSID[] = "ESP32-NAT-Router";
static constexpr char ACCESS_POINT_PASSWORD[] = "change-this-pass"; // At least 8 characters.

static constexpr uint8_t ACCESS_POINT_CHANNEL = 1;
static constexpr uint8_t MAX_CLIENTS = 4;

static esp_netif_t *s_ap_netif = nullptr;
static bool s_napt_enabled = false;

static bool check_esp_error(esp_err_t err, const char *operation)
{
    if (err == ESP_OK) {
        return true;
    }

    Serial.printf("%s failed: %s\n", operation, esp_err_to_name(err));
    return false;
}

static void configure_client_dns()
{
    // Offer Cloudflare DNS to downstream clients through the AP DHCP server.
    esp_netif_dns_info_t dns = {};
    dns.ip.type = ESP_IPADDR_TYPE_V4;
    IP4_ADDR(&dns.ip.u_addr.ip4, 1, 1, 1, 1);

    dhcps_offer_t offer_dns = OFFER_DNS;
    if (!check_esp_error(esp_netif_dhcps_option(
            s_ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER,
            &offer_dns, sizeof(offer_dns)), "Set DHCP DNS offer")) {
        return;
    }

    check_esp_error(esp_netif_set_dns_info(s_ap_netif, ESP_NETIF_DNS_MAIN, &dns),
                    "Set AP DNS");
}

static void enable_nat()
{
    if (s_napt_enabled || s_ap_netif == nullptr) {
        return;
    }

    if (check_esp_error(esp_netif_napt_enable(s_ap_netif), "Enable NAT")) {
        s_napt_enabled = true;
        Serial.println("NAT enabled on access point");
    }
}

static void on_wifi_event(WiFiEvent_t event)
{
    switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("Connecting to upstream Wi-Fi...");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("Upstream Wi-Fi disconnected; Arduino will reconnect");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("Upstream connected; IP=");
        Serial.println(WiFi.localIP());
        enable_nat();
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        Serial.println("A client connected to the access point");
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        Serial.println("A client disconnected from the access point");
        break;
    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(200);

    WiFi.onEvent(on_wifi_event);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_MODE_APSTA);

    if (!WiFi.softAP(ACCESS_POINT_SSID, ACCESS_POINT_PASSWORD,
                     ACCESS_POINT_CHANNEL, false, MAX_CLIENTS)) {
        Serial.println("Could not start the access point");
        return;
    }

    // Arduino starts the AP netif; use ESP-IDF's network API for DHCP DNS and NAPT.
    s_ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (s_ap_netif == nullptr) {
        Serial.println("Could not find the access-point network interface");
        return;
    }
    configure_client_dns();

    WiFi.begin(UPSTREAM_SSID, UPSTREAM_PASSWORD);

    Serial.printf("Access point '%s' is running; gateway is %s\n",
                  ACCESS_POINT_SSID, WiFi.softAPIP().toString().c_str());
}

void loop()
{
    delay(1000);
}
