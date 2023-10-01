#include "gnuplot_i.h"
#include <math.h>

#define SAMPLING_RATE 44100.0
#define CHANNELS_NUMBER 1
#define QUANTIFICATION_BITS 16
#define N 1024

char *RAW_FILE = "tmp-in.raw";

FILE *
sound_file_open_read (char *sound_file_name)
{
  char cmd[256];

  snprintf (cmd, 256,
	    "sox %s -e signed-integer -c %d -r %d -b %d %s",
	    sound_file_name,
	    CHANNELS_NUMBER, 
      (int)SAMPLING_RATE, 
      QUANTIFICATION_BITS, 
      RAW_FILE);

  system (cmd);

  return fopen (RAW_FILE, "rb");
}

void
sound_file_close_read (FILE *fp)
{
  fclose (fp);
}

int
sound_file_read (FILE *fp, double *frame)
{
  short tmp[N];
  int ret = fread(tmp, QUANTIFICATION_BITS/8, N, fp);
  for(int i=0; i<N; i++){
    frame[i] = (tmp[i]*1.0)/pow(2.0, QUANTIFICATION_BITS-1);
  }

  return (ret == N);
}



int
main (int argc, char *argv[])
{
  FILE *input;
  double frame[N];

  double *x_axis = malloc(N*sizeof(double)); /* abscisses */
  double *y_axis; /* ordonnees */
  int cpt = 0;

  
  

  gnuplot_ctrl *h = gnuplot_init();
  gnuplot_setstyle(h, "lines");
  gnuplot_cmd(h, "set yr [-1:1]");
  
  input = sound_file_open_read(argv[1]);

  char cmd[256];
  snprintf(cmd, 256, "play %s", argv[1]);

  if(fork() == 0){
    system(cmd);
  } else {
    while(sound_file_read (input, frame)){

      for(int i=0; i<N; i++){
        x_axis[i] = i/SAMPLING_RATE + cpt*N/SAMPLING_RATE;
      }

      cpt++;

      // affichage
      gnuplot_resetplot(h);
      gnuplot_plot_xy (h, x_axis, frame, N, "dessin");
      
      usleep(1000000 * (N/SAMPLING_RATE));
    }
  }
  
  free(x_axis);
  sound_file_close_read (input);
  exit (EXIT_SUCCESS);
}
