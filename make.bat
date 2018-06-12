zcc +zx -v -W -Wall -O0 -Ddeunavez -zorg=32768 -o chip8.bin chip8.c -lndos

del chip8.tap

appmake +zx -b chip8.bin -o chip8.tap --org 32768
tapmaker roms/MERLIN chip8.tap
tapmaker roms/BLINKY chip8.tap	
tapmaker roms/15PUZZLE chip8.tap
tapmaker roms/AIRPLANE chip8.tap
tapmaker roms/BLINKY chip8.tap
tapmaker roms/BLITZ chip8.tap
tapmaker roms/BREAKOUT chip8.tap
tapmaker roms/BRIX chip8.tap
tapmaker roms/C8PIC chip8.tap
tapmaker roms/CAVE chip8.tap
tapmaker roms/CONNECT4 chip8.tap
tapmaker roms/GUESS chip8.tap
tapmaker roms/HIDDEN chip8.tap
tapmaker roms/IBM chip8.tap
tapmaker roms/INVADERS chip8.tap
tapmaker roms/KALEID chip8.tap
tapmaker roms/MAZE chip8.tap
tapmaker roms/MERLIN chip8.tap
tapmaker roms/MISSILE chip8.tap
tapmaker roms/PONG chip8.tap
tapmaker roms/PONG2 chip8.tap
tapmaker roms/PONG3 chip8.tap
tapmaker roms/PUZZLE chip8.tap
tapmaker roms/PUZZLE2 chip8.tap
tapmaker roms/SYZYGY chip8.tap
tapmaker roms/TANK chip8.tap
tapmaker roms/TETRIS chip8.tap
tapmaker roms/TICTAC chip8.tap
tapmaker roms/UFO chip8.tap
tapmaker roms/VBRIX chip8.tap
tapmaker roms/VERS chip8.tap
tapmaker roms/WIPEOFF chip8.tap

del *.bin
del *.opt
del *.err
del zcc_opt.def