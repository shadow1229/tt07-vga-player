#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>

#include <SDL2/SDL.h>
#include <fstream>
#include <array>


#include "Vvga_clock.h"
#include "verilated.h"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

std::vector<std::byte> readFileData(const std::string& name) {
    std::ifstream inputFile(name, std::ios_base::binary);

    // Determine the length of the file by seeking
    // to the end of the file, reading the value of the
    // position indicator, and then seeking back to the beginning.
    inputFile.seekg(0, std::ios_base::end);
    auto length = inputFile.tellg();
    inputFile.seekg(0, std::ios_base::beg);

    // Make a buffer of the exact size of the file and read the data into it.
    std::vector<std::byte> buffer(length);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), length);

    inputFile.close();
    return buffer;
}

std::vector<int> build_mem()
{
    std::string fname;
    int size, i,j,k, buf_size;
    //std::vector<std::byte> mem(134217728);
    std::vector<int> mem(134217728);
    fname = "badapple80x60_wav.bin";
    std::vector<std::byte> buffer = readFileData(fname);

    buf_size = buffer.size();
    std::vector<std::byte>::iterator itor = buffer.begin();

    for (i=0; i < 134217728; i++) {
        //mem[i] = std::byte(0);
        mem[i] = 0;
    }

    for (i=0; i < buf_size; i++) {
        mem[8*i]  = (int(buffer[i]) & 128)>>7;
        mem[8*i+1] = (int(buffer[i]) & 64)>>6;
        mem[8*i+2] = (int(buffer[i]) & 32)>>5;
        mem[8*i+3] = (int(buffer[i]) & 16)>>4;
        mem[8*i+4] = (int(buffer[i]) & 8 )>>3;
        mem[8*i+5] = (int(buffer[i]) & 4 )>>2;
        mem[8*i+6] = (int(buffer[i]) & 2 )>>1;
        mem[8*i+7] = (int(buffer[i]) & 1 )>>0;
        //std::cout << int(*itor)<< std::endl;        //output : 1 2 3 4
        //std::cout << size <<"|"<< i <<"|"<< int(buffer[i])<< std::endl; 
    }
    /*
    for (i=0; i < 6500; i++) {
        std::cout << "i_frame" << i << std::endl;        //output : 1 2 3 4

        for (j=0; j < 30; j++){
            for (k=0; k < 40; k++){
                std::cout << int(mem[i*1200+j*40 + k]) << " ";
            }
            std::cout << std::endl; 

        }
        //std::cout << int(*itor)<< std::endl;        //output : 1 2 3 4
        //std::cout << size <<"|"<< i <<"|"<< int(buffer[i])<< std::endl; 
    }
    */
    return mem;
}
int main(int argc, char **argv) {

    //build QSPI memory
	std::vector<int> memory;
    memory =  build_mem();
    // QSPI init start
    //read at rising edge 
    std::array<int, 8>  inst           = {0,1,1,0,1,0,1,1};
    std::array<int, 8>  input_inst     = {0,0,0,0,0,0,0,0 };
    std::array<int, 24> input_addr     = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
    int time, i,j,k,l,cs, spi_clk_prev, spi_clk, clk_on, n_inst, n_addr, n_dummy, addr, data_0, data_1, data_2, data_3, is_same;
    int inst_on, addr_on, dummy_on, data_on;
    time = 0;
    cs = 1;
    //clk_on = 0;
    spi_clk_prev = 0;
    spi_clk = 0;
    
    inst_on = 1;
    addr_on = 0;
    dummy_on = 0;
    data_on = 0;

    n_inst = 0;
    n_addr = 0;
    n_dummy = 0;
    
    addr = 0;
    data_0 = 0;
    data_1 = 0;
    data_2 = 0;
    data_3 = 0;

    is_same = 0;

    // QSPI init end
	std::vector< uint8_t > framebuffer(WINDOW_WIDTH * WINDOW_HEIGHT * 4, 0);

	Verilated::commandArgs(argc, argv);

	Vvga_clock *top = new Vvga_clock;

	// perform a reset
	top->clk = 0;
	top->eval();

    cs = ((top->uio_out& 0b10000000) >>7);

	spi_clk_prev = spi_clk;
    spi_clk = ((top->uio_out& 0b01000000) >>6);

    data_3 = ((top->uio_out& 0b00100000) >>5);
    data_2 = ((top->uio_out& 0b00010000) >>4);
    data_1 = ((top->uio_out& 0b00001000) >>3);
    data_0 = ((top->uio_out& 0b00000100) >>2);

	//std::cout << "firstclk0 cs spi_clk uio_out 3 2 1 0 | "<<cs <<spi_clk<<data_3<<data_2<<data_1<<data_0<<" "<< int(top->uio_out) <<" "<<std::endl; 
	top->rst_n = 0;
	top->clk = 1;
	top->eval();
    cs = ((top->uio_out& 0b10000000) >>7);

	spi_clk_prev = spi_clk;
    spi_clk = ((top->uio_out& 0b01000000) >>6);

    data_3 = ((top->uio_out& 0b00100000) >>5);
    data_2 = ((top->uio_out& 0b00010000) >>4);
    data_1 = ((top->uio_out& 0b00001000) >>3);
    data_0 = ((top->uio_out& 0b00000100) >>2);

	//std::cout << "firstclk1 cs spi_clk uio_out 3 2 1 0 | "<<cs <<spi_clk<<data_3<<data_2<<data_1<<data_0<<" "<< int(top->uio_out) <<" "<<std::endl; 

	top->rst_n = 1;

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window =
	    SDL_CreateWindow(
	        "Framebuffer Verilator",
	        SDL_WINDOWPOS_UNDEFINED,
	        SDL_WINDOWPOS_UNDEFINED,
	        WINDOW_WIDTH,
	        WINDOW_HEIGHT,
	        0
	    );

	SDL_Renderer* renderer =
	    SDL_CreateRenderer(
	        window,
	        -1,
	        SDL_RENDERER_ACCELERATED
	    );

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	SDL_Event e;

	SDL_Texture* texture =
	    SDL_CreateTexture(
	        renderer,
	        SDL_PIXELFORMAT_ARGB8888,
	        SDL_TEXTUREACCESS_STREAMING,
	        WINDOW_WIDTH,
	        WINDOW_HEIGHT
	    );

	bool quit = false;

	int hnum = 0;
	int vnum = 0;

	while (true) {

		while (SDL_PollEvent(&e) == 1) {
			if (e.type == SDL_QUIT) {
				quit = true;
			} else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
				case SDLK_q:
					quit = true;
				default:
					break;
				}
			}
		}

		auto keystate = SDL_GetKeyboardState(NULL);

		top->rst_n = !keystate[SDL_SCANCODE_ESCAPE];

		//top->adj_hrs = keystate[SDL_SCANCODE_H];
		//top->adj_min = keystate[SDL_SCANCODE_M];
		//top->adj_sec = keystate[SDL_SCANCODE_S];





		// simulate for 20000 clocks
		for (time = 0; time < 20000; ++time) {

			top->clk = 0;
			top->eval();
            //std::cout << "clk 0 eval end"  << " " << time <<std::endl;
    
	        //assign uio_out = {spi_sel,spi_clk,spi_uio[3:0],1'b0,1'b0};
            //assign uio_oe = rst_n ? {2'b11,spi_oe[3:0] ,2'b00} : 8'b11000000;  

            cs = ((top->uio_out& 0b10000000) >>7);

			spi_clk_prev = spi_clk;
            spi_clk = ((top->uio_out& 0b01000000) >>6);

            data_3 = ((top->uio_out& 0b00100000) >>5);
            data_2 = ((top->uio_out& 0b00010000) >>4);
            data_1 = ((top->uio_out& 0b00001000) >>3);
            data_0 = ((top->uio_out& 0b00000100) >>2);

			//std::cout << time <<" clk:0 cs spi_clk uio_out 3 2 1 0 | "<<cs <<spi_clk<<data_3<<data_2<<data_1<<data_0<<" "<< int(top->uio_out) <<" "<<std::endl; 
			//std::cout << "uiooe cs spi_clk uio_out 3 2 1 0 | "<<((top->uio_oe& 128) >>7)<<((top->uio_oe& 64) >>6)<<((top->uio_oe& 32) >>5)<<((top->uio_oe& 16) >>4)<<((top->uio_oe& 8) >>3)<<((top->uio_oe& 4) >>2)<<((top->uio_oe& 2) >>1)<<((top->uio_oe& 1) >>0)<<std::endl ; 
			//std::cout << "spi_uio 3 2 1 0 | "<<((top->spi_uio& 8) >>3)<<((top->spi_uio& 4) >>2)<<((top->spi_uio& 2) >>1)<<((top->spi_uio& 1) >>0)<<" "<<top->spi_uio<<" "std::endl ; 

            if (cs ==0 && spi_clk_prev == 0 && spi_clk == 1) {
                //std::cout << "clk:0 / cs ==0 && spi_clk_prev == 0 && spi_clk == 1" <<std::endl;
                //std::cout <<"rising edge" << std::endl;
                if (inst_on == 1 && addr_on == 0 && dummy_on == 0 && data_on == 0 ) {
                    //std::cout << "n_inst " <<n_inst<<" inst "<<data_0<<std::endl;
                    input_inst[n_inst] = data_0; //change this to io0
                    n_inst += 1;
                    if (n_inst == 8) {
                        
                        is_same = 1;
                        n_inst  = 0;

                        for (i=0; i < 8; i++){
                            if (inst[i] != input_inst[i]) {
                                is_same = 0;
                            }
                        }

                        if (is_same == 1) {
                            inst_on = 0;
                            addr_on = 1;

                        }
                        
                        //check instruction correct
                        //std::cout << "inst " ;
                        for (i=0; i < 8; i++){
                            //std::cout << inst[i] << " ";
                        }
                        //std::cout << "input_inst " ;
                        for (i=0; i < 8; i++){
                            //std::cout << input_inst[i] << " ";
                        }
                        //std::cout << "is_same: " << is_same<<  std::endl;
                        is_same = 0;
                    }
                } else if (inst_on == 0 && addr_on == 1 && dummy_on == 0 && data_on == 0 ) { 
                    input_addr[n_addr] = data_0; //change this to io0
                    n_addr += 1;
                    if (n_addr == 24) {
                        //std::cout << "addr arr: "; 
                        for (i=0; i < 24; i++){
                            addr += input_addr[i] << (23-i);
                            //std::cout << input_addr[i]; 
                        }   
                        //std::cout << std::endl;
                        //std::cout << "addr: " << addr <<  std::endl;
                        addr *= 8; //byte address to bit address
                        n_addr = 0;
                        addr_on = 0;
                        dummy_on = 1;

                        //std::cout << time <<" mem_answer:           "; 
                        //for (i=0; i < 128; i++){
                        //    std::cout << memory[addr+i]; 
                        //}   
                        //std::cout << std::endl;
                    }
                } else if (inst_on == 0 && addr_on == 0 && dummy_on == 1 && data_on == 0 ) { 
                    //std::cout << time <<" cpp n_dummy: "<< n_dummy <<std::endl;

                    n_dummy += 1;
                    if (n_dummy == 8) {
                        //std::cout << time <<" cpp n_dummy_end "<< std::endl; 
                        n_dummy = 0;
                        dummy_on = 0;
                        data_on = 1;
                    }
                } else if (inst_on == 0 && addr_on == 0 && dummy_on == 0 && data_on == 1 ) { 
                    //memory[addr+0] -> IO3
                    //memory[addr+1] -> IO2
                    //memory[addr+2] -> IO1
                    //memory[addr+3] -> IO0
                    //std::cout << time <<" addr: "<< addr << " data 0 1 2 3 : " <<  memory[addr] << memory[addr+1] <<memory[addr+2] <<memory[addr+3] <<std::endl;
                    //===============================================
            		//cs = ((top->uio_out& 0b10000000) >>7);
					//spi_clk_prev = spi_clk;
            		//spi_clk = ((top->uio_out& 0b01000000) >>6);
            		//data_3 = ((top->uio_out& 0b01000000) >>5);
            		//data_2 = ((top->uio_out& 0b00100000) >>4);
            		//data_1 = ((top->uio_out& 0b00010000) >>3);
            		//data_0 = ((top->uio_out& 0b00001000) >>2);					
					
					top->uio_in = 128*cs + 64*spi_clk + 32*memory[addr+0] + 16*memory[addr+1] + 8*memory[addr+2] + 4*memory[addr+3];
					addr += 4;
                }
                
            } else if (cs ==1 && inst_on == 0 && addr_on == 0 && dummy_on == 0 && data_on == 1 ) {

                clk_on = 0;
                spi_clk_prev = 0;
                spi_clk = 0;
    
                inst_on = 1;
                addr_on = 0;
                dummy_on = 0;
                data_on = 0;

                n_inst = 0;
                n_addr = 0;
                n_dummy = 0;
    
                addr = 0;
                input_inst     = {0,0,0,0,0,0,0,0 };
                input_addr     = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
                //std::cout << "cs off - verilator" <<  std::endl;
            }

			top->clk = 1;
			top->eval();
            //std::cout << "clk 1 eval end"  << " " << time <<std::endl;

            cs = ((top->uio_out& 0b10000000) >>7);
			spi_clk_prev = spi_clk;
            spi_clk = ((top->uio_out& 0b01000000) >>6);
            data_3 = ((top->uio_out& 0b00100000) >>5);
            data_2 = ((top->uio_out& 0b00010000) >>4);
            data_1 = ((top->uio_out& 0b00001000) >>3);
            data_0 = ((top->uio_out& 0b00000100) >>2);
			//std::cout <<time << " clk:1 cs spi_clk uio_out 3 2 1 0 | "<<cs <<spi_clk<<data_3<<data_2<<data_1<<data_0<<" "<< int(top->uio_out) <<" "<<std::endl; 
			//std::cout << "uiooe cs spi_clk uio_out 3 2 1 0 | "<<((top->uio_oe& 128) >>7)<<((top->uio_oe& 64) >>6)<<((top->uio_oe& 32) >>5)<<((top->uio_oe& 16) >>4)<<((top->uio_oe& 8) >>3)<<((top->uio_oe& 4) >>2)<<((top->uio_oe& 2) >>1)<<((top->uio_oe& 1) >>0)<<std::endl ; 
			//std::cout << "spi_uio 3 2 1 0 | "<<((top->spi_uio& 8) >>3)<<((top->spi_uio& 4) >>2)<<((top->spi_uio& 2) >>1)<<((top->spi_uio& 1) >>0)<<" "<<top->spi_uio<<" "std::endl ; 


            if (cs ==0 && spi_clk_prev == 0 && spi_clk == 1) {
                //std::cout << "clk:1 / cs ==0 && spi_clk_prev == 0 && spi_clk == 1" <<std::endl;
                //std::cout <<"rising edge" << std::endl;
                if (inst_on == 1 && addr_on == 0 && dummy_on == 0 && data_on == 0 ) {
                    //std::cout << "n_inst " <<n_inst<<" inst "<<data_0<<std::endl;                    
                    input_inst[n_inst] = data_0; //change this to io0
                    n_inst += 1;
                    if (n_inst == 8) {
                        
                        is_same = 1;
                        n_inst  = 0;

                        for (i=0; i < 8; i++){
                            if (inst[i] != input_inst[i]) {
                                is_same = 0;
                            }
                        }

                        if (is_same == 1) {
                            inst_on = 0;
                            addr_on = 1;

                        }
                        
                        //check instruction correct
                        //std::cout << "inst " ;
                        for (i=0; i < 8; i++){
                            //std::cout << inst[i] << " ";
                        }
                        //std::cout << "input_inst " ;
                        for (i=0; i < 8; i++){
                            //std::cout << input_inst[i] << " ";
                        }
                        //std::cout << "is_same: " << is_same<<  std::endl;
                        is_same = 0;
                    }
                } else if (inst_on == 0 && addr_on == 1 && dummy_on == 0 && data_on == 0 ) { 
                    input_addr[n_addr] = data_0; //change this to io0
                    n_addr += 1;
                    if (n_addr == 24) {
                        //std::cout << "addr arr: "; 
                        for (i=0; i < 24; i++){
                            addr += input_addr[i] << (23-i);
                            //std::cout << input_addr[i]; 
                        }   
                        //std::cout << std::endl;
                        //std::cout << "addr: " << addr <<  std::endl;
                        addr *= 8; //byte address to bit address
                        n_addr = 0;
                        addr_on = 0;
                        dummy_on = 1;
                        //std::cout <<time << " mem_answer:           "; 
                        //for (i=0; i < 80; i++){
                        //    std::cout << memory[addr+i]; 
                        //}   
                        //std::cout << std::endl;
                    }
                } else if (inst_on == 0 && addr_on == 0 && dummy_on == 1 && data_on == 0 ) { 
                    //std::cout <<time << " n_dummy: "<< n_dummy <<std::endl;

                    n_dummy += 1;
                    if (n_dummy == 8) {
                        //std::cout <<time << " n_dummy_end "<< std::endl; 
                        n_dummy = 0;
                        dummy_on = 0;
                        data_on = 1;
                    }
                } else if (inst_on == 0 && addr_on == 0 && dummy_on == 0 && data_on == 1 ) { 
                    //memory[addr+0] -> IO3
                    //memory[addr+1] -> IO2
                    //memory[addr+2] -> IO1
                    //memory[addr+3] -> IO0
                    //std::cout << time <<" addr: "<< addr << " data 0 1 2 3 : " <<  memory[addr] << memory[addr+1] <<memory[addr+2] <<memory[addr+3] <<std::endl;
                    //===============================================
            		//cs = ((top->uio_out& 0b10000000) >>7);
					//spi_clk_prev = spi_clk;
            		//spi_clk = ((top->uio_out& 0b01000000) >>6);
            		//data_3 = ((top->uio_out& 0b01000000) >>5);
            		//data_2 = ((top->uio_out& 0b00100000) >>4);
            		//data_1 = ((top->uio_out& 0b00010000) >>3);
            		//data_0 = ((top->uio_out& 0b00001000) >>2);					
					
					top->uio_in = 128*cs + 64*spi_clk + 32*memory[addr+0] + 16*memory[addr+1] + 8*memory[addr+2] + 4*memory[addr+3];
					addr += 4;
                }
                
            } else if (cs ==1 && inst_on == 0 && addr_on == 0 && dummy_on == 0 && data_on == 1 ) {

                clk_on = 0;
                spi_clk_prev = 0;
                spi_clk = 0;
    
                inst_on = 1;
                addr_on = 0;
                dummy_on = 0;
                data_on = 0;

                n_inst = 0;
                n_addr = 0;
                n_dummy = 0;
    
                addr = 0;
                input_inst     = {0,0,0,0,0,0,0,0 };
                input_addr     = {0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0 };
                //std::cout << "cs off - verilator" <<  std::endl;

            }






            //std::cout << "framebuffer start" << " " << time << std::endl;

			// h and v blank logic
			if ((0 ==  ((top->uo_out & 0b10000000) >> 7)) && (0 == ((top->uo_out & 0b01000000) >> 6))) {
				hnum = -129; //-128 -1, since the value of hnum in the next clk become (-129 + 1) = -128 and ensure length of hbp
                //let's assume hbp is 1 
                //original method, hnum is -hbp :
                //hsync makes hnum to -1
                //in next clock, hnum will be -1+1 = 0, which means no hbp time! 
                //original method, hnum is -hbp -1:
                //hsync makes hnum to -2
                //in next clock, hnum will be -2+1 = -1, ensuring hbp clock 
				vnum = -29;
                //std::cout <<"vsync & hsync - verilator"<<std::endl;
			}

			// active frame
			//if ((hnum < 20) && (vnum < 20) && (hnum >= 0) && (vnum >= 0) ) {
			///X	framebuffer.at((vnum * WINDOW_WIDTH + hnum) * 4 + 0) = 0;
			//	framebuffer.at((vnum * WINDOW_WIDTH + hnum) * 4 + 1) = (0b11) << 6;
			//	framebuffer.at((vnum * WINDOW_WIDTH + hnum) * 4 + 2) = 0;
			//}
			//else 
            if ((hnum >= 0) && (hnum < 640) && (vnum >= 0) && (vnum < 480)) {
				framebuffer.at((vnum * WINDOW_WIDTH + hnum) * 4 + 0) = (top->uo_out & 0b00000011) >> 0 << 6;
				framebuffer.at((vnum * WINDOW_WIDTH + hnum) * 4 + 1) = (top->uo_out & 0b00001100) >> 2 << 6;
				framebuffer.at((vnum * WINDOW_WIDTH + hnum) * 4 + 2) = (top->uo_out & 0b00110000) >> 4 << 6;
			    //std::cout << "O hnum vnum colorval - verilator "<<hnum << " " <<vnum<<" "<< int((top->uo_out & 0b10000000) >> 7) <<" " <<int((top->uo_out & 0b01000000) >> 6)<<std::endl;

			} else{
			    //std::cout << "X hnum vnum colorval - verilator "<<hnum << " " <<vnum<<" "<< int((top->uo_out & 0b10000000) >> 7) <<" " <<int((top->uo_out & 0b01000000) >> 6)<<std::endl;


            }


			// keep track of encountered fields
			hnum++;
			if (hnum >= 640 + 24 + 40 -1) {
				hnum = -129;
				vnum++;
                //std::cout <<"hsync - verilator"<<std::endl;
			}
            //std::cout << "framebuffer end" << " " << time <<std::endl;

		}

		SDL_UpdateTexture(
		    texture,
		    NULL,
		    framebuffer.data(),
		    WINDOW_WIDTH * 4
		);

		SDL_RenderCopy(
		    renderer,
		    texture,
		    NULL,
		    NULL
		);

		SDL_RenderPresent(renderer);
	}

	top->final();
	delete top;

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}
