/*

 
 PIN 13 / PB7 is LED "L"
 
 ISR Tic-Timer1 16bit (Arduino-Pin 9,10 .. PB1,PB2) pins are not used
 
 50 Hz / 100 half periodes is 10 ms
 at 16 MHz clock it is 2.44 times a full 16 bit timer 1 register
 od 2 full 2^16 an 28928 of the third call
 
 Stepper (40s per revolution at 100 and half steps):
 Pin2 blue
 Pin3 Pink
 Pin4 Yellow
 Pin5 Orange
 
 */

volatile bool tick = false;

void setup() {

  /* set system clock to 8 MHz to archive 15.6 kHz PWm at the end */
  // CLKPR = (1 << CLKPCE);
  //  CLKPR = (1 << CLKPS0);
  pinMode(13, OUTPUT);// on-board LED
  pinMode(2, OUTPUT);// Stepper 1
  pinMode(3, OUTPUT);// Stepper 2
  pinMode(4, OUTPUT);// Stepper 3
  pinMode(5, OUTPUT);// Stepper 4


  /* init Timer */
  noInterrupts(); /* disable all interrupts */
  /* Timer 1 for ISR */
  /* stop all timer */
  //GTCCR = (1 << TSM) | (1 << PSRASY) | (1 << PSRSYNC); // halt all timers
  /* set to: Clear Timer on Compare Match (CTC) Mode */
  TCCR1A = (1 << COM1A0);   /* toggle OC1A, but it is just enable */
  TCCR1B =  (1 << WGM12);   /* enable CTC mode */
  OCR1A = 65535;            /* initinal step until the ISR writes a new value */
  TCCR1B |=  (1 << CS10);   /* Enable Clock for Timer (prescaler = 1) */
  TIMSK1 |= (1 << OCIE1A);  /* enable timer interrupt on output compare A */
  TCNT1 = 0;

  interrupts();  // enable all interrupts
  /* start all timer synchronuously */
  //GTCCR = 0; // release all timers
}

/* timer 1 ISR for setting the PWM channels */
ISR(TIMER1_COMPA_vect)
{
  tick= true;
  static uint8_t number_of_call = 0;
  bool pattern[8] = {true, true, false, false, false, false, false, false};
  static uint8_t stepper = 0;
  uint16_t isr_cnt = 0;

  number_of_call++;

  number_of_call%=3;
  if(number_of_call == 0)
  {

    isr_cnt = 28928 -1 ;
    OCR1A = isr_cnt;

  }
  else
  {
    OCR1A = 0xffff-1;
    return;
  }
  //digitalWrite(13,!digitalRead(13));
  stepper++;
    stepper++;
  stepper%=8;
  digitalWrite(3, pattern[stepper]);
  digitalWrite(5, pattern[(stepper + 4) % 8]);
  digitalWrite(2, pattern[(stepper + 2) % 8]);
  digitalWrite(4, pattern[(stepper + 6) % 8]);

}

void loop() {

  if(tick)
  {
    tick= false;
    //digitalWrite(13,!digitalRead(13));
  }
}

