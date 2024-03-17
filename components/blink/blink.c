#include "blink.h"
#include "freertos/FreeRTOS.h"

void configure(gpio_num_t gpio)
{
    gpio_reset_pin(gpio);
    gpio_set_direction(gpio, GPIO_MODE_OUTPUT);
}

void blink(gpio_num_t gpio, uint32_t direction)
{
    gpio_set_level(gpio, direction);
}