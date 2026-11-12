#!/usr/bin/env python
# tools/gen_keil.py — sinh project Keil MDK fw_arm/keil/nckh_m3.uvprojx cho firmware M3 (Gowin_EMPU GW1NS-4C), để mở bằng
# Keil uVision và bấm Build (F7) xem "Program Size: Code / RO-data / RW-data / ZI-data". Cùng nguồn, cùng scatter
# (fw_arm/gw1ns4c_8k.sct) với fw_arm/build.sh.
#   python -I tools/gen_keil.py <project .uvprojx mẫu của Keil 5>
# Mẫu: một .uvprojx bất kỳ do Keil 5 + Arm Compiler 6 tạo (ví dụ project vòng 1 SN32F407); script chỉ lấy khung XML.
# Device: ARMCM3 (ARM.Cortex_DFP). Không dùng RTE: startup, vector, thanh ghi là của dự án (fw_arm/src/driver).
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "fw_arm" / "keil" / "nckh_m3.uvprojx"
NHOM = {
    "core": ["cnn_core.c", "safety.c", "preprocess.c", "blocks.c", "pwr_btn.c", "imu_guard.c",
             "wifi_at.c", "gyro_cal.c", "raw_log.c", "nga_bt.c"],
    "driver": ["startup.c", "board.c", "uart.c", "i2c_mem.c", "imu.c", "cnn_drv.c"],
    "app": ["main.c", "app.c", "web.c"],
    "gen": ["cnn_rom_inc.c", "trang_web_gz.c"],
}


def sub1(s, pat, rep):
    s2, n = re.subn(pat, rep, s, count=1, flags=re.S)
    assert n == 1, pat
    return s2


def main():
    s = pathlib.Path(sys.argv[1]).read_text(encoding="utf8")
    s = sub1(s, r"<TargetName>.*?</TargetName>", "<TargetName>NCKH2627_M3</TargetName>")
    s = sub1(s, r"<Device>.*?</Device>", "<Device>ARMCM3</Device>")
    s = sub1(s, r"<Vendor>.*?</Vendor>", "<Vendor>ARM</Vendor>")
    s = sub1(s, r"<PackID>.*?</PackID>", "<PackID>ARM.Cortex_DFP.1.1.0</PackID>")
    s = sub1(s, r"<PackURL>.*?</PackURL>", "<PackURL>https://www.keil.com/pack/</PackURL>")
    s = sub1(s, r"<Cpu>.*?</Cpu>",
             '<Cpu>IRAM(0x20000000,0x00002000) IROM(0x00000000,0x00008000) CPUTYPE("Cortex-M3") CLOCK(27000000) ELITTLE</Cpu>')
    s = sub1(s, r"<FlashDriverDll>.*?</FlashDriverDll>", "<FlashDriverDll></FlashDriverDll>")
    s = sub1(s, r"<RegisterFile>.*?</RegisterFile>", "<RegisterFile></RegisterFile>")
    s = sub1(s, r"<SFDFile>.*?</SFDFile>", "<SFDFile></SFDFile>")
    s = sub1(s, r"<OutputName>.*?</OutputName>", "<OutputName>nckh_m3</OutputName>")
    s = sub1(s, r"<useUlib>\d</useUlib>", "<useUlib>1</useUlib>")                 # MicroLIB
    s = sub1(s, r"<v6Lang>\d+</v6Lang>", "<v6Lang>3</v6Lang>")                    # C99
    # -Os (Keil: 6 = -Os balanced); cnn_core.c dùng -O2 ở build.sh, ở đây để chung -Os
    s = re.sub(r"(<Cads>.*?)<Optim>\d+</Optim>", r"\g<1><Optim>6</Optim>", s, count=1, flags=re.S)
    inc = r"..\src\core;..\src\app;..\src\driver;..\src\gen"
    s = sub1(s, r"(<Cads>.*?<Define>).*?(</Define>.*?<IncludePath>).*?(</IncludePath>)",
             r"\g<1>__MICROLIB\g<2>" + inc.replace("\\", "\\\\") + r"\g<3>")
    s = sub1(s, r"<umfTarg>\d</umfTarg>", "<umfTarg>0</umfTarg>")
    s = sub1(s, r"<ScatterFile>.*?</ScatterFile>", r"<ScatterFile>..\\gw1ns4c_8k.sct</ScatterFile>")
    s = sub1(s, r"(<LDads>.*?<Misc>).*?(</Misc>)", r"\g<1>--entry=Reset_Handler --info=sizes,totals\g<2>")
    groups = []
    for g, files in NHOM.items():
        fl = "".join(f"""
            <File>
              <FileName>{f}</FileName>
              <FileType>1</FileType>
              <FilePath>..\\src\\{g}\\{f}</FilePath>
            </File>""" for f in files)
        groups.append(f"""
        <Group>
          <GroupName>{g}</GroupName>
          <Files>{fl}
          </Files>
        </Group>""")
    txt = "<Groups>" + "".join(groups) + "\n      </Groups>"
    s = sub1(s, r"<Groups>.*?</Groups>", lambda m: txt)
    s = re.sub(r"\s*<RTE>.*?</RTE>", "\n\n  <RTE>\n    <apis/>\n    <components/>\n    <files/>\n  </RTE>", s, flags=re.S)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(s, encoding="utf8", newline="\r\n")
    print(OUT)


if __name__ == "__main__":
    main()
