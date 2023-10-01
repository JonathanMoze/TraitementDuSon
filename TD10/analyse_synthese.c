#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include "gnuplot_i.h"


#define FE 44100
#define N 1024


typedef double frame[N];

typedef fftw_complex spectre[N];

/* input */
static char *RAW_IN = "tmp-in.raw";
static double cache_in[N/2];

/* output */
static char *RAW_OUT = "tmp-out.raw";
static char *FILE_OUT = "out.wav";
static double cache_out[N/2];

/* ctrl fftw */
 static fftw_plan plan1 = NULL;
 static fftw_plan plan2 = NULL;

/* ctrl gnuplot*/
static gnuplot_ctrl *h;

FILE *
sound_file_open_read (char *file_in)
{
  char cmd[256];
  snprintf(cmd, 256, "sox %s -c 1 -r %d -b 16 -e signed-integer %s", file_in, (int)FE, RAW_IN);
  system (cmd);  
  return fopen(RAW_IN, "rb");
}

void
sound_file_close_read (FILE *fp)
{
  fclose (fp);
}

int
sound_file_read (FILE *fp, frame s)
{
  int i, result;
  
  static short tmp[N/2];
  for (i=0; i<N/2; i++)
    {
      s[i] = cache_in[i];
    }

  bzero (tmp, N/2*sizeof(short));
  
  result = fread (tmp, sizeof(short), N/2, fp);
  
  for (i=0; i<N/2; i++)
    {
      s[N/2+i] = tmp[i] / 32768.0;
      cache_in[i] = s[N/2+i];
    }
  return (result == N/2);
}

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
  snprintf(cmd, 256, "sox -c 1 -r %d -b 16 -e signed-integer %s %s", (int)FE, RAW_OUT, FILE_OUT);
  system (cmd);
}

void
sound_file_write (frame s, FILE *fp)
{
  int i;
  static short tmp[N/2];
  for (i=0; i<N/2; i++)
    {
      double v = (s[i] + cache_out[i])/2;
      short t = (v < -1) ? -32768 : (v > 1) ? 32767 : (short) (v * 32767);
      cache_out[i] = s[N/2+i];
      tmp[i] = t;
    }
  fwrite (tmp, sizeof(short), N/2, fp);
}

void
fft_init (spectre in, spectre spec)
{
/* a completer */
	plan1 = fftw_plan_dft_1d(N, in, spec, FFTW_FORWARD, FFTW_ESTIMATE);
  plan2 = fftw_plan_dft_1d(N, in, spec, FFTW_BACKWARD, FFTW_ESTIMATE);
}

void
fft_exit (void)
{
  fftw_destroy_plan (plan1);
  fftw_destroy_plan (plan2);
}

void
fft (void)
{
 fftw_execute(plan1);
}

void
ifft (void)
{
  fftw_execute(plan2);
}

void
polar(spectre S, double *amplitude, double *phase)
{
  for(int n=0; n<N; n++){
		amplitude[n] = cabs(S[n])/N;
		phase[n] = carg(S[n]);
	}
}

void
comp(spectre S, double *amplitude, double *phase)
{
  for(int n=0; n<N; n++){
		S[n] = (amplitude[n]) * cexp(I*phase[n]);
	}
}

void
hann(frame s)
{
  int i;
  double w;
  for (i=0; i<N; i++)
    {
      w = 0.5 * (1 - cos (2*M_PI*i*1./N));
      s[i] *= w;
    }
}

void
coupeHaut(double *amplitude, double *phase,double freq){
  for(int i=freq; i<(N-freq);i++){
    amplitude[i] = 0;
    phase[i] = 0;
  }
}

void
coupeBas(double *amplitude, double *phase,double freq){
  for(int i=0; i<freq;i++){
    amplitude[i] = 0;
    phase[i] = 0;
  }
  for(int i=(N-freq); i<N;i++){
    amplitude[i] = 0;
    phase[i] = 0;
  }
}

void
robot(double *phase){
  for(int i=0; i<N;i++){
    phase[i] = 0;
  }
}


int
main (int argc, char *argv[])
{
  int i, f_coup;
  FILE *input, *output;
  frame s, amplitude, phase;
  spectre S1, S2;

  if (argc != 2)
    {
      printf ("usage: %s <soundfile>\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  fft_init(S1, S2);

  h = gnuplot_init();
  gnuplot_setstyle(h, "lines");

  input = sound_file_open_read (argv[1]);
  output = sound_file_open_write ();

  bzero (cache_in, N/2*sizeof(double));
  bzero (cache_out, N/2*sizeof(double));

  while (sound_file_read (input, s))
    {
      //hann(s);
      
      for (i=0; i<N; i++)
    	  S1[i] = s[i];
      
      fft();

      polar(S2, amplitude, phase);

      /*for(int i=0; i<N; i++){
        amplitude[i] = 0;
        
      }

      int index =440*((N*1.0)/FE); 
      amplitude[index] = 0.5;
      amplitude[N-index] = 0.5;
      phase[index] = 0.1;
      phase[N-index] = 0.1*i;*/

      // coupe-haut
      //coupeHaut(amplitude, phase, 800*((N*1.0)/FE));
      // coupe-bas
      //coupeBas(amplitude, phase, 3000*((N*1.0)/FE));

     
      // effet robot
      robot(phase);
     
      gnuplot_resetplot(h);
      gnuplot_plot_x(h , amplitude, N, "amplitude spectrum");
      //usleep(100000);

      comp(S1, amplitude, phase);
      
      ifft();

      for (i=0;i<N;i++)
    	  s[i] = creal(S2[i]);

      sound_file_write (s, output);
    }

  /* exit */
  sound_file_close_write (output);
  sound_file_close_read (input);
  fft_exit ();
  exit (EXIT_SUCCESS);
}
