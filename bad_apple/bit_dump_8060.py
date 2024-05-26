#!/usr/bin/env python3

import sys
import struct
from PIL import Image
#sound: 
# from scipy.io.wavfile import read
# a = read("adios.wav")
# numpy.array(a[1],dtype=float)
# array([ 128.,  128.,  128., ...,  128.,  128.,  128.])
#scipy.io.wavfile.write(filename, rate, data)

import os
import numpy as np
"""
os.system('ffmpeg -i badapple_sound.mp4 -ab 74880 -ac 1 -ar 74880 -vn badapple.wav')
from scipy.io.wavfile import read,write
wav_in = read("badapple.wav")
wav_arr = np.array(wav_in)[1] #[sample rate np.array(samples)]
wav_arr_out = np.zeros(wav_arr.shape)
for i in range(wav_arr.shape[0]):
    if wav_arr[i] < 0:
        wav_arr_out[i] = -32768
    else:
        wav_arr_out[i] = 32767
write('badapple_depth1.wav', 74880, wav_arr_out)       
#val_list = list(set(wav_arr)) #-32768 ~32767
#val_list.sort()
#print(val_list)

#print(wav_arr[1])

# array([ 128.,  128.,  128., ...,  128.,  128.,  128.])
"""

out_file = open("badapple80x60.bin", "wb")


bin_data = []
for i in range(1,6563):
    img = Image.open("frames/badapple%04d.png" % (i,)).resize((640,480))
    data = img.load()
    repeat_count = 0

    for y in range(0,480,8):
        for x in range(0,640,8):
            if data[x+4, y+4][0] > 100: 
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

