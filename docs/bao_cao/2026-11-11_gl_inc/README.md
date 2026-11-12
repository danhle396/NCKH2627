# Netlist sau đặt – nối dây (kế hoạch mục 8, cấp 3) — 11/11/2026

Lệnh: `bash rtl/scripts/gl_inc.sh --steps 58`. Nhãn: [mô phỏng netlist], không SDF (chỉ kiểm chức năng).

- Netlist cả chip (có EMPU) và netlist sau tổng hợp đều bị Gowin bản Education mã hóa; netlist `.vo` sau đặt – nối dây thì không. `cnn_apb` có 78 cổng, nhiều hơn 39 chân, nên đặt – nối dây vỏ bọc `rtl/sim/gl_wrap.v` (35 chân; đọc 32 bit qua 4 lần chọn byte) cho đúng part GW1NSR-LV4CQN48PC6/I5.
- Netlist có **3 pROM, 1 SDPB, 2 MULT, 176 dòng INIT_RAM**: nội dung ROM trọng số + bias nằm sẵn trong netlist.
- Thư viện `simlib/gw1n/prim_sim.v`, bỏ các mô hình FLASH* (Icarus 0.9.7 không đọc mảng 2 chiều; netlist không dùng).

| Kiểm (`gl_58buoc.log`) | Kết quả |
|---|---|
| 66 bước (58 luồng ngã + 8 tự kiểm), cùng testbench `tb_inc.v` với `-DGL` | **52/52 bước hợp lệ khớp CLASS, SCORE, LOGIT**; VALID, WARM, phép âm, WROM đúng; **PASS** |
| CYCLES | 70.010 (bằng RTL) |
| Thời gian mô phỏng | 1.393 s |

Lần chạy này in "0 khop l1/l2/l3" vì netlist không còn mảng đệm để so; testbench đã sửa để in rõ "netlist: không so l1/l2/l3".
