/* i2c_mem.c — xem i2c_mem.h. */
#include "i2c_mem.h"
#include "board.h"

#ifndef I2C_HALF_US
#define I2C_HALF_US 2u
#endif

static void scl_lo(void)  { GPIO0->OUTENSET = PIN_IMU_SCL; }
static void scl_hi(void)  { GPIO0->OUTENCLR = PIN_IMU_SCL; }
static void sda_lo(void)  { GPIO0->OUTENSET = PIN_IMU_SDA; }
static void sda_hi(void)  { GPIO0->OUTENCLR = PIN_IMU_SDA; }
static int  sda_in(void)  { return (GPIO0->DATA & PIN_IMU_SDA) != 0; }
static int  scl_in(void)  { return (GPIO0->DATA & PIN_IMU_SCL) != 0; }
static void dly(void)     { board_delay_us(I2C_HALF_US); }

static void scl_hi_wait(void)                /* chờ kéo dãn xung (tối đa ~100 µs) */
{
    scl_hi();
    for (int i = 0; i < 50 && !scl_in(); i++) board_delay_us(2);
}

void i2c_init(void)
{
    GPIO0->DATAOUT &= ~(PIN_IMU_SCL | PIN_IMU_SDA);
    i2c_release();
}

void i2c_release(void) { GPIO0->OUTENCLR = PIN_IMU_SCL | PIN_IMU_SDA; }

void i2c_hold_low(void)
{
    GPIO0->DATAOUT &= ~(PIN_IMU_SCL | PIN_IMU_SDA);
    GPIO0->OUTENSET = PIN_IMU_SCL | PIN_IMU_SDA;
}

static void start(void) { sda_hi(); scl_hi_wait(); dly(); sda_lo(); dly(); scl_lo(); dly(); }
static void stop(void)  { sda_lo(); dly(); scl_hi_wait(); dly(); sda_hi(); dly(); }

static int wr8(uint8_t b)
{
    for (int i = 7; i >= 0; i--) {
        if ((b >> i) & 1u) sda_hi(); else sda_lo();
        dly(); scl_hi_wait(); dly(); scl_lo();
    }
    sda_hi(); dly(); scl_hi_wait(); dly();
    int ack = !sda_in();
    scl_lo(); dly();
    return ack ? I2C_OK : I2C_NACK;
}

static uint8_t rd8(int ack)
{
    uint8_t b = 0;
    sda_hi();
    for (int i = 0; i < 8; i++) {
        dly(); scl_hi_wait(); dly();
        b = (uint8_t)((b << 1) | (uint8_t)sda_in());
        scl_lo();
    }
    if (ack) sda_lo(); else sda_hi();
    dly(); scl_hi_wait(); dly(); scl_lo(); sda_hi(); dly();
    return b;
}

int i2c_bus_clear(void)
{
    i2c_release();
    dly();
    for (int i = 0; i < 9 && !sda_in(); i++) { scl_lo(); dly(); scl_hi_wait(); dly(); }
    stop();
    return sda_in() ? I2C_OK : I2C_BUSY;
}

int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    start();
    int rc = wr8((uint8_t)(addr << 1));
    if (rc == I2C_OK) rc = wr8(reg);
    if (rc == I2C_OK) rc = wr8(val);
    stop();
    return rc;
}

int i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint32_t n)
{
    start();
    int rc = wr8((uint8_t)(addr << 1));
    if (rc == I2C_OK) rc = wr8(reg);
    if (rc == I2C_OK) {
        start();                             /* repeated start */
        rc = wr8((uint8_t)((addr << 1) | 1u));
    }
    if (rc == I2C_OK)
        for (uint32_t i = 0; i < n; i++) buf[i] = rd8(i + 1u < n);
    stop();
    return rc;
}
