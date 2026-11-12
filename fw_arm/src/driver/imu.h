/* imu.h — MPU6050 (module GY-521, AD0 = GND → 0x68) qua I²C mềm; thi hành hành động của core/imu_guard.c. */
#ifndef IMU_H
#define IMU_H
#include <stdint.h>
#include "imu_guard.h"

#define IMU_ADDR 0x68u

int  imu_config(void);                        /* WHO_AM_I = 0x68, ghi cấu hình, đọc lại kiểm; 1 = đạt */
int  imu_read(int16_t raw[6]);                /* AX AY AZ GX GY GZ (bỏ nhiệt độ); 1 = đạt */
/* thi hành hành động; trả về −1 nếu không cần imu_guard_done(), ngược lại 0/1 = kết quả */
int  imu_do(ig_act_t a);

#endif
