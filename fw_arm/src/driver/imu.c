/* imu.c — xem imu.h. Cấu hình (giữ như bản dự thi): 100 Hz, ±16 g, ±2000 °/s, DLPF 44 Hz,
 * INT mức cao đẩy kéo khi có dữ liệu mới. Thanh ghi theo docs/datasheet/MPU-6000_MPU-6050_Register_Map_Rev4.0.pdf (RM-MPU-6000A-00): đã đối chiếu 0x6B, 0x1A, 0x19, 0x1B, 0x1C, 0x37, 0x38, 0x3B, 0x75. */
#include "imu.h"
#include "i2c_mem.h"
#include "board.h"

static const uint8_t k_cfg[][2] = {
    { 0x6B, 0x01 },     /* PWR_MGMT_1: đánh thức, clock PLL theo gyro X */
    { 0x1A, 0x03 },     /* CONFIG: DLPF accel 44 Hz, gyro 42 Hz */
    { 0x19, 0x09 },     /* SMPLRT_DIV: 1 kHz / 10 = 100 Hz */
    { 0x1B, 0x18 },     /* GYRO_CONFIG: ±2000 °/s */
    { 0x1C, 0x18 },     /* ACCEL_CONFIG: ±16 g */
    { 0x37, 0x00 },     /* INT_PIN_CFG: xung mức cao, đẩy kéo, xóa khi đọc INT_STATUS */
    { 0x38, 0x01 },     /* INT_ENABLE: data ready */
};

int imu_config(void)
{
    uint8_t v = 0;
    if (i2c_read(IMU_ADDR, 0x75, &v, 1) != I2C_OK || v != 0x68u) return 0;
    for (unsigned i = 0; i < sizeof(k_cfg) / sizeof(k_cfg[0]); i++)
        if (i2c_write_reg(IMU_ADDR, k_cfg[i][0], k_cfg[i][1]) != I2C_OK) return 0;
    for (unsigned i = 0; i < sizeof(k_cfg) / sizeof(k_cfg[0]); i++)
        if (i2c_read(IMU_ADDR, k_cfg[i][0], &v, 1) != I2C_OK || v != k_cfg[i][1]) return 0;
    return 1;
}

int imu_read(int16_t raw[6])
{
    uint8_t b[14];
    if (i2c_read(IMU_ADDR, 0x3B, b, 14) != I2C_OK) return 0;
    for (int i = 0; i < 3; i++) {
        raw[i]     = (int16_t)((b[2 * i] << 8) | b[2 * i + 1]);
        raw[3 + i] = (int16_t)((b[8 + 2 * i] << 8) | b[9 + 2 * i]);
    }
    return 1;
}

int imu_do(ig_act_t a)
{
    switch (a) {
    case IG_ACT_BUS_CLEAR_CFG:
        (void)i2c_bus_clear();
        return imu_config();
    case IG_ACT_SOFT_RESET:
        (void)i2c_write_reg(IMU_ADDR, 0x6B, 0x80);          /* DEVICE_RESET */
        return -1;
    case IG_ACT_CFG:
        return imu_config();
    case IG_ACT_POWER_OFF:
        i2c_hold_low();                                      /* chặn cấp điện ngược qua R29, R30 và kéo lên của GY-521 */
        board_set(PIN_SENS_PWR, 0);
        return -1;
    case IG_ACT_POWER_ON:
        board_set(PIN_SENS_PWR, 1);
        return -1;
    case IG_ACT_RELEASE_CFG:
        i2c_release();
        (void)i2c_bus_clear();
        return imu_config();
    default:
        return -1;
    }
}
