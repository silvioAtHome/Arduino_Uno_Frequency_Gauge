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

  X27.168 315 Grad das sind ca. 945 steps in der Tabelle

  Buzzer Timer 2, Pin 11 (PB3, OC2A)


*/

volatile bool tick = false;
volatile bool locked_to_grid = false;
volatile float internal_speed = 50.0f;
volatile int16_t stepper_pos = 0;
#define ISRTIMERFREQUENCY 16.003e6

void setup() {

  /* init ADC */
  ADMUX = (1 << REFS0); //AVCC with external capacitor at AREF pin
  ADCSRA = _BV(ADEN); // enable ADC
  ADCSRB = 0;
  DIDR0 = 0b00000011; // Disable digital input buffer

  /* init pins */
  pinMode(13, OUTPUT);// on-board LED
  pinMode(2, OUTPUT);// Stepper 1
  pinMode(3, OUTPUT);// Stepper 2
  pinMode(4, OUTPUT);// Stepper 3
  pinMode(5, OUTPUT);// Stepper 4

  pinMode(11, OUTPUT);// Buzzer
  //digitalWrite(10, true);

  pinMode(8, INPUT_PULLUP);//
  pinMode(9, INPUT_PULLUP);//




  /* init serial */
  Serial.begin(115200);

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

  /* PWM Timer2 */
  /* OC2A ist Pin 11/PB3 */
  //DDRB |= (1 << PB3); /* enable output */
  TCCR2A = (1 << COM2A0);
  TCNT2 = 0;
  // OCR2A = 100;
  /* enable clock for pwm, no prescaler f_pwm = 8e6/510 = 15686.275 Hz, DS p 124 */
  TCCR2B = (1 << CS21);  /* prescaler = 64 */

  interrupts();  // enable all interrupts
  /* start all timer synchronuously */
  //GTCCR = 0; // release all timers
}

/* timer 1 ISR for setting the PWM channels */
ISR(TIMER1_COMPA_vect)
{
  static uint8_t number_of_call = 0, tick_cnt = 0, ready, i, init_probing = 3, buzzer_cntdwn = 0 , buzzer_freq = 0;
  int16_t stepper_command = 0;
  bool pattern[6] = {
    true, true, false, false, false, false
  };
  static int8_t stepper = 0;
  static int isr_timer = 28928; // default 50 Hz
  int isr_total;
  float vgrid, phasedetector, helpf;
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

  tick_cnt++;
  tick_cnt %= 100; // once a second
  if (tick_cnt == 0) tick = true;

  if (init_probing == 3 && stepper_pos == 0)
  {
    stepper_pos = 945;
    init_probing = 2;
  }
  if (init_probing == 2 && stepper_pos == 0)
  {
    stepper_pos = -945 / 2;
    init_probing = 1;
  }
  if (init_probing == 1 && stepper_pos == 0)
  {
    init_probing = 0;
  }
  if (init_probing == 0)
  {
    stepper_command = (int16_t)(300.0 * (internal_speed - 50.0f));
  }

  if (stepper_pos - stepper_command < 0)
  {
    stepper++;
    stepper_pos++;
  }

  if (stepper_pos - stepper_command > 0)
  {
    stepper--;
    stepper_pos--;
    if (stepper < 0)
    {
      stepper += 6;
    }
  }

  stepper %= 6;
  digitalWrite(2, pattern[stepper]);
  digitalWrite(3, pattern[(stepper + 3) % 6]);

  digitalWrite(4, pattern[(stepper + 1) % 6]);
  digitalWrite(5, pattern[(stepper + 4) % 6]);


  /* buzzer control */
  helpf = fabsf(internal_speed - 50.0f);

  if (buzzer_cntdwn > 0)
  {
    TCCR2A = (1 << COM2A0);
    buzzer_cntdwn--;
  }
  else
  {
    TCCR2A = 0;
    if (buzzer_freq > 0)
    {
      buzzer_freq--;
    }
    else
    {
      if (helpf > 0.05)
      {
        buzzer_freq = (uint8_t) (2.5 / helpf);
        buzzer_cntdwn = 5;
      }
    }
  }
}

void loop() {

  if (tick)
  {
    tick = false;
    digitalWrite(13, !digitalRead(13));
    Serial.print(internal_speed, 3);
    Serial.print(" ");
    Serial.println(stepper_pos);
  }
}
