#include <Mouse.h>
#include <Keyboard.h>
#define BITRONICS 1
#define STRELA 1
#define CALIBBTN 1
#define LOCKBTN 2
#define MOUSE 0
// Для стрелы
#if(STRELA)
  #include <Strela.h>
  #include <Wire.h>
  #define digitalWrite(a, b) uDigitalWrite(a, b)
  #define digitalRead(a) uDigitalRead(a)
  #define analogWrite(a, b) uAnalogWrite(a, b)
  #define analogRead(a) uAnalogRead(a)
  #define LED L1
  #define EMG1 P8
  #define EMG2 P7
  #define CALIBBTN S1
  #define LOCKBTN S2
  #define MOUSE 0
#endif


// Структура для флагов
struct
{
  //uint8_t flag : 1;
  uint8_t trig1 : 1;
  uint8_t trig2 : 1;
  uint8_t prevTrig1 : 1;
  uint8_t prevTrig2 : 1;
  uint8_t mouseDown : 1;
  uint8_t click : 1;
  uint8_t lock : 1;
  uint8_t prevLockBtn : 1;
  uint8_t lrud : 1;
} bools;

#define arrSize 150
#define THRESHOLD 15
#define CLICKTHRESHOLD 60
#define THRESHOLDFREQ 15
uint8_t val1[250], val1Filtr1[250];
uint8_t val2[250];
uint8_t val1Filtr;
uint16_t maxV1 = 0, minV1 = 0;
uint16_t maxV2 = 0, minV2 = 0;
uint16_t sData1 = 0; uint16_t sData2 = 0;
uint16_t freq1;
uint16_t freq2;
uint16_t threshold = 135;
uint16_t lrthreshold = 135;
uint16_t thresholdFreq = 61;

void setup()
{
  Serial.begin(115200);
  #if(MOUSE)
  Mouse.begin();
  #endif
  //Keyboard.begin();

  digitalWrite(LED, LOW);
  bools.lock = 1;
  while(millis() < 2000) calc();
  calibrate();
  bools.lock = 0;
}

void loop()
{
  static uint32_t timer1 = 0;
  if(millis() - timer1 >= 1)
  {
    timer1 = millis();
    calc();
    sendData();
    makeAMove();
  }
  if(digitalRead(CALIBBTN)) calibrate();
  bools.lock ^= (digitalRead(LOCKBTN) && !bools.prevLockBtn);
  bools.prevLockBtn = digitalRead(LOCKBTN);
}

void calibrate()
{
  threshold = maxV1 - minV1 + THRESHOLD;
  lrthreshold = maxV2 - minV2 + CLICKTHRESHOLD;
  thresholdFreq = freq1 + THRESHOLDFREQ;
}

// Плавающий
void calc() {
  switch(1)
  {
    case 1:
      freq1 = 0;
      
      for(uint8_t i = 249; i > 0; i--)
      {
        // freq1 += (val1[i] <= 128 && val1[i-1] >= 128) || (val1[i] >= 128 && val1[i-1] <= 128);
        val1[i] = val1[i - 1];
      }
      for(uint8_t i = 249; i > 0; i--)
      {
        freq1 += (val1[i] <= 5 && val1[i-1] >= 5) || (val1[i] >= 5 && val1[i-1] <= 5);
        val1Filtr1[i] = val1Filtr1[i - 1];
      }
      freq1 *= 4;
      val1[0] = analogRead(EMG1) >> 2;
      val1Filtr =   bools.trig1 ?
                    val1[0] :
                    (63 * val1Filtr + val1[0]) >> 6;
      val1Filtr1[0] = (7 * val1Filtr1[0] + val1Filtr) >> 3;
      
      maxV1 = 0;
      minV1 = 0;
      for (uint16_t k = 0; k < arrSize; k++)
      {
        if (val1[k] > maxV1)
          maxV1 = val1[k];
        if (val1[k] < minV1)
          minV1 = val1[k];
      }
      sData1 = maxV1 - minV1;
      bools.trig1 = sData1 > threshold;
    case 2:
      freq2 = 0;
      
      for(uint8_t i = 249; i > 0; i--)
      {
        freq2 += (val2[i] <= 128 && val2[i-1] >= 128) || (val2[i] >= 128 && val2[i-1] <= 128);
        val2[i] = val2[i - 1];
      }
      freq2 *= 4;
      val2[0] = analogRead(EMG2) >> 2;
      
      maxV2 = 0;
      minV2 = 0;
      for (uint16_t k = 0; k < arrSize; k++)
      {
        if (val2[k] > maxV2)
          maxV2 = val2[k];
        if (val2[k] < minV2)
          minV2 = val2[k];
      }
      sData2 = maxV2 - minV2;
    
      bools.trig2 = sData2 > lrthreshold;
  }
}

void sendData()
{
  #if(BITRONICS)
  Serial.write("A0");
  Serial.write(val1[0]);  
  Serial.write("A1");
  Serial.write(map(freq1, 0, 1024, 0, 256));
  Serial.write("A2");
  Serial.write(val1Filtr1[0]);
  Serial.write("A3");
  Serial.write(sData1);
  #else
  Serial.print(bools.lrud ? (bools.mouseDown ? "Right\n" : "Left\n") : "\n");
  #endif
}

// void makeAMove()
// {
//   static uint32_t timer = 0;
//   static uint32_t clickTimer = 0;
    
//   if(bools.prevTrig1 == 0 && bools.trig1 == 1) timer = millis();

//   if(bools.trig1 == 1)
//   {
//     if(millis() - timer > 600)
//     {
//       #if(MOUSE)
//       if(!bools.lock && millis() % ((millis() - timer > 1500) ? ((millis() - timer > 3000) ? 1 : 3) : 5) == 0) Mouse.move(bools.lrud ? (bools.mouseDown ? 1 : -1) : 0, bools.lrud ? 0 : (bools.mouseDown ? 1 : -1), 0);
//       #endif
//     }
//     else
//     {
//       bools.mouseDown |= freq1 > thresholdFreq;
//       bools.lrud = bools.trig2;
//     }
//   }

//   if(bools.prevTrig1 == 1 && bools.trig1 == 0)
//   {
//     bools.mouseDown = 0;
//   }

//   if(bools.prevTrig2 == 0 && bools.trig2 == 1) clickTimer = millis();

//   if(bools.prevTrig2 == 1 && bools.trig2 == 0)
//   {
//     #if(MOUSE)
//     if(millis() - clickTimer < 700 && !bools.lock) Mouse.click(MOUSE_LEFT);
//     #endif
//   }
//   bools.prevTrig1 = bools.trig1;
//   bools.prevTrig2 = bools.trig2;
// }

void makeAMove()
{
  static uint32_t timer = 0;

  if(bools.prevTrig1 == 0 && bools.trig1 == 1) timer = millis();

  if(bools.trig1 == 1)
  {
    if(millis() - timer > 600)
    {
      bools.lrud = true;
    }
    else
    {
      bools.mouseDown |= freq1 > 80;
    }
  }

  if(bools.prevTrig1 == 1 && bools.trig1 == 0)
  {
    bools.mouseDown = 0;
    bools.lrud = false;
  }

  bools.prevTrig1 = bools.trig1;
  bools.prevTrig2 = bools.trig2;
}