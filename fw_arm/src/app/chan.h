/*
 * chan.h — bit GPIO của M3 theo top-level RTL (gowin/src/top.v) và kế hoạch mục 12.5.6. Đổi ở đây thì đổi cả top.v.
 */
#ifndef CHAN_H
#define CHAN_H

#define PIN_IMU_SCL   (1u << 0)   /* I2C mềm: ghi 0 vào bit dữ liệu, bật/tắt cho phép xuất (cực máng hở) */
#define PIN_IMU_SDA   (1u << 1)
#define PIN_IMU_INT   (1u << 2)   /* vào: MPU6050 báo dữ liệu mới 100 Hz */
#define PIN_WIFI_RST  (1u << 3)   /* 1 = giữ ESP-01S ở reset (qua Q1) */
#define PIN_SENS_PWR  (1u << 4)   /* 1 = cấp nguồn MPU6050 (qua Q2, Q3) */
#define PIN_BUZ       (1u << 5)   /* 1 = còi kêu */
#define PIN_BTN_N     (1u << 6)   /* vào, 0 = nhấn */
#define PIN_LED       (1u << 7)
#define PIN_BUZ_2KHZ  (1u << 8)   /* 0 = còi chủ động (mức cao), 1 = còi thụ động (FPGA phát 2 kHz) */

#define PIN_OUT_MASK  (PIN_WIFI_RST | PIN_SENS_PWR | PIN_BUZ | PIN_LED | PIN_BUZ_2KHZ)

#endif
