#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define WIFI_SSID "AndroidAP"
#define WIFI_PASS "nrjy7881"

#define BLINK_GPIO GPIO_NUM_4

// Function prototype
void initIO(void);
void wifi_setup(void);

// event group
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {

	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;

	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;

	case SYSTEM_EVENT_STA_DISCONNECTED:
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;

	default:
		break;
	}

	return ESP_OK;
}

//-------------------------------------
// Main task
typedef enum {
	MAIN_INIT = 0, MAIN_CONNECT, MAIN_DISCONNECT, MAIN_RECONNECT
} main_task_t;

void main_task(void *pvParameter) {

	/*	static main_task_t statusFunc = MAIN_INIT;
	 EventBits_t eventBits;

	 while (1) {
	 switch (statusFunc) {

	 case MAIN_INIT:

	 ESP_ERROR_CHECK(esp_wifi_start())
	 ;

	 printf("Connecting to %s\n", WIFI_SSID);

	 // wait for connection
	 printf("Main task: waiting for connection to the wifi network... ");
	 xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
	 portMAX_DELAY);
	 printf("connected!\n");

	 // print the local IP address
	 tcpip_adapter_ip_info_t ip_info;
	 ESP_ERROR_CHECK(
	 tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info))
	 ;
	 printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	 printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	 printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));

	 statusFunc = MAIN_CONNECT;
	 break;

	 case MAIN_CONNECT:

	 eventBits = xEventGroupGetBits(wifi_event_group);

	 if ((eventBits & CONNECTED_BIT) == 0) {
	 printf("Wifi disconnected\n");
	 printf("Try to reconnect to %s\n", WIFI_SSID);
	 statusFunc = MAIN_RECONNECT;
	 }

	 break;

	 case MAIN_DISCONNECT:

	 break;

	 case MAIN_RECONNECT:
	 ESP_ERROR_CHECK(esp_wifi_connect())
	 ;
	 statusFunc = MAIN_INIT;

	 break;
	 }

	 TickType_t xLastWakeTime = xTaskGetTickCount();
	 //vTaskDelay(1000 / portTICK_RATE_MS);
	 vTaskDelayUntil(&xLastWakeTime, 100);
	 }*/
	// wait for connection
	printf("Main task: waiting for connection to the wifi network... ");
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true,
			portMAX_DELAY);
	printf("connected!\n");

	// print the local IP address
	tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
	printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
	printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
	printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));

	while (1) {
		EventBits_t eventBits;
		eventBits = xEventGroupGetBits(wifi_event_group);

		if ((eventBits & CONNECTED_BIT) == 0) {
			printf("Wifi disconnected\n");
			printf("Try to reconnect to %s\n", WIFI_SSID);
			ESP_ERROR_CHECK(esp_wifi_connect());
		}

		TickType_t xLastWakeTime = xTaskGetTickCount();
		//vTaskDelay(1000 / portTICK_RATE_MS);
		vTaskDelayUntil(&xLastWakeTime, 1000);
	}
}

void blink_task(void *pvParameter) {
	/* Configure the IOMUX register for pad BLINK_GPIO (some pads are
	 muxed to GPIO on reset already, but some default to other
	 functions and need to be switched to GPIO. Consult the
	 Technical Reference for a list of pads and their default
	 functions.)
	 */
//	EventBits_t eventBits;
	gpio_pad_select_gpio(BLINK_GPIO);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
	while (1) {
		/* Blink off (output low) */
		gpio_set_level(BLINK_GPIO, 0);

//		eventBits = xEventGroupGetBits(wifi_event_group);

		if ((xEventGroupGetBits(wifi_event_group) & CONNECTED_BIT) == 0) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
		} else {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}

		/* Blink on (output high) */
		gpio_set_level(BLINK_GPIO, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

// Main application
void app_main() {
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());

	// initialize the tcp stack
	tcpip_adapter_init();
	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_setup();

	initIO();
	// start the main task
	xTaskCreate(&blink_task, "blink_task", 1024, NULL, 5, NULL);

	// create the event group to handle wifi events
	wifi_event_group = xEventGroupCreate();

	// start the main task
	xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}

void initIO(void) {
	gpio_pad_select_gpio(GPIO_NUM_4);
	gpio_pad_select_gpio(GPIO_NUM_5);
	gpio_set_direction(GPIO_NUM_4, GPIO_MODE_DEF_OUTPUT);
	gpio_set_direction(GPIO_NUM_5, GPIO_MODE_DEF_OUTPUT);

}

void wifi_setup(void) {

	// initialize the wifi stack in STAtion mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	// configure the wifi connection and start the interface
	wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password =
	WIFI_PASS, }, };
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
}
