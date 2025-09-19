#include <sys/socket.h>
#include <netinet/in.h>
#include "esp_timer.h"



#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"


#define WIFI_SSID      "thewifi24Ghz"
#define WIFI_PASS      "thewifipassword"

static void wifi_init(void) {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	ESP_ERROR_CHECK(esp_wifi_init(&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT()));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASS,
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	printf("Connecting to WiFi...\n");
	ESP_ERROR_CHECK(esp_wifi_connect());

	// Wait for connection, yield to scheduler to avoid WDT
	wifi_ap_record_t ap_info;
	int retry = 0;
	while (retry < 100) { // ~10 seconds max
		if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
			printf("Connected to AP: %s\n", (char *)ap_info.ssid);
			break;
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
		retry++;
	}
	if (retry == 100) {
		printf("WiFi connection timeout!\n");
	}
}


void udp_server_task(void *pvParameters)
{
	const int port = 8000;
	int sock;
	struct sockaddr_in server_addr;
	struct sockaddr_storage source_addr;
	socklen_t socklen = sizeof(source_addr);
	char rx_buffer[128];

	// Create UDP socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock < 0) {
		printf("Unable to create socket: errno %d\n", errno);
		vTaskDelete(NULL);
		return;
	}

	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);

	// Bind socket to port
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("Socket unable to bind: errno %d\n", errno);
		close(sock);
		vTaskDelete(NULL);
		return;
	}
	printf("UDP server listening on port %d\n", port);

	while (1) {
		int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
						  (struct sockaddr *)&source_addr, &socklen);
		if (len < 0) {
			printf("recvfrom failed: errno %d\n", errno);
			break;
		} else if (len >= 32) {
			char addr_str[INET_ADDRSTRLEN];
			struct sockaddr_in *src_addr_in = (struct sockaddr_in *)&source_addr;
			inet_ntop(AF_INET, &src_addr_in->sin_addr, addr_str, sizeof(addr_str));
			printf("Received %d bytes from %s:%d\n", len,
				   addr_str, ntohs(src_addr_in->sin_port));
			// OSC decode: /nabo, ,iiii, 4 int32 args
			const uint8_t *data = (const uint8_t *)rx_buffer;
			// Parse address (should be "/nabo")
			if (memcmp(data, "/nabo", 5) == 0) {
				// Find end of address (null-terminated, padded to 4 bytes)
				int addr_end = 0;
				while (addr_end < len && data[addr_end] != 0) addr_end++;
				addr_end = (addr_end + 4) & ~3; // pad to next multiple of 4
				// Parse type tag (should be ,iiii)
				if (memcmp(&data[addr_end], ",iiii", 5) == 0) {
					int tag_end = addr_end;
					while (tag_end < len && data[tag_end] != 0) tag_end++;
					tag_end = (tag_end + 4) & ~3;
					// Now parse 4 int32s (big-endian)
					if (tag_end + 16 <= len) {
						uint8_t out[4];
						for (int i = 0; i < 4; ++i) {
							int32_t val = (data[tag_end + i*4] << 24) |
										  (data[tag_end + i*4 + 1] << 16) |
										  (data[tag_end + i*4 + 2] << 8) |
										  (data[tag_end + i*4 + 3]);
							out[i] = (uint8_t)val;
						}
						printf("OSC /nabo: header: %u, fixture_id: %u, control: %u, value: %u\n",
							   out[0], out[1], out[2], out[3]);
					} else {
						printf("OSC /nabo: not enough data for 4 int32 args\n");
					}
				} else {
					printf("OSC: type tag not ,iiii\n");
				}
			} else {
				printf("OSC: address not /nabo\n");
			}

		} else if (len > 0) {
			printf("Received packet too short (%d bytes)\n", len);
		}
		// Simple delay to avoid tight loop
		esp_rom_delay_us(10000); // 10 ms
	}

	if (sock != -1) {
		printf("Shutting down socket...\n");
		close(sock);
	}
	vTaskDelete(NULL);
}

void app_main(void)
{
	wifi_init();
	xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}