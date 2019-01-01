#include <SoftwareSerial.h>


#include "lsem.h"



//-- SOME NEEDED PROTOTYPES ---------------------------


//------------------------------------------------
//--- GLOBAL VARIABLES ---------------------------
//------------------------------------------------
char    GLBSeriaPortBufferUSB[60]; // a string to hold incoming data usb serial
char    GLBSeriaPortBufferBT[60]; // a string to hold incoming data usb serial

char    GLBauxString[60];
int     GLBSerialPortUSBIx=0;
int     GLBSerialPortBTIx=0;
bool    GLBSeriaPortBufferUSBReady = false; // whether the string is complete
bool    GLBSeriaPortBufferBTReady = false; // whether the string is complete




//------------------------------------------------
//-------    TIMER CALLBACKS PROTOTYPES  ---------
//------------------------------------------------

void GLBcallbackTimeoutLS(void);
void GLBcallbackPauseLS(void);
void GLBcallbackTimerFun(void);
void GLBcallbackTimerBTKeepAlive(void);
void GLBcallbackTimerBTActivity(void);


//------------------------------------------------
//-------    GLOBAL VARS TO THIS MODULE  ---------
//------------------------------------------------

#define BT_ARDUINO_PIN_TX 11
#define BT_ARDUINO_PIN_RX 12 
SoftwareSerial *bt;  //98:D3:32:20:FB:90
SimpleTimer GLBtimers;
int  GLBtimerFun=-1; 
int  GLBtimerBTKeepAlive=-1; 
int  GLBtimerBTActivity=-1; 

#define MS_TIMER_FUN              5000
#define MS_TIMER_BT_KEEP_ALIVE    10000
#define MS_TIMER_BT_ACTIVITY      3000


#define NUM_LEDS  47
#define DATA_PIN_LS 9
CRGB LS[NUM_LEDS];

LSEM ls(LS, NUM_LEDS, GLBcallbackPauseLS, GLBcallbackTimeoutLS);


int GLBhelloIndex=0;
#define HELLO_MESSAGES 5
const char boot1[] = ":LP0030:LT0040:LMA:LCff,66,ff";
const char boot2[] = ":LP0030:LT0040:LMT:LC00,ff,55";
const char boot3[] = ":LP0030:LT0040:LMt:LC4d,a6,ff";
const char boot4[] = ":LP0030:LT0040:LMK:LCFF,a6,ff";
const char boot5[] = ":LP0030:LT0040:LMk:LCFF,a6,ff";

const char *bootx[] = {  boot1, boot2, boot3, boot4, boot5 };

unsigned long GLBCrazyLoopCounter=0;



//------------------------------------------------
//--- GLOBAL FUNCTIONS ---------------------------
//------------------------------------------------
void playNoiseColor(int distance);
void playNoise(int distance);
void playNoisePink(int distance);
void playRolling(int distance);
void playKnightRider(int distance);
void playPing(int distance);
void doFun();

//------------------------------------------------
//-------    TIMER CALLBACKS     -----------------
//------------------------------------------------
void GLBcallbackTimeoutLS(void)
{
  //Serial.println(F("TimeoutLS"));
  ls.callbackTimeout();
}
//------------------------------------------------
void GLBcallbackPauseLS(void)
{
  //Serial.println(F("PauseLS"));
  ls.callbackPause();
}

//------------------------------------------------
void GLBcallbackTimerFun(void)
{
  Serial.println(F("GLBcallbackTimerFun"));

  if (GLBhelloIndex < HELLO_MESSAGES) return;
  if (!ls.isIdle()) return;

  doFun();
}

//------------------------------------------------
void GLBcallbackTimerBTKeepAlive(void)
{
  Serial.println(F("GLBcallbackTimerBTKeepAlive"));
  bt->print(":BT");
  bt->println(GLBCrazyLoopCounter);
}
//------------------------------------------------
void GLBcallbackTimerBTActivity(void)
{
  Serial.println(F("GLBcallbackTimerBTActivity"));
  GLBtimerBTActivity=-1;
}


//-------------------------------------------------------------

#define MAX_FUN_MODES 6
typedef void (*cbf)(int);  
cbf GLBplaySetptr[MAX_FUN_MODES] = {
  &playNoiseColor,&playRolling,&playNoisePink,&playNoise,&playKnightRider,playPing};    
uint8_t GLBplaySetptr_index=MAX_FUN_MODES-1;
//------------------------------------------------

void doFun(void)
{
 int distance = random(1,30);

 Serial.print(F("doFUN:"));
 Serial.println(distance);

 if (distance==0) return;
 if (distance>30) return;


 if ( ls.isIdle() ) {  
    //Try new set
    GLBplaySetptr_index=(GLBplaySetptr_index+1)%MAX_FUN_MODES;
 }
 // else {      //keep in the set  }
 GLBplaySetptr[GLBplaySetptr_index](distance);
}



//------------------------------------------------

void playNoiseColor(int distance)
{
   char aux[50];
   char aux2[30];

   //Serial.println(F("NOISE COLOR SET"));
   if (distance > 15) return;


   int delta=(distance%10)*5;

   int filter=(distance)*3; // More distance, more filter

   int flick=25+distance*4;  // More distance, less flicker

   sprintf(aux2,":LT0050:LMn:LG%04d:LP%04d",filter,flick);

   //Color by slots
   if (distance < 10) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xFF-delta,0,0);
   }
   else if (distance < 15) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xFF-delta,0xFF-delta,0);
   }
   else if (distance < 20) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0,0,0xFF-delta);
   }
   else if (distance < 25) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0,0xFF-delta,0xFF-delta);
   }
   else if (distance < 30) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xFF-delta,0xFF-delta,0xFF-delta);
   }
   ls.processCommands(aux);
 

}

//----------------------------------------------------------
void playRolling(int distance)
{
   char aux[50];
   char aux2[30];

   //Serial.println(F("ROLLING SET"));


   int delta=(distance%10)*5;


   int flick=20+distance*3;  // More distance, less flicker

   sprintf(aux2,":LT0050:LMC:LP%04d",flick);

   //Color by slots
   if (distance < 10) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xAA-delta,0,0);
   }
   else if (distance < 15) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xBB-delta,0xCC-delta,0x22);
   }
   else if (distance < 20) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0,0xFF-delta,0);
   }
   else if (distance < 25) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0,0x66-delta,0x55-delta);
   }
   else if (distance < 30) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0x00-delta,0x10-delta,0xaa-delta);
   }
   ls.processCommands(aux);
 

}

//----------------------------------------------------------
void playKnightRider(int distance)
{
   char aux[50];
   char aux2[30];

   //Serial.println(F("Knight rider SET"));


   int delta=(distance%10)*7;


   int flick=20+distance*3;  // More distance, less flicker

   sprintf(aux2,":LT0050:LMK:LP%04d",flick);

   //Color by slots
   if (distance < 10) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0,0,0xFF-delta);
   }
   else if (distance < 15) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,delta,0xFF-delta,0xFF);
   }
   else if (distance < 20) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0,0xFF-delta,0);
   }
   else if (distance < 25) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xff,delta,delta);
   }
   else if (distance < 30) { 
     sprintf(aux,"%s:LC%02X,%02X,%02X",aux2,0xff-delta,0xff-delta,0xff-delta);
   }
   ls.processCommands(aux);
 

}


//------------------------------------------------

void playNoise(int distance)
{
   char aux[50];

   // Serial.println(F("NOISE SET"));

   int flick=100+distance*4;  // More distance, less flicker
   
   if (ls.isIdle())  {
     sprintf(aux,":LT0050:LMN:LP%04d",flick);
     ls.processCommands(aux);
   }
} 

//------------------------------------------------

void playNoisePink(int distance)
{
   char aux[50];
   // Serial.println(F("PINK NOISE SET"));

   int filter=(distance)*3; // More distance, more filter
   int flick=200+distance*3;  // More distance, less flicker

   sprintf(aux,":LT0050:LMn:LG%04d:LP%04d:LC9F,17,64",filter,flick);


   ls.processCommands(aux);
} 



//----------------------------------------------------------
void playPing(int distance)
{
   char aux[120];
   sprintf(aux,":LT0030:LMK:LP0100:LC%02X,%02X,%02X:",
                 distance*4,distance*2,distance*6);
   ls.processCommands(aux);
}



//------------------------------------------------

void setup() { 
  //delay(2000);
  // Serial to debug AND comunication protocolo with PI              
  Serial.begin(9600);
  Serial.println(F("Setup... 'came on, be my baby, came on'"));
  GLBSeriaPortBufferUSB[0]=0;
  GLBSeriaPortBufferBT[0]=0;

  FastLED.addLeds<WS2812B,DATA_PIN_LS,GRB>(LS, NUM_LEDS);

  // NOTE Rx pin bt is my arduino serial TX
  // NOTE Tx pin bt is my arduino serial RX
  bt=new SoftwareSerial(BT_ARDUINO_PIN_TX, BT_ARDUINO_PIN_RX); 
  bt->begin(9600);

  GLBtimerFun=GLBtimers.setInterval((long int)MS_TIMER_FUN,GLBcallbackTimerFun);

  GLBtimerBTKeepAlive=GLBtimers.setInterval((long int)MS_TIMER_BT_KEEP_ALIVE,GLBcallbackTimerBTKeepAlive);
}

//------------------------------------------------
// SerialEvent occurs whenever a new data comes in the hardware serial RX
// run between each time loop() runs
void serialEvent() {
  while (Serial.available()) {
     char inChar = (char)Serial.read();
     if (inChar < 0x20) {
       GLBSeriaPortBufferUSBReady = true;
       GLBSeriaPortBufferUSB[GLBSerialPortUSBIx]=0;
       GLBSerialPortUSBIx=0;
       return;
     }
     GLBSeriaPortBufferUSB[GLBSerialPortUSBIx++]=inChar;
  }
}
//------------------------------------------------
void printString(char *s)
{
  Serial.print(F("Full String:"));
  int l=strlen(s);
  Serial.print(F(".Len:"));
  Serial.print(l);
  Serial.print(F(". Data:"));

  for (int i=0; i < 100; i++) {
    if (s[i] == 0){
      Serial.println();
      return;
    }
    else{
      Serial.write(s[i]);
    }
  }
}
//------------------------------------------------
void printStringBT(char *s)
{
  bt->print(F("Full String:"));
  int l=strlen(s);
  bt->print(F(".Len:"));
  bt->print(l);
  bt->print(F(". Data:"));

  for (int i=0; i < 100; i++) {
    if (s[i] == 0){
      bt->println();
      return;
    }
    else{
      bt->write(s[i]);
    }
  }
}
//------------------------------------------------
// Call explicitly in loop() 
void serialEventBT() {
  int inChar=0;
  while (bt->available()) {

     if (GLBtimerBTActivity==-1)
        GLBtimerBTActivity=GLBtimers.setTimeout((long int)MS_TIMER_BT_ACTIVITY,GLBcallbackTimerBTActivity);
     else
        GLBtimers.restartTimer(GLBtimerBTActivity);
     inChar = bt->read();
     if (inChar < 0x20) {
       /*DEBUG Serial.print(F("Char Rx from <0x20"));
       Serial.println(inChar,HEX);*/
       printStringBT(GLBSeriaPortBufferBT);
       GLBSeriaPortBufferBTReady = true;
       GLBSeriaPortBufferBT[GLBSerialPortBTIx]=0;
       printString(GLBSeriaPortBufferBT);  //DEBUG 
       GLBSerialPortBTIx=0;
       return;
     }
     GLBSeriaPortBufferBT[GLBSerialPortBTIx]=(char)inChar;

     /* DEBUG Serial.print(F("Char Rx from BT:"));
     Serial.print((char)inChar); 
     Serial.print(GLBSeriaPortBufferBT[GLBSerialPortBTIx]);
     Serial.print(F(". Pos:"));
     Serial.print(GLBSerialPortBTIx); 
     GLBSeriaPortBufferBT[GLBSerialPortBTIx+1]=0;
     Serial.print(F(". So far:"));
     printString(GLBSeriaPortBufferBT); */
     

     GLBSerialPortBTIx++;
  }
}

//-------------------------------------------------
void processSeriaPortBufferUSB()
{
  if (GLBSeriaPortBufferUSBReady){
    strcpy(GLBauxString,GLBSeriaPortBufferUSB);
    GLBSeriaPortBufferUSB[0]=0;
    GLBSeriaPortBufferUSBReady = false;
    ls.processCommands(GLBauxString);
  }
}

//-------------------------------------------------
void processSeriaPortBufferBT()
{
  if (GLBSeriaPortBufferBTReady){
    strcpy(GLBauxString,GLBSeriaPortBufferBT);
    GLBSeriaPortBufferBT[0]=0;
    GLBSeriaPortBufferBTReady = false;  
    ls.processCommands(GLBauxString);
  }
}



//-------------------------------------------------
//-------------------------------------------------
//-------------------------------------------------

unsigned int fastled=0;


void loop() { 
  GLBCrazyLoopCounter++; 

  // State machine as main controller execution
  if (GLBhelloIndex < HELLO_MESSAGES)
  {
    if (ls.isIdle())
    {
      Serial.print(F("Halo:"));
      Serial.println(GLBhelloIndex);
      Serial.println(bootx[GLBhelloIndex]); 
      ls.processCommands((char*)bootx[GLBhelloIndex]);
      GLBhelloIndex++; 
      //GLBhelloIndex=(GLBhelloIndex+1)%HELLO_MESSAGES; //TO FORCE INFINITE LOOP
    }
  }

  GLBtimers.run();

  processSeriaPortBufferUSB();
  serialEventBT();
  processSeriaPortBufferBT();
  ls.refresh();

  if (GLBtimerBTActivity==-1) { 
    // No rx bt activity, lets call fastled aggresively
    FastLED.show(); 
  } 
  else { 
    // rx bt activity, lets call fastled less aggresively, it disable interrupts 30us per pixel
    fastled++;
    if ((fastled%1000)==0) FastLED.show();
  }
} 

