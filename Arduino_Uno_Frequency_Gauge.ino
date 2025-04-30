/*


  PIN 13 / PB7 is LED "L"

  SPI on Arduino Uno
  SCK 13
  MISO 12
  MOSI 11

  ISR Tic-Timer1 16bit (Arduino-Pin 9,10 .. PB1,PB2) pins are not used

  50 Hz / 100 half periodes is 10 ms
  at 16 MHz clock it is 2.44 times a full 16 bit timer 1 register
  od 2 full 2^16 an 28928 of the third call

  Stepper (40s per revolution at 100 and half steps):
  Pin2 blue
  Pin3 Pink
  Pin4 Yellow
  Pin5 Orange

  https://simple-circuit.com/interfacing-arduino-ili9341-tft-display/

*/

#include <Adafruit_ILI9341.h>
// #include <Adafruit_GFX.h>

#define TFT_CS    8      // TFT CS  pin is connected to arduino pin 8
#define TFT_RST   9      // TFT RST pin is connected to arduino pin 9
#define TFT_DC    10     // TFT DC  pin is connected to arduino pin 10

volatile bool tick = false;
volatile bool locked_to_grid = false;
volatile float internal_speed = 50.0f;
unsigned int periodes = 0;
float values[200] = {0.0f};
int j;
#define ISRTIMERFREQUENCY 16.0e6

// initialize ILI9341 TFT library
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void testText() {
  tft.fillScreen(ILI9341_BLACK);
  //unsigned long start = micros();
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
  tft.println(1234.56);
  tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
  tft.println(0xDEADBEEF, HEX);
  tft.println();
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(5);
  tft.println("Groop");
  tft.setTextSize(2);
  tft.println("I implore thee,");
  tft.setTextSize(1);
  tft.println("my foonting turlingdromes.");
  tft.println("And hooptiously drangle me");
  tft.println("with crinkly bindlewurdles,");
  tft.println("Or I will rend thee");
  tft.println("in the gobberwarts");
  tft.println("with my blurglecruncheon,");
  tft.println("see if I don't!");
  return;
}


void setup() {

  /* init ADC */
  ADMUX = (1 << REFS0); //AVCC with external capacitor at AREF pin
  ADCSRA = _BV(ADEN); // enable ADC
  ADCSRB = 0;
  DIDR0 = 0b00000011; // Disable digital input buffer

  /* init pins */
  //pinMode(13, OUTPUT);// on-board LED
  pinMode(2, OUTPUT);// Stepper 1
  pinMode(3, OUTPUT);// Stepper 2
  pinMode(4, OUTPUT);// Stepper 3
  pinMode(5, OUTPUT);// Stepper 4

  /* init serial */
  Serial.begin(115200);

  /* init Timer */
  noInterrupts(); /* disable all interrupts */
  /* Timer 1 for ISR */
#if 1
  TCCR1A = (1 << COM1A0) | (1 << COM1A1);   /* toggle OC1A, but it is just enable */
#endif
  TCCR1B =  (1 << WGM12);   /* enable Clear Timer on Compare Match CTC mode */

  OCR1A = 65535;            /* initinal step until the ISR writes a new value */
  TCCR1B |=  (1 << CS10);   /* Enable Clock for Timer (prescaler = 1) */
  TIMSK1 |= (1 << OCIE1A);  /* enable timer interrupt on output compare A */
  TCNT1 = 0;

  interrupts();  // enable all interrupts

  tft.begin();
  //tft.fillScreen(ILI9341_GREEN);

  Serial.print("TCCR1A:");
  Serial.println(TCCR1A);
  Serial.print("TCCR1B:");
  Serial.println(TCCR1B);

}

/* timer 1 ISR for setting the PWM channels */
ISR(TIMER1_COMPA_vect)
{
  static uint8_t number_of_call = 0, tick_cnt = 0, ready, i, trace_cnt = 0;
  bool pattern[8] = {
    true, true, false, false, false, false, false, false
  };
  static uint8_t stepper = 0;
  static int isr_timer = 28928; // default 50 Hz.. 16e6/50/2-2*2^16 = 28928
  int isr_total;
  float vgrid, phasedetector;
  static float vgrid_old = 0.0f;


  number_of_call++;
  number_of_call %= 3;

  OCR1A = 0xffff - 1;
  if (number_of_call == 2)
  {
    OCR1A = (uint16_t) isr_timer;
  }

  if (number_of_call != 0)
  {
    return;
  }

  /* read adc channel 0 */
  ADMUX  &=  0xf0;
  ADMUX |=  0; // Select ADC Channel
  ADCSRA |= _BV(ADSC); // Start conversion
  while ( (ADCSRA & _BV(ADSC)) ); // wait for finishing

  vgrid = (float) ((int16_t)ADC - 512) / 512.0f;

  if (i % 2)
  {
    phasedetector = vgrid - vgrid_old;
  }
  else
  {
    phasedetector = -vgrid + vgrid_old;
  }
  i++;
  vgrid_old = vgrid;

  // P-Anteil
  isr_total = (int)(ISRTIMERFREQUENCY / (2.0 * internal_speed) - 5000.0 * phasedetector);
  isr_timer = isr_total - 2 * 65535;
  if (isr_timer < 19871) isr_timer = 19871;
  if (isr_timer > 39140) isr_timer = 39140;

  // I-Anteil
  internal_speed += 0.1 * phasedetector;
  if (internal_speed < 47.0f) internal_speed = 47.0f;
  if (internal_speed > 53.0f) internal_speed = 53.0f;

  if ( fabsf(phasedetector) < 0.1f)
  {
    if (ready > 0)ready--;
    if (ready == 0) locked_to_grid = true;
  }
  else
  {
    ready = 100;
  }
if(trace_cnt<200)
{
values[trace_cnt++]= internal_speed;
}

  tick_cnt++;
  tick_cnt %= 100; // once a second
  if (tick_cnt == 0) tick = true;

  stepper++;
  stepper++;
  stepper %= 8;
  /*digitalWrite(3, pattern[stepper]);
    digitalWrite(5, pattern[(stepper + 4) % 8]);
    digitalWrite(2, pattern[(stepper + 2) % 8]);
    digitalWrite(4, pattern[(stepper + 6) % 8]);
  */

}

void loop() {

  if (tick)
  {
    tick = false;
    //digitalWrite(13,!digitalRead(13));
    /*Serial.print(periodes);
      Serial.print(" ");*/
      //Serial.println(internal_speed,4);
      if(periodes == 3)
      {
        for(j=0;j<200;j++)
        {
          Serial.println(values[j],4);
        }
      }
    //delay(100);
    //Serial.println("after delay");
    //testText();
    periodes++;
  }
}
