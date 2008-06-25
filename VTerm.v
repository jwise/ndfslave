module MulDivDCM(input xtal, output clk);
	parameter div = 5;
	parameter mul = 2;
	
	wire CLKFX_BUF;
	wire GND_BIT = 0;
	BUFG CLKFX_BUFG_INST (.I(CLKFX_BUF),
				.O(clk));
	DCM_SP DCM_SP_INST (.CLKFB(GND_BIT), 
			.CLKIN(xtal), 
			.DSSEN(GND_BIT), 
			.PSCLK(GND_BIT), 
			.PSEN(GND_BIT), 
			.PSINCDEC(GND_BIT), 
			.RST(GND_BIT), 
			.CLKFX(CLKFX_BUF));
	defparam DCM_SP_INST.CLK_FEEDBACK = "NONE";
	defparam DCM_SP_INST.CLKDV_DIVIDE = 2.0;
	defparam DCM_SP_INST.CLKFX_DIVIDE = div;
	defparam DCM_SP_INST.CLKFX_MULTIPLY = mul;
	defparam DCM_SP_INST.CLKIN_DIVIDE_BY_2 = "FALSE";
	defparam DCM_SP_INST.CLKIN_PERIOD = 20.000;
	defparam DCM_SP_INST.CLKOUT_PHASE_SHIFT = "NONE";
	defparam DCM_SP_INST.DESKEW_ADJUST = "SYSTEM_SYNCHRONOUS";
	defparam DCM_SP_INST.DFS_FREQUENCY_MODE = "LOW";
	defparam DCM_SP_INST.DLL_FREQUENCY_MODE = "LOW";
	defparam DCM_SP_INST.DUTY_CYCLE_CORRECTION = "TRUE";
	defparam DCM_SP_INST.FACTORY_JF = 16'hC080;
	defparam DCM_SP_INST.PHASE_SHIFT = 0;
	defparam DCM_SP_INST.STARTUP_WAIT = "TRUE";
endmodule

module VTerm(
	input xtal,
	output wire vs, hs,
	output reg [2:0] red,
	output reg [2:0] green,
	output reg [1:0] blue
	);
	
	wire clk25;

	wire [11:0] x, y;
	wire border;

	MulDivDCM dcm25(xtal, clk25);
	defparam dcm25.div = 4;
	defparam dcm25.mul = 2;

	SyncGen sync(clk25, vs, hs, x, y, border);
	
	wire [7:0] cschar;
	wire [2:0] csrow;
	wire [7:0] csdata;
	
	wire [11:0] vraddr;
	wire [7:0] vrdata;
	
	reg [11:0] vwaddr = 0;
	
	wire odata;
	
	CharSet cs(cschar, csrow, csdata);
	VideoRAM vram(clk25, vraddr, vrdata, vwaddr, 8'h41, 1);
	VDisplay dpy(clk25, x, y, vraddr, vrdata, cschar, csrow, csdata, odata);
	
	always @(posedge clk25)
		vwaddr <= vwaddr + 1;
	
	always @(posedge clk25) begin
		red <= border ? 0 : {3{odata}};
		green <= border ? 0 : {3{odata}};
		blue <= border ? 0 : {2{odata}};
	end
endmodule

module SyncGen(
	input pixclk,
	output reg vs, hs,
	output reg [11:0] x, y,
	output reg border);
	
	parameter XRES = 640;
	parameter XFPORCH = 16;
	parameter XSYNC = 96;
	parameter XBPORCH = 48;
	
	parameter YRES = 480;
	parameter YFPORCH = 10;
	parameter YSYNC = 2;
	parameter YBPORCH = 29;
	
	always @(posedge pixclk)
	begin
		if (x >= (XRES + XFPORCH + XSYNC + XBPORCH))
		begin
			if (y >= (YRES + YFPORCH + YSYNC + YBPORCH))
				y = 0;
			else
				y = y + 1;
			x = 0;
		end else
			x = x + 1;
		hs <= (x >= (XRES + XFPORCH)) && (x < (XRES + XFPORCH + XSYNC));
		vs <= (y >= (YRES + YFPORCH)) && (y < (YRES + YFPORCH + YSYNC));
		border <= (x > XRES) || (y > YRES);
	end
endmodule

module CharSet(
	input [7:0] char,
	input [2:0] row,
	output wire [7:0] data);

	reg [7:0] rom [(256 * 8 - 1):0];
	
	initial
		$readmemb("ibmpc1.mem", rom);

	assign data = rom[{char, row}];
endmodule

module VideoRAM(
	input pixclk,
	input [11:0] raddr,
	output reg [7:0] rdata,
	input [11:0] waddr,
	input [7:0] wdata,
	input wr);
	
	reg [7:0] ram [80*25-1 : 0];
	
	always @(posedge pixclk)
		rdata <= ram[raddr];
	
	always @(posedge pixclk)
		if (wr)
			ram[waddr] <= wdata;
endmodule

module VDisplay(
	input pixclk,
	input [11:0] x,
	input [11:0] y,
	output wire [11:0] raddr,
	input [7:0] rchar,
	output wire [7:0] cschar,
	output wire [2:0] csrow,
	input [7:0] csdata,
	output reg data
	);

	wire [7:0] col = x[11:3];
	wire [5:0] row = y[9:3];
	reg [7:0] ch;
	reg [11:0] xdly;

	assign raddr = ({row,4'b0} + {row,6'b0} + {4'h0,col});
	assign cschar = rchar;
	assign csrow = y[2:0];
	
	always @(posedge pixclk)
		xdly <= x;
	
	always @(posedge pixclk)
		data = ((xdly < 80 * 8) && (y < 25 * 8)) ? csdata[7 - xdly[2:0]] : 0;
endmodule
