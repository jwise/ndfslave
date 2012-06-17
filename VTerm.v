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
	
	inout usb_dn, usb_dp
	);
	
	wire xbuf;
	IBUFG clkbuf(.O(xbuf), .I(xtal));
	
	wire clk10;
	
	MulDivDCM dcm10(xbuf, clk10);
	defparam dcm10.div = 10;
	defparam dcm10.mul = 2;
	
	wire clk48;
	MulDivDCM dcm48(xbuf, clk48);
	defparam dcm48.div = 25;
	defparam dcm48.mul = 24;
	
	wire usb_oe;
	
	wire usb_tx_dp;
	wire usb_tx_dn;
	assign usb_dp = usb_oe ? 1'bz : usb_tx_dp;
	assign usb_dn = usb_oe ? 1'bz : usb_tx_dn;
	
	reg [3:0] rst_ctr = 4'h0;
	always @(posedge clk48)
		if (rst_ctr != 4'hF)
			rst_ctr <= rst_ctr + 1;
	
	wire dropped_frame;
	wire misaligned_frame;
	wire crc16_err;
	wire usb_rst;
	
	usb1_core usb(
		.clk_i(clk48),
		.rst_i(&rst_ctr),
		
		.phy_tx_mode(1), /* differential mode */
		.usb_rst(usb_rst),
		
		.tx_dp(usb_tx_dp), .tx_dn(usb_tx_dn),
		.tx_oe(usb_oe),
		.rx_d(usb_dp), .rx_dp(usb_dp), .rx_dn(usb_dn),
		
		.dropped_frame(dropped_frame),
		.misaligned_frame(misaligned_frame),
		.crc16_err(crc16_err),
		
		.v_set_int(), .v_set_feature(), .wValue(), .wIndex(),
		.vendor_data(0),
		
		.usb_busy(), .ep_sel(),
		
		.ep1_cfg(`BULK | `IN | 14'd064),
		.ep1_din(0), .ep1_dout(),
		.ep1_we(), .ep1_re(),
		.ep1_empty(0), .ep1_full(0),
		.ep1_bf_en(0), .ep1_bf_size(0),
		
		.ep2_cfg(`BULK | `IN | 14'd064),
		.ep2_din(0), .ep2_dout(),
		.ep2_we(), .ep2_re(),
		.ep2_empty(0), .ep2_full(0),
		.ep2_bf_en(0), .ep2_bf_size(0),
		
		.ep3_cfg(`BULK | `IN | 14'd064),
		.ep3_din(0), .ep3_dout(),
		.ep3_we(), .ep3_re(),
		.ep3_empty(0), .ep3_full(0),
		.ep3_bf_en(0), .ep3_bf_size(0),
		
		.ep4_cfg(`BULK | `IN | 14'd064),
		.ep4_din(0), .ep4_dout(),
		.ep4_we(), .ep4_re(),
		.ep4_empty(0), .ep4_full(0),
		.ep4_bf_en(0), .ep4_bf_size(0),
		
		.ep5_cfg(`BULK | `IN | 14'd064),
		.ep5_din(0), .ep5_dout(),
		.ep5_we(), .ep5_re(),
		.ep5_empty(0), .ep5_full(0),
		.ep5_bf_en(0), .ep5_bf_size(0),
		
		.ep6_cfg(`BULK | `IN | 14'd064),
		.ep6_din(0), .ep6_dout(),
		.ep6_we(), .ep6_re(),
		.ep6_empty(0), .ep6_full(0),
		.ep6_bf_en(0), .ep6_bf_size(0),
		
		.ep7_cfg(`BULK | `IN | 14'd064),
		.ep7_din(0), .ep7_dout(),
		.ep7_we(), .ep7_re(),
		.ep7_empty(0), .ep7_full(0),
		.ep7_bf_en(0), .ep7_bf_size(0)
		);
		
	
	reg [7:0] ndfsm = 8'h00;

	wire [15:0] display =
	  btns[0] ? {rst_ctr, dropped_frame, misaligned_frame, crc16_err, usb_rst, usb_oe, usb_dp, usb_dn, 5'h00} :
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
