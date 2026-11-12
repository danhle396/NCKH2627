# ml/src/nga_bt.py — bản Python khớp từng bit của fw_arm/src/core/nga_bt.c (tầng 2 bắp tay, kế hoạch 7.2).
# Dùng để chọn ngưỡng trên dữ liệu bắp tay (tools/chon_nguong_tang2.py); kiểm khớp với C ở fw_arm/test/m3_test.py.
KHOI_MAU, VONG = 32, 36
CHO, XAC_NHAN, BO = 0, 1, 2


def sq3(v):
    return (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) & 0xFFFFFFFF


class NgaBT:
    def __init__(self, cfg):
        """cfg: dict roi_lsb, va_lsb, quay_lsb, yen_a, yen_g, tim_khoi, roi_khoi, yen_khoi, cho_ms."""
        self.c = dict(cfg)
        self.k = [(0, 0, 0, 0)] * VONG           # (a2_min, a2_max, w2_max, yên)
        self.i = self.n = self.m = 0
        self.dang_xet = 0
        self.d_moi = self.dau_hieu = self.t0 = 0
        self.n_xet = self.n_xac_nhan = self.n_bo = 0

    def _lui(self, d):
        return (self.i + VONG - 1 - d) % VONG

    def _quanh(self, d):
        return self.k[self._lui(self.d_moi - d)]

    def _danh_gia(self):
        c = self.c
        for d in range(-c["roi_khoi"], 1):
            if self.d_moi - d < self.n and self._quanh(d)[0] <= c["roi_lsb"] ** 2:
                self.dau_hieu |= 1
        for d in (-1, 0, 1):
            if 0 <= self.d_moi - d < self.n and self._quanh(d)[2] >= c["quay_lsb"] ** 2:
                self.dau_hieu |= 4
        for d in range(1, min(self.d_moi, c["yen_khoi"]) + 1):
            if self._quanh(d)[3]:
                self.dau_hieu |= 8

    def mau(self, b):
        a2, w2 = sq3(b[:3]), sq3(b[3:])
        if self.m == 0:
            self.a2_min = self.a2_max = a2
            self.w2_max = w2
            self.mn, self.mx = list(b), list(b)
        else:
            self.a2_min, self.a2_max, self.w2_max = min(self.a2_min, a2), max(self.a2_max, a2), max(self.w2_max, w2)
            self.mn = [min(x, y) for x, y in zip(self.mn, b)]
            self.mx = [max(x, y) for x, y in zip(self.mx, b)]
        self.m += 1
        if self.m < KHOI_MAU:
            return
        self.m = 0
        yen = int(all(self.mx[c] - self.mn[c] < (self.c["yen_a"] if c < 3 else self.c["yen_g"]) for c in range(6)))
        self.k[self.i] = (self.a2_min, self.a2_max, self.w2_max, yen)
        self.i = (self.i + 1) % VONG
        self.n = min(self.n + 1, VONG)
        if self.dang_xet:
            self.d_moi = min(self.d_moi + 1, VONG - 1)
            self._danh_gia()

    def bat_dau(self, now):
        best, a2 = 0, 0
        for d in range(min(self.c["tim_khoi"], self.n)):
            if self.k[self._lui(d)][1] > a2:
                a2, best = self.k[self._lui(d)][1], d
        self.dang_xet, self.d_moi, self.t0 = 1, best, now
        self.dau_hieu = 2 if (self.n and a2 >= self.c["va_lsb"] ** 2) else 0
        self.n_xet += 1
        self._danh_gia()

    def buoc(self, now):
        if not self.dang_xet:
            return CHO
        if self.dau_hieu == 0x0F:
            self.dang_xet = 0
            self.n_xac_nhan += 1
            return XAC_NHAN
        if (self.dau_hieu & 3) != 3 or self.d_moi > self.c["yen_khoi"] or (now - self.t0) & 0xFFFFFFFF >= self.c["cho_ms"]:
            self.dang_xet = 0
            self.n_bo += 1
            return BO
        return CHO
