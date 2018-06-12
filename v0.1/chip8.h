/* Emulador de Chip-8 para ZX Spectrum    *
 * Desarrollado por Alvaro Alea Fernandez *
 * Distribuido con licencia GPL V2        */

#ifndef __CHIP8_H
#define __CHIP8_H

extern u8_t mem[0x1000];
extern u8_t reg[0x10];   
extern int  stack[0x10];  
extern u8_t sp,stimer,dtimer; 
extern int  pc,i;
extern u8_t display[64*32];
extern unsigned char key[17];
extern u8_t debug;

#define FONT_OFFSET 0x38

void cls(void);
int do_cpu(void);
void reset(void);
u8_t drawsprite(u8_t x, u8_t y, u8_t size);
int main(void);
void decode(u16_t);

#endif //ifndef __CHIP8_H
