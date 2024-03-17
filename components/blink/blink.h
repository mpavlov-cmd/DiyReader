#ifndef BLINK_H
#define BLINK_H

#include "driver/gpio.h"

void configure(gpio_num_t gpio);
void blink(gpio_num_t gpio, uint32_t direction);

#endif