`define LUI		7'b0110111
`define AUIPC	7'b0010111
`define JAL		7'b1101111
`define JALR	7'b1100111
`define Bcc		7'b1100011
`define BEQ			3'd0
`define BNE			3'd1
`define BLT			3'd4
`define BGE			3'd5
`define BLTU		3'd6
`define BGEU		3'd7
`define Lx		7'b0000011
`define LB			3'd0
`define LH			3'd1
`define LW			3'd2
`define LBU			3'd4
`define LHU			3'd5
`define Sx		7'b0100011
`define SB			3'd0
`define SH			3'd1
`define SW			3'd2
`define ALU1	7'b0010011
`define ADDI		3'd0
`define SLI			3'd1
`define SLTI		3'd2
`define SLTIU		3'd3
`define XORI		3'd4
`define SRI			3'd5
`define ORI			3'd6
`define ANDI		3'd7
`define RR		7'b0110011
`define ADDSUB		3'd0
`define SLL			3'd1
`define SLT			3'd2
`define SLTU		3'd3
`define XOR			3'd4
`define SRAL		3'd5
`define OR			3'd6
`define AND			3'd7
`define FENCE	7'b0001111
`define SYSTEM	7'b1110011
`define SCALL		3'd0
`define RDCTI		3'd2


module friscv(rst_i, clk_i, bte_o, cti_o, bl_o, cyc_o, stb_o, ack_i, we_o, sel_o, adr_o, dat_i, dat_o);
input rst_i;
input clk_i;
output reg [1:0] bte_o;
output reg [2:0] cti_o;
output reg [5:0] bl_o;
output reg cyc_o;
output reg stb_o;
input ack_i;
output reg we_o;
output reg [3:0] sel_o;
output reg [31:0] adr_o;
input [31:0] dat_i;
output reg [31:0] dat_o;

parameter TRUE = 1'b1;
parameter FALSE = 1'b0;

parameter RESET = 6'd1;
parameter IFETCH1 = 6'd2;
parameter IFETCH2 = 6'd3;
parameter DECODE = 6'd4;
parameter EXECUTE = 6'd5;
parameter LOAD1 = 6'd8;
parameter LOAD2 = 6'd9;
parameter LOAD3 = 6'd10;
parameter LOAD4 = 6'd11;
parameter LOAD5 = 6'd12;
parameter STORE1 = 6'd16;
parameter STORE2 = 6'd17;
parameter STORE3 = 6'd18;
parameter STORE4 = 6'd19;
parameter STORE5 = 6'd20;
parameter LOAD_ICACHE = 6'd21;
parameter LOAD_ICACHE2 = 6'd22;
parameter RUN = 6'd23;
parameter byt = 2'd0;
parameter half = 2'd1;
parameter word = 2'd2;

wire clk = clk_i;
reg [5:0] state;
reg [31:0] pc,dpc,xpc;
reg [31:4] ibufadr;
reg [127:0] ibuf;
reg [31:0] regfile [31:0];
reg [31:0] ir,xir;
wire [6:0] opcode = ir[6:0];
wire [2:0] funct3 = ir[14:12];
reg [6:0] xopcode;
reg [2:0] xfunct3,mfunct3;
wire iisLuiz = opcode==`LUI && ir[11:7]==5'd0;
wire xisLuiz = xopcode==`LUI && xir[11:7]==5'd0;
wire [4:0] Ra = ir[19:15];
wire [4:0] Rb = ir[24:20];
reg [4:0] xRt,mRt,wRt;
reg [31:0] rfoa,rfob;
always @*
case(Ra)
5'd0:	rfoa <= 32'd0;
xRt:	rfoa <= res;
wRt:	rfoa <= wres;
default:	rfoa <= regfile[Ra];
endcase
always @*
case(Rb)
5'd0:	rfob <= 32'd0;
xRt:	rfob <= res;
wRt:	rfob <= wres;
default:	rfob <= regfile[Rb];
endcase
reg [31:0] a,b,imm,xb;
reg [31:0] res,ea,xres,mres,wres,lres;
reg [1:0] ld_size, st_size;
reg [31:0] insncnt;

wire xisLd = xopcode==`Lx;
wire xisSt = xopcode==`Sx;

reg advanceEX;
wire advanceWB = advanceEX;
wire advanceRF = advanceEX;
wire advanceIF = advanceRF & ihit;

reg isICacheReset;
reg isICacheLoad;
wire [31:0] insn;
wire ihit;

friscv_icache_ram u1
(
	.wclk(clk),
	.wa(adr_o[12:0]),
	.wr(isICacheLoad & ack_i),
	.i(dat_i),
	.rclk(~clk),
	.pc(pc[12:0]),
	.insn(insn)
);

friscv_itag_ram u2
(
	.wclk(clk),
	.wa(adr_o),
	.v(!isICacheReset),
	.wr((isICacheLoad & ack_i && (adr_o[3:2]==2'b11))|isICacheReset),
	.rclk(~clk),
	.pc(pc),
	.hit(ihit)
);

always @*
case(xopcode)
`LUI:	res <= imm;
`AUIPC:	res <= {xpc[31:12] + imm[31:12],12'h000};
`JAL:	res <= xpc;
`JALR:	res <= xpc;
`ALU1:
		case(xfunct3)
		`ADDI:	res <= a + imm;
		`SLTI:	res <= $signed(a) < $signed(imm);
		`SLTIU:	res <= a < imm;
		`XORI:	res <= a ^ imm;
		`ORI:	res <= a | imm;
		`ANDI:	res <= a & imm;
		`SLI:	res <= a << ir[24:20];
		`SRI:
			if (xir[30] & a[31])
				res <= (a >> xir[24:20]) | ~(32'hFFFFFFFF >> xir[24:20]);
			else
				res <= a >> xir[24:20];
		endcase
`RR:
		case(xfunct3)
		`ADDSUB:	res <= xir[30] ? a - b : a + b;
		`SLL:		res <= a << b[4:0];
		`SLT:		res <= $signed(a) < $signed(b);
		`SLTU:		res <= a < b;
		`XOR:		res <= a ^ b;
		`SRAL:
			if (xir[30] & a[31])
				res <= (a >> b[4:0]) | ~(32'hFFFFFFFF >> b[4:0]);
			else
				res <= a >> b[4:0];
		`OR:		res <= a | b;
		`AND:		res <= a & b;
		endcase
`Lx:	res <= lres;
default:	res <= 32'd0;
endcase

always @(posedge clk)
if (rst_i) begin
	insncnt <= 32'd0;
	ibufadr <= 28'd0;
	pc <= 32'h2000;
	state <= RESET;
	nop_ir();
	nop_xir();
	wb_nack();
	adr_o[3:2] <= 2'b11;
	isICacheReset <= TRUE;
	isICacheLoad <= FALSE;
	advanceEX <= TRUE;
end
else begin
case (state)
RESET:
begin
	adr_o <= adr_o + 32'd16;
	if (adr_o[12:4]==9'h1ff) begin
		isICacheReset <= FALSE;
		state <= RUN;
	end
end
RUN:
begin
	// IFETCH stage
	if (advanceIF) begin
		insncnt <= insncnt + 32'd1;
		ir <= insn;
		dpc <= pc;
		pc <= pc + 32'd4;
	end
	else begin
		if (!ihit)
			next_state(LOAD_ICACHE);
		if (advanceRF) begin
			if (!iisLuiz)
				nop_ir();
			dpc <= pc;
			pc <= pc;
		end
	end
	
	// DECODE / REGFETCH
	if (advanceRF) begin
		xir <= ir;
		xopcode <= opcode;
		xfunct3 <= funct3;
		xpc <= dpc;
		a <= rfoa;
		b <= rfob;
		case(opcode)
		`LUI:	imm <= {ir[31:12],12'd0};
		`AUIPC:	imm <= {ir[31:12],12'd0};
		`JAL:	imm <= {{12{ir[31]}},ir[19:12],ir[20],ir[30:21],1'b0};
		`JALR:	imm <= xisLuiz ? {xir[31:12],ir[31:20]} : {{20{ir[31]}},ir[31:20]};
		`Bcc:	imm <= {{20{ir[31]}},ir[7],ir[30:25],ir[11:8],1'b0};
		`Lx:	imm <= xisLuiz ? {xir[31:12],ir[31:20]} : {{20{ir[31]}},ir[31:20]};
		`Sx:	imm <= xisLuiz ? {xir[31:12],ir[31:20]} : {{20{ir[31]}},ir[31:25],ir[11:7]};
		`ALU1:	imm <= xisLuiz ? {xir[31:12],ir[31:20]} : {{20{ir[31]}},ir[31:20]};
		default:	imm <= 32'd0;
		endcase
		case(opcode)
		`LUI:	xRt <= ir[11:7];
		`AUIPC:	xRt <= ir[11:7];
		`JAL:	xRt <= ir[11:7];
		`JALR:	xRt <= ir[11:7];
		`Lx:	xRt <= ir[11:7];
		`ALU1:	xRt <= ir[11:7];
		`RR:	xRt <= ir[11:7];
		default:	xRt <= 5'd0;
		endcase
	end
	else if (advanceEX) begin
		if (!xisLuiz)
			nop_xir();
	end

	// EXECUTE
	if (advanceEX) begin
		wRt <= xRt;
		wres <= res;
		case(xopcode)
		`JAL:	begin pc <= xpc + imm; pc[0] <= 1'b0; nop_ir(); nop_xir(); $display("jal %h xpc=%h xir=%h", xpc+imm,xpc,xir); end
		`JALR:	begin pc <= a + imm; pc[0] <= 1'b0; nop_ir(); nop_xir(); end
		`Bcc:
				case(xfunct3)
				`BEQ:	if (a==b) begin pc <= xpc + imm; nop_ir(); nop_xir(); end
				`BNE:	if (a!=b) begin pc <= xpc + imm; nop_ir(); nop_xir(); end
				`BLT:	if ($signed(a) < $signed(b)) begin pc <= xpc + imm; nop_ir(); nop_xir(); end
				`BGE:	if ($signed(a) >= $signed(b)) begin pc <= xpc + imm; nop_ir(); nop_xir(); end
				`BLTU:	if (a < b) begin pc <= xpc + imm; nop_ir(); nop_xir(); end
				`BGEU:	if (a >= b) begin pc <= xpc + imm; nop_ir(); nop_xir(); end
				endcase
		`Lx:	begin
				next_state(LOAD2);
				advanceEX <= FALSE;
				mfunct3 <= xfunct3;
				ea <= a + imm;
				case(xfunct3)
				`LB,`LBU:	ld_size <= byt;
				`LH,`LHU:	ld_size <= half;
				`LW:		ld_size <= word;
				endcase
				end
		`Sx:	begin
				next_state(STORE2);
				advanceEX <= FALSE;
				mfunct3 <= xfunct3;
				ea <= b + imm;
				xb <= a;
				case(xfunct3)
				`SB:	st_size <= byt;
				`SH:	st_size <= half;
				`SW:	st_size <= word;
				endcase
				end
		endcase
	end
	else if (advanceWB) begin
		wRt <= 5'd0;
		wres <= 32'd0;
	end

	// WRITEBACK
	if (advanceWB) begin
		regfile[wRt] <= wres;
		if (wRt != 5'd0)
			$display("r%d = %h", wRt, wres);
	end

end	// RUN

LOAD1:
	begin
		ea <= a + imm;
		case(xfunct3)
		`LB,`LBU:	ld_size <= byt;
		`LH,`LHU:	ld_size <= half;
		`LW:		ld_size <= word;
		endcase
		next_state(LOAD2);
	end
LOAD2:
	begin
		wb_read1(ld_size,ea);
		next_state(LOAD3);
	end
LOAD3:
	if (ack_i) begin
		case(mfunct3)
		`LB:begin
			wb_nack();
			next_state(RUN);
			advanceEX <= TRUE;
			case(ea[1:0])
			2'd0:	wres <= {{24{dat_i[7]}},dat_i[7:0]};
			2'd1:	wres <= {{24{dat_i[15]}},dat_i[15:8]};
			2'd2:	wres <= {{24{dat_i[23]}},dat_i[23:16]};
			2'd3:	wres <= {{24{dat_i[31]}},dat_i[31:24]};
			endcase
			end
		`LBU:
			begin
			wb_nack();
			next_state(RUN);
			advanceEX <= TRUE;
			case(ea[1:0])
			2'd0:	wres <= dat_i[7:0];
			2'd1:	wres <= dat_i[15:8];
			2'd2:	wres <= dat_i[23:16];
			2'd3:	wres <= dat_i[31:24];
			endcase
			end
		`LH:
			case(ea[1:0])
			2'd0:	begin wres <= {{16{dat_i[15]}},dat_i[15:0]}; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd1:	begin wres <= {{16{dat_i[23]}},dat_i[23:8]}; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd2:	begin wres <= {{16{dat_i[31]}},dat_i[31:16]}; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd3:	begin wres[7:0] <= dat_i[31:24]; next_state(LOAD4); end
			endcase
		`LHU:
			case(ea[1:0])
			2'd0:	begin wres <= dat_i[15:0]; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd1:	begin wres <= dat_i[23:8]; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd2:	begin wres <= dat_i[31:16]; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd3:	begin wres[7:0] <= dat_i[31:24]; next_state(LOAD4); end
			endcase
		`LW:
			case(ea[1:0])
			2'd0:	begin wres <= dat_i; next_state(RUN); wb_nack(); advanceEX <= TRUE; end
			2'd1:	begin wres[23:0] <= dat_i[31:8]; next_state(LOAD4); end
			2'd2:	begin wres[15:0] <= dat_i[31:16]; next_state(LOAD4); end
			2'd3:	begin wres[7:0] <= dat_i[31:24]; next_state(LOAD4); end
			endcase
		endcase
	end
LOAD4:
	begin
		wb_read2(ld_size,ea);
		next_state(LOAD5);
	end
LOAD5:
	if (ack_i) begin
		wb_nack();
		next_state(RUN);
		advanceEX <= TRUE;
		case(mfunct3)
		`LH:	wres[31:8] <= {{16{dat_i[7]}},dat_i[7:0]};
		`LHU:	wres[31:8] <= dat_i[7:0];
		`LW:
			case(ea[1:0])
			2'd0:	;
			2'd1:	wres[31:24] <= dat_i[7:0];
			2'd2:	wres[31:16] <= dat_i[15:0];
			2'd3:	wres[31:8] <= dat_i[23:0];
			endcase
		endcase
	end

STORE1:
	begin
		ea <= b + imm;
		xb <= a;
		case(mfunct3)
		`SB:	st_size <= byt;
		`SH:	st_size <= half;
		`SW:	st_size <= word;
		endcase
		next_state(STORE2);
	end
STORE2:
	begin
		wb_write1(st_size,ea,xb);
		next_state(STORE3);
	end
STORE3:
	if (ack_i) begin
		wb_nack();
		if ((st_size==half && ea[1:0]==2'b11) || (st_size==word && ea[1:0]!=2'b00))
			next_state(STORE4);
		else begin
			advanceEX <= TRUE;
			next_state(RUN);
		end
	end
STORE4:
	begin
		wb_write2(st_size,ea,xb);
		next_state(STORE5);
	end
STORE5:
	if (ack_i) begin
		wb_nack();
		advanceEX <= TRUE;
		next_state(RUN);
	end

LOAD_ICACHE:
	begin
		isICacheLoad <= TRUE;
		wb_burst(6'd3,{pc[31:4],4'h0});
		next_state(LOAD_ICACHE2);
	end
LOAD_ICACHE2:
	if (ack_i) begin
		adr_o[3:2] <= adr_o[3:2] + 2'd1;
		if (adr_o[3:2]==2'b10)
			cti_o <= 3'b111;
		if (adr_o[3:2]==2'b11) begin
			isICacheLoad <= FALSE;
			wb_nack();
			next_state(RUN);
		end
	end

endcase
end

task wb_read1;
input [1:0] sz;
input [31:0] adr;
begin
	cyc_o <= 1'b1;
	stb_o <= 1'b1;
	adr_o <= adr;
	case(sz)
	byt:
		case(adr[1:0])
		2'd0:	sel_o <= 4'b0001;
		2'd1:	sel_o <= 4'b0010;
		2'd2:	sel_o <= 4'b0100;
		2'd3:	sel_o <= 4'b1000;
		endcase
	half:
		case(adr[1:0])
		2'd0:	sel_o <= 4'b0011;
		2'd1:	sel_o <= 4'b0110;
		2'd2:	sel_o <= 4'b1100;
		2'd3:	sel_o <= 4'b1000;
		endcase
	word:
		case(adr[1:0])
		2'd0:	sel_o <= 4'b1111;
		2'd1:	sel_o <= 4'b1110;
		2'd2:	sel_o <= 4'b1100;
		2'd3:	sel_o <= 4'b1000;
		endcase
	default:	sel_o <= 4'b0000;
	endcase
end
endtask;

task wb_read2;
input [1:0] sz;
input [31:0] adr;
begin
	cyc_o <= 1'b1;
	stb_o <= 1'b1;
	adr_o <= {adr[31:2]+30'd1,2'b00};
	case(sz)
	half:	sel_o <= 4'b0001;
	word:
		case(adr[1:0])
		2'd0:	sel_o <= 4'b0000;
		2'd1:	sel_o <= 4'b0001;
		2'd2:	sel_o <= 4'b0011;
		2'd3:	sel_o <= 4'b0111;
		endcase
	default:	sel_o <= 4'b0000;
	endcase
end
endtask;

task wb_burst;
input [5:0] bl;
input [31:0] adr;
begin
	bte_o <= 2'b00;
	cti_o <= 3'b001;
	cyc_o <= 1'b1;
	stb_o <= 1'b1;
	sel_o <= 4'hF;
	adr_o <= {adr[31:2],2'b00};
end
endtask

task wb_write1;
input [1:0] sz;
input [31:0] adr;
input [31:0] dat;
begin
	cyc_o <= 1'b1;
	stb_o <= 1'b1;
	we_o <= 1'b1;
	adr_o <= adr;
	case(sz)
	byt:
		begin
		dat_o <= {4{dat[7:0]}};
		case(adr[1:0])
		2'd0:	sel_o <= 4'b0001;
		2'd1:	sel_o <= 4'b0010;
		2'd2:	sel_o <= 4'b0100;
		2'd3:	sel_o <= 4'b1000;
		endcase
		end
	half:
		case(adr[1:0])
		2'd0:	begin sel_o <= 4'b0011; dat_o <= {2{dat[15:0]}}; end
		2'd1:	begin sel_o <= 4'b0110;	dat_o <= {dat[7:0],dat[15:0],dat[15:8]}; end
		2'd2:	begin sel_o <= 4'b1100; dat_o <= {2{dat[15:0]}}; end
		2'd3:	begin sel_o <= 4'b1000; dat_o <= {dat[7:0],dat[15:0],dat[15:8]}; end
		endcase
	word:
		case(adr[1:0])
		2'd0:	begin sel_o <= 4'b1111; dat_o <= dat; end
		2'd1:	begin sel_o <= 4'b1110;	dat_o <= {dat[23:0],dat[31:24]}; end
		2'd2:	begin sel_o <= 4'b1100;	dat_o <= {dat[15:0],dat[31:16]}; end
		2'd3:	begin sel_o <= 4'b1000;	dat_o <= {dat[7:0],dat[31:8]}; end
		endcase
	endcase
end
endtask

task wb_write2;
input [1:0] sz;
input [31:0] adr;
input [31:0] dat;
begin
	cyc_o <= 1'b1;
	stb_o <= 1'b1;
	we_o <= 1'b1;
	adr_o <= {adr[31:2]+30'd1,2'b00};
	case(sz)
	half:
		case(adr[1:0])
		2'd0:	;
		2'd1:	;
		2'd2:	;
		2'd3:	begin sel_o <= 4'b0001; dat_o <= {dat[7:0],dat[15:0],dat[15:8]}; end
		endcase
	word:
		case(adr[1:0])
		2'd0:	;
		2'd1:	begin sel_o <= 4'b0001; dat_o <= {dat[23:0],dat[31:24]}; end
		2'd2:	begin sel_o <= 4'b0011;	dat_o <= {dat[15:0],dat[31:16]}; end
		2'd3:	begin sel_o <= 4'b0111;	dat_o <= {dat[7:0],dat[31:8]}; end
		endcase
	endcase
end
endtask

task wb_nack;
begin
	bte_o <= 2'b00;
	cti_o <= 3'b000;
	cyc_o <= 1'b0;
	stb_o <= 1'b0;
	we_o <= 1'b0;
	sel_o <= 4'h0;
	adr_o <= 32'd0;
	dat_o <= 32'd0;
end
endtask

task nop_ir;
begin
	ir <= {12'b0,5'b0,3'b000,5'b0,7'b0010011};	// ADDI v0,x0,0
end
endtask

task nop_xir;
begin
	xopcode <= 7'h13;
	xfunct3 <= 3'b0;
	xRt <= 5'b0;
	xir <= {12'b0,5'b0,3'b000,5'b0,7'b0010011};	// ADDI v0,x0,0
end
endtask 

task next_state;
input [5:0] st;
begin
	state <= st;
end
endtask

function [127:0] fnStateName;
input [5:0] state;
case(state)
RESET:	fnStateName = "RESET ";
RUN:	fnStateName = "RUN ";
LOAD1:  fnStateName = "LOAD1 ";
LOAD2:  fnStateName = "LOAD2 ";
LOAD3:  fnStateName = "LOAD3 ";
LOAD4:  fnStateName = "LOAD4 ";
STORE1:  fnStateName = "STORE1 ";
STORE2:  fnStateName = "STORE2 ";
STORE3:  fnStateName = "STORE3 ";
STORE4:  fnStateName = "STORE4 ";
LOAD_ICACHE:	fnStateName = "LOAD_ICACHE ";
LOAD_ICACHE2:	fnStateName = "LOAD_ICACHE2 ";
endcase
endfunction

endmodule

module friscv_icache_ram(wclk, wa, wr, i, rclk, pc, insn);
input wclk;
input [12:0] wa;
input wr;
input [31:0] i;
input rclk;
input [12:0] pc;
output [31:0] insn;

reg [12:0] rpc;
reg [31:0] icache_ram [2047:0];
always @(posedge wclk)
	if (wr) icache_ram [wa[12:2]] <= i;
always @(posedge rclk)
	rpc <= pc;
assign insn = icache_ram[rpc[12:2]];

endmodule

module friscv_itag_ram(wclk, wa, v, wr, rclk, pc, hit);
input wclk;
input [31:0] wa;
input v;
input wr;
input rclk;
input [31:0] pc;
output hit;

reg [31:0] rpc;
reg [32:13] itag_ram [511:0];
always @(posedge wclk)
	if (wr) itag_ram[wa[12:4]] <= {v,wa[31:13]};
always @(posedge rclk)
	rpc <= pc;
wire [32:13] tag_out = itag_ram[rpc[12:4]];
assign hit = tag_out=={1'b1,rpc[31:13]};

endmodule

