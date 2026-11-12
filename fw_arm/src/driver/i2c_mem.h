/* i2c_mem.h — I²C master bằng phần mềm trên GPIO[0] (SCL), GPIO[1] (SDA) của M3 (kế hoạch O6, 0 LUT).
 * Cực máng hở: DATAOUT của hai bit luôn = 0; kéo thấp = bật cho phép xuất, thả = tắt (kéo lên có sẵn trên board).
 * Tốc độ đặt bằng I2C_HALF_US (mặc định 2 µs ≈ 250 kHz; 400 kHz chỉnh sau khi đo trên board). */
#ifndef I2C_MEM_H
#define I2C_MEM_H
#include <stdint.h>

#define I2C_OK      0
#define I2C_NACK   (-1)
#define I2C_BUSY   (-2)            /* SDA bị giữ thấp, gỡ bus không được */

void i2c_init(void);
int  i2c_bus_clear(void);          /* 9 xung SCL + STOP (bậc 1 của kế hoạch 7.5.4) */
int  i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
int  i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint32_t n);
void i2c_hold_low(void);           /* đưa SCL, SDA ra mức 0 trước khi cắt nguồn cảm biến (7.5.4 bậc 3) */
void i2c_release(void);

#endif
