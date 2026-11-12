#!/usr/bin/env python
# tools/thu_bap_tay.py — thu log thô 100 Hz từ thiết bị đeo bắp tay (firmware M3, core/raw_log.h) thành file CSV cho
# tools/windowing.py. Quy trình, danh sách hoạt động, an toàn: docs/quy_trinh_thu_du_lieu_bap_tay.md.
#
#   python -I tools/thu_bap_tay.py --nguoi s01 --hd nga_truoc --lan 1 3          # WiFi: lần 1, 2, 3 (Enter để bắt đầu/dừng)
#   python -I tools/thu_bap_tay.py --nguoi s01 --hd di_bo --lan 1 --giay 120       # tự dừng sau 120 s
#   python -I tools/thu_bap_tay.py --nguon COM5 --nguoi s00 --hd di_bo --lan 1     # qua USB–UART (thử trên bàn)
#
# Nguồn: "wifi" (mặc định) = máy tính nối WiFi NCKH2627 của thiết bị, mở http://192.168.4.1/raw (người ngã không vướng dây,
# kế hoạch 7.4); hoặc tên cổng COM (USB–UART, lệnh R1/R0). Ra data/raw/<người>_<hoạt động>_<lần>.csv (data/raw không lên git).
# Mỗi file: kiểm định dạng từng dòng, đếm mẫu mất (khoảng trống t ≥ 15 ms), in tóm tắt.
import argparse
import pathlib
import re
import socket
import sys
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from windowing import ACTS  # noqa: E402

HEADER = "t,ax,ay,az,gx,gy,gz,btn"
DONG = re.compile(r"^\d+(,-?\d+){6},[01]$")


class NguonWifi:
    def __init__(self, host, port):
        self.s = socket.create_connection((host, port), timeout=5)
        self.s.sendall(b"GET /raw HTTP/1.1\r\nHost: " + host.encode() + b"\r\nConnection: keep-alive\r\n\r\n")
        self.buf, self.head = b"", False

    def lines(self):
        while True:
            try:
                d = self.s.recv(4096)
            except socket.timeout:
                yield None
                continue
            if not d:
                return
            self.buf += d
            if not self.head:
                if b"\r\n\r\n" not in self.buf:
                    continue
                self.buf = self.buf.split(b"\r\n\r\n", 1)[1]
                self.head = True
            *ls, self.buf = self.buf.split(b"\n")
            for l in ls:
                yield l.decode("ascii", "replace").strip()

    def close(self):
        self.s.close()


class NguonUart:
    def __init__(self, port):
        import serial
        self.p = serial.Serial(port, 115200, timeout=0.5)
        self.p.write(b"R1\n")

    def lines(self):
        while True:
            l = self.p.readline()
            yield l.decode("ascii", "replace").strip() if l else None

    def close(self):
        self.p.write(b"R0\n")
        self.p.close()


def thu(nguon_ten, out, giay):
    src = NguonWifi("192.168.4.1", 80) if nguon_ten == "wifi" else NguonUart(nguon_ten)
    dung = threading.Event()
    if not giay:
        threading.Thread(target=lambda: (input(), dung.set()), daemon=True).start()
    t0, n, hong, ts, co_head = time.time(), 0, 0, [], False
    with open(out, "w", newline="\n", encoding="ascii") as f:
        f.write(HEADER + "\n")
        for l in src.lines():
            if dung.is_set() or (giay and time.time() - t0 >= giay):
                break
            if l is None or not l:
                continue
            if l == HEADER:
                co_head = True
                continue
            if not DONG.match(l):
                hong += 1
                continue
            f.write(l + "\n")
            ts.append(int(l.split(",", 1)[0]))
            n += 1
            if n % 500 == 0:
                print(f"\r  {n / 100:.0f} s", end="", flush=True)
    src.close()
    mat = sum(round((b - a) / 10) - 1 for a, b in zip(ts, ts[1:]) if b - a >= 15)
    print(f"\r  {out.name}: {n} mẫu ({n / 100:.1f} s), mất {mat}, dòng hỏng {hong}"
          f"{'' if co_head else ' (không thấy dòng tiêu đề: kiểm firmware)'}")
    return mat == 0 and hong == 0


def main():
    sys.stdout.reconfigure(encoding="utf8")
    ap = argparse.ArgumentParser()
    ap.add_argument("--nguon", default="wifi", help="wifi (mặc định) hoặc cổng COM")
    ap.add_argument("--nguoi", required=True, help="s01, s02, … (s00 = thử trên bàn, không vào dữ liệu)")
    ap.add_argument("--hd", required=True, choices=sorted(ACTS), help="hoạt động")
    ap.add_argument("--lan", type=int, nargs="+", required=True, help="số lần, ví dụ 1 hoặc 1 3 (từ 1 tới 3)")
    ap.add_argument("--giay", type=float, default=0, help="tự dừng sau N giây (0 = dừng bằng Enter)")
    ap.add_argument("--out", type=pathlib.Path, default=ROOT / "data" / "raw")
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=True)
    lans = range(a.lan[0], (a.lan[1] if len(a.lan) > 1 else a.lan[0]) + 1)
    for k in lans:
        f = a.out / f"{a.nguoi}_{a.hd}_{k}.csv"
        if f.exists():
            print(f"{f.name} đã có: bỏ qua (xóa file nếu muốn thu lại)")
            continue
        input(f"[{a.hd} lần {k}] Enter để BẮT ĐẦU ghi{'' if a.giay else ', Enter lần nữa để DỪNG'} … ")
        ok = thu(a.nguon, f, a.giay)
        if not ok:
            print("  ⚠ có mẫu mất hoặc dòng hỏng: nên thu lại lần này")


if __name__ == "__main__":
    main()
