// Helper for ESP-NOW receive callback signature (ESP-IDF v5.x)
#include <esp_now.h>
#include <stdio.h>

void my_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    char mac_str[18];
    if (recv_info && recv_info->src_addr) {
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
    } else {
        snprintf(mac_str, sizeof(mac_str), "NULL");
    }
    printf("Received from %s, len=%d\n", mac_str, len);
    printf("Data: ");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}
