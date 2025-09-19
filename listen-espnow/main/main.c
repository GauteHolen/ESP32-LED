

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "espnow_receiver.h"

void app_main(void)
{
	espnow_receiver_init();
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}