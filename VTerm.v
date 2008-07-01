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
	output reg [1:0] blue,
	input serrx,
	output sertx,
	input ps2c, ps2d);
	
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
	
	wire [10:0] vraddr;
	wire [7:0] vrdata;
	
	wire [10:0] vwaddr;
	wire [7:0] vwdata;
	wire [7:0] serdata;
	wire vwr, serwr;
	wire [10:0] vscroll;
	
	wire odata;
	
	wire [7:0] sertxdata;
	wire sertxwr;
	
	CharSet cs(cschar, csrow, csdata);
	VideoRAM vram(clk25, vraddr + vscroll, vrdata, vwaddr, vwdata, vwr);
	VDisplay dpy(clk25, x, y, vraddr, vrdata, cschar, csrow, csdata, odata);
	SerRX rx(clk25, serwr, serdata, serrx);
	SerTX tx(clk25, sertxwr, sertxdata, sertx);
	RXState rxsm(clk25, vwr, vwaddr, vwdata, vscroll, serwr, serdata);
	PS2 ps2(clk25, ps2c, ps2d, sertxwr, sertxdata);
	
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
	input [10:0] raddr,
	output reg [7:0] rdata,
	input [10:0] waddr,
	input [7:0] wdata,
	input wr);
	
	reg [7:0] ram [2047 : 0];
	
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
	output wire [10:0] raddr,
	input [7:0] rchar,
	output wire [7:0] cschar,
	output wire [2:0] csrow,
	input [7:0] csdata,
	output reg data);

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

`define IN_CLK 25000000
`define OUT_CLK 57600
`define CLK_DIV (`IN_CLK / `OUT_CLK)

module SerRX(
	input pixclk,
	output reg wr = 0,
	output reg [7:0] wchar = 0,
	input serialrx);

	reg [15:0] rx_clkdiv = 0;
	reg [3:0] rx_state = 4'b0000;
	reg [7:0] rx_data_tmp;
	

	always @(posedge pixclk)
	begin
		if ((rx_state == 0) && (serialrx == 0) /*&& (rx_hasdata == 0)*/)		/* Kick off. */
			rx_state <= 4'b0001;
		else if ((rx_state != 4'b0000) && (rx_clkdiv == 0)) begin
			if (rx_state != 4'b1010)
				rx_state <= rx_state + 1;
			else
				rx_state <= 0;
			case (rx_state)
			4'b0001:	begin end /* Twiddle thumbs -- this is the end of the half bit. */
			4'b0010:	rx_data_tmp[0] <= serialrx;
			4'b0011:	rx_data_tmp[1] <= serialrx;
			4'b0100:	rx_data_tmp[2] <= serialrx;
			4'b0101:	rx_data_tmp[3] <= serialrx;
			4'b0110:	rx_data_tmp[4] <= serialrx;
			4'b0111:	rx_data_tmp[5] <= serialrx;
			4'b1000:	rx_data_tmp[6] <= serialrx;
			4'b1001:	rx_data_tmp[7] <= serialrx;
			4'b1010:	if (serialrx == 1) begin
						wr <= 1;
						wchar <= rx_data_tmp;
					end
			endcase
		end
		
		if (wr)
			wr <= 0;
		
		if ((rx_state == 0) && (serialrx == 0) /*&& (rx_hasdata == 0)*/)		/* Wait half a period before advancing. */
			rx_clkdiv <= `CLK_DIV / 2 + `CLK_DIV / 4;
		else if (rx_clkdiv == `CLK_DIV)
			rx_clkdiv <= 0;
		else
			rx_clkdiv <= rx_clkdiv + 1;
	end
endmodule

module SerTX(
	input pixclk,
	input wr,
	input [7:0] char,
	output reg serial = 1);
	
	reg [7:0] tx_data = 0;
	reg [15:0] tx_clkdiv = 0;
	reg [3:0] tx_state = 4'b0000;
	reg tx_busy = 0;
	wire tx_newdata = wr && !tx_busy;

	always @(posedge pixclk)
	begin
		if(tx_newdata) begin
			tx_data <= char;
			tx_state <= 4'b0000;
			tx_busy <= 1;
		end else if (tx_clkdiv == 0) begin
			tx_state <= tx_state + 1;
			if (tx_busy)
				case (tx_state)
				4'b0000: serial <= 0;
				4'b0001: serial <= tx_data[0];
				4'b0010: serial <= tx_data[1];
				4'b0011: serial <= tx_data[2];
				4'b0100: serial <= tx_data[3];
				4'b0101: serial <= tx_data[4];
				4'b0110: serial <= tx_data[5];
				4'b0111: serial <= tx_data[6];
				4'b1000: serial <= tx_data[7];
				4'b1001: serial <= 1;
				4'b1010: tx_busy <= 0;
				default: $stop;
				endcase
		end
		
		if(tx_newdata || (tx_clkdiv == `CLK_DIV))
			tx_clkdiv <= 0;
		else
			tx_clkdiv <= tx_clkdiv + 1;
	end
endmodule

module RXState(
	input clk25,
	output reg vwr = 0,
	output reg [10:0] vwaddr = 0,
	output reg [7:0] vwdata = 0,
	output reg [10:0] vscroll = 0,
	input serwr,
	input [7:0] serdata);

	parameter STATE_IDLE = 4'b0000;
	parameter STATE_NEWLINE = 4'b0001;
	parameter STATE_CLEAR = 4'b0010;

	reg [3:0] state = STATE_CLEAR;
	
	reg [6:0] x = 0;
	reg [4:0] y = 0;
	
	reg [10:0] clearstart = 0;
	reg [10:0] clearend = 11'b11111111111;
	
	always @(posedge clk25)
		case (state)
		STATE_IDLE:	if (serwr) begin
					if (serdata == 8'h0A) begin
						state <= STATE_NEWLINE;
						vwr <= 0;
					end else if (serdata == 8'h0D) begin
						x <= 0;
						vwr <= 0;
					end else if (serdata == 8'h0C) begin
						clearstart <= 0;
						clearend <= 11'b11111111111;
						x <= 0;
						y <= 0;
						vscroll <= 0;
						state <= STATE_CLEAR;
					end else begin
						vwr <= 1;
						vwaddr <= ({y,4'b0} + {y,6'b0} + {4'h0,x}) + vscroll;
						vwdata <= serdata;
						if (x == 79) begin
							x <= 0;
							state <= STATE_NEWLINE;
						end else 
							x <= x + 1;
					end
				end
		STATE_NEWLINE:
			begin
				vwr <= 0;
				if (y == 24) begin
					vscroll <= vscroll + 80;
					clearstart <= (25 * 80) + vscroll;
					clearend <= (26*80) + vscroll;
					state <= STATE_CLEAR;
				end else begin
					y <= y + 1;
					state <= STATE_IDLE;
				end
			end
		STATE_CLEAR:
			begin
				vwr <= 1;
				vwaddr <= clearstart;
				vwdata <= 8'h20;
				clearstart <= clearstart + 1;
				if (clearstart == clearend)
					state <= STATE_IDLE;
			end
		endcase
endmodule

module PS2(
	input pixclk,
	input inclk,
	input indata,
	output reg wr,
	output reg [7:0] data
	);

	reg [3:0] bitcount = 0;
	reg [7:0] key = 0;
	reg keyarrow = 0, keyup = 0, parity = 0;

	
	/* Clock debouncing */
	reg lastinclk = 0;
	reg [6:0] debounce = 0;
	reg fixedclk = 0;
	reg [11:0] resetcountdown = 0;
	
	reg [6:0] unshiftedrom [127:0];	initial $readmemh("scancodes.unshifted.hex", unshiftedrom);
	reg [6:0] shiftedrom [127:0];	initial $readmemh("scancodes.shifted.hex", shiftedrom);
	
	reg mod_lshift = 0;
	reg mod_rshift = 0;
	reg mod_capslock = 0;
	wire mod_shifted = (mod_lshift | mod_rshift) ^ mod_capslock;
	
	reg nd = 0;
	reg lastnd = 0;
	
	always @(posedge pixclk) begin
		if (inclk != lastinclk) begin
			lastinclk <= inclk;
			debounce <= 1;
			resetcountdown <= 12'b111111111111;
		end else if (debounce == 0) begin
			fixedclk <= inclk;
			resetcountdown <= resetcountdown - 1;
		end else
			debounce <= debounce + 1;
		
		if (nd ^ lastnd) begin
			lastnd <= nd;
			wr <= 1;
		end else
			wr <= 0;
	end

	always @(negedge fixedclk) begin
		if (resetcountdown == 0)
			bitcount <= 0;
		else if (bitcount == 10) begin
			bitcount <= 0;
			if(parity != (^ key)) begin
				if(keyarrow) begin
					casex(key)
						8'hF0: keyup <= 1;
						8'hxx: keyarrow <= 0;
					endcase
				end
				else begin
					if(keyup) begin
						keyup <= 0;
						keyarrow <= 0;
						casex (key)
						8'h12: mod_lshift <= 0;
						8'h59: mod_rshift <= 0;
						endcase
						// handle this? I don't fucking know
					end
					else begin
						casex(key)
							8'hE0: keyarrow <= 1;	// handle these? I don't fucking know
							8'hF0: keyup <= 1;
							8'h12: mod_lshift <= 1;
							8'h59: mod_rshift <= 1;
							8'h14: mod_capslock <= ~mod_capslock;
							8'b0xxxxxxx: begin nd <= ~nd; data <= mod_shifted ? shiftedrom[key] : unshiftedrom[key]; end
							8'b1xxxxxxx: begin /* AAAAAAASSSSSSSS */ end
						endcase
					end
				end
			end
			else begin
				keyarrow <= 0;
				keyup <= 0;
			end
		end else
			bitcount <= bitcount + 1;

		case(bitcount)
			1: key[0] <= indata;
			2: key[1] <= indata;
			3: key[2] <= indata;
			4: key[3] <= indata;
			5: key[4] <= indata;
			6: key[5] <= indata;
			7: key[6] <= indata;
			8: key[7] <= indata;
			9: parity <= indata;
		endcase
	end

endmodule
