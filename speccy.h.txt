/* Emulador de Chip-8 para ZX Spectrum    *
 * Desarrollado por Alvaro Alea Fernandez *
 * Distribuido con licencia GPL V2        */

#ifndef __SPECCY_H
#define __SPECCY_H

#define INSTRPERFRAME 20

void updatedisplay(u8_t x,u8_t y, u8_t w, u8_t h);
void updatedisplaymini(u8_t x,u8_t y, u8_t w, u8_t h);
u8_t mygetk2(void);
u8_t mygetk(void);
u8_t mygetkey(void);
u8_t iskeypressed(u8_t key);
u8_t waitanykey(void);
u8_t menu(void);
void loadrom(void);
void pita (void);
void initsystem(void);
u8_t printstate(u8_t);	
void redefineteclas(void);	
void border(u8_t n);


#endif //ifndef __SPECCY_H
