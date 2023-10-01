#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define FE 44100
#define CHANNEL 2
#define DUREE 5
#define N FE*DUREE*CHANNEL

/* output */
static char *RAW_OUT = "tmp-out.raw";
static char *FILE_OUT = "out.wav";

FILE *
sound_file_open_write (void)
{
 return fopen (RAW_OUT, "wb");
}

void
sound_file_close_write (FILE *fp)
{
  char cmd[256];
  fclose (fp);
  snprintf(cmd, 256, "sox -c 1 -r %d -e signed-integer -b 16 %s %s", (int)FE, RAW_OUT, FILE_OUT);
  system (cmd);
}

void
sound_file_write (double *s, FILE *fp)
{
  int i;
  short tmp[N];
  for (i=0; i<N; i++)
    {
      tmp[i] = (short)(s[i]*32768);
    }
  fwrite (tmp, sizeof(short), N, fp);
}

double oscillateur(double amplitude, double frequence, double phi, int i)
{
  return amplitude * sin(2*M_PI* frequence * (i*1.0/FE) + phi);
}

int
main (int argc, char *argv[])
{
  int i;
  FILE *output;
  double s[N];
  srand(time(NULL));

  if (argc != 1)
    {
      printf ("usage: %s\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  output = sound_file_open_write ();
    
  int freq1 = 20;
  int freq2 = 400;

  double phi1 = 0;
  double phi2 = 0;

  double amplitude = 1;

  double c = 0.1;



  for (i=0; i<N-1; i+=2){
    //s[i] = 1;
    //s[i] = (((double)rand() / (double)RAND_MAX)*2)-1; 

    //s[i]   = oscillateur(amplitude, freq1, phi1, i);// ECHANTILLONS GAUCHE
    //s[i+1] = oscillateur(amplitude, freq2, phi2, i);// ECHANTILLONS DROITE

    s[i] = (oscillateur(amplitude, freq2 , phi2 + ((i*1.0/N)*10)*oscillateur(amplitude, freq1, phi1, i), i));
    s[i+1] = (oscillateur(amplitude, freq2+1 , phi2 + ((i*1.0/N)*10)*oscillateur(amplitude, freq1, phi1, i), i));

    //SYNTHESE ADDITIVE
    //s[i] =  (oscillateur(1, freq1, phi1, i) + oscillateur(1, freq2, phi2, i))/2;

    //AMPLITUDE MODULATION
    //double val = (oscillateur(1, freq1, phi1, i)+ c) * (oscillateur(1, freq2, phi2, i)) / (amplitude + c);
    //s[i] =  val; 

    //FREQUENCY MODULATION
    //s[i] =(oscillateur(amplitude, freq2 , phi2 + ((i*1.0/N)*10)*oscillateur(amplitude, freq1, phi1, i), i)); 

  }
  sound_file_write (s, output);
  sound_file_close_write (output);
  exit (EXIT_SUCCESS);
}
