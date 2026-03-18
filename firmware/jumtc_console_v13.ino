// Arduino Console: Flappy + Snake + Srv Recorder + Servo Ctrl
// Nano: UP=D4 DN=D5 RT=D3 LT=D6 SW=D2 POT=A0 OLED=A4/A5
// S1=D12 S2=D11 S3=D10 S4=D9
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

Adafruit_SSD1306 display(128,64,&Wire,-1);

#define BTN_UP     4
#define BTN_RIGHT  3
#define BTN_DOWN   5
#define BTN_LEFT   6
#define SWITCH_PIN 2
#define POT        A0
#define BATT_PIN   A1

// ---- Battery ----
byte lastBattPct=255;
byte getBattPct(){
  uint16_t sum=0;
  for(byte i=0;i<8;i++)sum+=analogRead(BATT_PIN);
  uint32_t vbat=(uint32_t)(sum>>3)*10000/1023;
  byte pct;
  if(vbat>=8000)pct=100;
  else if(vbat<=6800)pct=0;
  else pct=(byte)((vbat-6800)*100/1200);
  if(lastBattPct==255||pct>(lastBattPct+1)||(pct+1)<lastBattPct)
    lastBattPct=pct;
  return lastBattPct;
}

Servo servos[4];
const byte SRV_PINS[4]={12,11,10,9};

// ---- States ----
#define ST_MENU  0
#define ST_FCNT  1
#define ST_FLAP  2
#define ST_FOVR  3
#define ST_SNAK  4
#define ST_SOVR  5
#define ST_SMNU  6
#define ST_SMOD  7
#define ST_SPOT  8
#define ST_SBTN  9
#define ST_REC   10

byte gameState=ST_MENU;

// ---- Buttons ----
#define NB 5
#define IU 0
#define ID 1
#define IL 2
#define IR 3
#define IS 4
const byte BP[NB]={BTN_UP,BTN_DOWN,BTN_LEFT,BTN_RIGHT,SWITCH_PIN};
bool bS[NB]={0},bL[NB]={0},bR[NB]={0};
unsigned long bT[NB]={0};

void readBtns(){
  unsigned long n=millis();
  for(byte i=0;i<NB;i++){
    bool r=(digitalRead(BP[i])==LOW);
    if(r!=bR[i]){bR[i]=r;bT[i]=n;}
    if(n-bT[i]>=40){bL[i]=bS[i];bS[i]=bR[i];}
  }
}
bool bJ(byte i){return bS[i]&&!bL[i];}
bool bH(byte i){return bS[i];}
bool swOn(){return bS[IS];}

// ---- Timers ----
unsigned long tMn=0,tGm=0,tDr=0,tSw=0,tCd=0;
bool elapsed(unsigned long &t,unsigned int ms){
  if(millis()-t>=ms){t=millis();return 1;}return 0;
}

// ---- Menu ----
byte mCur=0;

// ---- Flappy ----
int bY=32,vel=0,pX=128,gY=20;
#define GAP 22
byte cX=0;
int fSc=0,fHi=0;
byte cdN=3;

// ---- Snake ----
#define CELL  4
#define COLS 32
#define ROWS 16
#define MXLEN 48
struct Pt{int8_t x,y;};
Pt snBody[MXLEN];
uint8_t snLen;
int8_t snDx,snDy,snNx,snNy;
Pt snFood;
uint16_t snSc,snHi;

// ---- Servo Recorder ----
// 8 keyframes × 4 servos = 32 bytes SRAM
#define REC_MAX 8
byte recAng[REC_MAX][4]; // [frame][servo] — stores 0-180 as byte
byte recCnt=0;           // frames recorded
byte recIdx=0;           // playback frame index
bool recPlay=false;      // playing back?
byte recSrv=0;           // servo being previewed while recording

// ---- Servo ----
byte sIdx=0,sMode=0;
int  sAng=0,swAng=0,swDir=1;
byte swSpd=2;

// ---- Helper: progress bar ----
void drawBar(int x,int y,int w,int h,int v,int vm){
  display.drawRect(x,y,w,h,WHITE);
  int f=map(constrain(v,0,vm),0,vm,0,w-2);
  if(f>0)display.fillRect(x+1,y+1,f,h-2,WHITE);
}

// ---- Helper: inverted title bar ----
void titleBar(const __FlashStringHelper* t,byte cx){
  display.fillRect(0,0,128,11,WHITE);
  display.setTextColor(BLACK);display.setTextSize(1);
  display.setCursor(cx,2);display.print(t);
  display.setTextColor(WHITE);
}


const uint8_t PROGMEM mc8rnx_logo[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE1, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xE1, 0xEF, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xDE, 0x80, 0x42, 0x4E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xCC, 0x00, 0x00, 0x0C, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x80, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x03, 0xFC, 0x00, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x8C, 0x00, 0x07, 0xFF, 0xC0, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x03, 0xF8, 0x04, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x1F, 0xF8, 0x38, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0E, 0x03, 0x81, 0xFF, 0xFF, 0xC1, 0x81, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0F, 0x07, 0x0F, 0xFF, 0xFF, 0xF9, 0xE0, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x0C, 0x3F, 0xF0, 0x1F, 0xFE, 0x70, 0x60, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x78, 0x18, 0x7F, 0x01, 0xC1, 0xFF, 0x30, 0x06, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x30, 0x01, 0xF8, 0x0F, 0xF8, 0x3F, 0xC0, 0x18, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x10, 0x03, 0xE5, 0x1F, 0xFC, 0x0F, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0xE0, 0x07, 0x83, 0xA7, 0xF6, 0xE7, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x60, 0x07, 0x2B, 0xEF, 0xFB, 0xE1, 0xF8, 0x07, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x1B, 0xDF, 0xFD, 0xE0, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x02, 0x1E, 0x33, 0x9F, 0xFC, 0xE0, 0xED, 0xC0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x3C, 0x7D, 0x3F, 0xFE, 0xDF, 0x7C, 0xC0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x06, 0x3C, 0x6C, 0x3F, 0xFE, 0x1B, 0x3E, 0xC0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x06, 0x3C, 0x5D, 0x1F, 0xF8, 0x5D, 0x3E, 0xE1, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x03, 0x06, 0x78, 0xFE, 0x03, 0xE0, 0x7F, 0xBE, 0xE1, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x00, 0x79, 0x83, 0x87, 0xF0, 0xE0, 0xFE, 0x60, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x0C, 0x20, 0xDC, 0xC3, 0xF1, 0x9D, 0xB2, 0x00, 0xC0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x0E, 0x30, 0x3E, 0x7F, 0xFF, 0x3E, 0x36, 0x00, 0x40, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x02, 0x0C, 0x78, 0xFE, 0xFF, 0xFF, 0xBF, 0x8F, 0x00, 0xC0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x06, 0x03, 0xF0, 0xFE, 0xFC, 0x1F, 0xBF, 0x8F, 0xE0, 0xC0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x03, 0xF3, 0xF0, 0xFF, 0xFF, 0x87, 0xE7, 0xE0, 0xC0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x03, 0x03, 0x3B, 0x00, 0xC0, 0x01, 0x80, 0x7E, 0x40, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x03, 0x07, 0x7F, 0xF0, 0x00, 0x00, 0x07, 0xFF, 0xF0, 0xC0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x07, 0xFF, 0xE0, 0x07, 0xF0, 0x03, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x87, 0xE4, 0x80, 0x3F, 0xFC, 0x00, 0xB3, 0x70, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x80, 0xE3, 0x00, 0x7F, 0xFF, 0x06, 0xE3, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x38, 0x0E, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x1F, 0xFC, 0x0F, 0x9C, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x3E, 0x07, 0xF0, 0x1E, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x60, 0x3E, 0x00, 0x1F, 0xE0, 0x00, 0xFC, 0xF0, 0xFC, 0x06, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x10, 0xFC, 0x00, 0x07, 0xFF, 0xFF, 0xF3, 0xC1, 0xFE, 0x09, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x0C, 0x08, 0x7F, 0xC0, 0x00, 0xFF, 0xFF, 0x8F, 0x83, 0xFE, 0x13, 0x60, 0x00, 0x00,
  0x00, 0x00, 0x10, 0xE8, 0x1F, 0xF0, 0x18, 0x06, 0x00, 0x0E, 0x07, 0xFC, 0x26, 0x68, 0x00, 0x00,
  0x00, 0x00, 0x17, 0x24, 0x0F, 0xFC, 0x1F, 0x80, 0x07, 0xC0, 0x1B, 0xD8, 0x60, 0xC8, 0x00, 0x00,
  0x00, 0x00, 0x09, 0x0F, 0x0D, 0xDC, 0x01, 0xE0, 0x0F, 0x80, 0x1D, 0xC0, 0x9C, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x06, 0x78, 0x81, 0xCD, 0xE0, 0x00, 0x00, 0x03, 0x0F, 0xC1, 0x11, 0x20, 0x00, 0x00,
  0x00, 0x00, 0x03, 0xA2, 0x41, 0xF3, 0xF4, 0x00, 0x00, 0x3F, 0x87, 0x82, 0xD1, 0x40, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xC3, 0xB0, 0x77, 0xFF, 0x90, 0x04, 0x7D, 0xD8, 0x0D, 0xCD, 0x80, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x99, 0xE8, 0x07, 0xEF, 0xFB, 0xFC, 0xE0, 0xF8, 0x36, 0x49, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x6F, 0x28, 0x07, 0xFF, 0xFF, 0x7C, 0x76, 0xE0, 0x62, 0x42, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x36, 0x49, 0x80, 0x9F, 0xBF, 0x3C, 0x7E, 0x01, 0x99, 0x2C, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x08, 0x5C, 0x60, 0x1B, 0xBF, 0x7E, 0x18, 0x0E, 0x9C, 0xD0, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x81, 0xE6, 0x00, 0x37, 0x76, 0x00, 0x18, 0xCE, 0x40, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x86, 0xC1, 0x00, 0x00, 0x00, 0x03, 0x18, 0x05, 0x80, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x70, 0x94, 0xE8, 0x00, 0x00, 0x74, 0x0C, 0xE2, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x24, 0xC8, 0xF7, 0xF9, 0x99, 0x26, 0x68, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0xB9, 0x90, 0x94, 0x80, 0xC9, 0xA5, 0x60, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x94, 0xF2, 0x4C, 0xC4, 0x80, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x90, 0x95, 0x12, 0x4A, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x82, 0x99, 0x12, 0x2D, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0xC3, 0x12, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void setup(){
  for(byte i=0;i<NB;i++)pinMode(BP[i],INPUT_PULLUP);
  for(byte i=0;i<4;i++)servos[i].attach(SRV_PINS[i]);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);
  display.clearDisplay();
  display.drawBitmap(0,0,mc8rnx_logo,128,64,WHITE);
  display.display();
  delay(3000);
  randomSeed(analogRead(A2));
  snHi=0;
  memset(recAng,90,sizeof(recAng));
}

void loop(){
  readBtns();
  switch(gameState){
    case ST_MENU: doMenu();  break;
    case ST_FCNT: doFCnt();  break;
    case ST_FLAP: doFlap();  break;
    case ST_FOVR: doFOvr();  break;
    case ST_SNAK: doSnake(); break;
    case ST_SOVR: doSOvr();  break;
    case ST_SMNU: doSMnu();  break;
    case ST_SMOD: doSMod();  break;
    case ST_SPOT: doSPot();  break;
    case ST_SBTN: doSBtn();  break;
    case ST_REC:  doRec();   break;
  }
}

// ======== MENU ========
void doMenu(){
  if(bJ(IU))mCur=(mCur==0)?3:mCur-1;
  if(bJ(ID))mCur=(mCur>=3)?0:mCur+1;
  if(bJ(IR)){
    switch(mCur){
      case 0: gameState=ST_SMNU; break;
      case 1: bY=32;vel=0;pX=128;gY=20;cX=0;fSc=0;cdN=3;tCd=millis();gameState=ST_FCNT; break;
      case 2: initSnake();gameState=ST_SNAK; break;
      case 3: recCnt=0;recIdx=0;recPlay=false;recSrv=0;gameState=ST_REC; break;
    }
    return;
  }
  if(!elapsed(tMn,80))return;
  byte bp=getBattPct();
  display.clearDisplay();

  // inverted title bar + battery
  display.fillRect(0,0,128,11,WHITE);
  display.setTextColor(BLACK);display.setTextSize(1);
  display.setCursor(2,2);display.print(F("JUMTC CONSOLE"));
  // % text right-aligned before battery icon, with gap
  // battery icon: 14px wide at x=113, nub at x=127
  // % text: max "100%" = 4chars*6=24px, so start at x=113-24-2=87
  display.setCursor(88,2);display.print(bp);display.print(F("%"));
  // battery icon — smaller: 13x7 at x=113,y=2
  display.drawRect(113,2,13,7,BLACK);  // shell
  display.fillRect(126,4,1,3,BLACK);   // nub
  byte bf=map(bp,0,100,0,11);
  display.fillRect(114,3,11,5,WHITE);  // white bg
  if(bf<11)display.fillRect(114+bf,3,11-bf,5,BLACK); // unfilled
  display.setTextColor(WHITE);

  // 4 rows, 13px each starting at y=12 — total 52px fits in 64-12=52 exactly
  const char* it[]={"Servo Ctrl","Flappy Bird","Snake","Srv Recorder"};
  for(byte i=0;i<4;i++){
    byte y=12+i*13;
    bool sel=(mCur==i);
    if(sel){display.fillRect(0,y,128,12,WHITE);display.setTextColor(BLACK);}
    else{display.drawRect(0,y,128,12,WHITE);display.setTextColor(WHITE);}
    uint16_t c=sel?BLACK:WHITE;
    // left arrow
    display.fillTriangle(3,y+2,3,y+9,8,y+5,c);
    // label size 1, vertically centred in 12px row (text=8px, pad=2px top)
    display.setTextSize(1);
    display.setCursor(14,y+2);display.print(it[i]);
    // right arrow on selected
    if(sel)display.fillTriangle(118,y+2,118,y+9,124,y+5,BLACK);
    display.setTextColor(WHITE);
  }
  display.display();
}

// ======== FLAPPY COUNTDOWN ========
void doFCnt(){
  if(millis()-tCd>=900){
    tCd=millis();
    if(cdN==0){tGm=millis();gameState=ST_FLAP;return;}
    cdN--;
  }
  display.clearDisplay();
  display.drawCircle(72,8,3,WHITE);display.drawCircle(32,16,2,WHITE);
  display.fillCircle(13,bY+2,3,WHITE);display.drawPixel(15,bY+1,BLACK);
  display.drawLine(0,62,127,62,WHITE);
  display.setTextSize(1);display.setCursor(0,0);display.print(F("S:0"));
  if(cdN>0){display.setTextSize(4);display.setCursor(54,16);display.print(cdN);}
  else{display.setTextSize(3);display.setCursor(28,20);display.print(F("GO!"));}
  display.display();
}

// ======== FLAPPY GAMEPLAY ========
void doFlap(){
  int fm=map(analogRead(POT),0,1023,75,20);
  if(!elapsed(tGm,fm))return;
  if(bH(IU))vel=-3;
  vel++;bY=constrain(bY+vel,1,55);
  pX-=(2+fSc/6);
  if(pX<-10){pX=128;gY=random(8,38);fSc++;}
  if(bY<=1||bY>=55||(pX<16&&pX>-8&&(bY<gY||bY>gY+GAP))){gameState=ST_FOVR;return;}
  display.clearDisplay();
  display.drawCircle((200-cX)%128,8,3,WHITE);display.drawCircle((160-cX)%128,16,2,WHITE);
  cX=(cX+1)%128;
  display.fillCircle(13,bY+2,3,WHITE);display.drawPixel(15,bY+1,BLACK);
  display.fillRect(pX,0,8,gY,WHITE);display.fillRect(pX,gY+GAP,8,64,WHITE);
  display.fillRect(pX-1,gY-4,10,4,WHITE);display.fillRect(pX-1,gY+GAP,10,4,WHITE);
  display.drawLine(0,62,127,62,WHITE);
  display.setTextSize(1);display.setCursor(0,0);display.print(F("S:"));display.print(fSc);
  display.display();
}

// ======== FLAPPY OVER ========
void doFOvr(){
  if(fSc>fHi)fHi=fSc;
  display.clearDisplay();
  titleBar(F("FLAPPY BIRD"),30);
  display.drawFastHLine(0,11,128,WHITE);
  display.setTextSize(2);display.setCursor(10,14);display.print(F("GAME OVER"));
  display.drawFastHLine(0,34,128,WHITE);
  display.setTextSize(1);
  display.setCursor(8,38);display.print(F("Score:"));display.print(fSc);
  display.setCursor(72,38);display.print(F("Best:"));display.print(fHi);
  if(fHi>0)drawBar(8,50,112,6,fSc,fHi);
  display.setCursor(35,57);display.print(F("RIGHT: menu"));
  display.display();
  if(bJ(IR))gameState=ST_MENU;
}

// ======== SNAKE ========
void snSpawnFood(){
  Pt f;bool ok;
  do{f.x=random(0,COLS);f.y=random(0,ROWS);ok=1;
    for(uint8_t i=0;i<snLen;i++)if(snBody[i].x==f.x&&snBody[i].y==f.y){ok=0;break;}
  }while(!ok);snFood=f;
}
void initSnake(){
  snLen=3;snDx=1;snDy=0;snNx=1;snNy=0;snSc=0;
  snBody[0]={COLS/2,ROWS/2};snBody[1]={COLS/2-1,ROWS/2};snBody[2]={COLS/2-2,ROWS/2};
  snSpawnFood();tGm=millis();
}
void doSnake(){
  if(bJ(IU)&&snDy!=1){snNx=0;snNy=-1;}
  if(bJ(ID)&&snDy!=-1){snNx=0;snNy=1;}
  if(bJ(IR)&&snDx!=-1){snNx=1;snNy=0;}
  if(bJ(IL)&&snDx!=1){snNx=-1;snNy=0;}
  if(bJ(IS)){gameState=ST_MENU;return;}
  if(!elapsed(tGm,map(analogRead(POT),0,1023,600,80)))return;
  snDx=snNx;snDy=snNy;
  Pt nh={(int8_t)(snBody[0].x+snDx),(int8_t)(snBody[0].y+snDy)};
  if(nh.x<0||nh.x>=COLS||nh.y<0||nh.y>=ROWS){gameState=ST_SOVR;return;}
  for(uint8_t i=0;i<snLen;i++)if(snBody[i].x==nh.x&&snBody[i].y==nh.y){gameState=ST_SOVR;return;}
  bool ate=(nh.x==snFood.x&&nh.y==snFood.y);
  uint8_t nl=ate?min((uint8_t)(snLen+1),(uint8_t)MXLEN):snLen;
  for(int8_t i=nl-1;i>0;i--)snBody[i]=snBody[i-1];
  snBody[0]=nh;snLen=nl;
  if(ate){snSc++;snSpawnFood();}
  // draw
  display.clearDisplay();
  int16_t fx=snFood.x*CELL,fy=snFood.y*CELL;
  display.drawPixel(fx+1,fy,WHITE);display.drawPixel(fx,fy+1,WHITE);
  display.drawPixel(fx+2,fy+1,WHITE);display.drawPixel(fx+1,fy+2,WHITE);display.drawPixel(fx+1,fy+1,WHITE);
  for(uint8_t i=0;i<snLen;i++){
    int16_t sx=snBody[i].x*CELL,sy=snBody[i].y*CELL;
    if(i==0)display.fillRect(sx,sy,CELL,CELL,WHITE);
    else display.fillRect(sx+1,sy+1,CELL-2,CELL-2,WHITE);
  }
  display.setTextSize(1);
  uint8_t dg=(snSc<10)?1:(snSc<100)?2:3;
  display.setCursor(128-dg*6-1,0);display.print(snSc);
  display.display();
}
void doSOvr(){
  if(snSc>snHi)snHi=snSc;
  display.clearDisplay();
  titleBar(F("SNAKE"),40);
  display.drawFastHLine(0,11,128,WHITE);
  display.setTextSize(2);display.setCursor(10,14);display.print(F("GAME OVER"));
  display.drawFastHLine(0,34,128,WHITE);
  display.setTextSize(1);
  display.setCursor(8,38);display.print(F("Score:"));display.print(snSc);
  display.setCursor(72,38);display.print(F("Best:"));display.print(snHi);
  if(snHi>0)drawBar(8,50,112,6,snSc,snHi);
  display.setCursor(35,57);display.print(F("RIGHT: menu"));
  display.display();
  if(bJ(IR))gameState=ST_MENU;
}

// ======== SERVO RECORDER ========
/*
 RECORD mode (recPlay=false):
   POT      — set angle (previewed live on recSrv)
   UP/DOWN  — cycle which servo to preview
   RIGHT    — stamp current POT angle into ALL 4 servos for new frame
   LEFT     — if frames exist: clear all; else: back to menu
   SLIDE    — switch to PLAY mode (needs ≥1 frame)

 PLAY mode (recPlay=true):
   Cycles through recorded frames every 600ms, writes all 4 servos
   POT      — adjust playback speed (600ms–80ms per step)
   SLIDE    — back to RECORD mode
   LEFT     — back to menu
*/
void doRec(){
  if(recPlay){
    // ---- PLAYBACK ----
    if(bJ(IL)){recPlay=false;gameState=ST_MENU;return;}
    if(!swOn()){recPlay=false;return;}          // slide OFF → back to record
    if(elapsed(tGm,map(analogRead(POT),0,1023,600,80))){
      recIdx=(recIdx+1)%recCnt;
      for(byte s=0;s<4;s++)servos[s].write(recAng[recIdx][s]);
    }
    if(!elapsed(tDr,60))return;
    display.clearDisplay();
    // title: "PLAY  3/8"
    display.fillRect(0,0,128,11,WHITE);
    display.setTextColor(BLACK);display.setTextSize(1);
    display.setCursor(4,2);display.print(F("PLAY"));
    display.setCursor(40,2);display.print(recIdx+1);display.print(F("/"));display.print(recCnt);
    // playback progress bar
    drawBar(80,2,44,7,recIdx+1,recCnt);
    display.setTextColor(WHITE);
    // 4 servo angle bars
    for(byte s=0;s<4;s++){
      byte y=13+s*12;
      display.setCursor(0,y+2);display.print(F("S"));display.print(s+1);
      drawBar(14,y,100,9,recAng[recIdx][s],180);
      // tick mark showing exact angle position
      byte tx=14+map(recAng[recIdx][s],0,180,0,98);
      display.drawFastVLine(tx,y-1,11,WHITE);
      // angle value right of bar
      display.setCursor(116,y+2);
      char tmp[4];snprintf(tmp,4,"%d",recAng[recIdx][s]);
      display.print(tmp);
    }
    display.setCursor(14,60);display.print(F("SW:record  LT:menu"));
    display.display();
  } else {
    // ---- RECORD ----
    if(bJ(IU))recSrv=(recSrv==0)?3:recSrv-1;
    if(bJ(ID))recSrv=(recSrv>=3)?0:recSrv+1;
    // stamp frame
    if(bJ(IR)&&recCnt<REC_MAX){
      byte ang=(byte)map(analogRead(POT),0,1023,0,180);
      for(byte s=0;s<4;s++)recAng[recCnt][s]=ang;
      recCnt++;
    }
    // preview selected servo live
    servos[recSrv].write(map(analogRead(POT),0,1023,0,180));
    // LEFT: clear or exit
    if(bJ(IL)){
      if(recCnt>0){recCnt=0;recIdx=0;}
      else gameState=ST_MENU;
      return;
    }
    // SLIDE → play (only if frames exist)
    if(swOn()&&recCnt>0){recPlay=true;recIdx=0;for(byte s=0;s<4;s++)servos[s].write(recAng[0][s]);return;}

    if(!elapsed(tDr,60))return;
    int a=map(analogRead(POT),0,1023,0,180);
    display.clearDisplay();
    // title: "REC  4/8"
    display.fillRect(0,0,128,11,WHITE);
    display.setTextColor(BLACK);display.setTextSize(1);
    display.setCursor(4,2);display.print(F("REC"));
    display.setCursor(40,2);display.print(recCnt);display.print(F("/"));display.print(REC_MAX);
    // frame slots (8 small squares)
    for(byte f=0;f<REC_MAX;f++){
      byte fx=80+f*6;
      if(f<recCnt)display.fillRect(fx,3,5,5,BLACK); // recorded=filled(black on white bar)
      else display.drawRect(fx,3,5,5,BLACK);          // empty=outline
    }
    display.setTextColor(WHITE);
    // big angle of selected servo
    display.setTextSize(3);
    char ab[5];snprintf(ab,5,"%d",a);
    uint8_t aw=strlen(ab)*18,ax=(128-aw)/2;
    display.setCursor(ax,13);display.print(ab);
    display.setTextSize(1);display.setCursor(ax+aw+1,13);display.print(F("o"));
    // which servo being previewed — small label below angle
    display.setCursor(ax,38);
    display.print(F("S"));display.print(recSrv+1);display.print(F(" live"));
    // progress bar
    drawBar(0,48,128,6,a,180);
    // hints
    display.setCursor(0,57);
    display.print(recCnt<REC_MAX?F("RT:stamp U/D:srv LT:clr"):F("FULL  LT:clear"));
    display.display();
  }
}

// ======== SERVO MENU ========
void doSMnu(){
  if(bJ(IU))sIdx=(sIdx==0)?3:sIdx-1;
  if(bJ(ID))sIdx=(sIdx>=3)?0:sIdx+1;
  if(bJ(IR)){gameState=ST_SMOD;return;}
  if(bJ(IL)){gameState=ST_MENU;return;}
  if(!elapsed(tMn,80))return;
  display.clearDisplay();
  // inverted title
  display.fillRect(0,0,128,11,WHITE);
  display.setTextColor(BLACK);display.setTextSize(1);
  display.setCursor(28,2);display.print(F("SELECT SERVO"));
  display.setTextColor(WHITE);
  // 4 rows of 13px each
  for(byte i=0;i<4;i++){
    byte y=12+i*13;
    bool sel=(i==sIdx);
    if(sel){display.fillRect(0,y,128,12,WHITE);display.setTextColor(BLACK);}
    else{display.drawRect(0,y,128,12,WHITE);display.setTextColor(WHITE);}
    uint16_t c=sel?BLACK:WHITE;
    // left arrow
    display.fillTriangle(3,y+2,3,y+9,8,y+5,c);
    // "SERVO 1" in size 1, centred in row
    display.setTextSize(1);
    display.setCursor(14,y+2);
    display.print(F("SERVO "));display.print(i+1);
    // right arrow on selected
    if(sel)display.fillTriangle(118,y+2,118,y+9,124,y+5,BLACK);
    display.setTextColor(WHITE);
  }
  display.display();
}

// ======== SERVO MODE ========
void doSMod(){
  if(bJ(IU)||bJ(ID))sMode=!sMode;
  if(bJ(IR)){sAng=0;swAng=0;swDir=1;swSpd=2;gameState=(sMode==0)?ST_SPOT:ST_SBTN;return;}
  if(bJ(IL)){gameState=ST_SMNU;return;}
  if(!elapsed(tMn,80))return;
  display.clearDisplay();
  // inverted title bar
  display.fillRect(0,0,128,11,WHITE);
  display.setTextColor(BLACK);display.setTextSize(1);
  display.setCursor(4,2);display.print(F("S"));display.print(sIdx+1);
  display.setCursor(20,2);display.print(F("CONTROL MODE"));
  display.setTextColor(WHITE);

  // Two stacked rows: y=13 (POT) and y=38 (BTN), height=22
  const byte by[2]={13,38};
  for(byte i=0;i<2;i++){
    bool sel=(sMode==i);
    if(sel){display.fillRect(0,by[i],128,22,WHITE);display.setTextColor(BLACK);}
    else{display.drawRect(0,by[i],128,22,WHITE);display.setTextColor(WHITE);}
    uint16_t c=sel?BLACK:WHITE;
    // arrow left side
    if(i==0)display.fillTriangle(5,by[i]+14,9,by[i]+6,13,by[i]+14,c);
    else     display.fillTriangle(5,by[i]+6, 9,by[i]+14,13,by[i]+6, c);
    // big mode name
    display.setTextSize(2);
    display.setCursor(20,by[i]+4);
    display.print(i==0?F("POT"):F("BTN"));
    display.setTextColor(WHITE);
  }
  display.display();
}

// ======== SERVO POT ========
void doSPot(){
  bool sw=swOn();
  if(sw&&elapsed(tSw,18)){
    swAng+=swDir*swSpd;
    if(swAng>=180){swAng=180;swDir=-1;}
    if(swAng<=0){swAng=0;swDir=1;}
    servos[sIdx].write(swAng);
  }
  if(sw){if(bJ(IU))swSpd=constrain(swSpd+1,1,10);if(bJ(ID))swSpd=constrain(swSpd-1,1,10);}
  else servos[sIdx].write(map(analogRead(POT),0,1023,0,180));
  if(bJ(IL)){gameState=ST_SMOD;return;}
  if(!elapsed(tDr,50))return;
  int a=sw?swAng:map(analogRead(POT),0,1023,0,180);
  display.clearDisplay();
  display.fillRect(0,0,128,11,WHITE);
  display.setTextColor(BLACK);display.setTextSize(1);
  display.setCursor(4,2);display.print(F("S"));display.print(sIdx+1);
  display.setCursor(20,2);display.print(sw?F("SWEEP"):F("POT"));
  if(sw){display.setCursor(74,2);display.print(F("SPD:"));display.print(swSpd);}
  display.setTextColor(WHITE);
  display.setTextSize(3);
  char ab[7];snprintf(ab,7,"%d",a);
  uint8_t aw=strlen(ab)*18,ax=(128-aw-8)/2;
  display.setCursor(ax,13);display.print(ab);
  display.setTextSize(1);display.setCursor(ax+aw+2,13);display.print(F("o"));
  display.setCursor(0,40);display.print(F("0"));
  drawBar(10,40,104,7,a,180);display.setCursor(118,40);display.print(F("~"));
  display.setCursor(0,54);display.print(sw?F("U/D:spd  LT:back"):F("SW:sweep  LT:back"));
  display.display();
}

// ======== SERVO BTN ========
void doSBtn(){
  bool dc=swOn();
  if(bJ(IU))sAng=constrain(sAng+(dc?-1:1),0,180);
  if(bJ(ID))sAng=constrain(sAng+(dc?-10:10),0,180);
  if(bJ(IR))servos[sIdx].write(sAng);
  if(bJ(IL)){sAng=0;gameState=ST_SMOD;return;}
  if(!elapsed(tMn,80))return;
  display.clearDisplay();
  display.fillRect(0,0,128,11,WHITE);
  display.setTextColor(BLACK);display.setTextSize(1);
  display.setCursor(4,2);display.print(F("S"));display.print(sIdx+1);
  display.setCursor(20,2);display.print(dc?F("DEC"):F("INC"));
  display.setCursor(68,2);display.print(dc?F("U:-1  D:-10"):F("U:+1  D:+10"));
  display.setTextColor(WHITE);
  display.setTextSize(3);
  char ab[7];snprintf(ab,7,"%d",sAng);
  uint8_t aw=strlen(ab)*18,ax=(128-aw-8)/2;
  display.setCursor(ax,13);display.print(ab);
  display.setTextSize(1);display.setCursor(ax+aw+2,13);display.print(F("o"));
  display.setCursor(0,40);display.print(F("0"));
  drawBar(10,40,104,7,sAng,180);display.setCursor(118,40);display.print(F("~"));
  display.setCursor(4,54);display.print(F("RT:apply SW:dir LT:back"));
  display.display();
}
