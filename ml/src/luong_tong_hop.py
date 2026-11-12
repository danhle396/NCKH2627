# ml/src/luong_tong_hop.py — luồng cảm biến TỔNG HỢP (INT8 50 Hz, 6 kênh, đúng định dạng A.2/A.3) cho các kiểm khớp bit.
#
# Từ 11/11/2026 repo không giữ dữ liệu công khai (D1 đã xóa để không lẫn kênh đầu/ngực với dữ liệu bắp tay; dữ liệu bắp tay
# nhóm tự thu, đặt ở data/). Kiểm khớp bit (golden tăng dần, RTL, netlist, firmware) không cần dữ liệu thật, chỉ cần phủ đủ
# dạng tín hiệu: trọng lực + trôi chậm + nhiễu, đổi tư thế, va đập (bão hòa ±127/−128), quay mạnh, đứng yên tuyệt đối,
# đoạn 0, xen kẽ cực trị, chỗ nhảy dữ liệu. KHÔNG dùng để huấn luyện hay đo độ chính xác.
import numpy as np

WIN, HOP = 128, 16


def _tron(x, k):
    """Lọc trung bình trượt k mẫu theo trục 0 (làm tín hiệu chậm)."""
    c = np.cumsum(np.vstack([np.zeros((1, x.shape[1])), x]), axis=0)
    y = (c[k:] - c[:-k]) / k
    return np.vstack([np.repeat(y[:1], k - 1, axis=0), y])


def luong(n_buoc, seed, su_kien=True):
    """Luồng đủ cho n_buoc bước trượt: (WIN + (n_buoc − 1)·HOP, 6) int8."""
    rng = np.random.default_rng(seed)
    n = WIN + (n_buoc - 1) * HOP
    x = np.zeros((n, 6))
    g = np.array([0.0, 0.0, 16.0])                      # 1 g = 16 mức INT8 (A.2)
    huong = np.repeat(g[None], n, axis=0)
    for _ in range(rng.integers(1, 4)):                 # đổi tư thế: trọng lực xoay sang trục khác
        t0 = int(rng.integers(0, n))
        v = rng.normal(size=3)
        huong[t0:] = 16.0 * v / np.linalg.norm(v)
    x[:, :3] = _tron(huong, 15) + 6 * _tron(rng.normal(size=(n, 3)), 9) + rng.normal(0, 1.0, (n, 3))
    x[:, 3:] = 20 * _tron(rng.normal(size=(n, 3)), 7) + rng.normal(0, 1.5, (n, 3))
    if su_kien:
        for _ in range(rng.integers(1, 4)):             # va đập: vài mẫu bão hòa + quay mạnh quanh đó
            t = int(rng.integers(20, n - 30))
            x[t:t + 3, :3] = rng.choice([127, -128], size=(3, 3))
            x[t - 10:t + 15, 3:] += rng.normal(0, 60, (25, 3))
        t = int(rng.integers(0, n - 60))                # đứng yên tuyệt đối
        x[t:t + 50] = np.round(x[t])
        if rng.random() < 0.5:                          # chỗ nhảy dữ liệu (như mất mẫu rồi nối lại)
            t = int(rng.integers(WIN, n))
            x[t:] += rng.normal(0, 30, 6)
    return np.clip(np.round(x), -128, 127).astype(np.int8)


def luong_bien(n_buoc, seed):
    """Luồng ngẫu nhiên đủ dải, có đoạn bão hòa +127, −128, đoạn 0 và xen kẽ cực trị."""
    rng = np.random.default_rng(seed)
    n = WIN + (n_buoc - 1) * HOP
    x = rng.integers(-128, 128, (n, 6)).astype(np.int8)
    a = n // 6
    x[a:a + 130] = 127
    x[2 * a:2 * a + 120] = -128
    x[3 * a:3 * a + 100] = 0
    x[4 * a:4 * a + 50:2] = 127
    x[4 * a + 1:4 * a + 50:2] = -128
    return x


def cua_so(so, seed):
    """`so` cửa sổ 128 × 6 int8 lấy từ các luồng tổng hợp khác nhau."""
    rng = np.random.default_rng(seed)
    out = []
    while len(out) < so:
        s = luong(8, int(rng.integers(1 << 30)))
        out += [s[i * HOP:i * HOP + WIN] for i in range(8)]
    return np.stack(out[:so])
