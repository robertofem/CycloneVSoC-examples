module axi_conduit_merger #(
  parameter ID_WIDTH      = 1,
  parameter DATA_WIDTH    = 32,
  parameter ADDRESS_WIDTH = 32,
  parameter AXUSER_WIDTH  = 5  
) (
// axi master
output       				m_awvalid,  
output [3:0] 				m_awlen  ,  
output [2:0] 				m_awsize ,  
output [1:0] 				m_awburst,  
output [1:0] 				m_awlock ,  
output [3:0] 				m_awcache,  
output [2:0] 				m_awprot ,  
input        				m_awready,  
output [AXUSER_WIDTH-1:0] 	m_awuser ,  
output       				m_arvalid,  
output [3:0] 				m_arlen  ,  
output [2:0] 				m_arsize ,  
output [1:0] 				m_arburst,  
output [1:0] 				m_arlock ,  
output [3:0] 				m_arcache,  
output [2:0] 				m_arprot ,  
input        				m_arready,  
output [AXUSER_WIDTH-1:0] 	m_aruser ,  
input        				m_rvalid ,  
input        				m_rlast  ,  
input  [1:0] 				m_rresp  ,  
output       				m_rready ,  
output       				m_wvalid ,  
output       				m_wlast  ,  
input        				m_wready ,  
input        				m_bvalid ,  
input  [1:0] 				m_bresp  ,  
output       				m_bready ,  
output [ADDRESS_WIDTH-1:0] 	m_awaddr ,   
output [ID_WIDTH-1:0] 		m_awid   ,   
output [ADDRESS_WIDTH-1:0] 	m_araddr ,   
output [ID_WIDTH-1:0] 		m_arid   ,   
input  [DATA_WIDTH-1:0] 	m_rdata  ,   
input  [ID_WIDTH-1:0] 		m_rid    ,   
output [DATA_WIDTH-1:0] 	m_wdata  ,   
output [DATA_WIDTH/8-1:0]  	m_wstrb  ,   
output [ID_WIDTH-1:0] 		m_wid    ,   
input  [ID_WIDTH-1:0] 		m_bid    ,   

// axi slave
input       				s_awvalid,  
input  [3:0] 				s_awlen  ,  
input  [2:0] 				s_awsize ,  
input  [1:0] 				s_awburst,  
input  [1:0] 				s_awlock ,  
input  [3:0] 				s_awcache,  
input  [2:0] 				s_awprot ,  
output         				s_awready,  
input  [AXUSER_WIDTH-1:0] 	s_awuser ,  
input       				s_arvalid,  
input  [3:0] 				s_arlen  ,  
input  [2:0] 				s_arsize ,  
input  [1:0] 				s_arburst,  
input  [1:0] 				s_arlock ,  
input  [3:0] 				s_arcache,  
input  [2:0] 				s_arprot ,  
output         				s_arready,  
input  [AXUSER_WIDTH-1:0] 	s_aruser ,  
output        				s_rvalid ,  
output        				s_rlast  ,  
output [1:0] 				s_rresp  ,  
input       				s_rready ,  
input       				s_wvalid ,  
input       				s_wlast  ,  
output        				s_wready ,  
output        				s_bvalid ,  
output [1:0] 				s_bresp  ,  
input       				s_bready ,  
input  [ADDRESS_WIDTH-1:0] 	s_awaddr,   
input  [ID_WIDTH-1:0] 		s_awid  ,   
input  [ADDRESS_WIDTH-1:0] 	s_araddr,   
input  [ID_WIDTH-1:0] 		s_arid  ,   
output [DATA_WIDTH-1:0] 	s_rdata ,   
output [ID_WIDTH-1:0] 		s_rid   ,   
input  [DATA_WIDTH-1:0] 	s_wdata ,   
input  [DATA_WIDTH/8-1:0]  	s_wstrb ,   
input  [ID_WIDTH-1:0] 		s_wid   ,   
output [ID_WIDTH-1:0] 		s_bid   ,  

// conduits 
input [3:0] 				c_awcache,
input [2:0] 				c_awprot ,
input [AXUSER_WIDTH-1:0] 	c_awuser,
input [3:0] 				c_arcache,
input [2:0] 				c_arprot ,
input [AXUSER_WIDTH-1:0] 	c_aruser,
	  
// clock and reset
input    					clk,
input    					rst_n 
);

wire                        axi_wbus_bp;
wire                        axi_rbus_bp;
wire [3:0] 					v_awcache;
wire [2:0] 					v_awprot;
wire [AXUSER_WIDTH-1:0] 	v_awuser;
reg  [3:0] 					r_awcache;
reg  [2:0] 					r_awprot;
reg  [AXUSER_WIDTH-1:0] 	r_awuser;
wire [3:0] 					v_arcache;
wire [2:0] 					v_arprot;
wire [AXUSER_WIDTH-1:0] 	v_aruser;
reg  [3:0] 					r_arcache;
reg  [2:0] 					r_arprot;
reg  [AXUSER_WIDTH-1:0] 	r_aruser;

assign axi_wbus_bp = s_awvalid & ~m_awready;
assign v_awcache  = (!axi_wbus_bp) ? c_awcache : r_awcache;
assign v_awprot   = (!axi_wbus_bp) ? c_awprot  : r_awprot;
assign v_awuser   = (!axi_wbus_bp) ? c_awuser  : r_awuser;
assign v_arcache  = (!axi_rbus_bp) ? c_arcache : r_arcache;
assign v_arprot   = (!axi_rbus_bp) ? c_arprot  : r_arprot;
assign v_aruser   = (!axi_rbus_bp) ? c_aruser  : r_aruser;

always @(posedge clk or negedge rst_n) begin
if (!rst_n) begin
  r_awcache <= 4'd0;
  r_awprot  <= 3'd0;
  r_awuser  <= {AXUSER_WIDTH{1'b0}};
  r_arcache <= 4'd0;
  r_arprot  <= 3'd0;
  r_aruser  <= {AXUSER_WIDTH{1'b0}};
  end
else begin
  r_awcache <= v_awcache;
  r_awprot  <= v_awprot;
  r_awuser  <= v_awuser;
  r_arcache <= v_arcache;
  r_arprot  <= v_arprot;
  r_aruser  <= v_aruser;
  end
end

// conduit signals replacement
assign m_awcache = r_awcache;
assign m_awprot  = r_awprot;
assign m_awuser  = r_awuser;
assign m_arcache = r_arcache;
assign m_arprot  = r_arprot;
assign m_aruser  = r_aruser;

// axi bus assignment
assign m_awvalid = s_awvalid   ;
assign m_awlen   = s_awlen     ;
assign m_awsize  = s_awsize    ;
assign m_awburst = s_awburst   ;
assign m_awlock  = s_awlock    ;
// assign m_awcache = s_awcache;
// assign m_awprot  = s_awprot ;
// assign m_awuser  = s_awuser ;
assign m_awaddr  = s_awaddr    ;
assign m_awid    = s_awid      ;
assign s_awready = m_awready   ;
assign m_arvalid = s_arvalid   ;
assign m_arlen   = s_arlen     ;
assign m_arsize  = s_arsize    ;
assign m_arburst = s_arburst   ;
assign m_arlock  = s_arlock    ;
// assign m_arcache = s_arcache;
// assign m_arprot  = s_arprot ;
// assign m_aruser  = s_aruser ;
assign m_araddr  = s_araddr    ;
assign m_arid    = s_arid      ;
assign s_arready = m_arready   ;
assign s_rvalid  = m_rvalid    ;
assign s_rlast   = m_rlast     ;
assign s_rresp   = m_rresp     ;
assign s_rdata   = m_rdata     ;
assign s_rid     = m_rid       ;
assign m_rready  = s_rready    ;
assign m_wvalid  = s_wvalid    ;
assign m_wlast   = s_wlast     ;
assign m_wdata   = s_wdata     ;
assign m_wstrb   = s_wstrb     ;
assign m_wid     = s_wid       ;
assign s_wready  = m_wready    ;
assign s_bvalid  = m_bvalid    ;
assign s_bresp   = m_bresp     ;
assign s_bid     = m_bid       ;
assign m_bready  = s_bready    ;      

endmodule
