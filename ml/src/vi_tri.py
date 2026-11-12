# ml/src/vi_tri.py — cấu hình vị trí đeo (O9) cho phía Python, đọc thẳng từ fw_arm/src/app/vi_tri_deo.h (một nguồn duy nhất).
# doi_truc() làm đúng bước đầu của app_on_raw() trong fw_arm/src/app/app.c: chọn trục người từ trục cảm biến (±1/±2/±3,
# đảo dấu −32768 → 32767), trừ offset gia tốc ACC_OFF_* theo trục người, bão hòa INT16. Con quay trong log thô đã trừ
# offset hiệu chỉnh (core/raw_log.h). Khớp từng giá trị với firmware: fw_arm/test/m3_test.py.
import pathlib
import re

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[2]
H = ROOT / "fw_arm" / "src" / "app" / "vi_tri_deo.h"


def doc_cau_hinh(path=H):
    s = pathlib.Path(path).read_text(encoding="utf8")

    def so(ten):
        m = re.search(rf"#define\s+{ten}\s+\(?\s*(-?\d+)\s*\)?", s)
        if not m:
            raise ValueError(f"không thấy {ten} trong {path}")
        return int(m.group(1))
    ten = re.search(r'#define\s+VT_TEN\s+"([^"]+)"', s).group(1)
    return {"ten": ten, "truc": [so("TRUC_X"), so("TRUC_Y"), so("TRUC_Z")],
            "acc_off": [so("ACC_OFF_X"), so("ACC_OFF_Y"), so("ACC_OFF_Z")]}


def _sat16(x):
    return np.clip(x, -32768, 32767)


def _pick(v, code):
    x = v[:, abs(code) - 1].astype(np.int64)
    return np.where(x == -32768, 32767, -x) if code < 0 else x


def doi_truc(raw, cfg=None):
    """raw (N, 6): AX AY AZ GX GY GZ theo trục cảm biến (con quay đã trừ offset) → (N, 6) int64 theo trục người."""
    cfg = cfg or doc_cau_hinh()
    raw = np.asarray(raw, np.int64)
    out = np.zeros_like(raw)
    for k, code in enumerate(cfg["truc"]):
        out[:, k] = _sat16(_pick(raw[:, :3], code) - cfg["acc_off"][k])
        out[:, 3 + k] = _pick(raw[:, 3:], code)
    return out
