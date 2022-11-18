#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define FLASH_PORT 4
#define LED_PORT 33

static const char* TAG = "LED";

struct led_state_t {
  uint8_t flash;
  uint8_t led;
};

static struct led_state_t led_state = {0, 0};

static void apply_led_state() {
  gpio_set_level(FLASH_PORT, led_state.flash);
  gpio_set_level(LED_PORT, led_state.led);
}

esp_err_t led_get_handler(httpd_req_t* req) {
  httpd_resp_set_type(req, "application/json");
  char* buf = malloc(100);
  snprintf(buf, 100, "{\"flash\":%d,\"led\":%d}", led_state.flash,
           led_state.led);
  httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t toggle_led_handler(httpd_req_t* req) {
  led_state.led = !led_state.led;
  apply_led_state();
  return led_get_handler(req);
}

esp_err_t toggle_flash_handler(httpd_req_t* req) {
  led_state.flash = !led_state.flash;
  apply_led_state();
  return led_get_handler(req);
}

void led_init() {
  gpio_set_direction(FLASH_PORT, GPIO_MODE_OUTPUT);
  gpio_set_direction(LED_PORT, GPIO_MODE_OUTPUT);
  gpio_set_level(FLASH_PORT, 0);
  gpio_set_level(LED_PORT, 0);
  ESP_LOGI(TAG, "LED initialized");
}