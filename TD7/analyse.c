#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <complex.h>
#include <fftw3.h>
#include <sndfile.h>

#include <math.h>

#include "gnuplot_i.h"

#define	FRAME_SIZE 4096
#define HOP_SIZE 4096
#define THO_MAX 1024

static gnuplot_ctrl *h;
static fftw_plan plan;

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

void
fft_init (complex in[FRAME_SIZE], complex spec[FRAME_SIZE])
{
  plan = fftw_plan_dft_1d(FRAME_SIZE, in, spec, FFTW_FORWARD, FFTW_ESTIMATE);
}

void
fft_exit (void)
{
  fftw_destroy_plan(plan);
}

void
fft_process (void)
{
  fftw_execute(plan);
}

void print_note(int pitch, int octave)
{
	char *note;
	switch(pitch){
		case 0:
			note = "Do";
			break;
		case 1:
			note = "Do#";
			break;
		case 2:
			note = "Ré";
			break;
		case 3:
			note = "Ré#";
			break;
		case 4:
			note = "Mi";
			break;
		case 5:
			note = "Fa";
			break;
		case 6:
			note = "Fa#";
			break;
		case 7:
			note = "Sol";
			break;
		case 8:
			note = "Sol#";
			break;
		case 9:
			note = "La";
			break;
		case 10:
			note = "La#";
			break;
		case 11:
			note = "Si";
			break;
		default:
			note = "INDEF";
			break;
	}
	printf("Note: %s et octave: %d\n", note, octave);

}

void get_auto(double *buffer, double *bufferC, int *pitch, int *octave)
{
	for(int t=0; t<THO_MAX; t++){
		double valT = 0;
		for(int i=0; i<(FRAME_SIZE-t); i++){
			valT += buffer[i]*buffer[i+t];
		}
		bufferC[t] = (1.0/FRAME_SIZE)*valT;
	}
	int index = 1;
	while(!(bufferC[index-1] > bufferC[index] && bufferC[index+1] > bufferC[index])){index++;}
	double ampMax=bufferC[index];
	for(int i=index; i<FRAME_SIZE-10; i++){
		if(bufferC[i] > ampMax){
			ampMax = bufferC[i];
			index = i;
		}
	}
	double F0 = 44100*1.0/index;
	*pitch = (int)(round(57+12*log2(F0/440)))%12;
	*octave = round(57+12*log2(F0/440))*1.0/12;
}

void get_chroma(double *amp_buffer, double *chroma,int samplerate){
	for(int i=0; i<12; i++){
		chroma[i]=0.0;
	}
	for(int k=1; k<(FRAME_SIZE/2)-1; k++){
		double amp = amp_buffer[k];
		if(amp_buffer[k-1]<amp && amp>amp_buffer[k+1]){
			int freq = (k*samplerate)/FRAME_SIZE;
			int pitch = (int)(round(57+12*log2(freq/440)))%12;
			chroma[pitch] += pow(amp, 2);
		}
	}
}


void get_threshold_audibility(double *threshold, double*bark_abs, int samplerate)
{
	for(int i=0; i<FRAME_SIZE/2; i++){
		double freq = (i*samplerate)/(FRAME_SIZE);
		threshold[i] = 3.64*pow((freq/1000),-0.8) - 6.5*expf(-0.6*pow(((freq/1000)-3.3), 2)) + 1e-3 * pow((freq/1000),4);

		if(freq <= 500)
			bark_abs[i] = freq/100;
		else
			bark_abs[i] = 9+4*log2(freq/1000);
	}
}

void get_E_byFrame(double *E, double *buffer, int frame)
{	
	double e = 0.0;
	for(int i=0; i<FRAME_SIZE; i++){
		e += pow(buffer[i], 2);
	}
	e = sqrt(e*(1.0/FRAME_SIZE));
	E[frame] =  e;
}


int
main (int argc, char * argv [])
{	
	char 		*progname, *infilename;
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

	complex samples[FRAME_SIZE];
	double amplitude[FRAME_SIZE];
	complex spectrum[FRAME_SIZE];
	double bufferC[FRAME_SIZE];
	//double bufferThreshold[FRAME_SIZE/2];
	//double bufferBark[FRAME_SIZE/2];
	double bufferEnergie[FRAME_SIZE];
	//double chroma[12];

	int pitch;
	int octave;

	int lastPitch=-1;
	int lastOctave=-1;

	/* FFT init */
	fft_init(samples, spectrum);
		
	while (read_samples (infile, new_buffer, sfinfo.channels)==1)
	  {
	    /* Process Samples */
	    //printf("Processing frame %d\n",nb_frames);

	    /* hop size */
	    fill_buffer(buffer, new_buffer);

	    // fft process
	    for (i=0; i < FRAME_SIZE; i++)
	      {
		// Fenetre Hann
		//samples[i] = buffer[i]*(0.5-0.5*cos(2.0*M_PI*(double)i/FRAME_SIZE));
		// Fenetre rect
			samples[i] = buffer[i];
	      }
	    for (i=FRAME_SIZE; i < FRAME_SIZE; i++)
	      {
			samples[i] = 0.0;
	      }

	    
	    fft_process();

	    // spectrum contient les complexes résultats de la fft
	    for (i=0; i < FRAME_SIZE; i++)
	      {
			amplitude[i] = cabs(spectrum[i]);
	      }


	    int imax=0;
	    double max = 0.0;
	    
	    for (i=0; i < FRAME_SIZE/2; i++){
			if (amplitude[i] > max)
		  	{
		    	max = amplitude[i];
		    	imax = i;
		  	}    
	    }


		/*get_threshold_audibility(bufferThreshold, bufferBark,sfinfo.samplerate);
		for(int i=0; i<FRAME_SIZE/2; i++){
			amplitude[i] = 20*log10(amplitude[i]/1e-6);
		}
		*/
		get_E_byFrame(bufferEnergie, buffer, nb_frames);
		

		/* plot amplitude */
	    //gnuplot_resetplot(h);
	    //gnuplot_plot_xy(h, bufferBark, bufferThreshold, FRAME_SIZE/2,"seuil d'audibilité");
		//gnuplot_plot_xy(h, bufferBark, amplitude, FRAME_SIZE/2,"amplitude(DB)");
	    ///sleep(1);


	
		
		
		//MONOPHONIE
		get_auto(buffer, bufferC, &pitch, &octave);
		if(bufferEnergie[nb_frames] > 0.04){
			if(pitch != lastPitch || octave != lastOctave){
				print_note(pitch, octave);
				lastPitch = pitch;
				lastOctave = octave;
			}
		} else {
			printf("blanc \n");
			lastPitch = -1;
			lastOctave = -1;
		}

		//POLYPHONIE
		
		/* PLOT */
		/*
		get_chroma(amplitude, chroma ,sfinfo.samplerate);
	    gnuplot_resetplot(h);
	    gnuplot_plot_x(h, chroma,12,"temporal frame");
	    sleep(1);*/
		
		

	    /* plot amplitude */
	    /*gnuplot_resetplot(h);
	    gnuplot_plot_x(h,bufferC,FRAME_SIZE,"amplitude");
	    sleep(1);*/
    
	    /* PLOT */
	    /*gnuplot_resetplot(h);
	    gnuplot_plot_x(h,amplitude,FRAME_SIZE/2,"temporal frame");
	    sleep(1);*/
    
    	    
	    nb_frames++;
	  }
	gnuplot_resetplot(h);
	gnuplot_plot_x(h, bufferEnergie, nb_frames,"energie");
	sleep(100);
	sf_close (infile) ;

	/* FFT exit */
	fft_exit();
	
	return 0 ;
} /* main */

