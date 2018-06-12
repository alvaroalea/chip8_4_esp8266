/* Emulador de Chip-8 para ESP8266
 * Basado en el emulador para ZX Spectrum *
 * Desarrollado por Alvaro Alea Fernandez *
 * Distribuido con licencia GPL V2        *
 * (C) 2018                               */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <SSD1306Wire.h>
SSD1306Wire  lcd(0x3c,5,4);

const char* ssid = "yellowice";
const char* password = "yellowlife";

#define INSTRPERFRAME 15
#define DEBUG3

// #include "chip8.h"
#define FONT_OFFSET 0x38

// realmente estos valores deberian ser de memoria y no a parte
// tampoco tengo en memoria la bios-128 para introducir programas
byte mem[0x1000]; 
byte reg[0x10];   // mem 0x0 a mem 0xF
int  stack[0x10];  // mem 0x16 a mem 0x36
unsigned char sp,stimer,dtimer; //sp deberia ser tb un int
int  pc,i;
unsigned char display[64*32];
unsigned char keys[5];
unsigned int emustat=0; 

void cls(void);
int do_cpu(void);
void chip8_reset(void);
unsigned char drawsprite(unsigned char x, unsigned char y, unsigned char size);



// NORMAL .INO FILE FROM HERE:

void updatedisplay(unsigned char x,unsigned char y, unsigned char w, unsigned char h){
  int c,d,x2,y2;
  x2=(x+w-1) & 0x3F; // como los sprites son cicliclos, es posible
  if (x2<x) {        // tener que dibujar 2 o 4 cuatro trocitos en
    x=0;w=64;        // la pantalla, si eso pasa, dibujo toda una
  }                  // tira que abarque los 2 trozos o la pantalla entera. 
  y2=(y+h-1) & 0x1F; 
  if (y2<y) {
    y=0;h=32;
  }
  lcd.clear(); //hasta que sepa como borra pixeles, hay que actualizar toda la pantalla de golpe.  
  for (d=y;d<(y+h);d++)
    for (c=x;c<(x+w);c++) {
      if (display[d*64+c]==1) {
        lcd.setPixel(c*2,d*2);
        lcd.setPixel(c*2+1,d*2);
        lcd.setPixel(c*2,d*2+1);
        lcd.setPixel(c*2+1,d*2+1);
//      } else {
//        unplot(c,d);
      }
    }
  lcd.display();
#ifdef DEBUG
  Serial.println("Update en Pantallas");
#endif

}

unsigned char iskeypressed(unsigned char key){
  unsigned char ret=0;
#ifdef DEBUG
  Serial.print("pregunta tecla:");  Serial.print(" => ");

  Serial.println(key);
#endif
  if (key==keys[0] && (digitalRead(0)==0 )) ret=1;  // Izquierda
  if (key==keys[1] && (digitalRead(12)==0 )) ret=1; // arriba
  if (key==keys[2] && (digitalRead(13)==0 )) ret=1; // abajo
  if (key==keys[3] && (digitalRead(14)==0 )) ret=1; // fuego
  return (ret);
}

unsigned char waitanykey(void){  // FIXME: que funcione el redefinido
  unsigned char ret=0;
  do {
    if (digitalRead(0)==0 ) ret=keys[0];  
    if (digitalRead(12)==0) ret=keys[1]; 
    if (digitalRead(13)==0) ret=keys[2]; 
    if (digitalRead(14)==0) ret=keys[3]; 
  } while (ret==0) ;
  return (ret-1);
}

unsigned char readkey(void){ //solo se usa para el menu.
  unsigned char ret=0;
    if (digitalRead(0)==0 ) ret=1;  
    if (digitalRead(12)==0) ret+=2; 
    if (digitalRead(13)==0) ret+=4; 
    if (digitalRead(14)==0) ret+=8; 
  return (ret);
}


void loadrom(int index){
  Dir dir;
  File f;
  String filename;
  int c;
  static unsigned char font[16*5]=
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
// TECLAS POR DEFECTO PARA: IZQUIERDA, ARRIBA, ABAJO, FUEGO
static unsigned char teclas[]={5,4,6,8,0}; // Breakout, airplane
// static unsigned char teclas[]={1,3,6,8,7}; // Binky
// static unsigned char teclas[]={12,4,1,13,0}; // Pong
// static unsigned char teclas[]={1,2,7,10,11,15,12,13}; // WipeOut,vers
// static unsigned char teclas[]={0,1,4,7,0}; // vbrix
// static unsigned char teclas[]={5,6,4,0,0}; // ufo
// static unsigned char teclas[]={7,6,3,8,0}; // syzyz
// static unsigned char teclas[]={15,2,8,6,0}; // cave

    // cargamos la memoria con el fontmap
   for (c=0;c<(16*5);c++)
      mem[FONT_OFFSET+c]=font[c];

    // cargamos la ROM
   dir = SPIFFS.openDir("/");
   while (dir.next()) {
      filename = String(dir.fileName());
      if (-1!=filename.lastIndexOf(String(".CH8"))) {
         if (i==index) {
            f=SPIFFS.open(dir.fileName(),"r");
            f.read(&mem[0x200],f.size());
            f.close();
            filename.setCharAt(filename.length()-2,'K');
#ifdef DEBUG3
            Serial.println(filename.c_str());
#endif
            if (SPIFFS.exists(filename)) {
               // El fichero abrio, recupearamos la configuracion de teclas.
#ifdef DEBUG3
            Serial.print("Abriendo: ");
            Serial.println(filename.c_str());
#endif
                f=SPIFFS.open(dir.fileName(),"r");
                if (!f) {
#ifdef DEBUG3
                  Serial.println("Abrio!. ");
#endif
                  f.read(&keys[0],4); 
                  f.close();
                }
            } else { 
#ifdef DEBUG3
               Serial.println("teclas por defecto. ");
#endif
               for (c=0;c<5;c++)    // no hay fichero .CK8, asi que valor por defecto.
                   keys[c]=teclas[c];
            }
         }
         i++;
      }
   }
   chip8_reset();
}

void pita (void){
      if (stimer>0) // se supone que stimer>0 un bit de altavoz cambia de sentido cada vez que se le llama a unos 1200Hz.
          digitalWrite(16,0); // pero solo encedemos un led, por ahora.
      else
          digitalWrite(16,1);
}

 void setup() {
  // put your setup code here, to run once:
  pinMode(0,INPUT_PULLUP);
  pinMode(12,INPUT_PULLUP);
  pinMode(13,INPUT_PULLUP);
  pinMode(14,INPUT_PULLUP);
  pinMode(16,OUTPUT);
  digitalWrite(16,1);

  Serial.begin(115200);
  Serial.println("Booting");
  SPIFFS.begin();
//  SPIFFS.format();


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
/*  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  } */
 
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
 
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  
  lcd.init();
  lcd.flipScreenVertically();
  lcd.setFont(ArialMT_Plain_10);
  lcd.setTextAlignment(TEXT_ALIGN_CENTER);
  lcd.drawString(64, 0, "Chip8 Emulator");
  lcd.drawString(64, 15, "for ESP8266");
  lcd.drawString(64, 30, "(C) 2018 Alvaro Alea");
  lcd.display();
  delay(1000);
}

void loop() {
  static int c=0;
  static int index=0;
  static int pos=0;
  static int lastkey=0;
  static int nfiles = 0;
  int key=0;
  int i=0;
  int fil=12;
#define FILS 12
#define NLIN 4
  
  Dir dir;
  String filename;
  
  ArduinoOTA.handle();

  switch (emustat) 
  {
  case 0x00: // IMPRIMIR LA PANTALLA DE SELECCION DE ARCHIVO
  dir = SPIFFS.openDir("/");
  lcd.clear();
  lcd.setTextAlignment(TEXT_ALIGN_CENTER);
  lcd.drawString(64, 0, "SELECT GAME");
  lcd.setTextAlignment(TEXT_ALIGN_LEFT);
  Serial.println("Archivos:");
while (dir.next()) {
    filename = String(dir.fileName());
//    if (0==ext.compareTo(String(".CH8"))) {
    if (-1!=filename.lastIndexOf(String(".CH8"))) {
       Serial.print(i);
       if (i>=index && i<(index+NLIN)) {
        Serial.print(" * ");
        if (i==(index+pos)) filename=String("> " + filename + " <");
        lcd.drawString(0, fil,filename.c_str());
        fil=fil+FILS;
       } else {
        Serial.print(" = ");
       }
       Serial.println(filename.c_str());
       i++;
    }
}
lcd.display();
Serial.print("TOTAL:");
Serial.println(i);
nfiles=i-1;
emustat=1;

    break;
  case 0x01: // ESPERAR UNA TECLA
    key=readkey();
    if (key!=lastkey) {
    if (key==0) {
      switch (lastkey)
      {
        case 2: //ARR
           if (pos>0) {
             pos--;
             emustat=0;
           } else {
            if (index>0) {
              index--;
              emustat=0;
            }
           }
           break;
        case 4: //ABA
           if (nfiles>(index+pos)) {
            if (pos<(NLIN-1)) {
              pos++;
              emustat=0;
            } else {
                index++;
                emustat=0;
            }
           }
           break;
        case 1: //IZQ
        case 8: //FIRE
           loadrom(index+pos);
           updatedisplay(0,0,64,32);
           emustat=2;
           break;
      }
    }
    lastkey=key;  
    }
    break;  
  case 0x02: // LA EMULACION EN SI
   
    // velocidad del chip8 calculada a ojo para que sygzi sea jugable 
    // 20 instrucciones por frame, 60 frames por segundo (NTSC) awesome 1200Hz.
    if (c < INSTRPERFRAME) {     
      do_cpu();
 //       if (stimer>0)  // en realidad como va a 1200Hz, el pita deberia alternar el bit cada vez que se ejecuta
          pita();       // y asi tendriamos el sonido 
      delay(1); 
    }
    if (c>=INSTRPERFRAME) {
      updatedisplay(0,0,64,32);
      if (dtimer>0) // se supone que diminuye en cada frame, 60Hz
        dtimer--;
      if (stimer>0) 
        stimer--; 
      c=0;
    } else {
      c++;
    }
    break;
  }
}

// CHIP8.C FILE FROM HERE


void cls(void)
{
  int c;
  for (c=0;c<(64*32);c++) display[c]=0;
  updatedisplay(0,0,64,32);
}

unsigned char drawsprite(unsigned char x, unsigned char y, unsigned char size)
{
  unsigned char ret=0;
  unsigned char c,d,dat,pr,px,po;
  int dir;
  for (c=0;c<size;c++)
  {
    dat=mem[i+c];
    for (d=0;d<8;d++)
    {
      dir= ((y+c)%32)*64 + ((x+d)%64); //probably mas rapido con rots y mask
      pr=display[dir];
      px=( dat & 0x80) >> 7;
      dat=dat << 1;
      po= pr ^ px;
      if ((pr==1) && (po==0)) 
        ret=1;
      display[dir]=po;
    }
  }
//  updatedisplay(x,y,8,size);
  return (ret); 
}

void chip8_reset(void)
{
  cls();
  pc=0x200;
  sp=0;  // �necesario?
#ifdef DEBUG
  Serial.println("CLS");
#endif
}

/* El nucleo del la CPU del Chip8 */
/* Ejecuta una unica instruccion  */
/* 34 instrucciones de 2bytes     */

int do_cpu(void)   
{
  int inst,in2,c,r;
//  int  in1,x,y,zz,r;
  unsigned char in1,x,y,zz;
#ifdef DEBUG
  Serial.print("PC=");
  Serial.print(pc);
  Serial.print(",I=");
#endif 
  inst=(mem[pc++]<<8) + mem[pc++] ;
//  inst=((mem[pc++] & 0xFF)<<8) + (mem[pc++] & 0xFF );
#ifdef DEBUG
  Serial.println(inst);
#endif 
  in1=(inst >>12) & 0xF;
  x = (inst >> 8) & 0xF;
  y = (inst >> 4) & 0xF;
  zz = inst & 0x00FF;
#ifdef DEBUG1
   Serial.print(" => ");
   debug_decode(inst);
#endif 

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
    case 0x8: // instruccion extendida matematicas
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
//          printf("%x=%x-%x-%x\n",reg[x],reg[x]/100,(reg[x]/10) % 10,(reg[x]) % 10);
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

#ifdef DEBUG
void debug_decode(unsigned short inst)   
{
  int in2,c,r;
  unsigned char in1,x,y,zz;
  static int debug=0; 
  in1=(inst >>12) & 0xF;
  x = (inst >> 8) & 0xF;
  y = (inst >> 4) & 0xF;
  zz = inst & 0x00FF;
  char sbuffer[64];
  sprintf(sbuffer,"%x",inst);
  Serial.print(sbuffer);
  Serial.print(" <");
  switch (in1)
  {
    case 0x1: // jmp xyz
      sprintf(sbuffer,"jmp  %x",inst & 0x0FFF);
      break;
    case 0x2: // call xyz
      sprintf(sbuffer,"call  %x",inst & 0xFFF);
      break;           
    case 0x3: // skeq :  salta si r(x) igual a yz 
      sprintf(sbuffer,"skeq v[%x],%x",x, zz);
      break;
    case 0x4: //skne : salta si r(x) distinto de yz
      sprintf(sbuffer,"skne v[%x],%x",x, zz);
      break;
    case 0x5: //skeq : salta si r(x) igual a r(y)
      sprintf(sbuffer,"skeq v[%x],v[%x]",x, y);
      break;
    case 0x6:  // ld 
      sprintf(sbuffer,"ld   v[%x],%x",x,zz);
      break;
    case 0x7:
      sprintf(sbuffer,"add  v[%x],%x",x,zz);
      break;
    case 0x9:
      sprintf(sbuffer,"skne v[%x],v[%x]",x, y);
      break;
    case 0xa: // ld i,
      sprintf(sbuffer,"ld   i,%x",inst & 0xFFF);;
      break;
    case 0xb: // jmi xxx
      sprintf(sbuffer,"jpi");
      break;
    case 0xc: // ldrnd
      sprintf(sbuffer,"rnd  v[%x],%x",x,zz);
      break;
    case 0xd: // draw sprite 
      sprintf(sbuffer,"draw v[%x],v[%x],%x",x,y, inst & 0xF);
      break;
    case 0x0: // instruccion extendida
      switch (zz)
      {
        case 0xE0: 
          sprintf(sbuffer,"cls");
          break;
        case 0xEE: // ret
          sprintf(sbuffer,"ret");
          break;
      }
      break;
    case 0x8: // instruccion extendida
      in2= inst & 0xF;
      switch (in2)
      {
        case 0x0: // ld
          sprintf(sbuffer,"ld   v[%x],v[%x]",x,y);
          break;
        case 0x1: // or
          sprintf(sbuffer,"or   v[%x],v[%x]",x,y);
          break;
        case 0x2: // and
          sprintf(sbuffer,"and  v[%x],v[%x]",x,y);
          break;
        case 0x3: // xor
          sprintf(sbuffer,"xor  v[%x],v[%x]",x,y);
          break;
        case 0x4: // addc
          sprintf(sbuffer,"addc v[%x],v[%x]",x,y);
          break;
        case 0x5: // subc 
          sprintf(sbuffer,"subc v[%x],v[%x]",x,y);
          break;
        case 0x6: // shl
          sprintf(sbuffer,"shl  v[%x]",x);
          break;
        case 0x7: // rsubc 
          sprintf(sbuffer,"rsbc v[%x],v[%x]",x,y);
          break;
        case 0xe: // shr
          sprintf(sbuffer,"shr  v[%x]",x);
          break;
      }
      break;
    case 0xe: // instruccion extendida
      switch (zz)
      {
        case 0x9E: // skipifkey
          sprintf(sbuffer,"skek v[%x]",x);
          break;
        case 0xA1: // skipifnokey
          sprintf(sbuffer,"sknk v[%x]",x);
          break;
      }
      break;
    case 0xf: // instruccion extendida
      switch (zz)
      {
        case 0x07: // getdelay
          sprintf(sbuffer,"ld   v[%x],dt",x);
          break;
        case 0x0A: // waitkey 
          sprintf(sbuffer,"wait v[%x]",x);
          break;
        case 0x15: // setdelay
          sprintf(sbuffer,"ld   dt,v[%x]",x);
          break;
        case 0x18: // setsound
          sprintf(sbuffer,"ld   st,v[%x]",x);
          break;
        case 0x1E: // add i,
          sprintf(sbuffer,"add  i,v[%x]",x);
          break;
        case 0x29: // font i  
          sprintf(sbuffer,"font v[%x]",x);
          break;
        case 0x33: // bcd
          sprintf(sbuffer,"bcd  v[%x]",x);
          break;
        case 0x55: // str
          sprintf(sbuffer,"str  %x",x);
          break;
        case 0x65: // ldr
          sprintf(sbuffer,"ldr %x",x);
          break;
      }
      break;
  }
Serial.print(sbuffer);
Serial.println(">");
}
#endif

