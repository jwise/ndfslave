/////////////////////////////////////////////////////////////////////
////                                                             ////
////  Descriptor ROM                                             ////
////                                                             ////
////                                                             ////
////  Author: Rudolf Usselmann                                   ////
////          rudi@asics.ws                                      ////
////                                                             ////
////                                                             ////
////  Downloaded from: http://www.opencores.org/cores/usb1_funct/////
////                                                             ////
/////////////////////////////////////////////////////////////////////
////                                                             ////
//// Copyright (C) 2000-2002 Rudolf Usselmann                    ////
////                         www.asics.ws                        ////
////                         rudi@asics.ws                       ////
////                                                             ////
//// This source file may be used and distributed without        ////
//// restriction provided that this copyright statement is not   ////
//// removed from the file and that any derivative work contains ////
//// the original copyright notice and the associated disclaimer.////
////                                                             ////
////     THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY     ////
//// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   ////
//// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS   ////
//// FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL THE AUTHOR      ////
//// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,         ////
//// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    ////
//// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE   ////
//// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR        ////
//// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF  ////
//// LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT  ////
//// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT  ////
//// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         ////
//// POSSIBILITY OF SUCH DAMAGE.                                 ////
////                                                             ////
/////////////////////////////////////////////////////////////////////

//  CVS Log
//
//  $Id: usb1_rom1.v,v 1.1.1.1 2002-09-19 12:07:29 rudi Exp $
//
//  $Date: 2002-09-19 12:07:29 $
//  $Revision: 1.1.1.1 $
//  $Author: rudi $
//  $Locker:  $
//  $State: Exp $
//
// Change History:
//               $Log: not supported by cvs2svn $
//
//
//
//
//

`include "usb1_defines.v"

module usb1_rom1(clk, adr, dout);
input		clk;
input	[6:0]	adr;
output	[7:0]	dout;

reg	[7:0]	dout;

always @(posedge clk)
	case(adr)	// synopsys full_case parallel_case

		// ====================================
		// =====    DEVICE Descriptor     =====
		// ====================================

	   7'h00:	dout <= 8'd18;	// this descriptor length
	   7'h01:	dout <= 8'h01;	// descriptor type
	   7'h02:	dout <= 8'h00;	// USB version low byte
	   7'h03:	dout <= 8'h01;	// USB version high byte
	   7'h04:	dout <= 8'hff;	// device class
	   7'h05:	dout <= 8'h00;	// device sub class
	   7'h06:	dout <= 8'hff;	// device protocol
	   7'h07:	dout <= 8'd64;	// max packet size
	   7'h08:	dout <= 8'hfe;	// vendor ID low byte
	   7'h09:	dout <= 8'hff;	// vendor ID high byte
	   7'h0a:	dout <= 8'h01;	// product ID low byte
	   7'h0b:	dout <= 8'h40;	// product ID high byte
	   7'h0c:	dout <= 8'h01;	// device rel. number low byte
	   7'h0d:	dout <= 8'h00;	// device rel. number high byte
	   7'h0e:	dout <= 8'h01;	// Manufacturer String Index
	   7'h0f:	dout <= 8'h02;	// Product Descr. String Index
	   7'h10:	dout <= 8'h00;	// S/N String Index
	   7'h11:	dout <= 8'h01;	// Number of possible config.

		// ====================================
		// ===== Configuration Descriptor =====
		// ====================================
	   7'h12:	dout <= 8'h09;	// this descriptor length
	   7'h13:	dout <= 8'h02;	// descriptor type
	   7'h14:	dout <= 8'd32;	// total data length low byte
	   7'h15:	dout <= 8'd00;	// total data length high byte
	   7'h16:	dout <= 8'h01;	// number of interfaces
	   7'h17:	dout <= 8'h01;	// number of configurations
	   7'h18:	dout <= 8'h00;	// Conf. String Index
	   7'h19:	dout <= 8'h40;	// Config. Characteristics
	   7'h1a:	dout <= 8'h00;	// Max. Power Consumption

		// ====================================
		// =====   Interface Descriptor   =====
		// ====================================
	   7'h1b:	dout <= 8'h09;	// this descriptor length
	   7'h1c:	dout <= 8'h04;	// descriptor type
	   7'h1d:	dout <= 8'h00;	// interface number
	   7'h1e:	dout <= 8'h00;	// alternate setting
	   7'h1f:	dout <= 8'h02;	// number of endpoints
	   7'h20:	dout <= 8'hff;	// interface class
	   7'h21:	dout <= 8'h01;	// interface sub class
	   7'h22:	dout <= 8'hff;	// interface protocol
	   7'h23:	dout <= 8'h00;	// interface string index

		// ====================================
		// =====   Endpoint 1 Descriptor  =====
		// ====================================
	   7'h24:	dout <= 8'h07;	// this descriptor length
	   7'h25:	dout <= 8'h05;	// descriptor type
	   7'h26:	dout <= 8'h81;	// endpoint address
	   7'h27:	dout <= 8'h02;	// endpoint attributes
	   7'h28:	dout <= 8'h00;	// max packet size low byte
	   7'h29:	dout <= 8'h01;	// max packet size high byte
	   7'h2a:	dout <= 8'h01;	// polling interval

		// ====================================
		// =====   Endpoint 2 Descriptor  =====
		// ====================================
	   7'h2b:	dout <= 8'h07;	// this descriptor length
	   7'h2c:	dout <= 8'h05;	// descriptor type
	   7'h2d:	dout <= 8'h02;	// endpoint address
	   7'h2e:	dout <= 8'h02;	// endpoint attributes
	   7'h2f:	dout <= 8'h00;	// max packet size low byte
	   7'h30:	dout <= 8'h01;	// max packet size high byte
	   7'h31:	dout <= 8'h01;	// polling interval

		// ====================================
		// ===== String Descriptor Lang ID=====
		// ====================================

	   7'h47:	dout <= 8'd04;	// this descriptor length
	   7'h48:	dout <= 8'd03;	// descriptor type

	   7'h49:	dout <= 8'd09;	// Language ID 0 low byte
	   7'h4a:	dout <= 8'd04;	// Language ID 0 high byte

		// ====================================
		// =====   String Descriptor 0    =====
		// ====================================

	   7'h4b:	dout <= 8'd020;	// this descriptor length
	   7'h4c:	dout <= 8'd03;	// descriptor type
	   7'h4d:	dout <= "E";
	   7'h4e:	dout <= 8'h00;
	   7'h4f:	dout <= "m";
	   7'h50:	dout <= 8'h00;
	   7'h51:	dout <= "a";
	   7'h52:	dout <= 8'h00;
	   7'h53:	dout <= "r";
	   7'h54:	dout <= 8'h00;
	   7'h55:	dout <= "h";
	   7'h56:	dout <= 8'h00;
	   7'h57:	dout <= "a";
	   7'h58:	dout <= 8'h00;
	   7'h59:	dout <= "v";
	   7'h5a:	dout <= 8'h00;
	   7'h5b:	dout <= "i";
	   7'h5c:	dout <= 8'h00;
	   7'h5d:	dout <= "l";
	   7'h5e:	dout <= 8'h00;
	   
		// ====================================
		// =====   String Descriptor 1    =====
		// ====================================

	   7'h60:	dout <= 8'd22;	// this descriptor length
	   7'h61:	dout <= 8'd03;	// descriptor type
	   7'h62:	dout <= "N";
	   7'h63:	dout <= 8'h00;
	   7'h64:	dout <= "D";
	   7'h65:	dout <= 8'h00;
	   7'h66:	dout <= "F";
	   7'h67:	dout <= 8'h00;
	   7'h68:	dout <= " ";
	   7'h69:	dout <= 8'h00;
	   7'h6a:	dout <= "t";
	   7'h6b:	dout <= 8'h00;
	   7'h6c:	dout <= "o";
	   7'h6d:	dout <= 8'h00;
	   7'h6e:	dout <= " ";
	   7'h6f:	dout <= 8'h00;
	   7'h70:	dout <= "U";
	   7'h71:	dout <= 8'h00;
	   7'h72:	dout <= "S";
	   7'h73:	dout <= 8'h00;
	   7'h74:	dout <= "B";
	   7'h75:	dout <= 8'h00;

		// ====================================
		// ====================================

	   //default:	dout <= 8'd00;
	endcase

endmodule
