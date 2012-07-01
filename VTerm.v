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
	input [3:0] alive,
	output wire [7:0] cath,
	output wire [3:0] ano_out);
	
	reg [3:0] ano = 4'b1110;
	assign ano_out = ano | ~alive;

	reg [9:0] counter = 10'h0;
	
	always @(posedge clk10) begin
		counter <= counter + 10'h1;
		if (counter == 10'h0)
			ano <= {ano[0], ano[3:1]};
	end
	
	wire [3:0] num;
	Decode7 deco(num, cath);
	
	assign num = (~ano[3]) ? display[15:12] :
	             (~ano[2]) ? display[11:8] :
	             (~ano[1]) ? display[7:4] :
	                         display[3:0];
endmodule

module VTerm(
	input xtal,
	input [3:0] btns,
	output wire [7:0] cath,
	output wire [3:0] ano,
	output reg  led,
	
	input ndf_r_b_n,
	output reg ndf_re_n,
	output reg [1:0] ndf_ce_n = 2'b11,
	output reg ndf_cle,
	output reg ndf_ale,
	output reg ndf_we_n,
	output reg ndf_wp_n,
	
	inout [7:0] ndf_io,
	
	input epp_astb_n, epp_dstb_n, epp_wr_n,
	output reg epp_wait_n,
	inout [7:0] epp_dq
	);
	
	wire xbuf;
	IBUFG clkbuf(.O(xbuf), .I(xtal));
	
	wire clk_ndf;
	
	MulDivDCM dcm10(xbuf, clk_ndf);
	defparam dcm10.div = 5;
	defparam dcm10.mul = 2;
	
	reg epp_astb_n_s0, epp_astb_n_s;
	reg epp_dstb_n_s0, epp_dstb_n_s;
	reg epp_wr_n_s0, epp_wr_n_s;
	reg [7:0] epp_d_s0, epp_d_s;
	reg [7:0] epp_q;
	
	reg [7:0] epp_curad;
	
	assign epp_dq = epp_wr_n_s ? epp_q : 8'bzzzzzzzz;
	
	always @(posedge clk_ndf) begin
		/* synchronize in signals */
		epp_astb_n_s0 <= epp_astb_n;
		epp_astb_n_s <= epp_astb_n_s0;
		
		epp_dstb_n_s0 <= epp_dstb_n;
		epp_dstb_n_s <= epp_dstb_n_s0;
		
		epp_wr_n_s0 <= epp_wr_n;
		epp_wr_n_s <= epp_wr_n_s0;
		
		epp_d_s0 <= epp_dq;
		epp_d_s <= epp_d_s0;
	end
	
	reg [7:0] ndfsm = 8'h00;

	wire [15:0] display =
	  btns[0] ? 16'h1234 :
	  btns[1] ? 16'h4567 :
	  btns[2] ? 16'h89AB :
	  btns[3] ? 16'hCDEF :
	            {ndf_io, ndfsm[3:0], 2'b00, ~ndf_ce_n[1:0]};
	wire [3:0] display_alive =
	  {{2{~ndf_re_n || ~ndf_we_n}}, ndfsm != 'h00, 1'b1};
	
	Drive7Seg drive(clk_ndf, display, display_alive, cath, ano);
	
	/* Flash the LED when the device is busy. */
	reg [20:0] led_countdown = 'h0;
	always @(posedge clk_ndf)
		if (led_countdown != 'h0)
			led_countdown <= led_countdown - 1;
		else if (led != ndf_r_b_n) begin
			led <= ndf_r_b_n;
			led_countdown <= led_countdown - 1;
		end
	
	reg [7:0] ndfsm_next;
	reg [7:0] ndf_io_w;
	
	assign ndf_io = ndf_re_n ? ndf_io_w : 8'bzzzzzzzz;
	
	reg epp_wait_n_next = 0;
	reg [7:0] epp_q_next = 0;
	reg ndf_re_n_next = 0;
	reg ndf_we_n_next = 0;
	reg [7:0] ndf_io_w_next = 0;
	reg ce_lat_next = 0;
	
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
		epp_wait_n_next = 0;
		epp_q_next = 0;

		ndf_io_w_next = 8'hAA;
		ndf_re_n_next = 1;
		ndf_cle = 0;
		ndf_ale = 0;
		ndf_we_n_next = 1;
		ndf_wp_n = 0;
		ndfsm_next = ndfsm;
		ce_lat_next = 0;
		
		case (ndfsm)
		'h00:	/* Wait a cycle to let the data bus settle after the initial drive fight. */
			if (~epp_astb_n_s || ~epp_dstb_n_s)
				ndfsm_next = 'h0B;
		'h0B:	/* Wait for a character. */
			casex ({epp_astb_n_s, epp_dstb_n_s, epp_wr_n_s, epp_curad})
			{3'b100, 8'h41}: /* 'A' -- address */
				ndfsm_next = 'h02;
			{3'b100, 8'h43}: /* 'C' -- command */
				ndfsm_next = 'h04;
			{3'b101, 8'h44}: /* 'D' -- data */
				ndfsm_next = 'h0C;
			{3'b101, 8'h42}: /* 'B' -- busy */
				ndfsm_next = 'h09;
			{3'b100, 8'h45}: /* 'E' -- chip enable */
				ndfsm_next = 'h0A;
			{3'b010, 8'hxx}:
				epp_wait_n_next = 1;
			{3'b11x, 8'hxx}:
				ndfsm_next = 'h00;
			endcase
		
		'h01:	begin /* ACK on write. */
			epp_wait_n_next = 1;
			if (epp_dstb_n_s) ndfsm_next = 'h00;
			end
		
		/* Address */
		'h02:	begin /* Setup for address. */
			ndf_io_w_next = epp_d_s;
			ndf_ale = 1;
			ndf_we_n_next = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h03:	begin /* Hold for address. */
			ndf_io_w_next = epp_d_s;
			ndf_ale = 1;
			ndf_we_n_next = 1;
			ndfsm_next = 'h01;
			end
		
		/* Command */
		'h04:	begin /* Setup for command. */
			ndf_io_w_next = epp_d_s;
			ndf_cle = 1;
			ndf_we_n_next = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h05:	begin /* Hold for command. */
			ndf_io_w_next = epp_d_s;
			ndf_cle = 1;
			ndf_we_n_next = 1;
			ndfsm_next = 'h01;
			end
		
		/* Read */
		'h0C:	/* Wait for ready. */
			if (ndf_r_b_n)
				ndfsm_next = 'h06;
		'h06:	begin /* Setup for read */
			ndf_re_n_next = 0;
			epp_q_next = ndf_io;
			ndfsm_next = ndfsm + 1;
			end
		'h07:	begin /* Read */
			ndf_re_n_next = 0;
			epp_q_next = ndf_io;
			ndfsm_next = ndfsm + 1;
			end
		'h08:	begin /* Read */
			ndf_re_n_next = 0;
			epp_q_next = ndf_io;
			epp_wait_n_next = 1;
			if (epp_dstb_n_s) ndfsm_next = 'h00;
			end
		
		/* Busy */
		'h09:	begin /* Wait for ready */
			epp_wait_n_next = ndf_r_b_n;
			epp_q_next = {8{ndf_r_b_n}};
			if (epp_dstb_n_s) ndfsm_next = 'h00;
			end
		
		/* Chip enable */
		'h0A:	begin
			ce_lat_next = 1;
			ndfsm_next = 'h01;
			end
		endcase
	end
	
	always @(posedge clk_ndf) begin
		ndfsm <= ndfsm_next;
		
		ndf_re_n <= ndf_re_n_next;
		ndf_we_n <= ndf_we_n_next;
		ndf_io_w <= ndf_io_w_next;
		epp_q <= epp_q_next;
		epp_wait_n <= epp_wait_n_next;
		if (~epp_astb_n_s && ~epp_wr_n_s)
			epp_curad <= epp_d_s;
		if (ce_lat_next)
			ndf_ce_n <= epp_d_s[1:0];
	end
endmodule
