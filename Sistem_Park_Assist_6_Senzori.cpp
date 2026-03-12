#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define TRIG_FS 22
#define ECHO_FS 23
#define TRIG_FC 24
#define ECHO_FC 25
#define TRIG_FD 26
#define ECHO_FD 27
#define TRIG_SS 28
#define ECHO_SS 29
#define TRIG_SC 30
#define ECHO_SC 31
#define TRIG_SD 32
#define ECHO_SD 33

#define LED_FS_V 34
#define LED_FS_G 35
#define LED_FS_R 36
#define LED_FC_V 37
#define LED_FC_G 38
#define LED_FC_R 39
#define LED_FD_V 40
#define LED_FD_G 41
#define LED_FD_R 42
#define LED_SS_V 43
#define LED_SS_G 44
#define LED_SS_R 45
#define LED_SC_V 46
#define LED_SC_G 47
#define LED_SC_R 48
#define LED_SD_V 49
#define LED_SD_G 50
#define LED_SD_R 51

#define BUZZER 11
#define BUTON 12
#define LDR A0

LiquidCrystal_I2C lcd(0x27,16,2);

const float PRAG_SIGUR=100.0;
const float PRAG_ATENTIE=50.0;
const float PRAG_PERICOL=20.0;
const int MARIME_FILTRU=5;
const unsigned long INTERVAL=100;

struct Zona{
int trig,echo,lv,lg,lr;
float buf[MARIME_FILTRU];
int idx;
unsigned long timp;
float dist;
int nivel;
};

Zona z[6]={
{TRIG_FS,ECHO_FS,LED_FS_V,LED_FS_G,LED_FS_R},
{TRIG_FC,ECHO_FC,LED_FC_V,LED_FC_G,LED_FC_R},
{TRIG_FD,ECHO_FD,LED_FD_V,LED_FD_G,LED_FD_R},
{TRIG_SS,ECHO_SS,LED_SS_V,LED_SS_G,LED_SS_R},
{TRIG_SC,ECHO_SC,LED_SC_V,LED_SC_G,LED_SC_R},
{TRIG_SD,ECHO_SD,LED_SD_V,LED_SD_G,LED_SD_R}
};

bool activ=true;
unsigned long ultimBip=0,debounce=0,lcdT=0,serialT=0;
int stareButon=HIGH,ultimaStare=HIGH,zonaLCD=0;

const char* nume[6]={"F-Stanga","F-Centru","F-Dreapta","S-Stanga","S-Centru","S-Dreapta"};

float citeste(int t,int e){
digitalWrite(t,LOW);delayMicroseconds(2);
digitalWrite(t,HIGH);delayMicroseconds(10);
digitalWrite(t,LOW);
long d=pulseIn(e,HIGH,30000);
if(d==0)return 999.0;
return d*0.034/2;
}

float medie(float* v){
float s=0;
for(int i=0;i<MARIME_FILTRU;i++)s+=v[i];
return s/MARIME_FILTRU;
}

int nivel(float d){
if(d>=PRAG_SIGUR)return 0;
if(d>=PRAG_ATENTIE)return 1;
if(d>=PRAG_PERICOL)return 2;
return 3;
}

void led(Zona& a,bool noapte){
if(noapte){
digitalWrite(a.lv,LOW);
digitalWrite(a.lg,LOW);
digitalWrite(a.lr,(a.nivel>=2));
}else{
digitalWrite(a.lv,(a.nivel==0));
digitalWrite(a.lg,(a.nivel==1));
digitalWrite(a.lr,(a.nivel>=2));
}
}

void oprit(){
for(int i=0;i<6;i++){
digitalWrite(z[i].lv,LOW);
digitalWrite(z[i].lg,LOW);
digitalWrite(z[i].lr,LOW);
}
digitalWrite(BUZZER,LOW);
}

void setup(){
Serial.begin(9600);
lcd.init();lcd.backlight();
for(int i=0;i<6;i++){
pinMode(z[i].trig,OUTPUT);
pinMode(z[i].echo,INPUT);
pinMode(z[i].lv,OUTPUT);
pinMode(z[i].lg,OUTPUT);
pinMode(z[i].lr,OUTPUT);
z[i].idx=0;
z[i].dist=999;
z[i].nivel=0;
z[i].timp=i*15;
for(int j=0;j<MARIME_FILTRU;j++)z[i].buf[j]=999;
}
pinMode(BUZZER,OUTPUT);
pinMode(BUTON,INPUT_PULLUP);
}

void loop(){
unsigned long t=millis();
int cit=digitalRead(BUTON);
if(cit!=ultimaStare)debounce=t;
if(t-debounce>50){
if(cit!=stareButon){
stareButon=cit;
if(stareButon==LOW)activ=!activ;
}}
ultimaStare=cit;

if(!activ){
oprit();
if(t-lcdT>300){
lcdT=t;
lcd.setCursor(0,0);lcd.print("Sistem oprit    ");
lcd.setCursor(0,1);lcd.print("                ");
}
return;
}

bool noapte=analogRead(LDR)<200;
int maxNivel=0;

for(int i=0;i<6;i++){
if(t-z[i].timp>=INTERVAL){
z[i].timp=t;
z[i].buf[z[i].idx]=citeste(z[i].trig,z[i].echo);
z[i].dist=medie(z[i].buf);
z[i].idx=(z[i].idx+1)%MARIME_FILTRU;
z[i].nivel=nivel(z[i].dist);
}
led(z[i],noapte);
if(z[i].nivel>maxNivel)maxNivel=z[i].nivel;
}

unsigned long intBip=0;
if(maxNivel==1)intBip=500;
else if(maxNivel==2)intBip=200;
else if(maxNivel==3)intBip=50;

if(maxNivel==3){
digitalWrite(BUZZER,HIGH);
}else if(maxNivel>=1){
if(t-ultimBip>=intBip){
ultimBip=t;
digitalWrite(BUZZER,!digitalRead(BUZZER));
}
}else digitalWrite(BUZZER,LOW);

if(t-lcdT>400){
lcdT=t;
zonaLCD=(zonaLCD+1)%6;
Zona& a=z[zonaLCD];
lcd.setCursor(0,0);
lcd.print(nume[zonaLCD]);lcd.print(": ");
lcd.print(a.dist,1);lcd.print("cm   ");
lcd.setCursor(0,1);
lcd.print("Nivel: ");
if(a.nivel==0)lcd.print("Sigur      ");
else if(a.nivel==1)lcd.print("Atentie    ");
else if(a.nivel==2)lcd.print("Pericol    ");
else lcd.print("Frt aproape");
}

if(t-serialT>500){
serialT=t;
for(int i=0;i<6;i++){
Serial.print(nume[i]);Serial.print(":");
Serial.print(z[i].dist,1);Serial.print("cm(N");
Serial.print(z[i].nivel);Serial.print(") ");
}
Serial.println();
}
}
