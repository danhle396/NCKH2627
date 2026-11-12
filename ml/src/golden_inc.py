# ml/src/golden_inc.py — golden của bộ tăng tốc tính tăng dần (kế hoạch O1, O2, O3). Chỉ số nguyên.
#
# Mô hình địa chỉ cho rtl/src/cnn_inc.v: mọi bộ nhớ là mảng byte 1 chiều, đúng như RTL sẽ đọc ghi.
#   ROM (O2): trọng số 1 làn như E2 (Conv: BASE + (g·5 + k)·IC + ic; Dense: DB + g·F + i), rồi bias INT32 4 byte
#             little-endian ở BIAS_BASE + 4·b (thứ tự lớp 1, 2, 3, Dense).
#   Đệm đặc trưng (1 SDPB 2.048 × 8), vòng theo bộ đếm bước s (0…7):
#       L1: 64 vị trí × 16 kênh ở 0    + ((j + 8s) mod 64)·16 + c
#       L2: 32 vị trí × 16 kênh ở 1024 + ((m + 4s) mod 32)·16 + c
#       L3: 16 vị trí × 32 kênh ở 1536 + ((r + 2s) mod 16)·32 + c
#   Mẫu vào mỗi bước (STAGE, 144 byte = 24 mẫu × 6 kênh, i = mẫu·6 + kênh): x[0..3] rồi x[108..127] của cửa sổ.
#       Bốn mẫu đầu cửa sổ cần để tính lại L1[0] (sát mép trái, chịu đệm 0); M3 giữ cả cửa sổ nên gửi lại được.
#       STAGE nằm ngay trong đệm, ở 6 ô L3 sẽ tính lại trong bước này (r = 12…15, 0, 1; 192 B):
#           1536 + ((12 + 2s + i/32) mod 16)·32 + (i mod 32)
#       Các ô này không còn ai đọc sau Dense của bước trước, và chỉ bị ghi đè khi tính lớp 3, sau khi lớp 1 đọc xong STAGE.
#       (GW1NS-4 không có RAM phân tán; để STAGE riêng thì tốn 1.152 thanh ghi hoặc thêm 1 khối BSRAM.)
#
# Mỗi bước (cửa sổ trượt 16 mẫu = 8 vị trí L1 = 4 vị trí L2 = 2 vị trí L3) chỉ tính:
#   L1 j ∈ {0, 55…63}   (0, 63: sát mép; 55…62: vị trí mới)
#   L2 m ∈ {0, 1, 26…31}
#   L3 r ∈ {0, 1, 12…15}
#   Dense đủ 512 đặc trưng.
# Vị trí "trong" (không chạm đệm 0) chỉ phụ thuộc dữ liệu luồng, nên dời 1 bước vẫn đúng: L1 1…62, L2 2…29, L3 2…13.
# Kết quả hợp lệ khi đã có 8 bước kể từ SOFT_RST (WARM = 8). Chứng minh bằng chạy thử: golden_inc_check().
import numpy as np

K, PAD, AMAX = 5, 2, 127
WIN, HOP = 128, 16
L1_SET = [0] + list(range(55, 64))
L2_SET = [0, 1] + list(range(26, 32))
L3_SET = [0, 1] + list(range(12, 16))
STAGE_POS = list(range(0, 4)) + list(range(108, 128))         # vị trí trong cửa sổ của 24 mẫu STAGE
OFF1, OFF2, OFF3, BUF_BYTES = 0, 1024, 1536, 2048
WARM_STEPS = 8


def pack_rom_inc(P):
    """ROM 1 làn + bias trong ROM (O2). Trả về (list int8 có dấu, bases [L1, L2, L3, Dense], BIAS_BASE)."""
    rom, bases = [], []
    for i in (1, 2, 3):
        w = np.asarray(P[f"w{i}"])
        OC, IC, _ = w.shape
        bases.append(len(rom))
        for g in range(OC):
            for k in range(K):
                for ic in range(IC):
                    rom.append(int(w[g, ic, k]))
    bases.append(len(rom))
    for r in np.asarray(P["wd"]):
        rom += [int(v) for v in r]
    bias_base = len(rom)
    for v in np.concatenate([np.asarray(P[f"b{i}"]) for i in (1, 2, 3)] + [np.asarray(P["bd"])]):
        u = int(v) & 0xFFFFFFFF
        rom += [((u >> (8 * b)) & 0xFF) - (256 if (u >> (8 * b)) & 0x80 else 0) for b in range(4)]
    return rom, bases, bias_base


def stage_of(win):
    """Cửa sổ (128, 6) int8 → 144 byte STAGE mà M3 ghi vào NEW_DATA."""
    return np.asarray(win, np.int8)[STAGE_POS].reshape(-1)


def check_shape(P):
    sh = [tuple(np.asarray(P[f"w{i}"]).shape) for i in (1, 2, 3)]
    assert sh == [(16, 6, 5), (16, 16, 5), (32, 16, 5)], f"bộ tăng tốc tăng dần viết cho N2 16-16-32, gặp {sh}"
    assert P["head"] == "flatten" and np.asarray(P["wd"]).shape[1] == 512


class IncEngine:
    """Một bộ tăng tốc. buf_init: nội dung đệm lúc bật nguồn (mặc định 0; thử với ngẫu nhiên để chứng minh WARM)."""

    def __init__(self, P, buf_init=None):
        check_shape(P)
        self.P = P
        self.rom, self.bases, self.bias_base = pack_rom_inc(P)
        self.nout = int(np.asarray(P["wd"]).shape[0])
        self.sh = list(P["n"])
        self.buf = [0] * BUF_BYTES if buf_init is None else [int(v) for v in buf_init]
        self.s = 0
        self.warm = 0
        self.macs = 0
        self.outputs = 0

    def soft_rst(self):
        self.s, self.warm = 0, 0           # đệm giữ nguyên nội dung cũ (RTL không xóa BSRAM)

    # ---- địa chỉ ----
    def a1(self, j, c):
        return OFF1 + ((j + 8 * self.s) & 63) * 16 + c

    def a2(self, m, c):
        return OFF2 + ((m + 4 * self.s) & 31) * 16 + c

    def a3(self, r, c):
        return OFF3 + ((r + 2 * self.s) & 15) * 32 + c

    def a_stage(self, i):
        return OFF3 + ((12 + 2 * self.s + (i >> 5)) & 15) * 32 + (i & 31)

    def write_stage(self, stage):
        """M3 ghi 144 byte NEW_DATA khi bộ tăng tốc rảnh: vào đệm ở các ô L3 sắp tính lại."""
        st = [int(v) & 0xFF for v in np.asarray(stage, np.int8).reshape(-1)]
        assert len(st) == 144
        for i, v in enumerate(st):
            self.buf[self.a_stage(i)] = v

    def bias(self, b):
        v = 0
        for i in range(4):
            v |= (self.rom[self.bias_base + 4 * b + i] & 0xFF) << (8 * i)
        return v - (1 << 32) if v & 0x80000000 else v

    @staticmethod
    def requant(v, n):
        return 0 if v <= 0 else min(v >> n, AMAX)

    def _conv(self, layer, positions, L, IC, OC, src, dst, bb, n):
        """Một lớp Conv + ReLU + MaxPool chỉ ở các vị trí sau gộp `positions`. src(pos, ic) → byte; dst(j, g) → địa chỉ."""
        base = self.bases[layer]
        for j in positions:
            for g in range(OC):
                best = None
                for q in range(2):
                    t = 2 * j + q
                    acc = self.bias(bb + g)
                    wa = base + g * K * IC
                    for k in range(K):
                        pos = t - PAD + k
                        for ic in range(IC):
                            xv = src(pos, ic) if 0 <= pos < L else 0
                            acc += self.rom[wa] * xv
                            wa += 1
                            self.macs += 1
                    best = acc if best is None else max(best, acc)
                self.buf[dst(j, g)] = self.requant(best, n)
                self.outputs += 1

    def step(self, stage):
        """Một bước trượt: stage = 144 byte (stage_of), ghi vào đệm rồi START. Trả về (lớp, SCORE, logits, hợp lệ)."""
        self.write_stage(stage)
        idx = {p: i for i, p in enumerate(STAGE_POS)}

        def src_in(pos, ic):                                # chỉ đọc vị trí 0…3, 108…127 (assert qua KeyError)
            v = self.buf[self.a_stage(idx[pos] * 6 + ic)]
            return v - 256 if v > 127 else v

        self._conv(0, L1_SET, 128, 6, 16, src_in, self.a1, 0, self.sh[0])
        self._conv(1, L2_SET, 64, 16, 16, lambda p, c: self.buf[self.a1(p, c)], self.a2, 16, self.sh[1])
        self._conv(2, L3_SET, 32, 16, 32, lambda p, c: self.buf[self.a2(p, c)], self.a3, 32, self.sh[2])
        logits = []
        for g in range(self.nout):
            acc = self.bias(64 + g)
            wa = self.bases[3] + g * 512
            for t in range(16):
                for c in range(32):
                    acc += self.rom[wa] * self.buf[self.a3(t, c)]
                    wa += 1
                    self.macs += 1
            logits.append(acc)
        cls = int(np.argmax(logits))                        # bằng nhau → chỉ số nhỏ
        top2 = max(v for i, v in enumerate(logits) if i != cls)
        score = int(np.clip((logits[cls] - top2) >> self.P["score_shift"], 0, 127))
        self.warm = min(self.warm + 1, WARM_STEPS)
        valid = self.warm == WARM_STEPS
        self.s = (self.s + 1) & 7
        return cls, score, np.array(logits, np.int64), valid

    def layers(self):
        """Đặc trưng logic của cửa sổ vừa tính (sau step): l1 (64, 16), l2 (32, 16), l3 (16, 32), như golden_model."""
        s = (self.s - 1) & 7
        l1 = np.array([[self.buf[OFF1 + ((j + 8 * s) & 63) * 16 + c] for c in range(16)] for j in range(64)])
        l2 = np.array([[self.buf[OFF2 + ((m + 4 * s) & 31) * 16 + c] for c in range(16)] for m in range(32)])
        l3 = np.array([[self.buf[OFF3 + ((r + 2 * s) & 15) * 32 + c] for c in range(32)] for r in range(16)])
        return l1, l2, l3


def windows_of(stream):
    """Luồng (N, 6) int8 → các cửa sổ 128 mẫu trượt 16."""
    n = (len(stream) - WIN) // HOP + 1
    return [stream[i * HOP:i * HOP + WIN] for i in range(n)]
