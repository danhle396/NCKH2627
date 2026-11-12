# Bằng chứng trích từ bản dự thi 2026

Các file gốc dưới đây đã bị xóa khỏi máy khi sắp xếp thư mục (kế hoạch mục 13). Chúng vẫn nằm trong lịch sử git của repo `danhle396/fpga2026gowin`, **commit `f26bea8`**. Muốn xem nguyên văn: `git show f26bea8:<đường dẫn>` trong một bản clone của repo cũ.

File này chỉ chép lại sự kiện và nhãn nguồn để kế hoạch (mục 7.5.1, 12.5) dẫn tới; không thêm kết luận mới.

## 1. MPU6050 treo, phải rút pin (nhật ký board)

Nguồn: `PROJECT_MASTER.md` (dòng 15 và nhật ký các dòng 1111–1132, commit `f26bea8`). Nhãn: **board**.

| Ngày | Sự kiện | Ghi chú trong nhật ký |
|---|---|---|
| 24/10 | Trên web, MPU6050 ngừng ra mẫu, `imu_init` = −1 (không trả lời WHO_AM_I). Khi chạy pin, MPU6050 không trả lời I²C sau khi chuyển nguồn, kể cả khi MCU reset | Thêm `i2c0_bus_recover()` (9 xung SCL + STOP) và `imu_lam_lai()`; in mức SDA/SCL để phân biệt bus treo với lỏng dây/mất nguồn |
| 28/10 | Hai lần mất MPU6050 sau khi nằm, lăn | Nghi tiếp xúc dây breadboard hoặc bus I²C treo |
| 28/10 | Lần thứ 3 trong ngày: mất sau 44.681 mẫu (≈ 447 s), sau đó mỗi giây `cau hinh lai, ket qua=-1, SDA=1 SCL=1` | "Không phải bus treo"; cả 3 lần **đều sau khi cử động** |
| 28/10 | Lần 4, **sau khi đã thay toàn bộ dây**: mất sau 51.183 mẫu (≈ 512 s), `ket qua=-1, SDA=1 SCL=1`, `i2c_loi=0`; MCU vẫn chạy | Nghi hàn header module, nguồn 3,3 V của module, hoặc chip treo cần cắt nguồn |
| 28/10 | Lần 5, **khi để yên** (≈ 19 phút sau khi bật): hơn 389 lần gỡ bus + cấu hình lại đều −1, SDA=1 SCL=1. Đèn module vẫn sáng; VCC của GY-521 nối 3,3 V của ESP32. **Rút pin 5 s rồi cắm lại → chạy lại** | Kết luận trong nhật ký: không phải dây lỏng do rung; MPU6050 treo bên trong, chỉ cắt nguồn mới hết. Nghi 3,3 V của ESP32 sụt/nhiễu khi phát WiFi **[CẦN XÁC MINH – chưa đo]** |
| 28/10 | Chuyển VCC + GND của module sang EVK: đèn sáng nhưng `WHO_AM_I=0x00` từ lúc khởi động; cắm lại về ESP32 thì chạy | Nghi chọn sai chân trên EVK (header EVK không có chân 3,3 V) |
| 28/10 | Nối VCC module vào GPIO25 của ESP32 (để tự cắt nguồn 300 ms): cảm biến không chạy; nối lại 3,3 V thì chạy → nhóm chốt giữ 3,3 V, `MPU_NGUON_GPIO 0` | Nguyên nhân GPIO25 không chạy chưa đo **[CẦN XÁC MINH – nguồn: VOM]** |

Lần ngày 3/11: `docs/bao_cao/2026-11-03_phat_lai_tren_board/README.md` (commit `f26bea8`) ghi trong `sau_khoi_phuc_serial.log` thiết bị báo 0 mẫu vì MPU6050 treo khoảng 16:06; "cần tắt nguồn khoảng 5 giây để cảm biến hoạt động lại". Nhãn: **board**, nguyên nhân chưa đo.

## 2. Thử cắt nguồn cảm biến bằng GPIO25 (ESP32)

Nguồn: `esp32/nguc_nga_hub/nguc_nga_hub.ino` dòng 48–57 (commit `f26bea8`). Nguyên văn phần chú thích và hằng số:

```c
// Nguồn MPU6050: VCC của module GY-521 nối chân 3,3 V của ESP32, GND ở ESP32. MPU6050 có lúc treo (đèn sáng, không
// trả lời I²C, chỉ cắt nguồn mới hết): MCU báo "MPU6050 ... ket qua=-1" và > MPU_TREO_MS không có mẫu.
// MPU_NGUON_GPIO 1 (thử 28/10): VCC nối GPIO25, ESP32 tắt chân MPU_CAT_MS rồi bật lại để gỡ treo. Nhóm nối VCC vào
// GPIO25 thì cảm biến không chạy, nối 3,3 V thì chạy → nhóm chốt 28/10 giữ 3,3 V: MPU_NGUON_GPIO 0, ESP32 không đụng
// GPIO25, MPU treo thì chỉ báo trong log, người dùng rút pin ≈ 5 s. Chỉ dựa vào dòng báo lỗi của MCU, không dựa vào
// "không có mẫu" đơn thuần: lúc khởi động MCU hiệu chuẩn gyro vài giây chưa gửi mẫu.
#define MPU_NGUON_GPIO  0
#define MPU_NGUON_PIN   25
#define MPU_TREO_MS     2000u
#define MPU_CAT_MS      300u
#define MPU_CAT_CACH_MS 10000u      // hai lần cắt cách nhau ít nhất 10 s
```

## 3. Gỡ bus I²C và cấu hình lại (MCU SN32F407)

Nguồn: `firmware/src/app_main.c` dòng 36–48 (commit `f26bea8`). Nguyên văn:

```c
/* Mất MPU6050 (chạy board 24/10: sau khi cắm/rút nguồn, MPU6050 có lúc ngủ lại, có lúc không trả lời I2C):
 * gỡ bus I2C (9 xung SCL + STOP), khởi động lại I2C0, ghi lại cấu hình A.1. In mức SDA/SCL đo trước khi gỡ:
 * SDA hoặc SCL = 0 → bus bị giữ thấp (đã gỡ); cả hai = 1 mà vẫn -1 → MPU6050 không có điện hoặc lỏng dây. */
static int imu_lam_lai(const char *luc, uint8_t *who)
{
    int lv = i2c0_bus_recover();
    i2c0_init(I2C_HZ);
    int bad = imu_init(who);
    uart_puts("# MPU6050 "); uart_puts(luc); put_kv(": cau hinh lai, ket qua=", bad);
    put_kv(", SDA=", lv & 1); put_kv(" SCL=", (lv >> 1) & 1);
    uart_puts(bad == 0 ? " OK\n" : (lv & 3) != 3 ? " (bus I2C bi giu thap, da go)\n"
                                                   : " (khong tra loi: kiem day/nguon MPU6050)\n");
    return bad;
}
```

Hàm `i2c0_bus_recover()` và `imu_init()` có trong `fw_arm/src/tham_khao/i2c0.c`, `imu.c` (chép từ `firmware/src/`).

## 4. Còi PZ1 và âm 2 kHz

Nguồn: `firmware/src/board.h` dòng 35–40 và `PROJECT_MASTER.md` nhật ký 28/10 (dòng 1124–1125), commit `f26bea8`. Nhãn: **board** (nhóm nghe thử).

- Còi PZ1 của EVK: dương nối thẳng P3.0, âm xuống GND; tài liệu EVK **không ghi còi chủ động hay thụ động**.
- 28/10: nhóm báo còi 500 Hz quá nhỏ. MCU phát thử 6 kiểu (mức cao liên tục cho còi chủ động; 500 / 2000 / 2700 / 3500 / 4000 Hz, ≥ 2 kHz bằng ngắt timer đảo chân). Nhóm nghe thử trên board và **chốt 2 kHz** ("nghe rất ok"), fix cứng `COI_KIEU_CHOT = COI_KIEU_2KHZ`.
- Nhật ký ghi: âm lượng còn lại bị giới hạn bởi mức 3,3 V của chân GPIO (còi nối thẳng chân, không qua transistor).

`board.h` dòng 35–40 nguyên văn:

```c
/* Còi PZ1 trên P3.0 (dương PZ1 nối thẳng BUZ_P30, âm xuống GND; tài liệu EVK không ghi còi chủ động hay thụ động).
 * Kiểu âm (28/10, nhóm: còi 500 Hz quá nhỏ): nhóm nghe thử 6 kiểu trên board và chốt 2 kHz (COI_KIEU_CHOT,
 * node_link.h). ESP32 nhắc lệnh NODE_CMD_COI_KIEU mỗi 1 s; MCU vẫn nhận các kiểu khác để thử lại nếu đổi còi. */
#define BUZZER_ACTIVE_HIGH  1
#ifndef COI_KIEU_MAC_DINH
#define COI_KIEU_MAC_DINH   COI_KIEU_CHOT       /* 2 kHz, cả khi chưa nhận lệnh kiểu từ ESP32 */
```

## 5. Số đo của mô hình và RTL bản dự thi

- Tài nguyên tổng hợp và đặt – nối dây trên Kiwi Primer 20K (GW2A): `fpga_project.rpt.txt`, `fpga_project_syn_resource.html` trong thư mục này. Nhãn: **tổng hợp**.
- `rtl/scripts/test.sh` chạy trên bản đã sắp xếp (11/11/2026): `tb_e2` P = 1 và P = 4, **10/10 vector khớp golden** (logit + lớp), 258.080 chu kỳ/lần suy luận với P = 1. Nhãn: **mô phỏng** (Icarus 0.9.7).
