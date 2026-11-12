// config.v — Gowin_EMPU(GW1NS-4C) cho NCKH2627 (tương đương chọn trong IP Core Generator):
// GPIO (không IOBUF trong IP), UART0, UART1, USER_INT0, APB2 Master 1, SRAM 8 KB = 4 BSRAM (NUM_BRAMS_4).
// rtl/scripts/build_gowin.sh --sram 2|4|16 thay dòng NUM_BRAMS_* trên bản sao.
`define GOWIN_GPIO_SUPPORT
`define GOWIN_UART0_SUPPORT
`define GOWIN_UART1_SUPPORT
`define GOWIN_INT0_SUPPORT
`define GOWIN_APB2_SUPPORT
`define GOWIN_APB2_M1_SUPPORT
`define NUM_BRAMS_4
