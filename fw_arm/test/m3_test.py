#!/usr/bin/env python
# fw_arm/test/m3_test.py — chạy mã máy Cortex-M3 của firmware (fw_arm/build/test.axf, `bash fw_arm/build.sh test`) trong
# Unicorn và so với golden Python. Cần venv có unicorn + numpy (không cài vào Python của máy).
#
#   python fw_arm/test/m3_test.py [--n 200] [--report docs/bao_cao/<thư mục>]
#
# Kiểm:
#   1. CNN phần mềm (core/cnn_core.c, mục 10 bước 8) == golden E2 hai lớp (ml/src/golden_model.py): lớp, SCORE, 2 logit,
#      trên 50 vector của export_mem.py + N cửa sổ tổng hợp (ml/src/luong_tong_hop.py), ring cắt ở vị trí ngẫu nhiên. Đếm lệnh mỗi lần chạy.
#   2. cnn_stage_from_window == golden_inc.stage_of (144 byte NEW_DATA)
#   3. cnn_selftest_stage(0..7) == 144 byte tự kiểm của tools/export_inc.py (luồng LFSR)
#   4. đổi mã 1 → 3 (O3): chuỗi lớp [1, 1] và [1, 0, 1] vào SUSPECT; không đổi mã thì không bao giờ vào SUSPECT
import argparse
import pathlib
import struct
import sys

import numpy as np
from unicorn import UC_ARCH_ARM, UC_MODE_MCLASS, UC_MODE_THUMB, UC_HOOK_CODE, Uc
from unicorn.arm_const import UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_SP

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "ml" / "src"))
sys.path.insert(0, str(ROOT / "tools"))
import golden_inc as gi  # noqa: E402
from golden_model import forward  # noqa: E402

FLASH, RAM, RAM_SIZE = 0x00000000, 0x20000000, 0x00200000
IO = 0x20100000                     # vùng dữ liệu test (ngoài 8 KB SRAM thật, chỉ có trong giả lập)
STOP = 0x0000FFF0                   # LR giả: chạy tới đây là hàm đã trả về


def load_elf(path):
    b = path.read_bytes()
    assert b[:4] == b"\x7fELF" and b[4] == 1 and b[5] == 1
    e_phoff, e_shoff = struct.unpack_from("<II", b, 28)
    e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHHHH", b, 42)
    segs = []
    for i in range(e_phnum):
        p_type, p_off, p_vaddr, _, p_filesz, p_memsz, _, _ = struct.unpack_from("<8I", b, e_phoff + i * e_phentsize)
        if p_type == 1:
            segs.append((p_vaddr, b[p_off:p_off + p_filesz] + bytes(p_memsz - p_filesz)))
    secs = [struct.unpack_from("<10I", b, e_shoff + i * e_shentsize) for i in range(e_shnum)]
    syms = {}
    for s in secs:
        if s[1] == 2:                                   # SHT_SYMTAB
            strtab = secs[s[6]]
            for k in range(s[5] // 16):
                st_name, st_value, _, _, _, _ = struct.unpack_from("<IIIBBH", b, s[4] + k * 16)
                n0 = strtab[4] + st_name
                name = b[n0:b.index(b"\0", n0)].decode()
                if name:
                    syms[name] = st_value
    return segs, syms


class M3:
    def __init__(self, axf):
        segs, self.sym = load_elf(axf)
        self.uc = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        self.uc.mem_map(FLASH, 0x00100000)
        self.uc.mem_map(RAM, RAM_SIZE)
        for a, d in segs:
            self.uc.mem_write(a, d)
        self.count = 0
        self.uc.hook_add(UC_HOOK_CODE, self._cnt)

    def _cnt(self, uc, addr, size, ud):
        self.count += 1

    def call(self, name, *args):
        regs = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2]
        for r, v in zip(regs, args):
            self.uc.reg_write(r, v & 0xFFFFFFFF)
        self.uc.reg_write(UC_ARM_REG_SP, RAM + 0x00080000)
        self.uc.reg_write(UC_ARM_REG_LR, STOP | 1)
        self.count = 0
        self.uc.emu_start(self.sym[name] | 1, STOP, count=200_000_000)
        assert self.uc.reg_read(UC_ARM_REG_PC) == STOP, f"{name} không trả về"
        return self.uc.reg_read(UC_ARM_REG_R0)

    def wr(self, a, data):
        self.uc.mem_write(a, bytes(data))

    def rd(self, a, n):
        return bytes(self.uc.mem_read(a, n))


def load_params(p):
    z = np.load(p)
    P = {k: z[k] for k in z.files}
    P["n"] = [int(v) for v in P["n"]]
    P["head"] = str(P["head"])
    P["score_shift"] = int(P["score_shift"])
    return P


def ring_of(win, off):
    w = np.asarray(win, np.int8).reshape(-1)
    r = np.zeros(768, np.int8)
    for i in range(768):
        r[(off + i) % 768] = w[i]
    return r.tobytes()


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--axf", type=pathlib.Path, default=ROOT / "fw_arm/build/test.axf")
    ap.add_argument("--params", type=pathlib.Path, default=ROOT / "ml/out/mo_hinh_thu/params_int8.npz")
    ap.add_argument("--n", type=int, default=200)
    ap.add_argument("--report", type=pathlib.Path)
    a = ap.parse_args()
    P = load_params(a.params)
    m = M3(a.axf)
    rng = np.random.default_rng(11)
    rep = []

    def log(s):
        print(s)
        rep.append(s)

    # 1. CNN phần mềm
    import export_mem as em
    X, _ = em.vectors(argparse.Namespace(data="", nvec=50), np.random.default_rng(4))
    from luong_tong_hop import cua_so
    X += list(cua_so(a.n, 12))
    bad, cnts, ncls = 0, [], [0, 0]
    for k, x in enumerate(X):
        off = int(rng.integers(0, 768))
        m.wr(IO, ring_of(x, off))
        m.call("t_sw", IO, off, IO + 0x1000)
        c, s, l0, l1 = struct.unpack("<4i", m.rd(IO + 0x1000, 16))
        cnts.append(m.count)
        gc, gs, gl = forward(x, P)
        ok = (c, s, l0, l1) == (gc, gs, int(gl[0]), int(gl[1]))
        bad += not ok
        ncls[gc] += 1
        if not ok and bad <= 3:
            log(f"  sai vector {k}: M3 {(c, s, l0, l1)}, golden {(gc, gs, gl.tolist())}")
    n = len(X)
    log(f"{'PASS' if bad == 0 else 'FAIL'}  CNN phần mềm trên M3 == golden E2 hai lớp: {n - bad}/{n} cửa sổ "
        f"(50 vector export_mem + {a.n} cửa sổ tổng hợp; lớp golden NORMAL {ncls[0]}, FALL {ncls[1]})")
    log(f"      số lệnh Cortex-M3 mỗi lần suy luận: {min(cnts)}…{max(cnts)} (trung vị {int(np.median(cnts))}) "
        f"[giả lập Unicorn, đếm lệnh, chưa phải chu kỳ]")
    ok_all = bad == 0

    # 2. STAGE từ cửa sổ
    bad = 0
    for x in X[:60]:
        off = int(rng.integers(0, 768))
        m.wr(IO, ring_of(x, off))
        m.call("t_stage", IO, off, IO + 0x1000)
        bad += m.rd(IO + 0x1000, 144) != gi.stage_of(x).tobytes()
    log(f"{'PASS' if bad == 0 else 'FAIL'}  cnn_stage_from_window == golden_inc.stage_of: {60 - bad}/60")
    ok_all &= bad == 0

    # 3. STAGE tự kiểm
    from export_inc import lfsr_stream
    sw = gi.windows_of(lfsr_stream())
    bad = 0
    for st in range(8):
        m.call("t_selftest_stage", st, IO + 0x1000)
        bad += m.rd(IO + 0x1000, 144) != gi.stage_of(sw[st]).tobytes()
    log(f"{'PASS' if bad == 0 else 'FAIL'}  cnn_selftest_stage(0..7) == luồng LFSR của export_inc.py: {8 - bad}/8")
    ok_all &= bad == 0

    # 4. đổi mã 1 → 3 và bỏ phiếu 2/3
    SUSPECT = 2
    cases = [([1, 1], 1, True), ([1, 0, 1], 1, True), ([0, 1, 0, 0, 1, 0], 1, False), ([1, 1, 1, 1], 0, False)]
    bad = 0
    for seq, mp, want in cases:
        m.wr(IO, bytes(seq))
        stt = m.call("t_vote", IO, len(seq), mp)
        bad += (stt == SUSPECT) != want
        log(f"      chuỗi lớp {seq}, {'đổi mã 1→3' if mp else 'KHÔNG đổi mã'}: trạng thái {stt} "
            f"({'SUSPECT' if stt == SUSPECT else 'không SUSPECT'}), mong đợi {'SUSPECT' if want else 'không'}")
    log(f"{'PASS' if bad == 0 else 'FAIL'}  2/3 FALL vẫn vào SUSPECT sau khi đổi mã; không đổi mã thì không bao giờ có phiếu")
    ok_all &= bad == 0

    # 5. tự gỡ cảm biến (kế hoạch 7.5): hành động 0 NONE 1 BUS_CLEAR_CFG 2 SOFT_RESET 3 CFG 4 POWER_OFF 5 POWER_ON
    #    6 RELEASE_CFG; trạng thái 0 OK 1 L2_WAIT 2 L3_OFF 3 L3_ON_WAIT 4 LOST
    def ig():
        m.call("t_ig_get", IO + 0x3000)
        return struct.unpack("<8I", m.rd(IO + 0x3000, 32))
    steps, bad = [], 0

    def expect(now, want, done=None):
        nonlocal bad
        a_ = m.call("t_ig_poll", now)
        if done is not None:
            m.call("t_ig_done", now, done)
        steps.append(a_)
        if a_ != want:
            bad += 1
            log(f"      sai ở t = {now} ms: hành động {a_}, mong đợi {want}")
    m.call("t_ig_init", 0)
    expect(0, 4); expect(999, 0); expect(1000, 5); expect(1099, 0); expect(1100, 6, 1)        # khởi động: bậc 3
    s0 = ig()
    raw = IO + 0x3100
    for t in range(1110, 1500, 10):                                                           # chạy bình thường
        m.wr(raw, struct.pack("<6h", t, 1, 2, 3, 4, 5))
        m.call("t_ig_sample", t, 1, raw)
    expect(1500, 0)
    expect(1551, 1, 0)                                         # mất INT > 50 ms → bậc 1, lỗi
    expect(1552, 2)                                            # → bậc 2: DEVICE_RESET
    expect(1640, 0); expect(1652, 3, 0)                        # sau 100 ms cấu hình lại, lỗi → bậc 3
    expect(1653, 4); expect(2653, 5); expect(2753, 6, 0)       # cắt nguồn 1 s, cấp lại, lỗi → bậc 4
    s1 = ig()
    expect(12752, 0); expect(12753, 4); expect(13753, 5); expect(13853, 6, 1)    # 10 s sau thử lại bậc 3: hồi phục
    s2 = ig()
    for t in range(13860, 14900, 10):                                            # 6 trục đứng yên tuyệt đối 1 s
        m.wr(raw, struct.pack("<6h", 7, 7, 7, 7, 7, 7))
        m.call("t_ig_sample", t, 1, raw)
    expect(14900, 1, 1)                                        # → bậc 1, hồi phục ngay
    for t in (14910, 14920, 14930):                            # 3 lần đọc lỗi liên tiếp
        m.call("t_ig_sample", t, 0, raw)
    expect(14931, 1, 1)
    s3 = ig()
    ok5 = (bad == 0 and s0[0] == 0 and s0[2] == 1 and s1[0] == 4 and s1[1] == 1 and s1[6] == 1 and s2[0] == 0
           and s2[1] == 0 and s3[3] == 3 and s3[4] == 1 and s3[5] == 2 and s3[6] == 1)
    log(f"{'PASS' if ok5 else 'FAIL'}  tự gỡ cảm biến 4 bậc (imu_guard): khởi động cắt nguồn; mất INT → bậc 1 → 2 → 3 → 4 "
        f"(MẤT CẢM BIẾN), 10 s sau thử lại và hồi phục; 6 trục đứng yên 1 s và 3 lần đọc lỗi → bậc 1. "
        f"Đếm go_bus {s3[3]}, reset_mem {s3[4]}, cat_nguon {s3[5]}, mat_cb {s3[6]}")
    ok_all &= ok5

    # 6. WiFi: máy trạng thái AT + HTTP với module ESP-01S giả
    S = m.sym
    rx_n = 0

    def feed(b):
        nonlocal rx_n
        if struct.unpack("<I", m.rd(S["t_rx_i"], 4))[0] == rx_n:
            rx_n = 0
            m.wr(S["t_rx_i"], struct.pack("<I", 0))
        m.wr(S["t_rx"] + rx_n, b)
        rx_n += len(b)
        m.wr(S["t_rx_n"], struct.pack("<I", rx_n))

    def take():
        n = struct.unpack("<I", m.rd(S["t_tx_n"], 4))[0]
        b = m.rd(S["t_tx"], n)
        m.wr(S["t_tx_n"], struct.pack("<I", 0))
        return b

    def poll(now, k=1, room=100000):
        r = 0
        for _ in range(k):
            m.wr(S["t_tx_room"], struct.pack("<I", room))
            r = m.call("t_wifi_poll", now)
        return r
    fails = []
    m.call("t_wifi_init", 0)
    poll(0); rst1 = struct.unpack("<I", m.rd(S["t_rst"], 4))[0]
    poll(60); rst0 = struct.unpack("<I", m.rd(S["t_rst"], 4))[0]
    feed(b"\r\nready\r\n"); poll(70, 2)
    want = [b"ATE0\r\n", b"AT+CWMODE=2\r\n", b'AT+CWSAP="NCKH2627","nckh2627",5,3\r\n', b"AT+CIPMUX=1\r\n",
            b"AT+CIPSERVER=1,80\r\n"]
    for i, w_ in enumerate(want):
        got = take()
        if got != w_:
            fails.append(f"lệnh {i}: {got!r}")
        feed(b"\r\nOK\r\n"); poll(80 + i, 2)
    st = poll(100)
    if not (rst1 == 1 and rst0 == 0 and (st >> 8) == 1):
        fails.append(f"khởi động: rst {rst1}/{rst0}, ready {st >> 8}")

    def http(req, now, room=100000):
        feed(b"0,CONNECT\r\n\r\n+IPD,0," + str(len(req)).encode() + b":" + req)
        poll(now, 2)
        body = b""
        for _ in range(4):
            cmd = take()
            if not cmd.startswith(b"AT+CIPSEND=0,"):
                break
            n = int(cmd.split(b",")[1])
            feed(b">")
            got = b""
            for _ in range(200):
                poll(now, 1, room)
                got += take()
                if len(got) >= n:
                    break
            body += got
            feed(b"\r\nSEND OK\r\n"); poll(now, 2)
        close = cmd if not cmd.startswith(b"AT+CIPSEND") else take()
        feed(b"0,CLOSED\r\n\r\nOK\r\n"); poll(now, 2)
        return body, close
    r1, c1 = http(b"GET /data HTTP/1.1\r\nHost: 192.168.4.1\r\n\r\n", 200)
    if not (r1.startswith(b"HTTP/1.1 200 OK") and r1.endswith(b'{"t":1,"st":"MONITOR"}') and c1 == b"AT+CIPCLOSE=0\r\n"):
        fails.append(f"GET /data: {r1[:60]!r} … {c1!r}")
    r2, _ = http(b"GET / HTTP/1.1\r\n\r\n", 300, room=200)        # vòng TX giả chỉ nhận 200 byte mỗi lần poll
    h2 = r2.split(b"\r\n\r\n", 1)
    if not (b"Content-Encoding: gzip" in h2[0] and len(h2) == 2 and len(h2[1]) == 1500 and h2[1][:2] == b"\x1f\x8b"):
        fails.append(f"GET /: {len(r2)} byte")
    r3, _ = http(b"POST /huy HTTP/1.1\r\nContent-Length: 0\r\n\r\n", 400)
    huy = struct.unpack("<I", m.rd(S["t_req_huy"], 4))[0]
    if not (r3.endswith(b"ok") and huy == 1):
        fails.append(f"POST /huy: {r3[-20:]!r}, gọi hủy {huy} lần")
    feed(b"+IPD,0,5:GET /"); poll(500, 2); take()               # module không trả '>' → quá hạn 3 s → đóng kết nối
    poll(3600, 2); poll(3601); take(); feed(b"\r\nERROR\r\n"); poll(3700, 2)
    m.call("t_wifi_get", IO + 0x3000)
    nres, nreq, nerr = struct.unpack("<3I", m.rd(IO + 0x3000, 12))
    if not (nres == 0 and nreq == 4 and nerr == 1 and (poll(3800) & 0xFF) == 4):
        fails.append(f"quá hạn: reset {nres}, yêu cầu {nreq}, lỗi {nerr}")
    for f in fails:
        log("      " + f)
    log(f"{'PASS' if not fails else 'FAIL'}  WiFi AT + HTTP (module giả): khởi động 5 lệnh AT, GET /data (JSON), GET / "
        f"(gzip 1.500 B, 2 khúc CIPSEND, vòng TX đầy giữa chừng), POST /huy gọi hủy cảnh báo, module im lặng → quá hạn, "
        f"đóng kết nối, về chờ")
    ok_all &= not fails

    # 7. luồng thô GET /raw qua module giả: đầu HTTP giữ kết nối, khúc ≥ 512 B, khúc lẻ sau 200 ms, "0,CLOSED" → dừng
    fails = []
    feed(b"0,CONNECT\r\n\r\n+IPD,0,28:GET /raw HTTP/1.1\r\nHost: x\r\n\r\n")
    poll(4000, 2)
    c1 = take()
    feed(b">"); poll(4000, 2); h = take(); feed(b"\r\nSEND OK\r\n"); poll(4001, 2)
    on1 = struct.unpack("<I", m.rd(S["t_raw_on"], 4))[0]
    if not (c1.startswith(b"AT+CIPSEND=0,") and h.startswith(b"HTTP/1.1 200 OK") and b"text/csv" in h and on1 == 1
            and b"CIPCLOSE" not in take()):
        fails.append(f"bắt đầu /raw: {c1!r} {h[:40]!r} raw_on {on1}")
    src = bytes(range(32, 127)) * 8
    m.wr(S["t_rawsrc"], src[:700]); m.wr(S["t_rawsrc_n"], struct.pack("<I", 700))
    got = b""
    for t_ in (4010, 4011, 4012, 4300, 4301, 4302):
        poll(t_, 1)
        cmd = take()
        if cmd.startswith(b"AT+CIPSEND=0,"):
            feed(b">"); poll(t_, 2); got += take(); feed(b"\r\nSEND OK\r\n"); poll(t_, 2)
    if got != src[:700]:
        fails.append(f"dữ liệu thô: nhận {len(got)}/700 byte, khớp {got == src[:len(got)]}")
    feed(b"0,CLOSED\r\n"); poll(4400, 2)
    if struct.unpack("<I", m.rd(S["t_raw_on"], 4))[0] != 0:
        fails.append("kết nối đóng mà chưa dừng ghi thô")
    for f in fails:
        log("      " + f)
    log(f"{'PASS' if not fails else 'FAIL'}  luồng thô GET /raw (module giả): đầu HTTP giữ kết nối, 700 byte gửi đủ "
        f"(khúc 640 B + khúc lẻ sau 200 ms), không CIPCLOSE, kết nối đóng → dừng ghi")
    ok_all &= not fails

    # 8. raw_log: định dạng dòng CSV
    rr = np.random.default_rng(3)
    smp = np.column_stack([np.cumsum(rr.integers(9, 12, 40)), rr.integers(-32768, 32768, (40, 6)),
                           rr.integers(0, 2, 40)]).astype(np.int32)
    m.wr(IO, smp.tobytes())
    nb = m.call("t_raw_log", IO, len(smp), IO + 0x4000)
    want = "t,ax,ay,az,gx,gy,gz,btn\n" + "".join(",".join(str(int(v)) for v in r) + "\n" for r in smp)
    ok8 = m.rd(IO + 0x4000, nb).decode() == want
    log(f"{'PASS' if ok8 else 'FAIL'}  raw_log: 40 mẫu (cực trị INT16, nút) ra đúng từng ký tự dòng CSV của windowing.py")
    ok_all &= ok8

    # 9. đường ống thiết bị == windowing.py: log thô → app_on_raw (đổi trục, offset) → tiền xử lý → cửa sổ
    sys.path.insert(0, str(ROOT / "tools"))
    from windowing import tien_xu_ly
    from vi_tri import doc_cau_hinh
    cfg = doc_cau_hinh()
    bad = 0
    for k, n in enumerate((100, 256, 257, 700, 1000)):
        raw = np.clip(np.cumsum(rr.integers(-400, 401, (n, 6)), axis=0), -32768, 32767).astype(np.int16)
        raw[n // 3] = [32767, -32768, 32767, -32768, 32767, -32768]
        m.wr(IO, raw.tobytes())
        m.call("t_app_pre", IO, n, IO + 0x8000)
        c_win = np.frombuffer(m.rd(IO + 0x8000, 768), np.int8)
        q = tien_xu_ly(raw, cfg).reshape(-1)
        py_win = np.concatenate([np.zeros(max(0, 768 - len(q)), np.int8), q[-768:]])
        bad += not np.array_equal(c_win, py_win)
    log(f"{'PASS' if bad == 0 else 'FAIL'}  thiết bị (app_on_raw + preprocess.c) == windowing.py (vi_tri.py + golden): "
        f"{5 - bad}/5 luồng, cửa sổ trùng từng byte (cấu hình {cfg['ten']}, TRUC {cfg['truc']})")
    ok_all &= bad == 0

    # 10. tầng 2 bắp tay: C (core/nga_bt.c) == Python (ml/src/nga_bt.py) từng bước
    from nga_bt import NgaBT, XAC_NHAN, BO
    keys = ("roi_lsb", "va_lsb", "quay_lsb", "yen_a", "yen_g", "tim_khoi", "roi_khoi", "yen_khoi", "cho_ms")

    def kich_ban(kind, seed):
        r = np.random.default_rng(seed)
        b = np.zeros((32 * 40, 6), np.int64)
        b[:, 2] = 2048
        b[:, :] += r.integers(-300, 300, b.shape)                                 # cử động thường
        t = 32 * 20 + int(r.integers(0, 32))
        if kind in ("nga", "khong_yen"):
            b[t - 25:t, :3] = r.integers(-200, 200, (25, 3))                         # rơi: |a| nhỏ
        b[t:t + 3, :3] = r.choice([30000, -30000], (3, 3))                            # va chạm
        b[t - 20:t + 20, 3:] += r.integers(-8000, 8000, (40, 3))                      # quay
        if kind != "khong_yen":
            b[t + 32 * 2:, :] = b[t + 32 * 2]                                         # nằm yên
        return np.clip(b, -32768, 32767).astype(np.int16)
    tong, bad, kq = 0, 0, {}
    for k in range(40):
        kind = ("nga", "khong_roi", "khong_yen", "ngau_nhien")[k % 4]
        c = dict(roi_lsb=int(rr.integers(400, 1200)), va_lsb=int(rr.integers(4000, 9000)),
                 quay_lsb=int(rr.integers(2000, 6000)), yen_a=int(rr.integers(100, 400)), yen_g=int(rr.integers(100, 400)),
                 tim_khoi=int(rr.integers(4, 12)), roi_khoi=int(rr.integers(1, 4)), yen_khoi=int(rr.integers(4, 10)),
                 cho_ms=4000)
        b = kich_ban(kind, k) if kind != "ngau_nhien" else rr.integers(-9000, 9000, (32 * 40, 6)).astype(np.int16)
        m.wr(IO, struct.pack("<9I", *[c[x] for x in keys]))
        m.call("t_nga_init", IO)
        py = NgaBT(c)
        m.wr(IO + 0x100, b[:32 * 22].tobytes())
        m.call("t_nga_mau", IO + 0x100, 32 * 22)
        for s_ in b[:32 * 22]:
            py.mau([int(v) for v in s_])
        now = 22 * 320
        m.call("t_nga_bat_dau", now); py.bat_dau(now)
        for blk in range(22, 40):
            r_c = m.call("t_nga_buoc", now)
            r_p = py.buoc(now)
            tong += 1
            same = (r_c & 0xFF) == r_p and ((r_c >> 8) & 0xFF) == py.dau_hieu and ((r_c >> 16) & 0xFF) == py.d_moi
            bad += not same
            if r_p in (XAC_NHAN, BO):
                kq[kind] = kq.get(kind, []) + [r_p]
                break
            m.wr(IO + 0x100, b[32 * blk:32 * (blk + 1)].tobytes())
            m.call("t_nga_mau", IO + 0x100, 32)
            for s_ in b[32 * blk:32 * (blk + 1)]:
                py.mau([int(v) for v in s_])
            now += 320
    tom = {k: f"{v.count(XAC_NHAN)} xác nhận / {v.count(BO)} bỏ" for k, v in kq.items()}
    ok10 = bad == 0 and kq.get("nga", []).count(XAC_NHAN) == 10 and BO in kq.get("khong_roi", []) \
        and kq.get("khong_yen", []).count(BO) == 10
    log(f"{'PASS' if ok10 else 'FAIL'}  tầng 2 bắp tay C == Python: {tong - bad}/{tong} bước trùng (quyết định, dấu hiệu, "
        f"khối); kịch bản {tom} [ngưỡng thử ngẫu nhiên, KHÔNG phải ngưỡng thật]")
    ok_all &= ok10
    log("PASS" if ok_all else "FAIL")
    if a.report:
        a.report.mkdir(parents=True, exist_ok=True)
        (a.report / "m3_test.log").write_text("\n".join(rep) + "\n", encoding="utf8")
    sys.exit(0 if ok_all else 1)


if __name__ == "__main__":
    main()
