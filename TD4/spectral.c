#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <complex.h>
#include <fftw3.h>
#include <sndfile.h>
#include <math.h>
#include <time.h>
#include "gnuplot_i.h"

/* taille de la fenetre */
#define	FRAME_SIZE 4096
/* avancement */
#define HOP_SIZE 4096

#define FFT_SIZE 4096

static gnuplot_ctrl *h;

static void
print_usage (char *progname)
{	printf ("\nUsage : %s <input file> \n", progname) ;
	puts ("\n"
		) ;

} 
static void
fill_buffer(double *buffer, double *new_buffer)
{
  int i;
  double tmp[FRAME_SIZE-HOP_SIZE];
  
  /* save */
  for (i=0;i<FRAME_SIZE-HOP_SIZE;i++)
    tmp[i] = buffer[i+HOP_SIZE];
  
  /* save offset */
  for (i=0;i<(FRAME_SIZE-HOP_SIZE);i++)
    {
      buffer[i] = tmp[i];
    }
  
  for (i=0;i<HOP_SIZE;i++)
    {
      buffer[FRAME_SIZE-HOP_SIZE+i] = new_buffer[i];
    }
}

static int
read_n_samples (SNDFILE * infile, double * buffer, int channels, int n)
{

  if (channels == 1)
    {
      /* MONO */
      int readcount ;

      readcount = sf_readf_double (infile, buffer, n);

      return readcount==n;
    }
  else if (channels == 2)
    {
      /* STEREO */
      double buf [2 * n] ;
      int readcount, k ;
      readcount = sf_readf_double (infile, buf, n);
      for (k = 0 ; k < readcount ; k++)
	buffer[k] = (buf [k * 2]+buf [k * 2+1])/2.0 ;

      return readcount==n;
    }
  else
    {
      /* FORMAT ERROR */
      printf ("Channel format error.\n");
    }
  
  return 0;
} 

static int
read_samples (SNDFILE * infile, double * buffer, int channels)
{
  return read_n_samples (infile, buffer, channels, HOP_SIZE);
}

// EXO 1
void dft (double *s, complex *S){
	for(int m=0; m<FRAME_SIZE ; m++){
		complex m_tmp = 0 + 0*I;
		for(int n=0; n<FRAME_SIZE; n++){
			m_tmp += s[n]*cexp(-I*((2*M_PI)/FRAME_SIZE)*(n*m));
		}
		S[m] = m_tmp;
	}
}

static fftw_plan plan_forward;
static fftw_plan plan_backward;
complex data_in[FFT_SIZE];
complex data_out[FFT_SIZE];

void fft_init_forward(){
	plan_forward = fftw_plan_dft_1d(FFT_SIZE, data_in, data_out, FFTW_FORWARD, FFTW_ESTIMATE);
}

void fft_init_backward(){
	plan_backward = fftw_plan_dft_1d(FFT_SIZE, data_in, data_out, FFTW_BACKWARD, FFTW_ESTIMATE);
}

void fft_exit(){
	fftw_destroy_plan (plan_forward);
	fftw_destroy_plan (plan_backward);
}

void fft_exec(double *s){
	for (int i=0; i<FRAME_SIZE; i++){
		data_in[i] = s[i];
	}
	fftw_execute (plan_forward);
}

void fft_exec_back(double *s){
	fftw_execute (plan_backward);
	for (int i=0; i<FRAME_SIZE; i++){
		s[i] = creal(data_out[i])/FRAME_SIZE;
	}
}

// EXO 4
void cartesian_to_polar (complex *S, double *amp, double *phs){
	for(int n=0; n<FRAME_SIZE; n++){
		amp[n] = cabs(S[n]);
		phs[n] = carg(S[n]);
	}
}

void polar_to_cartesian (double *amp, double *phs, complex *S){
	for(int n=0; n<FRAME_SIZE; n++){
		S[n] = amp[n] * cexp(I*phs[n]);
	}
}

int
main (int argc, char * argv [])
{	char 		*progname, *infilename;
	SNDFILE	 	*infile = NULL ;
	SF_INFO	 	sfinfo ;

	progname = strrchr (argv [0], '/') ;
	progname = progname ? progname + 1 : argv [0] ;

	if (argc != 2)
	{	print_usage (progname) ;
		return 1 ;
		} ;

	infilename = argv [1] ;

	if ((infile = sf_open (infilename, SFM_READ, &sfinfo)) == NULL)
	{	printf ("Not able to open input file %s.\n", infilename) ;
		puts (sf_strerror (NULL)) ;
		return 1 ;
		} ;

	/* Read WAV */
	int nb_frames = 0;
	double new_buffer[HOP_SIZE];
	double buffer[FRAME_SIZE];

	/* Plot Init */
	h=gnuplot_init();
	gnuplot_setstyle(h, "lines");
	
	int i;
	for (i=0;i<(FRAME_SIZE/HOP_SIZE-1);i++)
	  {
	    if (read_samples (infile, new_buffer, sfinfo.channels)==1)
	      fill_buffer(buffer, new_buffer);
	    else
	      {
		printf("not enough samples !!\n");
		return 1;
	      }
	  }

	/* Info file */
	printf("sample rate %d\n", sfinfo.samplerate);
	printf("channels %d\n", sfinfo.channels);
	printf("size %d\n", (int)sfinfo.frames);
	double abs[FFT_SIZE];
	for(int i=0; i<FFT_SIZE; i++){
		abs[i] = (i*sfinfo.samplerate)/(FFT_SIZE);
	}

	double buffer_amp[FFT_SIZE];
	double buffer_phs[FFT_SIZE];
	double buffer_out[FRAME_SIZE];
	double buffer_fft[FFT_SIZE];

	clock_t t1, t2;
	//double delta_t_dft=0;
	double delta_t_fftw=0;

	fft_init_forward();
	fft_init_backward();

	int freq = 0;
	double ampl = 0;
	while (read_samples (infile, new_buffer, sfinfo.channels)==1)
	{
		/* Process Samples */
		printf("Processing frame %d\n",nb_frames);

		/* hop size */
		fill_buffer(buffer, new_buffer);
		/*complex S[FRAME_SIZE];
		t1 = clock();
		dft(buffer, S);
		t2 = clock();
		delta_t_dft += (t2 - t1);*/

		for(int i=0; i<FFT_SIZE; i++){
			if(i<FRAME_SIZE){
				buffer_fft[i] = buffer[i]*(0.5-((0.5)*cos((2*M_PI*i)/FRAME_SIZE)));
				//buffer_fft[i] = buffer[i];
			} else {
				buffer_fft[i] = 0;
			}
		}
		

		t1 = clock();
		fft_exec(buffer_fft);
		t2 = clock();
		delta_t_fftw += (t2 - t1);
		cartesian_to_polar (data_out, buffer_amp, buffer_phs);


		for(int i=0; i<FFT_SIZE/2; i++){
			if((buffer_amp[i]/(FRAME_SIZE/2)) > ampl){
				freq = i;
				ampl = buffer_amp[i]/(FRAME_SIZE/2);
			}
		}

		polar_to_cartesian (buffer_amp, buffer_phs, data_in);
		fft_exec_back(buffer_out);

		
		/* PLOT */
		gnuplot_resetplot(h);
		gnuplot_plot_xy(h, abs, buffer_amp ,FRAME_SIZE/2,"dft");
		//gnuplot_plot_x(h, buffer_amp ,FRAME_SIZE,"dft");
		sleep(FRAME_SIZE/sfinfo.samplerate);

		nb_frames++;
	}

	double al = 20*log10(buffer_amp[freq-1]);
	double ac = 20*log10(buffer_amp[freq]);
	double ar = 20*log10(buffer_amp[freq+1]);

	double d = 0.5*((al-ar)/(al-2*ac+ar));
	printf("d : %f\n", d);

	double indice = freq+d;
	double estimated_freq = indice*(sfinfo.samplerate/FRAME_SIZE);
	double estimated_ampl = ac-(al-ar)*(d/4);


	//printf ("Le temps moyen écoulé pour la DFT est de %f secondes\n", (delta_t_dft / CLOCKS_PER_SEC)/nb_frames);
	printf ("Le temps moyen écoulé pour la FFTBDX est de %f secondes\n", (delta_t_fftw / CLOCKS_PER_SEC)/nb_frames);
	printf("Fréquence max : %f, d'amplitude %f +ou- %f hertz\n", estimated_freq, estimated_ampl, 0.5*(sfinfo.samplerate/FFT_SIZE));


	fft_exit();
	sf_close (infile) ;

	return 0 ;
} /* main */

