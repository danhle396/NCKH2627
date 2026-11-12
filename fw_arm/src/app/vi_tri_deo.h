/*
 * vi_tri_deo.h — mọi thứ phụ thuộc vị trí đeo (kế hoạch O9). Vị trí duy nhất: BẮP TAY (chốt 11/11/2026).
 * Muốn thêm vị trí khác sau này: thêm một khối #elif và một mô hình huấn luyện bằng dữ liệu của chính vị trí đó.
 *
 *   TRUC_X/Y/Z   trục người (X trước, Y trái, Z lên) lấy từ trục nào của MPU6050: ±1/±2/±3 = ± trục X/Y/Z
 *   ACC_OFF_*    offset gia tốc theo trục người (LSB), trừ sau khi đổi trục
 *   TILT_EN, T_TILT   điều kiện nghiêng ở bước bất động của Safety FSM (A.8)
 *   NGA_BAT      bật tầng 2 (4 dấu hiệu, viết mới cho bắp tay ở bước 7)
 *
 * MỌI SỐ CỦA BẮP TAY CHƯA CHỌN: chọn trên dữ liệu bắp tay nhóm tự thu (kế hoạch mục 10 bước 7). Không có số nào lấy từ
 * dữ liệu vị trí khác (11/11/2026 đã xóa cấu hình ngực và mọi ngưỡng đo trên ngực/đầu). Trong lúc chờ:
 *   - đổi trục để nguyên trục cảm biến, offset 0 (đo khi lắp GY-521 lên băng bắp tay, bước 7);
 *   - tắt điều kiện nghiêng và tắt tầng 2: hệ thống chạy đúng A.8 (CNN 2/3 phiếu FALL → SUSPECT → bất động → PRE_ALARM).
 *     Tầng 2 và nhận dạng tư thế của ngực đã xóa (11/11/2026, thiết kế và ngưỡng theo dữ liệu ngực/đầu); bước 7
 *     viết mới cho bắp tay (kế hoạch 7.2) rồi định nghĩa ngưỡng ở đây.
 */
#ifndef VI_TRI_DEO_H
#define VI_TRI_DEO_H

#define VT_BAP_TAY      1

#ifndef FW_VI_TRI_DEO
#define FW_VI_TRI_DEO   VT_BAP_TAY
#endif

#if FW_VI_TRI_DEO == VT_BAP_TAY
#define VT_TEN          "bap_tay"
#define TRUC_X          1               /* [CHƯA CHỌN] */
#define TRUC_Y          2
#define TRUC_Z          3
#define ACC_OFF_X       0               /* [CHƯA CHỌN] */
#define ACC_OFF_Y       0
#define ACC_OFF_Z       0
#define TILT_EN         0               /* [CHƯA CHỌN] */
#define T_TILT          0
#ifndef NGA_BAT
#define NGA_BAT         0               /* bật ở bước 7, sau khi có đủ NGA_* dưới đây */
#endif
/* Khi NGA_BAT = 1 phải định nghĩa (chọn bằng tools/chon_nguong_tang2.py trên dữ liệu bắp tay, core/nga_bt.h):
 *   NGA_ROI_LSB NGA_VA_LSB NGA_QUAY_LSB NGA_YEN_A NGA_YEN_G NGA_TIM_KHOI NGA_ROI_KHOI NGA_YEN_KHOI NGA_CHO_MS */
#else
#error "FW_VI_TRI_DEO không hợp lệ"
#endif

#endif
