#ifndef CONFIG_H
#define CONFIG_H

#ifndef portTICK_PERIOD_MS
#ifdef configTICK_RATE_HZ
#define portTICK_PERIOD_MS (1000 / configTICK_RATE_HZ)
#else
#define portTICK_PERIOD_MS 1
#endif
#endif
#define LED_PIN 2
#define LOG_UART true

#define REFRESH_SEND_HZ 200
#define REFRESH_USB_HZ 200

#endif // CONFIG_H