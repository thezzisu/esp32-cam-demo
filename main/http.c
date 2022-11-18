#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include "esp_eth.h"
#include "esp_netif.h"
#include "esp_tls_crypto.h"

esp_err_t jpg_httpd_handler(httpd_req_t *req);
esp_err_t jpg_stream_httpd_handler(httpd_req_t *req);

static const char *TAG = "HTTP";
static const char *index_html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>ESP32-CAM</title>"
    "</head>"
    "<body>"
    "<center><h1>ESP32-CAM</h1></center>"
    "<center><img src=\"stream.mjpg\" width=\"640\" height=\"480\"></center>"
    "</body>"
    "</html>";

static esp_err_t index_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, R"", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static const httpd_uri_t index = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler,
};

static esp_err_t restart_post_handler(httpd_req_t *req) {
  httpd_resp_send_chunk(req, NULL, 0);
  esp_restart();
  return ESP_OK;
}

static const httpd_uri_t restart = {
    .uri = "/restart",
    .method = HTTP_POST,
    .handler = restart_post_handler,
};

static const httpd_uri_t capture = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = jpg_httpd_handler,
};

static const httpd_uri_t stream = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = jpg_stream_httpd_handler,
};

static httpd_handle_t start_webserver() {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &index);
    httpd_register_uri_handler(server, &capture);
    httpd_register_uri_handler(server, &stream);
    httpd_register_uri_handler(server, &restart);
#if CONFIG_EXAMPLE_BASIC_AUTH
    httpd_register_basic_auth(server);
#endif
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server) {
  // Stop the httpd server
  return httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  httpd_handle_t *server = (httpd_handle_t *)arg;
  if (*server) {
    ESP_LOGI(TAG, "Stopping webserver");
    if (stop_webserver(*server) == ESP_OK) {
      *server = NULL;
    } else {
      ESP_LOGE(TAG, "Failed to stop http server");
    }
  }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data) {
  httpd_handle_t *server = (httpd_handle_t *)arg;
  if (*server == NULL) {
    ESP_LOGI(TAG, "Starting webserver");
    *server = start_webserver();
  }
}

void http_init() {
  static httpd_handle_t server = NULL;
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &connect_handler, &server));
  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
  server = start_webserver();
}