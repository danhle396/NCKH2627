// rtl/sim/gl_wrap.v — vỏ bọc chỉ dùng cho mô phỏng netlist sau đặt – nối dây (rtl/scripts/gl_inc.sh).
// cnn_apb có 78 cổng, nhiều hơn 39 chân của GW1NSR-LV4C QN48, nên ở đây: pwdata còn 13 bit (đủ cho mọi thanh ghi ghi:
// CTRL 2 bit, NEW_DATA 8 bit, WROM_ADDR 13 bit), prdata được chốt vào thanh ghi khi giao dịch đọc xong và đọc ra từng
// byte qua bsel. Tổng 35 chân. Logic thêm chỉ là thanh ghi chốt và bộ chọn byte, không đụng đường tính.
module gl_wrap (
    input  wire        pclk,
    input  wire        psel,
    input  wire        penable,
    input  wire        pwrite,
    input  wire [7:2]  paddr,
    input  wire [12:0] pwdata,
    input  wire [1:0]  bsel,
    output wire [7:0]  rbyte,
    output wire        pready,
    output wire        irq
);
    wire [31:0] prdata;
    wire        pslverr;
    reg  [31:0] hold = 32'd0;
    cnn_apb u (.pclk(pclk), .psel(psel), .penable(penable), .pwrite(pwrite), .paddr({paddr, 2'b00}),
               .pwdata({19'd0, pwdata}), .prdata(prdata), .pready(pready), .pslverr(pslverr), .irq(irq));
    always @(posedge pclk) if (psel && penable && pready && !pwrite) hold <= prdata;
    assign rbyte = hold[8*bsel +: 8];
endmodule
