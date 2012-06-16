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
	
	inout [7:0] ndf_io);
	
	wire clk10;
	
	MulDivDCM dcm10(xtal, clk10);
	defparam dcm10.div = 10;
	defparam dcm10.mul = 2;
	
	reg [7:0] ndfsm = 8'h00;

	wire [15:0] display =
	  btns[0] ? 16'h0123 :
	  btns[1] ? 16'h4567 :
	  btns[2] ? 16'h89AB :
	  btns[3] ? 16'hCDEF :
	            {ndf_io, ndfsm};
	
	Drive7Seg drive(clk10, display, cath, ano);
	
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
		
		case (ndfsm)
		'h00:	/* Wait for ready. */
			if (ndf_r_b_n) ndfsm_next = ndfsm + 1;
		'h01:	begin /* Setup for reset. */
			ndf_io_w = 8'hFF;
			ndf_cle = 1;
			ndf_we_n = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h02:	begin /* Release reset; wait for not ready to continue */
			ndf_io_w = 8'hFF;
			ndf_cle = 1;
			ndf_we_n = 1;
			if (!ndf_r_b_n) ndfsm_next = ndfsm + 1;
			end
		'h03:	/* Wait for ready to continue */
			if (ndf_r_b_n) ndfsm_next = ndfsm + 1;
		'h04:	begin /* Setup for ID read command */
			ndf_io_w = 8'h90;
			ndf_cle = 1;
			ndf_we_n = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h05:	begin /* Release */
			ndf_io_w = 8'h90;
			ndf_cle = 1;
			ndf_we_n = 1;
			ndfsm_next = ndfsm + 1;
			end
		'h06:	begin /* Setup for ID address */
			ndf_io_w = 8'h00;
			ndf_ale = 1;
			ndf_we_n = 0;
			ndfsm_next = ndfsm + 1;
			end
		'h07:	begin /* Release */
			ndf_io_w = 8'h00;
			ndf_ale = 1;
			ndf_we_n = 1;
			ndfsm_next = ndfsm + 1;
			end
		'h08:	begin /* Read! */
			ndf_re_n = 0;
			end
		endcase
	end
	always @(posedge clk10)
		ndfsm <= ndfsm_next;
endmodule
