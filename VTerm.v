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

module Decode7(
	input [3:0] num,
	output reg [7:0] segs);
	
	always @(*)
		case (num)
		4'h0: segs = ~8'b00111111;
		4'h1: segs = ~8'b00000110;
		4'h2: segs = ~8'b01011011;
		4'h3: segs = ~8'b01001111;
		4'h4: segs = ~8'b01100110;
		4'h5: segs = ~8'b01101101;
		4'h6: segs = ~8'b01111101;
		4'h7: segs = ~8'b00000111;
		4'h8: segs = ~8'b01111111;
		4'h9: segs = ~8'b01101111;
		4'hA: segs = ~8'b01110111;
		4'hB: segs = ~8'b01111100;
		4'hC: segs = ~8'b00111001;
		4'hD: segs = ~8'b01011110;
		4'hE: segs = ~8'b01111001;
		4'hF: segs = ~8'b01110001;
		endcase
	
endmodule

module Drive7Seg(
	input clk10,
	input [15:0] display,
	output wire [7:0] cath,
	output reg [3:0] ano = 4'b1110);

	reg [7:0] counter = 8'h0;
	
	always @(posedge clk10) begin
		counter <= counter + 8'h1;
		if (counter == 8'h0)
			ano <= {ano[0], ano[3:1]};
	end
	
	wire [3:0] num;
	Decode7 deco(num, cath);
	
	assign num = (~ano[3]) ? display[15:12] :
	             (~ano[2]) ? display[11:8] :
	             (~ano[1]) ? display[7:4] :
	                         display[3:0];
endmodule

`define IN_CLK 10000000
`define OUT_CLK 57600
`define CLK_DIV (`IN_CLK / `OUT_CLK)

module SerRX(
	input pixclk,
	output reg wr = 0,
	output reg [7:0] wchar = 0,
	input serial_raw);

	reg [15:0] rx_clkdiv = 0;
	reg [3:0] rx_state = 4'b0000;
	reg [7:0] rx_data_tmp;
	
	reg serialrx_0 = 0;
	reg serialrx = 0;

	always @(posedge pixclk)
	begin
		/* Flop it twice. */
		serialrx_0 <= serial_raw;
		serialrx   <= serialrx_0;
		
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
	output reg tx_busy = 0,
	output reg serial = 1);
	
	reg [7:0] tx_data = 0;
	reg [15:0] tx_clkdiv = 0;
	reg [3:0] tx_state = 4'b0000;
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


module VTerm(
	input xtal,
	input [3:0] btns,
	output wire [7:0] cath,
	output wire [3:0] ano,
	
	input ndf_r_b_n,
	output reg ndf_re_n,
	output reg ndf_ce_n,
	output reg ndf_cle,
	output reg ndf_ale,
	output reg ndf_we_n,
	output reg ndf_wp_n,
	
	inout [7:0] ndf_io,
	
	input serrx,
	output wire sertx,
	
	input epp_astb_n, epp_dstb_n, epp_wr_n,
	output reg epp_wait_n,
	inout [7:0] epp_dq
	);
	
	wire xbuf;
	IBUFG clkbuf(.O(xbuf), .I(xtal));
	
	wire clk10;
	
	MulDivDCM dcm10(xbuf, clk10);
	defparam dcm10.div = 10;
	defparam dcm10.mul = 2;
	
	reg epp_astb_n_s0, epp_astb_n_s;
	reg epp_dstb_n_s0, epp_dstb_n_s;
	reg epp_wr_n_s0, epp_wr_n_s;
	reg [7:0] epp_d_s0, epp_d_s;
	reg [7:0] epp_q;
	
	reg [7:0] epp_curad;
	
	assign epp_dq = epp_wr_n ? epp_q : 8'bzzzzzzzz;
	
	always @(posedge clk10) begin
		/* synchronize in signals */
		epp_astb_n_s0 <= epp_astb_n;
		epp_astb_n_s <= epp_astb_n_s0;
		
		epp_dstb_n_s0 <= epp_dstb_n;
		epp_dstb_n_s <= epp_dstb_n_s0;
		
		epp_wr_n_s0 <= epp_wr_n;
		epp_wr_n_s <= epp_wr_n_s0;
		
		epp_d_s0 <= epp_dq;
		epp_d_s <= epp_d_s0;
		
		if (~epp_astb_n_s && ~epp_wr_n_s)
			epp_curad <= epp_d_s;
	end
	
	always @(*) begin
		epp_wait_n = 0;
		epp_q = 0;
		if (~epp_astb_n_s && ~epp_wr_n_s)
			epp_wait_n = 1;
		if (~epp_dstb_n_s && epp_wr_n_s) begin
			epp_wait_n = 1;
			epp_q = ~epp_curad;
		end
	end
	
	reg [7:0] ndfsm = 8'h00;

	wire [15:0] display =
	  btns[0] ? 16'h1234 :
	  btns[1] ? 16'h4567 :
	  btns[2] ? 16'h89AB :
	  btns[3] ? 16'hCDEF :
	            {ndf_io, ndfsm};
	
	Drive7Seg drive(clk10, display, cath, ano);
	
	wire rx_strobe;
	wire [7:0] rx_data;
	SerRX rx(clk10, rx_strobe, rx_data, serrx);
	
	reg tx_strobe;
	reg [7:0] tx_data;
	wire tx_busy;
	SerTX tx(clk10, tx_strobe, tx_data, tx_busy, sertx);
	
	reg [7:0] ndfsm_next;
	reg [7:0] ndf_io_w;
	
	assign ndf_io = ndf_we_n ? 8'bzzzzzzzz : ndf_io_w;
	
	/* Tcls  - CLE setup
	 * Tclh - CLE hold
	 * Tds -- data setup
	 * Tdh -- data hold
	 * Twp - time of WE
	 * Twh - time between WE
	 * everything is relative to the rising edge of WEn!
	 *
	 * ID read: 0x90, address 0x00
	 * Tar - ALE fall to REn fall
	 * Trea - time from RE fall to data out
	 *
	 * All the timings are *far* faster than 10MHz, so I should be clear
	 *
	 * R/B is open drain
	 * FFh must be first command, wait for R/B to rise again
	 */
	always @(*) begin
		ndf_io_w = 8'h00;
		ndf_re_n = 1;
		ndf_ce_n = 0;
		ndf_cle = 0;
		ndf_ale = 0;
		ndf_we_n = 1;
		ndf_wp_n = 0;
		ndfsm_next = ndfsm;
		tx_strobe = 0;
		tx_data = 8'h00;
		
		case (ndfsm)
		'h00:	/* Wait for a character. */
			case ({rx_strobe, rx_data})
			9'h141: /* 'A' -- address */
				ndfsm_next = 'h02;
			9'h143: /* 'C' -- command */
				ndfsm_next = 'h05;
			9'h152: /* 'R' -- read */
				ndfsm_next = 'h08;
			9'h142: /* 'B' -- busy */
				ndfsm_next = 'h0A;
			9'h150: /* 'P' -- ping */
				ndfsm_next = 'h01;
			endcase
		
		'h01:	begin /* Ping. */
			tx_strobe = 1;
			tx_data = 8'h41; /* A - ack */
			if (!tx_busy) ndfsm_next = 'h00;
			end
		
		/* Address */
		'h02:	/* Wait for another character. */
			if (rx_strobe) ndfsm_next = ndfsm + 1;
		'h03:	begin /* Setup for address. */
			ndf_io_w = rx_data;
			ndf_ale = 1;
			ndf_we_n = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h04:	begin /* Hold for address. */
			ndf_io_w = rx_data;
			ndf_ale = 1;
			ndf_we_n = 1;
			ndfsm_next = 'h01;
			end
		
		/* Command */
		'h05:	/* Wait for another character. */
			if (rx_strobe) ndfsm_next = ndfsm + 1;
		'h06:	begin /* Setup for command. */
			ndf_io_w = rx_data;
			ndf_cle = 1;
			ndf_we_n = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h07:	begin /* Hold for command. */
			ndf_io_w = rx_data;
			ndf_cle = 1;
			ndf_we_n = 1;
			ndfsm_next = 'h01;
			end
		
		/* Read */
		'h08:	begin /* Setup for read */
			ndf_re_n = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h09:	begin /* Read */
			ndf_re_n = 0;
			tx_strobe = 1;
			tx_data = ndf_io;
			if (!tx_busy) ndfsm_next = 'h00;
			end
		
		/* Busy */
		'h0A:	/* Wait for ready */
			if (ndf_r_b_n) ndfsm_next = 'h01;
		endcase
	end
	always @(posedge clk10)
		ndfsm <= ndfsm_next;
endmodule
