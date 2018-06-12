/* Emulador de Chip-8 para ZX Spectrum    *
 * Desarrollado por Alvaro Alea Fernandez *
 * Distribuido con licencia GPL V2        */

/* Actualmente pido discrecion y no distribucion   *
 * de este codigo a cualquiera que lo vea          *
 * de igual manera pido que me cedan el (C) de     *
 * cualquier cambio, para evitar posible problemas *
 * con posteriores cambios de licencia             *
 * si no estas deacuerdo, no lo toques             */

/* La razon es sencilla: va a existir una version  *
 * comercial y otra GPL                            */

/* Este archivo contiene las Funcines que *
 * Dependen de la arquitectura            *
 * en este caso: Para Libreria Allegro    */


#include <allegro.h>

#define BIG_PIXEL

void updatedisplay(int x, int y, int w, int h)
{
	int c,d,x2,y2;
//	printf("actualizo x=%x ,y=%x de %x x %x\n",x,y,w,h);
	x2=(x+w-1) & 0x3F; // como los sprites son cicliclos, es posible
	if (x2<x)          // tener que dibujar 2 o 4 cuatro trocitos en
	{                  // la pantalla, si eso pasa, dibujo toda una
		x=0;w=64;  // tira que abarque los 2 trozos
	}		   // o la pantalla entera.	
	y2=(y+h-1) & 0x1F; 
	if (y2<y) 
	{
		y=0;h=32;
	}
		
	for (d=y;d<(y+h);d++)
		for (c=x;c<(x+w);c++)
		{
			if (display[d*64+c]==1)
			{
#ifndef BIG_PIXEL
	plot(c,d);
#else
				plot(c*2,d*2);
				plot(c*2+1,d*2);
				plot(c*2,d*2+1);
				plot(c*2+1,d*2+1);
#endif
			}
			else
			{
#ifndef BIG_PIXEL
				unplot(c,d);
#else
				unplot(c*2,d*2);
				unplot(c*2+1,d*2);
				unplot(c*2,d*2+1);
				unplot(c*2+1,d*2+1);
#endif
			}
		}
}


u8_t iskeypressed(u8_t key)
{
	u8_t c,ret=0;
//	printf("k=%x\n",key);
	c=(u8_t)toupper(getk());
	if (c=='0' && key==0) ret=1; 	
	if (c=='1' && key==1) ret=1; 
	if (c=='2' && key==2) ret=1; 
	if (c=='3' && key==3) ret=1; 
	if (c=='4' && key==4) ret=1; 
	if (c=='5' && key==5) ret=1; 
	if (c=='6' && key==6) ret=1; 
	if (c=='3' && key==7) ret=1; 
	if (c=='8' && key==8) ret=1; 
	if (c=='9' && key==9) ret=1; 
	if (c=='A' && key==0xA) ret=1; 
	if (c=='B' && key==0xB) ret=1; 
	if (c=='C' && key==0xC) ret=1; 
	if (c=='D' && key==0xD) ret=1; 
	if (c=='E' && key==0xE) ret=1; 
	if (c=='F' && key==0xF) ret=1; 
//	if (c!=0) printf("BING %x=%x\n",key,c);
	return (ret);
}
			
u8_t waitanykey()
{
	u8_t c;
	u8_t ret=100;
//	printf("K\n");
	while (ret==100)
	{
        	c=(u8_t)toupper(getkey());
//		c='5';
		if (c=='0' ) ret=0; 
		if (c=='1' ) ret=1; 
		if (c=='2' ) ret=2; 
		if (c=='3' ) ret=3; 
		if (c=='4' ) ret=4; 
		if (c=='5' ) ret=5; 
		if (c=='6' ) ret=6; 
		if (c=='7' ) ret=7; 
		if (c=='8' ) ret=8; 
		if (c=='9' ) ret=9; 
		if (c=='A' ) ret=0xA; 
		if (c=='B' ) ret=0xB; 
		if (c=='C' ) ret=0xC; 
		if (c=='D' ) ret=0xD; 
		if (c=='E' ) ret=0xE; 
		if (c=='F' ) ret=0xF;
	}
	return (ret);
}

void loadrom(void)
{
	int c;
	static u8_t font[16*5]=
   	{
	   0xf0,0x90,0x90,0x90,0xf0, 0x20,0x60,0x20,0x20,0x70,
	   0xf0,0x10,0xf0,0x80,0xf0 ,0xf0,0x10,0xf0,0x10,0xf0,
	   0x90,0x90,0xf0,0x10,0x10 ,0xf0,0x80,0xf0,0x10,0xf0,
	   0xf0,0x80,0xf0,0x90,0xf0 ,0xf0,0x10,0x20,0x40,0x40,
	   0xf0,0x90,0xf0,0x90,0xf0 ,0xf0,0x90,0xf0,0x10,0xf0,
	   0xf0,0x90,0xf0,0x90,0x90 ,0xe0,0x90,0xe0,0x90,0xe0,
	   0xf0,0x80,0x80,0x80,0xf0 ,0xe0,0x90,0x90,0x90,0xe0,
	   0xf0,0x80,0xf0,0x80,0xf0 ,0xf0,0x80,0xf0,0x80,0x80,
   	};
   	clg();
   	printf("ZX Chip8 V0.1 (C) 2005 Alvaro Alea Fernandez\n");
   	printf("Distribuido con licencia GPL V2\n\n");
//   	printf("Char=%x int=%x\n",sizeof(char),sizeof(int));
   	// cargamos la memoria con el fontmap
   	for (c=0;c<(16*5);c++)
   		mem[FONT_OFFSET+c]=font[c];
   
   	// cargamos la ROM
   	printf("Cargando Rom...[Pulse Una Tecla]\n");
   	getkey();
   	tape_load_block(&mem[0x200],17,255); //la cabezera de las narices.
   	tape_load_block(&mem[0x200],(mem[0x20C]<<8)+mem[0x20B],0);
//   	tape_load_block(&mem[0x200],(mem[0x20C]<<8)+( mem[0x20B] & 0xFF) ,0);
   	clg(); 
   	reset();
}

void pita (void)
{
//	   bit_frequency(0.01,440);
	   bit_beep(400,10);
}

void initsystem(void)
{
	// El spectrum no necesita inicializacion.
}
