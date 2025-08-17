#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "ds18b20.h"

LOG_MODULE_REGISTER(main);

int main(void) {

    LOG_INF("nRF7002DK + DS18B20 example");
    ds18b20_init();
    while (1) {
        float temp = ds18b20_read_temp();
        LOG_INF("Temperature: %.2f °C", temp);
        k_sleep(K_SECONDS(2));
    }
    return 0;
}
