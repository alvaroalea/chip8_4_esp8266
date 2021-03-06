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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "chip8.h"
#include "speccy.h"

// realmente estos valores deberian ser de memoria y no a parte
// tampoco tengo en memoria la bios-128 para introducir programas
u8_t mem[0x1000]; 
u8_t reg[0x10];   // mem 0x0 a mem 0xF
int  stack[0x10];  // mem 0x16 a mem 0x36
u8_t sp,stimer,dtimer; //sp deberia ser tb un int
int  pc,i;
u8_t display[64*32]; 
unsigned char key[17]={ 'X','1','2','3','Q','W','E','A','S','D','Z','C','4','R','F','V','M'};
u8_t debug;

#ifdef deunavez
#include "speccy.c"
#endif


void cls(void)
{
	int c;
	for (c=0;c<(64*32);c++) display[c]=0;
#ifndef UPDATE_BY_FRAMES
	updatedisplay(0,0,64,32);
#endif
//	printf("vamos bien\n");
}

/* Opcionalmente aceleramos en ensamblador ciertas rutinas */
//#ifdef deunavez
//#define drawsprite speccydrawsprite
//#else
//#endif

u8_t drawsprite(u8_t x, u8_t y, u8_t size)
{
	u8_t ret=0;
	u8_t c,d,dat,pr,px,po;
	int dir;
	for (c=0;c<size;c++)
	{
		dat=mem[i+c];
		for (d=0;d<8;d++)
		{
			dir= ((y+c)%32)*64 + ((x+d)%64); //probably mas rapido con rots y mask
//			dir= ((y+c) & 0x1F << 6 ) + ((x+d) & 0x3F);	
			pr=display[dir];
			px=( dat & 0x80) >> 7;
			dat=dat << 1;
			po= pr ^ px;
			if ((pr==1) && (po==0)) 
				ret=1;
			display[dir]=po;
		}
	}
#ifndef UPDATE_BY_FRAMES
	if (debug==0)
		updatedisplay(x,y,8,size);
	else
		updatedisplaymini(x,y,8,size);
#endif
	return (ret);	
}

void reset(void)
{
	cls();
	pc=0x200;
	sp=0;  // ┐necesario?
}

int main(void)
{
	unsigned char oldkey[17];
	int c,d,quit;
	u8_t showalldebug;
//   	printf("Primero\n");
   	initsystem();
//   	printf("Systema Iniciado\n");
   	while (1)
   {
	menu();
	clg();
	if (debug==1)
	{
		//strcopy(oldkey,key,17);
		//strcopy(key,"X123QWEASDZC4RFVM",17);
		updatedisplaymini(0,0,64,32);
		showalldebug=1;
	 } else 
		updatedisplay(0,0,64,32);
	
   	quit=1;
   	while (quit)
   	{       // calculado a ojo para que sygzi sea jugable 
		for (c=0;c< INSTRPERFRAME ;c++) // spectrum es muy lento btw
	   	{
	   		if (debug==1)
			{
				showalldebug=printstate(showalldebug);
				if (mygetkey()==key[16])
				{
					quit=0;
					break;
				}
			} else {
				if (mygetk()==key[16]) 
				{
					quit=0;
					break;
				}
			}
			do_cpu();
	   		if (stimer>0)
		   		pita();
	   	}
   	   	if (dtimer>0) // nadie garantiza los 60Hz.
			dtimer--;
	   	if (stimer>0)
	   		stimer--;
		if (stimer>0)
			border(2);
		else
			border(0);
   	}
	if (debug==1)
	{
	//strcopy(key,oldkey,17);
	}
   }
   return(0);
}

/* El nucleo del la CPU del Chip8 */
/* Ejecuta una unica instruccion  */
/* 34 instrucciones de 2bytes     */

int do_cpu(void)   
{
	int inst,in2,c,r;
//	int  in1,x,y,zz,r;
	u8_t in1,x,y,zz;
	inst=(mem[pc++]<<8) + mem[pc++] ;
//	inst=((mem[pc++] & 0xFF)<<8) + (mem[pc++] & 0xFF );
	in1=(inst >>12) & 0xF;
	x = (inst >> 8) & 0xF;
	y = (inst >> 4) & 0xF;
	zz = inst & 0x00FF;

	switch (in1)
	{
		case 0x2: // call xyz
			//printf("estoy en %x y llamo...(guardo en %d)\n",pc,sp);
			stack[sp]=pc;
			sp= ++sp & 0x0F;  // el stack es de solo 16 niveles.
			pc= inst & 0xFFF; 
			
			break;           // :-) 
		case 0x1: // jmp xyz
			pc=inst & 0x0FFF;
			break;
		case 0x3: // skeq :  salta si r(x) igual a yz 
			if (reg[x] == zz) 
				pc+=2;
			break;
		case 0x4: //skne : salta si r(x) distinto de yz
			if (reg[x] != zz) 
				pc+=2;
			break;
		case 0x5: //skeq : salta si r(x) igual a r(y)
			if (reg[x] == reg[y])
				pc+=2; 
			break;
		case 0x6:  // ld 
		        reg[x]=zz;
			break;
		case 0x7:
		        reg[x]=reg[x]+zz;
			break;
		case 0x9:
			if (reg[x] != reg[y]) 
				pc+=2;
			break;
		case 0xa: // ld i,
			i=inst & 0xFFF;
			break;
		case 0xb: // jmi xxx
			pc=(inst & 0xFFF) + reg[0];
			break;
		case 0xc: // ldrnd
			reg[x]= rand() & zz;
			break;
		case 0xd: // draw sprite 
			reg[0xF]=drawsprite( reg[x], reg[y], inst & 0xF);
			break;
		case 0x0: // instruccion extendida
			switch (zz)
			{
				case 0xE0: 
					cls();
					break;
				case 0xEE: // ret
					sp = --sp & 0xF;
					pc=stack[sp] & 0xFFF;
					//printf("vuelvo a %x... (pillo de %d)\n",pc,sp);
					break;
			}
			break;
		case 0x8: // instruccion extendida
			in2= inst & 0xF;
			switch (in2)
			{
				case 0x0: // ld
					reg[x] = reg[y];
					break;
				case 0x1: // or
					reg[x] |= reg[y];
					break;
				case 0x2: // and
					reg[x] &= reg[y];
					break;
				case 0x3: // xor
					reg[x] ^= reg[y];
					break;
				case 0x4: // addc
					r=reg[x] + reg[y];
					reg[x] = r & 0xFF;
					reg[0xf]=( ((r & 0xF00) !=0)  ? 1:0);
					break;
				case 0x5: // subc 
					r=reg[x] - reg[y];
					reg[x]=r & 0xff;
			 		reg[0xf]=( (r < 0) ? 0:1);
					break;
				case 0x6: // srl
					reg[0xf]=reg[x] & 0x1;
					reg[x] = reg[x] >> 1;
					break;
				case 0x7: // rsubc 
					r=reg[y] - reg[x];
					reg[x] = r & 0xFF;
					reg[0xf]=( (r < 0) ? 0:1); 
					break;
				case 0xe: // srr
					reg[0xF]=( reg[x] & 0x80) >> 8;
					reg[x] = (reg[x] << 1) & 0xFF;
					break;
			}
			break;
		case 0xe: // instruccion extendida
			switch (zz)
			{
				case 0x9E: // skipifkey
					if ( iskeypressed(reg[x]) )
						pc+=2;
					break;
				case 0xA1: // skipifnokey
					if ( ! iskeypressed(reg[x]) )
						pc+=2;
					break;
			}
			break;
		case 0xf: // instruccion extendida
			switch (zz)
			{
				case 0x07: // getdelay
					reg[x]=dtimer;
					break;
				case 0x0A: // waitkey 
					reg[x]=waitanykey();
					break;
				case 0x15: // setdelay
					dtimer=reg[x];
					break;
				case 0x18: // setsound
					stimer=reg[x];
					break;
				case 0x1E: // add i,
					i+=reg[x];
					break;
				case 0x29: // font i  
					i= FONT_OFFSET + (reg[x]*5);
					break;
				case 0x33: // bcd
					mem[i]=reg[x]/100;
					mem[i+1]=(reg[x]/10) % 10;
					mem[i+2]=(reg[x]) % 10;
//					printf("%x=%x-%x-%x\n",reg[x],reg[x]/100,(reg[x]/10) % 10,(reg[x]) % 10);
					break;
				case 0x55: // str
					for (c=0;c<=x;c++)
						mem[i+c]=reg[c];
					break;
				case 0x65: // ldr
					for (c=0;c<=x;c++)
						reg[c]=mem[i+c]; 
					break;
			}
			break;
	}

	return (0);
}
	


void decode(u16_t inst)   
{
	int in2,c,r;
	u8_t in1,x,y,zz;
	static int debug=0;	
	in1=(inst >>12) & 0xF;
	x = (inst >> 8) & 0xF;
	y = (inst >> 4) & 0xF;
	zz = inst & 0x00FF;

	switch (in1)
	{
		case 0x2: // call xyz
			printf("call  %x        ",inst & 0xFFF);
			break;           
		case 0x1: // jmp xyz
			printf("jmp  %x         ",inst & 0x0FFF);
			break;
		case 0x3: // skeq :  salta si r(x) igual a yz 
			printf("skeq v[%x],%x      ",x, zz);
			break;
		case 0x4: //skne : salta si r(x) distinto de yz
			printf("skne v[%x],%x      ",x, zz);
			break;
		case 0x5: //skeq : salta si r(x) igual a r(y)
			printf("skeq v[%x],v[%x]   ",x, y);
			break;
		case 0x6:  // ld 
		        printf("ld   v[%x],%x     ",x,zz);
			break;
		case 0x7:
		        printf("add  v[%x],%x     ",x,zz);
			break;
		case 0x9:
			printf("skne v[%x],v[%x]    ",x, y);
			break;
		case 0xa: // ld i,
			printf("ld   i,%x          ",inst & 0xFFF);;
			break;
		case 0xb: // jmi xxx
			printf("jpi               ");
			break;
		case 0xc: // ldrnd
			printf("rnd  v[%x],%x       ",x,zz);
			break;
		case 0xd: // draw sprite 
			printf("draw v[%x],v[%x],%x",x,y, inst & 0xF);
			break;
		case 0x0: // instruccion extendida
			switch (zz)
			{
				case 0xE0: 
					printf("cls              ");
					break;
				case 0xEE: // ret
					printf("ret              ");
					break;
			}
			break;
		case 0x8: // instruccion extendida
			in2= inst & 0xF;
			switch (in2)
			{
				case 0x0: // ld
					printf("ld   v[%x],v[%x]   ",x,y);
					break;
				case 0x1: // or
					printf("or   v[%x],v[%x]   ",x,y);
					break;
				case 0x2: // and
					printf("and  v[%x],v[%x]   ",x,y);
					break;
				case 0x3: // xor
					printf("xor  v[%x],v[%x]   ",x,y);
					break;
				case 0x4: // addc
					printf("addc v[%x],v[%x]   ",x,y);
					break;
				case 0x5: // subc 
					printf("subc v[%x],v[%x]   ",x,y);
					break;
				case 0x6: // shl
					printf("shl  v[%x]        ",x);
					break;
				case 0x7: // rsubc 
					printf("rsbc v[%x],v[%x]   ",x,y);
					break;
				case 0xe: // shr
					printf("shr  v[%x]        ",x);
					break;
			}
			break;
		case 0xe: // instruccion extendida
			switch (zz)
			{
				case 0x9E: // skipifkey
					printf("skek v[%x]        ",x);
					break;
				case 0xA1: // skipifnokey
					printf("sknk v[%x]        ",x);
					break;
			}
			break;
		case 0xf: // instruccion extendida
			switch (zz)
			{
				case 0x07: // getdelay
					printf("ld   v[%x],dt     ",x);
					break;
				case 0x0A: // waitkey 
					printf("wait v[%x]        ",x);
					break;
				case 0x15: // setdelay
					printf("ld   dt,v[%x]     ",x);
					break;
				case 0x18: // setsound
					printf("ld   st,v[%x]     ",x);
					break;
				case 0x1E: // add i,
					printf("add  i,v[%x]      ",x);
					break;
				case 0x29: // font i  
					printf("font v[%x]        ",x);
					break;
				case 0x33: // bcd
					printf("bcd  v[%x]        ",x);
					break;
				case 0x55: // str
					printf("str  %x           ",x);
					break;
				case 0x65: // ldr
					printf("ldr %x            ",x);
					break;
			}
			break;
	}
}
	
