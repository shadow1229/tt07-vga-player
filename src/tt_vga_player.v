`default_nettype none
module tt_um_shadow1229_vga_player (
    input  wire [7:0] ui_in,    // Dedicated inputs
    output wire [7:0] uo_out,   // Dedicated outputs
    input  wire [7:0] uio_in,   // IOs: Input path
    output wire [7:0] uio_out,  // IOs: Output path
    output wire [7:0] uio_oe,   // IOs: Enable path (active high: 0=input, 1=output)
    input  wire       ena,      // always 1 when the design is powered, so you can ignore it
    input  wire       clk,      // clock
    input  wire       rst_n     // reset_n - low to reset
    );
    
    wire hsync;
    wire vsync;
    reg [3:0] spi_uio_out; //wire ->reg is this right?
    wire [5:0] rrggbb;
    wire [5:0] color_on;
    wire [5:0] color_off;    


    /*ui_in
    0: 74880Hz 1-bit PCM(0) / 9360Hz 8-bit PWM(1)
    1: Tiny VGA(0) / VGA clock PMOD(1)
    2: color_off(0) - R
    3: color_off(0) - G
    4: color_off(0) - B
    5: color_on(1)  - R
    6: color_on(1)  - G
    7: color_on(1)  - B

    // https://github.com/mole99/tiny-vga
    // https://github.com/TinyTapeout/tt-vga-clock-pmod
    */
    assign color_on[5:0]  = {ui_in[5], ui_in[5], ui_in[6], ui_in[6], ui_in[7], ui_in[7]}; 
    assign color_off[5:0] = {ui_in[2], ui_in[2], ui_in[3], ui_in[3], ui_in[4], ui_in[4]}; 
    assign uo_out[0] = ui_in[1] ? hsync_q   :rrggbb[5];
    assign uo_out[1] = ui_in[1] ? vsync_q   :rrggbb[3];
    assign uo_out[2] = ui_in[1] ? rrggbb[0] :rrggbb[1];
    assign uo_out[3] = ui_in[1] ? rrggbb[1] :vsync_q;
    assign uo_out[4] = ui_in[1] ? rrggbb[2] :rrggbb[4];
    assign uo_out[5] = ui_in[1] ? rrggbb[3] :rrggbb[2];
    assign uo_out[6] = ui_in[1] ? rrggbb[4] :rrggbb[0];
    assign uo_out[7] = ui_in[1] ? rrggbb[5] :hsync_q;
    
    assign uio_out = ui_in[0] ? {spi_sel,spi_clk,spi_uio_out[3:0],sound_p_pwm,sound_n_pwm}: {spi_sel,spi_clk,spi_uio_out[3:0],sound_p_pcm,sound_n_pcm};
    assign uio_oe = rst_n ? {2'b11,spi_oe[3:0] ,2'b11} : 8'b11000000;  

    //PWM modulation: LTC2644
    //W25Q128JVSSIQ address: FFFFFF https://www.adafruit.com/product/5634
    wire reset = ~rst_n;

    reg [127:0] cache_row; //10Byte for line / 3x2 byte for sound
    reg [79:0] data_row;
    reg cache_done;
    reg data_done;

    wire [9:0] x_px;          // X position for actual pixel.
    wire [9:0] y_px;          // Y position for actual pixel.
    wire [9:0] vc;          // vertical counter (0 ~480+40-1)
    //wire [9:0] hc;          // horizontal counter (0 ~ 640+192-1) (52*16-1)
    // blocks are 16 x 16 px. (40 x 30 -> 640 x 480)
    /* verilator lint_off WIDTH */
    wire [6:0] x_block = (x_px) >> 3;
    wire [6:0] v_block = (vc) >> 3;
    /* verilator lint_on WIDTH */
    wire activevideo;    
    reg [1:0]frame_iter; //24fps - show same frame for three times
    reg [15:0] n_frame;
    //reg [6:0] x_block_q; //to match up signals using x_block_q
    reg [6:0] v_block_q;
    reg hsync_q;
    reg vsync_q;
    reg activevideo_q;
    //reg draw;
    reg overflow;

    wire px_clk;
    wire spi_clk;
    wire [7:0] read_cmd = 8'b01101011; //Fast Read Quad Output (6Bh, MSB -> LSB, read at rising clk of spi_clk. = falling clk of px_clk)
    wire spi_sel;
    assign px_clk = clk;
    assign spi_clk = ~clk && spi_clk_on;
    assign spi_sel = ~spi_sel_on;   
    reg  [3:0] spi_oe;
    reg spi_sel_on;
    reg spi_clk_on;
    reg spi_cmd_on;
    reg spi_addr_on;
    reg spi_dummy_on;
    reg spi_read_on;
    reg [3:0] spi_cmd_n;
    reg [6:0] spi_addr_n;
    reg [3:0] spi_dummy_n;
    reg [5:0] spi_read_n;
    reg [23:0] addr; // this addr tells byte address, not bit address! keep in mind that

    //sound
    reg  [15:0] sound_reg;
    //PCM
    reg  [3:0] sound_block_pcm; //+1 (x_px = 223 / 639 (+1 each 8*52 clocks))/ resets at next data
    reg        pcm_0_aux; //auxilliary register for pcm sound, used at sound_block updating clock
    reg sound_p_pcm;
    reg sound_n_pcm;
    //PWM
    reg [7:0] counter_pwm; //+1 per each clk
    reg [7:0] threshold_pwm;
    reg sound_p_pwm;
    reg sound_n_pwm;

    VgaSyncGen vga_0 (.data_done(data_done), .px_clk(px_clk), .hsync(hsync), .vsync(vsync), .x_px(x_px), .y_px(y_px),.vc(vc), .activevideo(activevideo), .reset(reset));
    assign rrggbb = activevideo_q &&  data_row[79 - x_block] ? color_on : color_off; 
 
    always @(posedge px_clk) begin
        //$display("x_px %d %b | y_px %d %b | hsync %d | vsync %d | activevideo %d | cache_done %d | data_done %d", x_px, x_px, y_px, y_px, hsync, vsync, activevideo, cache_done, data_done);
        if(reset || overflow == 1) begin

            cache_row <= 0;
            data_row <= 0;
            cache_done <= 0;
            data_done <= 0;

            frame_iter <= 0;
            n_frame <= 0;
            //x_block_q <= 0;
            v_block_q <= 0;
            hsync_q <= 1;
            vsync_q <= 1;
            activevideo_q <= 0;
            //draw <= 0;
            overflow <= 0;

            spi_oe <= 0;
            spi_sel_on <= 0;
            spi_clk_on <= 0;
            spi_cmd_on <= 0;
            spi_addr_on <= 0;
            spi_dummy_on <= 0;
            spi_read_on <= 0;
            spi_cmd_n <= 0;
            spi_addr_n <= 0;
            spi_dummy_n <= 0;
            spi_read_n <= 0;
            addr <= 0;

            sound_reg <= 0;
            sound_block_pcm <= 0;
            pcm_0_aux <= 0;
            sound_p_pcm <=0;
            sound_n_pcm <= 1;    
            counter_pwm <= 0; //+1 per each clk
            threshold_pwm <= 0;
            sound_p_pwm <=0;
            sound_n_pwm <= 1; 
			spi_uio_out <= 0;
        end


        //SPI flash module
        if (cache_done == 0) begin

            if (spi_sel_on == 0) begin
                spi_sel_on <= 1'b1;
                
            end else begin
                if (spi_clk_on == 0 && spi_cmd_on == 0 && spi_addr_on == 0 && spi_dummy_on == 0 && spi_read_on == 0 ) begin
                    //$display("x_px %d %b | y_px %d %b | hsync %d | vsync %d | activevideo %d | cache_done %d | data_done %d", x_px, x_px, y_px, y_px, hsync, vsync, activevideo, cache_done, data_done);

                    //spi_clk_on <= 1'b1;
                    spi_oe[0] <= 1'b1;
                    spi_cmd_on <= 1'b1;
                    spi_cmd_n <= 0;
                    cache_row <= 0;

                end else if (spi_cmd_on == 1 && spi_addr_on == 0 && spi_dummy_on == 0 && spi_read_on == 0 ) begin
                    spi_clk_on <= 1'b1; //cmd and clk should work simultaneously
                    
                    spi_uio_out[0] <= read_cmd[7-spi_cmd_n];
                    spi_uio_out[1] <= 0;
                    spi_uio_out[2] <= 0;
                    spi_uio_out[3] <= 0;
                    //$display("cmd is %d | %b", spi_cmd_n, read_cmd[7-spi_cmd_n]); //01101011. works fine         //read_cmd = 8'b01101011
                    spi_cmd_n <= spi_cmd_n + 1;

                    if (spi_cmd_n == 7) begin
                        spi_cmd_on <= 0;
                        spi_addr_on <= 1;
                        spi_addr_n <= 0;
                        if (frame_iter <2 && v_block_q == 64) begin 
                            //bit_addr <= n_frame * 128*65 + v_block_q*128 + data_done*128 - 8320; 
                            addr <= n_frame * 16*65 + v_block_q*16 + data_done*16 - 1040;
                        end else begin 
                            //bit_addr <= n_frame * 128*65 + v_block_q*128 + data_done*128 ;
                            addr <= n_frame * 16*65 + v_block_q*16 + data_done*16;
                        end
                        //$display("n_frame %d | frame_iter %d | y_px %d %b | v_block_q %d %b | addr%d",n_frame, frame_iter, y_px, y_px, v_block_q, v_block_q, addr); //01101011. works fine         //read_cmd = 8'b01101011
                    end
                end else if (spi_clk_on == 1 && spi_cmd_on == 0 && spi_addr_on == 1 && spi_dummy_on == 0 && spi_read_on == 0 ) begin
                    spi_uio_out[0] <= addr[23-spi_addr_n];
                    spi_uio_out[1] <= 0;
                    spi_uio_out[2] <= 0;
                    spi_uio_out[3] <= 0;
                    //$display("addr is %d | %d | %b | %b", spi_addr_n, addr, addr, addr[23-spi_addr_n]); //01101011. works fine         //read_cmd = 8'b01101011
                    spi_addr_n <= spi_addr_n + 1;

                    if (spi_addr_n == 23) begin
                        //$display("addr is %d | %d | %b | %b", spi_addr_n, addr, addr, addr[23-spi_addr_n]); //01101011. works fine         //read_cmd = 8'b01101011

                        spi_addr_on <= 0;
                        spi_dummy_on <= 1;
                        spi_dummy_n <= 0;
                    end
                end else if (spi_clk_on == 1 && spi_cmd_on == 0 && spi_addr_on == 0 && spi_dummy_on == 1 && spi_read_on == 0 ) begin
                    spi_oe[0] <= 1'b0;

                    //$display("dummy_n is %d ", spi_dummy_n); //01101011. works fine         //read_cmd = 8'b01101011
                    spi_dummy_n <= spi_dummy_n + 1;
                    //$display("veri dummy_n %d", spi_dummy_n);

                    if (spi_dummy_n == 7) begin
                        spi_dummy_on <= 0;
                        spi_read_on <= 1;
                        spi_read_n <= 0;
                        //$display("veri dummy_n last");
                    end
                end else if (spi_clk_on == 1 && spi_cmd_on == 0 && spi_addr_on == 0 && spi_dummy_on == 0 && spi_read_on == 1 ) begin
                    //1 clk delay in uio_in
                    //$display("read_n %d %b", spi_read_n,uio_in[5:2]);
                    case (spi_read_n)
                          1: begin
                            cache_row[127:124] <= uio_in[5:2];
                        end
                          2: begin
                            cache_row[123:120] <= uio_in[5:2];
                        end
                          3: begin
                            cache_row[119:116] <= uio_in[5:2];
                        end
                          4: begin
                            cache_row[115:112] <= uio_in[5:2];
                        end
                          5: begin
                            cache_row[111:108] <= uio_in[5:2];
                        end

                          6: begin
                            cache_row[107:104] <= uio_in[5:2];
                        end
                          7: begin
                            cache_row[103:100] <= uio_in[5:2];
                        end
                          8: begin
                            cache_row[99:96] <= uio_in[5:2];
                        end
                          9: begin
                            cache_row[95:92] <= uio_in[5:2];
                        end
                         10: begin
                            cache_row[91:88] <= uio_in[5:2];
                        end

                         11: begin
                            cache_row[87:84] <= uio_in[5:2];
                        end
                         12: begin
                            cache_row[83:80] <= uio_in[5:2];
                        end
                         13: begin
                            cache_row[79:76] <= uio_in[5:2];
                        end
                         14: begin
                            cache_row[75:72] <= uio_in[5:2];
                        end
                         15: begin
                            cache_row[71:68] <= uio_in[5:2];
                        end

                         16: begin
                            cache_row[67:64] <= uio_in[5:2];
                        end
                         17: begin
                            cache_row[63:60] <= uio_in[5:2];
                        end
                         18: begin
                            cache_row[59:56] <= uio_in[5:2];
                        end
                         19: begin
                            cache_row[55:52] <= uio_in[5:2];
                        end
                         20: begin
                            cache_row[51:48] <= uio_in[5:2];
                        end

                         21: begin
                            cache_row[47:44] <= uio_in[5:2];
                        end
                         22: begin
                            cache_row[43:40] <= uio_in[5:2];
                        end
                         23: begin
                            cache_row[39:36] <= uio_in[5:2];
                        end
                         24: begin
                            cache_row[35:32] <= uio_in[5:2];
                        end
                         25: begin
                            cache_row[31:28] <= uio_in[5:2];
                        end

                         26: begin
                            cache_row[27:24] <= uio_in[5:2];
                        end
                         27: begin
                            cache_row[23:20] <= uio_in[5:2];
                        end
                         28: begin
                            cache_row[19:16] <= uio_in[5:2];
                        end
                         29: begin
                            cache_row[15:12] <= uio_in[5:2];
                        end
                         30: begin
                            cache_row[11: 8] <= uio_in[5:2];
                        end
                         31: begin
                            cache_row[7 : 4] <= uio_in[5:2];
                        end
                         32: begin
                            cache_row[3 : 0] <= uio_in[5:2];
                            spi_read_on <= 0;
                            cache_done <= 1;
                            //$display("read end cache_row  : %b", cache_row);
                            //$display("read end data_row   : %b\n", data_row);
                        end

                    default begin 

                    end                    
                    endcase
                    
                    
                    //$display("read_n is %d ", spi_read_n); //01101011. works fine         //read_cmd = 8'b01101011
                    //$display("spi_uio_in: %b %b", uio_in, uio_in[5:2]);
                    //$display("cache_row: %b\n", cache_row);
                    spi_read_n <= spi_read_n + 1;
                    
                end
            end
        end else begin
            if (spi_clk_on == 1 && spi_cmd_on == 0 && spi_addr_on == 0 && spi_dummy_on == 0 && spi_read_on == 0) begin
                spi_clk_on <= 0;
                spi_sel_on <= 0;

                //$display("spi_sel_off");
            end
        end
        //SPI flash module end

        //happens only once, at the first time
        if(data_done == 1'b0 && cache_done == 1'b1) begin
            data_done <= 1'b1;
            cache_done <= 1'b0;
            data_row[79:0] <= cache_row[79:0];
        end

        //cache to data
        //x_block_q <= x_block; //is this correct? using reg here
        v_block_q <= v_block;
        hsync_q <= hsync;
        vsync_q <= vsync;
        activevideo_q <= activevideo;  
        //$display("  n_frame %d frame_iter %d x_px %d y_px %d sound_reg %b sound_block_pcm %d sound_reg[15-sound_block_pcm] %b sound_p_pcm %b", n_frame, frame_iter, x_px, y_px, sound_reg, sound_block_pcm, sound_reg, sound_p_pcm);     
        //$display("data_row: %b", data_row);        
       
        if (x_px == 639 && y_px%8 == 7 && cache_done == 1'b1) begin //right before the sound_reg update, used for pcm
            case (frame_iter)
                0: begin
                    pcm_0_aux <= cache_row[127];
                end
                1: begin
                    pcm_0_aux <= cache_row[111];
                end
                2: begin
                    pcm_0_aux <=  cache_row[95];
                end
            endcase
            //$display("  n_frame %d frame_iter %d x_px %d y_px %d sound_reg %b sound_block_pcm %d sound_reg[15-sound_block_pcm] %b sound_p_pcm %b", n_frame, frame_iter, x_px, y_px, sound_reg, sound_block_pcm, sound_reg, sound_p_pcm);     
    
        end
        //update sound and video data        
        if (x_px == 832 && y_px%8 == 0 && cache_done == 1'b1) begin
            cache_done <= 1'b0;
            data_row[79:0] <= cache_row[79:0]; // video update
            sound_block_pcm <= 0;
            //sound_block_pwm <= 0;
            case (frame_iter)
                0: begin
                    sound_reg[15:0] <= cache_row[127:112];
                    threshold_pwm[7:0] <= cache_row[127:120]; 
                end
                1: begin
                    sound_reg[15:0] <= cache_row[111:96];
                    threshold_pwm[7:0] <= cache_row[111:104]; 
                end
                2: begin
                    sound_reg[15:0] <= cache_row[95:80];
                    threshold_pwm[7:0] <= cache_row[95:88]; 
                end
            endcase
            sound_p_pcm <= pcm_0_aux; 
            sound_n_pcm <= ~pcm_0_aux;             
            //$display("x_px %d %b | y_px %d %b | hsync %d | vsync %d | activevideo %d | cache_done %d | data_done %d", x_px, x_px, y_px, y_px, hsync, vsync, activevideo, cache_done, data_done);
            //$display("cache->data"); //01101011. works fine         //read_cmd = 8'b01101011
            //$display("cache2data cache_row: %b", cache_row);
            //$display("cache2data data_row:  %b\n", data_row);
            //$display("  n_frame %d frame_iter %d x_px %d y_px %d sound_reg %b sound_block_pcm %d sound_reg[15-sound_block_pcm] %b sound_p_pcm %b", n_frame, frame_iter, x_px, y_px, sound_reg, sound_block_pcm, sound_reg, sound_p_pcm);     

        end else begin
            sound_p_pcm <= sound_reg[15-sound_block_pcm]; 
            sound_n_pcm <= ~sound_reg[15-sound_block_pcm]; 
        end

        if (x_px == 832 && y_px%8 == 4) begin
            threshold_pwm[7:0] <= sound_reg[7:0];
        end

        
        //PCM update
        if (x_px == 223) begin
            sound_block_pcm <= sound_block_pcm + 1;
        end
        if (x_px == 639) begin
            sound_block_pcm <= sound_block_pcm + 1;
        end


     
        //PWM update (y 4px / each 256 clk)
        counter_pwm <= counter_pwm + 1;
        if (counter_pwm[7:0] < threshold_pwm[7:0] ) begin
            sound_p_pwm <= 1;
            sound_n_pwm <= 0;
        end else begin
            sound_p_pwm <= 0;
            sound_n_pwm <= 1;
        end
        //update frame_iter
        //1. frame_iter_need_update becomes 1 at y_px = -40 
        // 480 - 1   /  -192 mod 1024
        if (y_px == 479 && x_px == 832 ) begin
            frame_iter <= frame_iter + 1'b1;
        end

        //72Hz -> 24 fps
        if(y_px == 479 && x_px == 833 && frame_iter == 2'b11) begin
            n_frame <= n_frame + 1'b1;
            frame_iter <= 2'b00;
        end

        if(y_px == 479 && x_px == 834 && n_frame == 16131) begin
            overflow <= 1; //(2^24 byte / 1040 byte per frame = 16131.9384615 -> overflow happens at 16131th frame)
        end
  
    end

endmodule

