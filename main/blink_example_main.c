/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "iot_button.h"
#include "iot_knob.h"
#include "sdkconfig.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 8
#define GPIO_BUTTON 9
#define GPIO_KNOB_A 10
#define GPIO_KNOB_B 6

#define DIAL_R 0xC8
#define DIAL_L 0x38
#define DIAL_PRESS 0x01
#define DIAL_RELEASE 0x00

static uint8_t s_led_state = 0;
static uint8_t s_led_red = 5;
static uint8_t s_led_green = 0;
static uint8_t s_led_blue = 0;
static button_handle_t s_btn = 0;
static knob_handle_t s_knob = 0;
static uint16_t s_led_period = 1000;

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

static void _button_press_down_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN: BUTTON_PRESS_DOWN");
    if (s_led_red)
    {
        s_led_red = 0;
        s_led_green = 5;
    }
    else if (s_led_green)
    {
        s_led_green = 0;
        s_led_blue = 5;
    }
    else
    {
        s_led_blue = 0;
        s_led_red = 5;
    }
}

static void _button_press_up_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN: BUTTON_PRESS_UP");
}

static void _knob_right_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "KONB: KONB_RIGHT");
    if (s_led_period < 3100)
        s_led_period = s_led_period + 100;
}

static void _knob_left_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "KONB: KONB_LEFT");
    if (s_led_period > 100)
        s_led_period = s_led_period - 100;
}

static void _button_init(void)
{
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 1000,
        .short_press_time = 200,
        .gpio_button_config = {
            .gpio_num = GPIO_BUTTON,
            .active_level = 0,
        },
    };
    s_btn = iot_button_create(&cfg);
    iot_button_register_cb(s_btn, BUTTON_PRESS_DOWN, _button_press_down_cb, NULL);
    iot_button_register_cb(s_btn, BUTTON_PRESS_UP, _button_press_up_cb, NULL);
}

static void _knob_init(void)
{
    knob_config_t cfg = {
        .default_direction = 0,
        .gpio_encoder_a = GPIO_KNOB_A,
        .gpio_encoder_b = GPIO_KNOB_B,
    };
    s_knob = iot_knob_create(&cfg);
    if (NULL == s_knob)
    {
        ESP_LOGE(TAG, "knob create failed");
    }

    iot_knob_register_cb(s_knob, KNOB_LEFT, _knob_left_cb, NULL);
    iot_knob_register_cb(s_knob, KNOB_RIGHT, _knob_right_cb, NULL);
}

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state)
    {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, s_led_red, s_led_green, s_led_blue);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    }
    else
    {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "portTICK_PERIOD_MS %lu", portTICK_PERIOD_MS);

    /* Configure the peripheral according to the LED type */
    configure_led();

    _button_init();
    _knob_init();

    while (1)
    {
        ESP_LOGI(TAG, "s_led_period %d", s_led_period);
        // ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        /* Toggle the LED state */
        s_led_state = !s_led_state;
        vTaskDelay(s_led_period / portTICK_PERIOD_MS);
    }
}
