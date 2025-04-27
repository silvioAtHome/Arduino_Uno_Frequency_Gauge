#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Points per periode
#define PPP 16

// #define ISRTIMERFREQUENCY 72e6
// ISRTIMERFREQUENCY / 16 = 4.5 MHz
#define ISRTIMERFREQUENCY 16.0e6

/* Grenzen

47 Hz
16e6/(2*47) -2*2^16
 ans  =
 
    39140.766  
    
53 Hz:
16e6/(2*53) -2*2^16
 ans  =
 
    19871.396

*/

//gcc main.c -o sim -lm
//gcc main.c -o sim.exe -lm && ./sim.exe 49 50 -180 > testdata.dat && gnuplot plot.plt

float time_to_phase(float time, float * phase)
{
//    phi=2*%pi*(chirp_f_low*t+df/(df/chirp_f_acc)*0.5*t.^2);
float t1,t2,acc;

t1=2.0f;
t2=3.0f;
acc=1.0f;
if(time <= t1) return 0.0f;
if(time > t1 && time <= t2)
    {
    *phase += 2.0f * M_PI * acc * 0.5f * powf(time-t1,2.0f);
    return (time-t1) * acc;
    }
if(time > t2)
    {
    *phase += 2.0f * M_PI * acc * 0.5f * powf(t2-t1,2.0f);
    *phase += 2.0f * M_PI * (t2-t1) * acc * (time-t2);
    return (t2-t1) * acc;
    }

}

int main(int argc, char **argv)
{

  int i;
  float fstart, fgrid, phase;
  int isr_timer,isr_total;
  float time = 0.0, internal_speed, vgrid, grid_phase,vgrid_old,df;
  float phasedetector, filt;

  if (argc != 4)
  {
    printf("sim fstart fgrid phase\n");
    return -1;
  }


  if (sscanf(argv[1], "%f", &fstart) != 1 ||
      sscanf(argv[2], "%f", &fgrid) != 1 ||
      sscanf(argv[3], "%f", &phase) != 1)
  {
    printf("Fehler: Parameter lesefehler\n");
  }

// ab hier kann es losgehen

  i = 0;

  printf("#fgrid: %.3f\n", fgrid);
  printf("#fstart: %.3f\n", fstart);
  printf("#phasestart: %.3f\n", phase);

  printf("#1:i,2:time, 3:vgrid, 4:grid_phase, 5:fgrid, 6:internal_speed, 7:phasedetector, 8:filt, 9: isr_timer\n");

  i = 0;
  time = 0.0;
  internal_speed = fstart;

  isr_timer = 0;
  printf("# max ISR Timer at f_min %i\n", isr_timer);

  while (i < 500)
  {
    grid_phase = 2.0f * M_PI * fgrid * time + phase / 180.0 * M_PI;
    df=time_to_phase(time, &grid_phase);
    grid_phase = fmodf(grid_phase, 2.0f * M_PI);

    // ADC 2V peakpeak
    vgrid = 0.1 + 0.9 * sinf(grid_phase) + 0.01 * (float)(rand() % 10 - 5) / 5.0;

    if(i % 2) 
    {
    	phasedetector = vgrid-vgrid_old;
    }
    else
    {
    	phasedetector = -vgrid+vgrid_old;
    }
    filt = phasedetector;
    vgrid_old = vgrid;

    // P-Anteil 5000
    isr_total = (int)(ISRTIMERFREQUENCY / (2.0*internal_speed) - 5000.0 * filt);
    isr_timer = isr_total - 2*65535;
    
    if(isr_timer < 19871) 
    {
    	isr_timer = 19871;
    }
    if(isr_timer > 39140) 
    {
    	isr_timer = 39140;
    }
    // I-Anteil 0.35
    internal_speed += 0.1 * filt;
    if (internal_speed < 47.0f) internal_speed = 47.0f;
    if (internal_speed > 53.0f) internal_speed = 53.0f;

    printf("%i %f %f %f %f %f %f %f %i\n", i, time, vgrid, grid_phase, fgrid + df, internal_speed, phasedetector, filt,isr_timer);

    i++;
    time += ((float) (isr_timer + 2*65535) + 1) / ISRTIMERFREQUENCY;
  }

}


