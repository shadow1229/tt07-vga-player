#!/usr/bin/env python3

import sys
import struct
from PIL import Image

out_file = open("badapple40x30.bin", "wb")


bin_data = []
for i in range(1,6563):
    img = Image.open("frames/badapple%04d.png" % (i,)).resize((640,480))
    data = img.load()
    repeat_count = 0

    for y in range(0,480,16):
        for x in range(0,640,16):
            if data[x+8, y+8][0] > 100: 
                colour =   1
            else: 
                colour = 0

            bin_data.append(colour)


######
len_data = len(bin_data)
for i in range(len_data//8):
    #print(bin_data[4*i], bin_data[4*i+1] , bin_data[4*i+2] , bin_data[4*i+3], (8*bin_data[4*i] + 4*bin_data[4*i+1] + 2*bin_data[4*i+2] + 1*bin_data[4*i+3]), bytes([8*bin_data[4*i] + 4*bin_data[4*i+1] + 2*bin_data[4*i+2] + 1*bin_data[4*i+3]]) )
    vals = [ str(bin_data[8*i + j]) for j in range(8)]
    vals_str = " ".join(vals)
    val = 0
    for j in range(8):
        val += bin_data[8*i + j]
        if j < 7:
            val *= 2
    
    #print(vals_str)  
    out_file.write(bytes([val]))