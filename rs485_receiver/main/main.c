#include <string.h>
#include "driver/uart.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"
#include "esp_log.h"
#include "setup_uart.h"
#include "receiver.h"

void app_main(void)
{
    setup_uart();

    while (1) {
        receive_message_task();
    }
}