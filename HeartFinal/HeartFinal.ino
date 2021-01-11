/* This is latest candidate production code.  Started as halloween.  This
 * is the latest, cleaned up version.  Halloween stuff removed.
 * Added a few new animations after Christmas 2020, and rearranged order.
 * Lots of old leftovers not purged, sort of as examples/reminders.  
 * The memory stuff is no longer relevant.
 */
 
#include <Adafruit_NeoPixel.h>
#include <jim.h>
#include <MemoryFree.h>
#include "RTClib.h"
#include <Dusk2Dawn.h>

RTC_DS3231 rtc; // RTC is running CDT
// TIMESTAMP_FULL // YYYY-MM-DDTHH:MM:SS
DateTime now;  // this is from RTClib
Dusk2Dawn Elmhurst(41.87, -87.95, -6);  // -6 is CST; see checktime()

// I think this is for original 32U4 before RTC, so now not used
//#define PROD
#ifdef PROD
#define PIN A2
#define DEMOPIN1 A0
#define DEMOPIN2 A1
#endif

// Define ALWAYSON so heart doesn't turn off at sunset
#define ALWAYSON
// turn on for production pro mini
#define RTC
// new Arduino with RTC uses A3 for data, 8,9 for demo jumper, 3 to power RTC
#ifdef RTC
#define PIN A3
#define DEMOPIN1 8
#define DEMOPIN2 9
#define RTCPOWER 3
#endif

// turn this one on for dev heart
//#define WEMOS
#ifdef WEMOS
#define PIN 2
#define DEMOPIN1 A0
#define DEMOPIN2 A1
#endif

// 207 for prod, 208 for dev???
//#define NUMPIXELS 207
#define NUMPIXELS 208

// MAXBRIGHT 255 for prod, 50 for dev
#define MAXBRIGHT 255
//#define MAXBRIGHT 50

// for simple reference to colors
// the bCOLOR one are for bubble sort video
#define BLUE 0x000000FFL
#define bBLUE 0x0000000FL
#define GREEN 0x0000FF00L
#define DIMGREEN 0x00004400L
#define RED 0x00FF0000L
#define bRED 0x000F0000L
#define CYAN 0x00007F7FL
#define LIGHTCYAN 0x00003030L
#define YELLOW 0x007F7F00L
#define MAGENTA 0x007F007FL
#define WHITE 0x00909061L
#define DIMWHITE 0x00606051L
#define bWHITE 0x00050505L
#define BLACK 0x00000000L
#define ORANGE 0x00ff2000L
#define PURPLE 0x00f8008aL
uint32_t basecolors[]={RED,GREEN,BLUE,CYAN,YELLOW,MAGENTA,BLACK};
int NUMCOLORS=sizeof(basecolors)/sizeof(long)-1;  // black is a special case
bool dodemo; // for demo mode jumper A0-A1


// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// gotta declare these for func ptr array :(
  void happy(unsigned long),jcolorwipe(unsigned long),
    rainbowCycle(unsigned long),rwb1(unsigned long),rwb2(unsigned long),
    rwb3(unsigned long),rwb4(unsigned long),jrbow1(unsigned long),
    jrbow2(unsigned long),race(unsigned long),bubble(unsigned long),rockets(unsigned long),
    rwb5(unsigned long),rwb6(unsigned long);

// animations will appear in this order
  void (*funcptrs[])(unsigned long)={happy,rwb1,jcolorwipe,rwb2,
      rainbowCycle,rwb5,jrbow1,rwb6,jrbow2,race,bubble,rockets};
  // names of animations for debug display
  // could use PROGMEM for this if desperate for RAM
  char fnames[][13]={"happy","jcolorwipe","rainbowCycle","rwb1","rwb2",
    "rwb3","rwb4","jrbow1","jrbow2","race","bubble","rockets"};
      
  int numfuncs=sizeof(funcptrs)/sizeof(void (*));
  uint32_t color;// general purpose  // so why global?
  int maxbright=MAXBRIGHT;  // in case a func needs to mess with it
  int minfreeram;   // keep track of how close to getting into trouble
  int half=NUMPIXELS/2;
  bool flip=0;  // which half is purple
  
 // This is what allows treating LEDs as if in X-Y plane
 //PROGMEM const int xsort[]={153, 154, 152, 155, 151, 156, 150, 157, 149, 158, 148, 159, 147, 160, 146, 161, 145, 162, 144, 163, 143, 164, 142, 165, 141, 166, 140, 167, 139, 168, 169, 138, 170, 137, 171, 136, 172, 135, 173, 174, 134, 175, 133, 176, 132, 177, 178, 131, 179, 130, 180, 181, 129, 182, 128, 183, 184, 127, 185, 126, 186, 125, 187, 188, 124, 189, 123, 190, 122, 191, 192, 121, 193, 120, 194, 119, 195, 118, 196, 117, 197, 116, 198, 115, 199, 114, 200, 113, 201, 112, 202, 111, 203, 110, 204, 109, 108, 205, 107, 106, 206, 105, 0, 103, 104, 207, 102, 101, 1, 100, 2, 99, 98, 3, 97, 4, 96, 5, 95, 6, 94, 7, 93, 8, 92, 91, 9, 10, 90, 11, 89, 12, 88, 13, 87, 14, 86, 15, 16, 85, 17, 84, 18, 83, 19, 20, 82, 21, 81, 22, 80, 23, 24, 79, 25, 78, 26, 27, 77, 28, 76, 29, 30, 75, 31, 74, 32, 73, 33, 34, 72, 35, 71, 36, 70, 37, 69, 38, 39, 68, 40, 67, 41, 66, 42, 65, 43, 64, 44, 63, 45, 62, 46, 61, 47, 60, 48, 59, 49, 58, 50, 57, 51, 56, 52, 55, 53, 54,54};
 PROGMEM const int xsort[]={153, 154, 152, 155, 151, 156, 150, 157, 149, 158, 148, 159, 147, 160, 146, 161, 145, 162, 144, 163, 143, 164, 142, 165, 141, 166, 140, 167, 139, 168, 169, 138, 170, 137, 171, 136, 172, 135, 173, 174, 134, 175, 133, 176, 132, 177, 178, 131, 179, 130, 180, 181, 129, 182, 128, 183, 184, 127, 185, 126, 186, 125, 187, 188, 124, 189, 123, 190, 122, 191, 192, 121, 193, 120, 194, 119, 195, 118, 196, 117, 197, 116, 198, 115, 199, 114, 200, 113, 201, 112, 202, 111, 203, 110, 204, 109, 108, 205, 107, 106, 206, 105, 0, 103, 104, 207, 102, 101, 1, 100, 2, 99, 98, 3, 97, 4, 96, 5, 95, 6, 94, 7, 93, 8, 92, 91, 9, 10, 90, 11, 89, 12, 88, 13, 87, 14, 86, 15, 16, 85, 17, 84, 18, 83, 19, 20, 82, 21, 81, 22, 80, 23, 24, 79, 25, 78, 26, 27, 77, 28, 76, 29, 30, 75, 31, 74, 32, 73, 33, 34, 72, 35, 71, 36, 70, 37, 69, 38, 39, 68, 40, 67, 41, 66, 42, 65, 43, 64, 44, 63, 45, 62, 46, 61, 47, 60, 48, 59, 49, 58, 50, 57, 51, 56, 52, 55, 53, 54};
 PROGMEM const int ysort[]={0, 207, 1, 206, 205, 2, 204, 3, 203, 4, 202, 5, 201, 6, 200, 7, 199, 8, 198, 9, 10, 197, 11, 196, 12, 195, 13, 194, 14, 193, 15, 192, 16, 191, 17, 190, 18, 189, 188, 19, 187, 20, 186, 21, 185, 22, 184, 23, 183, 24, 182, 25, 181, 26, 180, 27, 179, 28, 178, 29, 177, 30, 176, 31, 175, 32, 174, 33, 173, 34, 172, 35, 171, 36, 170, 37, 169, 38, 168, 39, 167, 40, 166, 41, 165, 42, 164, 43, 163, 44, 162, 45, 161, 46, 160, 47, 159, 48, 158, 49, 157, 50, 156, 51, 155, 52, 154, 53, 153, 54, 152, 55, 151, 56, 150, 57, 149, 58, 148, 59, 147, 60, 146, 61, 145, 62, 144, 63, 103, 104, 143, 64, 102, 105, 142, 65, 101, 106, 141, 66, 100, 107, 140, 67, 108, 99, 109, 139, 68, 98, 110, 97, 138, 69, 111, 96, 137, 70, 112, 95, 136, 71, 113, 94, 135, 72, 114, 93, 134, 73, 115, 92, 133, 74, 116, 91, 132, 75, 117, 90, 118, 131, 76, 89, 119, 88, 130, 77, 120, 87, 129, 78, 121, 86, 122, 85, 128, 79, 123, 84, 124, 127, 80, 83, 125, 126, 81, 82,82};
 
// This is the storage for any func that needs a bunch of objects, like the racer
// or fireworks.  Also blood drops or purple blobs.
 #define MAXOBJ 12
 // global object storage
  struct block{
    bool live;
    byte loc;
    int  age;
    byte size;
    int speed;
    uint32_t color;
  };
  struct block objs[MAXOBJ];



//*********** SETUP *******************
void setup() {
#ifdef RTC
  pinMode(RTCPOWER,OUTPUT);
  digitalWrite(RTCPOWER,HIGH);  // low current power to RTC
#endif
  pinMode(DEMOPIN1,OUTPUT);   // run demo mode if open at boot
  digitalWrite(DEMOPIN1,LOW);
  pinMode(DEMOPIN2,INPUT_PULLUP);
  dodemo=digitalRead(DEMOPIN2)?TRUE:FALSE;  // remove jumper for demo mode
  char timestr[6];

  delay(1000);
  blink(1);
// MUST BE IFDEF'D OUT FOR LEONARDO!
#ifndef __AVR_ATmega32U4__ 
  Serial.begin(57600);
  blink(2);
  while(!Serial);
  sp("\n");
  if(dodemo){sp("demo ");}
  spl("hello new");
  blink(3);
#endif

#ifdef RTC
    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    }else{spl("found RTC");}
    if (rtc.lostPower()) {
      spl("RTC lost power");
    }
    now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    sp("  ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
#else // hard code something
  // 10AM should pass checktime() WRONG: now ifdef around checktime
  now=DateTime((uint16_t)2020,(uint8_t)6,(uint8_t)1,(uint8_t)10,(uint8_t)0,(uint8_t)0);
#endif // RTC

    int risemins=Elmhurst.sunrise(now.year(),now.month(),now.day(),TRUE);
    int setmins=Elmhurst.sunset(now.year(),now.month(),now.day(),TRUE);
    Dusk2Dawn::min2str(timestr,risemins);
    sp2("Sunrise mins:",risemins);sp2("=",timestr);
    Dusk2Dawn::min2str(timestr,setmins);
    sp2("  Sunset mins:",setmins);spl2("=",timestr);


  strip.begin();
  blank(0); // all set to off; set to maxbright for next thing
sp("NUMPIXELS=");sp(NUMPIXELS);
sp("  strip.numPixels()=");spl(strip.numPixels());

  int xssize=sizeof(xsort)/sizeof(int);
  int yssize=sizeof(ysort)/sizeof(int);
  sp2("xxsize:",xssize);
  spl2("  yxsize:",yssize);
  minfreeram=freeRam();
  spl2("start free ram=",minfreeram);
  spl2("fm:",freeMemory());

  for(int i=0;i<MAXOBJ;i++)objs[i].live=FALSE;


#if 0
  cloop();
  while(1);
#endif

// strip.show() tester
// results: strip.show() takes ~5.3ms
#if 0
  unsigned long start=millis(),counter=0,x=0;
  for(int i=0;i<NUMPIXELS;i++)strip.setPixelColor(i,CYAN);
  strip.setBrightness(50);
  strip.show();
  while(millis()-start<5000){
    counter++;
    if(counter%10==0){
      x++;
      strip.show();
    }//end if
  }//end while
  strip.setBrightness(0);
  strip.show();
  sp("counter: ");
  sp(counter);
  sp("  x: ");spl(x);
  while(1);
#endif

}//end setup()



//=========== MAIN LOOP ===============
// The usual way to run only one animation while writing/debugging
// was to use #if 1.  When done, they were set to #if 0 and left there.
void loop() {
  unsigned long wipespeed=10;
  unsigned long ws2=10;
  static int fnum=0;
  randomSeed(analogRead(A0));
  bool islit=TRUE;  // state of heart

//------- utility marking absolute addrs in red every 10 (cyan at 5)
#if 0
for(int i=0;i<NUMPIXELS;i+=10){
    strip.setPixelColor(i,RED);
    strip.setPixelColor(i+5,LIGHTCYAN);
} //end for
  strip.setPixelColor(105,GREEN);  //verifying center o fnotch
  strip.show();
  while(1);
  delay(10000);
#endif

#if 0
  sp("lighting last pixels 2...");//getting final count of LEDs in dev heart
  strip.setPixelColor(200,RED);
  strip.setPixelColor(201,GREEN);
  strip.setPixelColor(202,BLUE);
  delay(10);
  strip.show();
  delay(50);
  //strip.show();
  spl(" done");
  while(1);
  delay(4000);
#endif

#if 0
  while(1){
  race(10L);
  }
#endif

#if 0
  while(1){
    bubble(20L);
  //race(wipespeed);
  }//end while
#endif

#if 0
while(1){
  rockets(10L);
}//end while
#endif

#if 0
  while(1){
  //rainbow(50L);
  //blank(300);
  rainbowCycle(10L);
  blank(300);
  }//end while
#endif

#if 0
  while(1){
    //blank(50);
    //spl("rwb1");
    rwb3(10L);
/*    delay(300);
    //blank(50);
    spl("rwb2");
    rwb2(10L);
    delay(300);
    //blank(50);
    spl("rwb3");
    rwb3(10L);
    delay(300);
    //blank(50);
    spl("rwb4");
    rwb4(10L);
    delay(300);
*/  }//end while
#endif

#if 0
  while(1){
//spl("rwb6");
    rwb5(10L);
    delay(3000);
    blank(200);
    rwb6(10L);
    delay(3000);
    blank(200);
  }//end while 1
#endif



  //========== MAIN PRODUCTION LOOP ============
  sp("numfuncs=");spl(numfuncs);
    strip.setBrightness(maxbright);
// demo: run thru animations only, in order (plus a little fade)
  if(dodemo){
    while(1){for(int i=0;i<numfuncs;i++)     
      (funcptrs[i])(wipespeed);
      idle(32,10000);
      }//end while
    }//end if dodemo
    else{
// production: now runs animations in order instead of random order
    while(1){
#ifndef RTC
      islit=TRUE;
#else
      islit=checktime();  // should we be on
#endif
      if(!islit){
        for(int i=0;i<NUMPIXELS;i++)strip.setPixelColor(i,0L);
        strip.show();
        continue;
      }
      // main slow color fade
      idle(300,random(30000,40000));  // main production line
      sp2("func ",fnum);spl2(" ",fnames[fnum]);
      blank(50);
      // do next animation  
      (funcptrs[fnum])(wipespeed);
      if(++fnum>(numfuncs-1))fnum=0; // now goes in order
    }//end production while
  }// end else production, not demo
}//end loop
//=====================================


//----- MAIN IDLE ROUTINE - SLOW COLOR CHANGE
void idle(unsigned long wipespeed,long dur){
  static uint8_t idlecol=85;
  uint32_t color;
  unsigned long start=millis();
  while((millis()-start)<dur){
    color=Wheel(idlecol++);
    for(int i=0;i<NUMPIXELS;i++){
      strip.setPixelColor(i,color);
      }// end for
      strip.show();
      delay(wipespeed);
    }//end while
  }//end idle


//------- FREE RAM SNAPSHOT -----
int freeRam () 
{
extern int __heap_start, *__brkval; 
int v;
long addr;
addr=(long)&v; 
spl2("&v=",addr);
return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}// end freeRam

//------ FREE RAM CHECK -----
void checkRam(){
  int ram=freeRam();
  if(ram<minfreeram){
    minfreeram=ram;
    spl2("free ram down to ",ram);
  }//end if
    spl2("free ram is ",ram);
}//end checkram()

//---------- NEOPIXEL UTILITIES -------
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}//end Wheel

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }//end for
}// end colorWipe

void Adafruit_NeoPixel::setBrightness(uint8_t pixnum, uint8_t b) {
  // hacked from original to change just one pixel - JW
  // Stored brightness value is different than what's passed.
  // This simplifies the actual scaling math later, allowing a fast
  // 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
  // adding 1 here may (intentionally) roll over...so 0 = max brightness
  // (color values are interpreted literally; no scaling), 1 = min
  // brightness (off), 255 = just below max brightness.
    uint8_t  c, *ptr = pixels;
    b += 1;
    ptr+=(3*pixnum);
    for(int i=0;i<3;i++){
      c = *ptr;
      *ptr++ = (c * b) >> 8;
    }//end for
}// end setBrightness

//------- utility: all LEDs off, delay, back to full bright for next
// Since setBrightness is destructive, LEDs will remain off after this.
// There is no show(), as there's nothing to show until next routine
// turns some LEDs on.
void blank(unsigned long darktime){
static unsigned long int d=0;
  strip.setBrightness(0);
  //for(int i=0;i<NUMPIXELS;i++)strip.setPixelColor(i,0L);
  //delay(d+=2);if(d>20)d=0;
  //spl(d);
  strip.show();
  delay(darktime);
  strip.setBrightness(maxbright);
}// end blank





//=============================================
//================= ANIMATIONS ================
//=============================================
// Set random small blobs to a random color
// for dur ms
#define SIZE 4
void happy(unsigned long int dur){
  int blob;
  unsigned long int start=millis();
  uint32_t color;
  dur*=800;
  blank(0);
  while(millis()-start<dur){
      blob=random(NUMPIXELS/SIZE);
      color=Wheel(random(255));
    for(int i=0;i<SIZE;i++){
      strip.setPixelColor(blob*SIZE+i,color);
   }//end for i
   strip.show();
   delay(10);
  }//end while
}// end happy


//------- nice color wipe set
void jcolorwipe(unsigned long wipespeed){
  blank(200);
  colorWipe(RED, wipespeed); // Red
  colorWipe(GREEN, wipespeed); // Green
  colorWipe(BLUE, wipespeed); // Blue
  //rainbow(20);
  blank(0);
  //rainbowCycle(15);
  //onerainbow(15);
  //delay(4000);
}//end jcolorwipe


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(unsigned long wait) {
  uint16_t i, j,k=3;
  for(j=0; j<256*k; j++) { // k cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}//end rainbowCycle


//----------- RWB side to side
void rwb1(unsigned long wipespeed){
  //------- RWB left->right, right->left
  blank(0);
  for (int i=NUMPIXELS-1;i>=0;i--){
  if(i<59)color=BLUE; //blue
  else if(i<137)color=WHITE;
  else color=RED;
  strip.setPixelColor(pgm_read_word(&xsort[i]),color);
  strip.show();
  delay(wipespeed);  
 }//end for
 delay(3000);
}//end rwb1


//------- RWB up->down
void rwb2(unsigned long wipespeed){
    //------- RWB top->bottom, bottom->top
  blank(0);
  for (int i=NUMPIXELS;i>=0;i--){
  if(i<58)color=BLUE;
  else if(i<122)color=WHITE;
  else color=RED;
  strip.setPixelColor(pgm_read_word(&ysort[i]),color);
  strip.show();
  delay(wipespeed);  
  }//end for
  delay(3000);
}//end rwb2


void rwb3(unsigned long wipespeed){
  blank(0);
  for (int i=0;i<NUMPIXELS;i++){
  if(i<59)color=BLUE;
  else if(i<137)color=WHITE;
  else color=RED;
  strip.setPixelColor(pgm_read_word(&xsort[i]),color);
  strip.show();
  delay(wipespeed);  
 }//end for
 delay(3000);
}// end rwb3


void rwb4(unsigned long wipespeed){
  blank(0);
  for (int i=0;i<=NUMPIXELS;i++){
   if(i<58)color=BLUE;
    else if(i<122)color=WHITE;
   else color=RED;
   strip.setPixelColor(pgm_read_word(&ysort[i]),color);
   strip.show();
   delay(wipespeed);  
   }//end for
 delay(3000);

}//end rwb4

// one-by-one turn on
#define P1 30
#define P2 60
#define P3 150
#define P4 180
void rwb5(unsigned long wipespeed){
  int i;
  unsigned long color;
  bool done=FALSE;
  blank(0);
//for(i=0;i<NUMPIXELS;i++)strip.setPixelColor(i,CYAN);
//  strip.show();
//  delay(200);
//  blank(0);
  while(!done){
    done=TRUE;
    // check if all pixels are lit
    for(i=0;i<NUMPIXELS;i++){
      if(strip.getPixelColor(i)==BLACK){
        done=FALSE;
        //break;
      }//end if
    }// end for  
    // might execute code below one extra time
    // boy this is wasteful!
    do{i=random(0,NUMPIXELS+1);}
      while(strip.getPixelColor(i)!=BLACK);
    // finally have next black pixel 
    if(i<P1||i>P4)color=BLUE;
      else if((i>=P1&&i<=P2)||(i<=P4&&i>=P3))color=WHITE;
        else color=RED;
    strip.setPixelColor(i,color);
    strip.show();
    delay(wipespeed*3);
  }//end while
  delay(3000);
    
  }// end rwb5


// RWB sequential from peak
#define P5 20
#define P6 83
#define P7 127
#define P8 185
void rwb6(unsigned long wipespeed){
  int i,j;
  unsigned long color;
  bool done=FALSE;
  blank(0);
  for(i=0;i<NUMPIXELS;i++){
    j=half+i;
    if(j>NUMPIXELS)j-=NUMPIXELS;
    if(j>P5&&j<P6)color=RED;
      else if((j>=P7&&j<=P8))color=BLUE;
        else color=WHITE;
    strip.setPixelColor(j,color);
    strip.show();
    delay(wipespeed);
  }//end for
  delay(3000);
    
  }// end rwb6


//------ attempt at rainbows
void jrbow1(unsigned long wipespeed){
  //----------- L->R rainbow
   blank(0);
  for(int i=NUMPIXELS-1;i>=0;i--){
    color=Wheel((170-(240L*i)/NUMPIXELS)&255);
    strip.setPixelColor(pgm_read_word(&xsort[i]),color);
    strip.show();
    delay(wipespeed);
  }//end for
  delay(4000);
  blank(0);
}//end jrbow1


void jrbow2(unsigned long wipespeed){
//----------- T->B rainbow
  blank(0);
  for(int i=NUMPIXELS-1;i>=0;i--){
    color=Wheel((170-(240L*i)/NUMPIXELS)&255);
    strip.setPixelColor(pgm_read_word(&ysort[i]),color);
    strip.show();
    delay(wipespeed);
  }//end for
  delay(4000);
  blank(0);
}// end jrbow2


//--------- racing pixels ---------
// sets cnt pixels racing around for dur ms
// now ends gracefully when all have 'fallen out' of the bottom
#define RDURATION 10000L
void race(unsigned long int spd){
  int i,cnt;
  unsigned long startms;
  struct block *pix=objs;
  bool anyleft,die;  //exit condition for update loop
  byte *locptr; // pointer to loc for speed
  checkRam();
  spl2("fm in race:",freeMemory());

  blank(100);
  cnt=random(6,MAXOBJ+1);
  spl2("racing ",cnt);
  //---- create racers ---
  for(i=0;i<cnt;i++){
    pix[i].loc=random(0,NUMPIXELS);
    pix[i].size=1;
    pix[i].live=TRUE;
    pix[i].speed=random(2)?random(1,4):-random(1,4);
    pix[i].color=basecolors[random(NUMCOLORS)];
    strip.setPixelColor(pix[i].loc,pix[i].color);
  }//end for
  strip.show();

  startms=millis();
  //----- update racers -----
  do{
    anyleft=FALSE;
    die=(millis()-startms)<RDURATION; // die means don't go thru bottom
    for(i=0;i<cnt;i++)strip.setPixelColor(pix[i].loc,0L); // blank all
    for(i=0;i<cnt;i++){
      if(pix[i].live){
        anyleft=TRUE;
        locptr=&pix[i].loc;
        (*locptr)+=pix[i].speed;
        if(*locptr>(NUMPIXELS-1)){
          *locptr-=NUMPIXELS;
          pix[i].live=die;
          }
        else if(*locptr<=0){
          *locptr+=NUMPIXELS;
          pix[i].live=die;
          }
        if(pix[i].live)strip.setPixelColor(*locptr,pix[i].color);
      }//end if live
    }//update for
    strip.show();
    delay(spd*3);
  }//end do while
  while(anyleft);
}//end race()


// DIMBUBBLES is for video
//#define DIMBUBBLES
//-------- BUBBLE SORT -------------
void bubble(unsigned long speed){
  int i,j;
#ifdef DIMBUBBLES
  long rwb[]={bRED,bWHITE,bBLUE},col;
#else
  long rwb[]={RED,WHITE,BLUE},col;
#endif
  unsigned long bubdel;
  bool ldone,rdone;
  blank(100);
  strip.setBrightness(255);
  sp("bubbles  ");
  //------- spew RWB bubbles
  for(i=1;i<=half;i+=2){  // how close to full are we
    for(j=0;j<=i;j+=2){  //populate this round's pixels
      strip.setPixelColor(half+j,  rwb[(i-j)%3]);
      strip.setPixelColor(half+j+1,rwb[(i-j)%3]);
      strip.setPixelColor(half-j,  rwb[(i-j)%3]);      
      strip.setPixelColor(half-j-1,rwb[(i-j)%3]);      
    }//end for j
    //strip.setPixelColor(105,rwb[1]);  //hack
    strip.show();
    delay(speed*6);
  }//end for i
    //strip.setPixelColor(106,rwb[1]);  //hack
    strip.setPixelColor(0,rwb[0]);
    strip.show();//hack
  delay(800);
  spl("done filling");
  delay(100);

/*
#define NUMPERROW 10
  i=0;
  while(i<NUMPIXELS){
    for(j=0;j<NUMPERROW;j++){sp2(i+j,"=");sp2(strip.getPixelColor(i+j)," ");delay(2);}
    spl();
    delay(100);
    i+=NUMPERROW;
}//end while

while(1);
*/

for (i=0;i<210;i++){
  uint32_t c;
  c=strip.getPixelColor(i);
  if(c==0){sp2(i,"=");spl(c);}
}

//while(1);

  //------- bubble sort! -------
  bubdel=speed*18;
  sp("starting sort...");
    int iter=0;
    int diff;
    uint32_t c1,c2;
    int randval=4;
  do{
    ldone=TRUE;
    rdone=TRUE;
    // assumes pixel i+1 same as i
    //for(i=0;i<NUMPIXELS;i+=random(1,randval)){ // make it a little more random looking?
    for(i=0;i<NUMPIXELS;i+=2*random(1,randval)){ // make it a little more random looking?
      c1=strip.getPixelColor(i);
      c2=strip.getPixelColor(i+2);
      if(c2==0)c2=c1; // crude
      diff=(c1&3)-(c2&3);
      if(i<(half)){ // left half of heart
        if(diff<0){
          strip.setPixelColor(i,c2);
          strip.setPixelColor(i+1,c2);
          strip.setPixelColor(i+2,c1);
          strip.setPixelColor(i+3,c1);
          ldone=FALSE;       
          }// end if diff<0
      }else{      //right half of heart
        if(diff>0){
          strip.setPixelColor(i,c2);
          strip.setPixelColor(i+1,c2);
          strip.setPixelColor(i+2,c1);
          strip.setPixelColor(i+3,c1);
          rdone=FALSE;
          }
      }//end else
    }//end for
    strip.show();
    delay(bubdel);
    iter++;
    // final cleanup, since random can skip some
    if(randval!=2 && ldone && rdone){
      randval=2;
      rdone=FALSE;
      spl("setting randval to 2");
    }
  }while(!(ldone&rdone));
  spl2("done  iterations:",iter);
  strip.setBrightness(127);
  strip.show();
  delay(30);
  strip.setBrightness(255);
  strip.show();
  delay(5000);
  strip.setBrightness(MAXBRIGHT);
}//end bubble


//--------- ROCKETS -----
#define ROCKETSIZE 5
#define MAXSPARK 8
#define SPARKDUR 10000
void rockets(unsigned long spd){
  int i;
  unsigned long starttime,lasttime,currmillis,timetilnext;
  struct block *sparks=objs;   // point to global storage
  for(i=0;i<MAXOBJ;i++)sparks[i].live=FALSE;  // init all to dead
  blank(300);
  // make rockets
  for(i=0;i<ROCKETSIZE;i++){
    strip.setPixelColor(i,WHITE);
    strip.setPixelColor(NUMPIXELS-i,WHITE);
    strip.show();
    delay(50);
  }//end for

// send rockets up
  for(i=ROCKETSIZE;i<half;i++){
    strip.setPixelColor(i,WHITE);
    strip.setPixelColor(NUMPIXELS-i,WHITE);
    //strip.setPixelColor(i-ROCKETSIZE+5,DIMWHITE); // fading exhaust?
    //strip.setPixelColor(NUMPIXELS-(i-ROCKETSIZE+5),DIMWHITE);
    strip.setPixelColor(i-ROCKETSIZE,0L);
    strip.setPixelColor(NUMPIXELS-(i-ROCKETSIZE),0L);
    strip.show();
    delay(40);
  }//end for 
  for(i=half;i<half+2;i++){  //eat rockets at notch
    strip.setPixelColor(i-ROCKETSIZE,0L);
    strip.setPixelColor(NUMPIXELS-(i-ROCKETSIZE),0L);
    strip.show();
    delay(50);
  }//end for 

  //---- initial fireball -----
    blank(2);
    starttime=currmillis=millis();
    while((currmillis-starttime)<1000){
      currmillis=millis();
    for(i=half-1;i<half+2;i++){
      strip.setPixelColor(i,basecolors[(int)random(NUMCOLORS+2)]); // incl black   
      }// end for  
      strip.show();
      delay(30);
    }// ene while

//explosions
  int slot;
  bool needspark=TRUE,done=FALSE,nomoresparks=FALSE;
  int tmpage;
  starttime=millis();
  while(!nomoresparks){
    currmillis=millis();  //general purpose
    //---- create sparks ----
    if(needspark){
      slot=findslot();
      if(slot!=-1){   //got a slot
     //spl2("using slot ",slot);
        needspark=FALSE;
        sparks[slot].live=TRUE;
        sparks[slot].color=basecolors[random(NUMCOLORS)];
        sparks[slot].loc=half;
        sparks[slot].speed=(int)random(2)?random(1,4):-random(1,4);
        sparks[slot].age=(int)(random(20,90)/abs(sparks[slot].speed));
        strip.setPixelColor(sparks[slot].loc,sparks[slot].color);   
        timetilnext=random(100,300);
        lasttime=currmillis;
        }//end if got one
        else{spl("no slot");}
      }//end if needspark

    //------ update fireball-----
    if(!done){
    for(i=half-1;i<half+2;i++){
      strip.setPixelColor(i,basecolors[(int)random(NUMCOLORS+2)]); // incl black     
    }//end for fireball
    }//end if !done
    else
      {for(i=half-1;i<half+2;i++)strip.setPixelColor(i,0L);} //turn 'em off
    strip.show();

    
    //-------- update sparks------
    nomoresparks=TRUE;
    for(i=0;i<MAXOBJ;i++){
      if(!sparks[i].live)continue; //nothing to see here
      nomoresparks=FALSE;
      strip.setPixelColor(sparks[i].loc,0L);  // blank old spark
      if((sparks[i].age--)<=0){
        sparks[i].live=FALSE; //die here
      }else{  // still alive
        sparks[i].loc+=sparks[i].speed;
        strip.setPixelColor(sparks[i].loc,sparks[i].color);  // paint new loc
        tmpage=sparks[i].age;
        if(tmpage<6)strip.setBrightness(sparks[i].loc,(1<<(3+tmpage)));
      }//end else still alive
    }//end update for
    strip.show();

    //------ time for another? -------
    if(!done && (currmillis-lasttime)>timetilnext)needspark=TRUE;

    //------ time to be done? ----
    if((currmillis-starttime)>SPARKDUR)done=TRUE;

    delay(30);  // needs to scale with spd
  }// end while sparks
  delay(1000);  
}//end rockets


  // storage helper funcs
  // returns first available slot or -1 if none
  int findslot(){
    for(int i=0;i<MAXOBJ;i++)if(!objs[i].live)return i;
    return -1;
  }//end findslot


// Checks current time (in mins after midnight) against sunrise/sunset.
// Returns TRUE if heart should be on, FALSE if off
// Uses seat-of-the-pants offsets from actual computed rise/set.
// If we never reset RTC from DST, AND we always tell the sunrise/set
// funcs it's DST (which is sometimes a lie), I think it should work.
// Here are the hand-tweaked offsets that do about what I want.
#define AFTERSUNRISE 15
#define BEFORESUNSET 20
  bool checktime(){
    unsigned long current;
    static unsigned long lasttime;
#ifdef ALWAYSON
    return TRUE;
#endif
    // refresh global object 'now' once/min; should wrap OK
    current=millis();
    if((current-lasttime)> 60000L){
      now=rtc.now();
      lasttime=current;
    }//end if millis
    int years=now.year();
    int months=now.month();
    int days=now.day();
    // last param is isDST
    int risemins=Elmhurst.sunrise(years,months,days,TRUE);
    int setmins=Elmhurst.sunset(years,months,days,TRUE);
    int mins=now.minute()+60*now.hour();
    bool retval=FALSE; // for before sunrise
    if(mins>(risemins+AFTERSUNRISE)) retval=TRUE;
    if(mins>(setmins-BEFORESUNSET)) retval=FALSE;
//sp("checktime retval is ");spl(retval);
    return retval;
    }//end checktime


void blink(int num){
  for(int i=0;i<num;i++){
    digitalWrite(13,HIGH);
    delay(20);
    digitalWrite(13,LOW);
    delay(300);
  }//end for
}//end blink

//These two test funcs return a uint32_t with the passed
// color at new brightness.

uint32_t jbright(uint32_t color,uint8_t bright){
  uint32_t rcolor,gcolor,bcolor;
  uint32_t retval;
  if(bright==255)return(color);
  if(bright==0)return(0L);
  
  rcolor=((color & 0xff0000)*bright)>>8;
  gcolor=((color & 0x00ff00)*bright)>>8;
  bcolor=((color & 0x0000ff)*bright)>>8;
  retval=(bcolor&0xff)|(gcolor&0xff00)|(rcolor&0xff0000);
  return(retval);  
}// end jbright()

void pcolor(uint32_t color,bool cr){
  byte val;
  val=(color&0xff0000)>>16;
  sp(val);sps;
  val=(uint32_t)((color&0xff00)>>8);
  sp(val);sps;
  val=color&0xff;
  sp(val);sps;
  if(cr)spl();
}//end pcolor

// test print some colors
void cloop(){
  int i,j,o;
  uint32_t c;
  j=120;
  o=5;
  for(i=0;i<256;i+=j){
    c=Wheel(i+o);
    sp(i);sp(':');
    pcolor(c,TRUE);
  }//end for
}//end cloop
