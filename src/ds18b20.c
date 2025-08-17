#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ds18b20);

#define DS18B20_NODE DT_ALIAS(ds18b20)

#if !DT_NODE_HAS_STATUS(DS18B20_NODE, okay)
#error "DS18B20 node is not okay"
#endif

static const struct gpio_dt_spec ds18b20 =
    GPIO_DT_SPEC_GET(DS18B20_NODE, gpios);

static void dq_output(void) {
    gpio_pin_configure_dt(&ds18b20, GPIO_OUTPUT);
}
static void dq_input(void) {
    gpio_pin_configure_dt(&ds18b20, GPIO_INPUT | GPIO_PULL_UP);
}
static void dq_write(int val) {
    gpio_pin_set_dt(&ds18b20, val);
}
static int dq_read(void) {
    return gpio_pin_get_dt(&ds18b20);
}

/* 1-Wire reset + presence detect */
static int onewire_reset(void) {
    dq_output();
    dq_write(0);
    k_busy_wait(480);    // pull low 480 µs
    dq_input();
    k_busy_wait(70);     // wait 70 µs
    int presence = !dq_read(); // DS18B20 pulls low
    k_busy_wait(410);    // finish timeslot
    return presence;
}

/* Write a bit */
static void onewire_write_bit(int bit) {
    dq_output();
    dq_write(0);
    if (bit) {
        k_busy_wait(6);   // short low for "1"
        dq_input();
        k_busy_wait(64);
    } else {
        k_busy_wait(60);  // long low for "0"
        dq_input();
        k_busy_wait(10);
    }
}

/* Read a bit */
static int onewire_read_bit(void) {
    int bit;
    dq_output();
    dq_write(0);
    k_busy_wait(6);
    dq_input();
    k_busy_wait(9);   // sample
    bit = dq_read();
    k_busy_wait(55);  // finish slot
    return bit;
}

/* Write a byte */
static void onewire_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

/* Read a byte */
static uint8_t onewire_read_byte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        int bit = onewire_read_bit();
        data >>= 1;
        if (bit) {
            data |= 0x80;
        }
    }
    return data;
}

/* High-level: read temperature */
float ds18b20_read_temp(void) {
    if (!onewire_reset()) {
        LOG_ERR("No DS18B20 detected!");
        return -1000.0f;
    }

    onewire_write_byte(0xCC); // SKIP ROM
    onewire_write_byte(0x44); // CONVERT T

    k_msleep(750); // wait max conversion time

    onewire_reset();
    onewire_write_byte(0xCC); // SKIP ROM
    onewire_write_byte(0xBE); // READ SCRATCHPAD

    uint8_t lsb = onewire_read_byte();
    uint8_t msb = onewire_read_byte();

    int16_t raw = (msb << 8) | lsb;
    return raw / 16.0f; // DS18B20 is 1/16 °C per bit
}

int ds18b20_init()
{
  if (!gpio_is_ready_dt(&ds18b20)) {
        LOG_ERR("GPIO not ready");
        return -1;
    }
    return 0;
}