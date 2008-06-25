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
	
	always @(posedge clk25) begin
		red <= border ? 0 : 3'b100;
		green <= border ? 0 : 0;
		blue <= border ? 0 : 0;
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
