#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "keyboard.h"

int main()
{
    stdio_init_all();
    if (cyw43_arch_init())
    {
        printf("cyw43_init_error\n");
        return 0;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    btstack_main();

    while(1) {

    }
    return 0;
}