![](../../workflows/gds/badge.svg) ![](../../workflows/docs/badge.svg) ![](../../workflows/test/badge.svg) ![](../../workflows/fpga/badge.svg)

# Tiny Tapeout Verilog Project Template

- [Read the documentation for project](docs/info.md)

## How it works
This project plays binary-colored 80px x 60px @ 24fps video recorded in SPI NOR flash, playing with 640px x 480px @72Hz VGA.
Additionally, the project plays PCM or PWM Audio recorded in same flash chip. 
The input chooses type of audio(PCM/PWM), type of VGA PMOD, and the color of the video.
Also, uio[7:2] is used for SPI communication and uio[1:0] is used for audio output.
Finally, output is used as video output.
 
## How to test

Hook up a VGA monitor to the outputs and provide a clock at 31.5 MHz. 
choose the type of audio output with input[0], and choose the type of VGA PMOD with the input[1].
color of pixels which turned off (which data of the pixel is 0) is selected with inputs[4:2] (2:R, 3:G, 4:B), and 
color of pixels which turned on (which data of the pixel is 1) is selected with inputs[7:5] (5:R, 6:G, 7:B).

Data structure of SPI flash chip:
1. Data address starts with 0x000000.
2. Each frame takes 65 x 128 bit, where each 128 bits are used for video/audio data for each line(640px x 8px).
2.1. Since the video uses 24fps, while the VGA uses 72Hz, each frame is shown three times in the VGA.
2.2. Thus, each line(=128 bits) are uses as following way:
     line[127:0] = {audio_0[15:0], audio_1[15:0], audio_2[15:0], video[79:0]},
     where audio_i is audio data for the line in the (i+1)th iteration.
2.3 The first 5 lines in the frame are used for porch and vsync, which means video data in the line is ignored.
    However, audio data in the line still valid. Also, this is why each frame uses 65 x 128 bit and not 60 x 128 bit.
3. Due to limitation of data, maximum amount of 16131 frames will be supported. reaching 16132th frame will restart the project. (check overflow in tt_vga_player.v)
For more infomation, check bit_dump_8060_wav.py in bad_apple folder.

## External hardware
Audio - For PCM, using piezo on uio[1:0] would work. For PWM, external DAC like LTC2644 chip is needed (not tested though)
Set input[0] low to use 74880Hz 1-bit PCM mode and high to 9360Hz 8-bit PWM mode.

VGA PMOD - you can use one of these VGA PMODs:

* https://github.com/mole99/tiny-vga
* https://github.com/TinyTapeout/tt-vga-clock-pmod
Set input[1] low to use tiny-vga and high to use vga-clock

SPI flash (W25Q128JVSSIQ)
* https://www.adafruit.com/product/5634


## What is Tiny Tapeout?

Tiny Tapeout is an educational project that aims to make it easier and cheaper than ever to get your digital and analog designs manufactured on a real chip.

To learn more and get started, visit https://tinytapeout.com.

## Set up your Verilog project

1. Add your Verilog files to the `src` folder.
2. Edit the [info.yaml](info.yaml) and update information about your project, paying special attention to the `source_files` and `top_module` properties. If you are upgrading an existing Tiny Tapeout project, check out our [online info.yaml migration tool](https://tinytapeout.github.io/tt-yaml-upgrade-tool/).
3. Edit [docs/info.md](docs/info.md) and add a description of your project.
4. Adapt the testbench to your design. See [test/README.md](test/README.md) for more information.

The GitHub action will automatically build the ASIC files using [OpenLane](https://www.zerotoasiccourse.com/terminology/openlane/).

## Enable GitHub actions to build the results page

- [Enabling GitHub Pages](https://tinytapeout.com/faq/#my-github-action-is-failing-on-the-pages-part)

## Resources

- [FAQ](https://tinytapeout.com/faq/)
- [Digital design lessons](https://tinytapeout.com/digital_design/)
- [Learn how semiconductors work](https://tinytapeout.com/siliwiz/)
- [Join the community](https://tinytapeout.com/discord)
- [Build your design locally](https://docs.google.com/document/d/1aUUZ1jthRpg4QURIIyzlOaPWlmQzr-jBn3wZipVUPt4)

## What next?

- [Submit your design to the next shuttle](https://app.tinytapeout.com/).
- Edit [this README](README.md) and explain your design, how it works, and how to test it.
- Share your project on your social network of choice:
  - LinkedIn [#tinytapeout](https://www.linkedin.com/search/results/content/?keywords=%23tinytapeout) [@TinyTapeout](https://www.linkedin.com/company/100708654/)
  - Mastodon [#tinytapeout](https://chaos.social/tags/tinytapeout) [@matthewvenn](https://chaos.social/@matthewvenn)
  - X (formerly Twitter) [#tinytapeout](https://twitter.com/hashtag/tinytapeout) [@matthewvenn](https://twitter.com/matthewvenn)
