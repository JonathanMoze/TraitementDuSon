/* plugin.c
 */

/*****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>

/*****************************************************************************/

#include "ladspa.h"

/*****************************************************************************/

/* The port numbers for the plugin: */

#define PLUGIN_INPUT1    0
#define PLUGIN_INPUT2    1
#define PLUGIN_OUTPUT1    2
#define PLUGIN_OUTPUT2    3
#define PLUGIN_PARAM1    4
#define PLUGIN_PARAM2    5

/*****************************************************************************/

/* The structure used to hold port connection information (and gain if
   runAdding() is in use) and state (actually there's no further state
   to store here). */

typedef struct {

  /* Ports:
     ------ */

  LADSPA_Data * m_pfInputBuffer1;
  LADSPA_Data * m_pfInputBuffer2;
  LADSPA_Data * m_pfOutputBuffer1;
  LADSPA_Data * m_pfOutputBuffer2;
  LADSPA_Data * m_pfParam1;
  LADSPA_Data * m_pfParam2;


} Plugin;



int sr;
LADSPA_Data *save1;
LADSPA_Data *save2;

/* ctrl fftw */
static fftw_plan plan1 = NULL;
static fftw_plan plan2 = NULL;

/*****************************************************************************/

/* Construct a new plugin instance. */
LADSPA_Handle 
instantiatePlugin(const LADSPA_Descriptor * Descriptor,
		       unsigned long             SampleRate) {

  sr = SampleRate;
  return  malloc(sizeof(Plugin));
}

/*****************************************************************************/

/* Connect a port to a data location. */
void 
connectPortToPlugin(LADSPA_Handle Instance,
			 unsigned long Port,
			 LADSPA_Data * DataLocation) {
  switch (Port) {
  case PLUGIN_INPUT1:
    ((Plugin *)Instance)->m_pfInputBuffer1 = DataLocation;
    break;
  case PLUGIN_INPUT2:
    ((Plugin *)Instance)->m_pfInputBuffer2 = DataLocation;
    break;
  case PLUGIN_OUTPUT1:
    ((Plugin *)Instance)->m_pfOutputBuffer1 = DataLocation;
    break;
  case PLUGIN_OUTPUT2:
    ((Plugin *)Instance)->m_pfOutputBuffer2 = DataLocation;
    break;
  case PLUGIN_PARAM1:
    ((Plugin *)Instance)->m_pfParam1 = DataLocation;
    break;
  case PLUGIN_PARAM2:
    ((Plugin *)Instance)->m_pfParam2 = DataLocation;
    break;
  }
}

/*****************************************************************************/
// FFT THINGS

void
fft_init (fftw_complex *in, fftw_complex *spec, int N)
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
polar(fftw_complex *S, double *amplitude, double *phase, int N)
{
  for(int n=0; n<N; n++){
		amplitude[n] = cabs(S[n])/(N*0.5);
		phase[n] = carg(S[n]);
	}
}

void
comp(fftw_complex *S, double *amplitude, double *phase, int N)
{
  for(int n=0; n<N; n++){
    S[n] = (amplitude[n]) * cexp(I*phase[n]);
	}
}



/*****************************************************************************/
//TRAVAIL FAIT EN BINOME AVEC VICTOR BARREY
//INFORMATIONS POUR LES PARAMETRES:
//  pfParam1 = seuil sur les amplitudes des fréquence (entre 0 et 1)
//  pfParam2 = fréquence de coupure (entre 0 et 0.5)
//  
//PARAMETRES "OPTIMAUX" => pfParam1=0.005 pfParam2=0.08
//
/*****************************************************************************/

/* Run a delay line instance for a block of SampleCount samples. */
void 
runPlugin(LADSPA_Handle Instance,
	 unsigned long SampleCount) {
  
  Plugin * psPlugin;
  LADSPA_Data * pfInput1;
  LADSPA_Data * pfInput2;
  LADSPA_Data * pfOutput1;
  LADSPA_Data * pfOutput2;
  LADSPA_Data pfParam1;
  LADSPA_Data pfParam2;
  int i;
 
  psPlugin = (Plugin *)Instance;
  pfOutput1 = psPlugin->m_pfOutputBuffer1;
  pfOutput2 = psPlugin->m_pfOutputBuffer2;
  pfInput1 = psPlugin->m_pfInputBuffer1;
  pfInput2 = psPlugin->m_pfInputBuffer2;
  pfParam1 = *(psPlugin->m_pfParam1);
  pfParam2 = *(psPlugin->m_pfParam2);

  int freqCoupure = pfParam2*SampleCount;

  /* TODO */
  fftw_complex IN[SampleCount];
  fftw_complex OUT[SampleCount];
  double amplitude[SampleCount];
  double phase[SampleCount];

  fft_init(IN, OUT, SampleCount);

  for(int i=0; i<SampleCount; i++)
  {
    IN[i] = pfInput1[i];
  }

  fft();

  polar(OUT, amplitude, phase, SampleCount);


  for(int i=freqCoupure; i<(SampleCount/2); i++)
  {
    if(amplitude[i] < pfParam1){
      amplitude[i] = 0;
      amplitude[SampleCount - i] = 0; 
      phase[i] = 0;
      phase[SampleCount - i] = 0; 
    }
  }


  comp(IN, amplitude, phase, SampleCount);

  ifft();

  for(int i=0; i<SampleCount; i++)
  {
    pfOutput1[i] = creal(OUT[i]);
  }



  for(int i=0; i<SampleCount; i++)
  {
    IN[i] = pfInput2[i];
  }

  fft();

  polar(OUT, amplitude, phase, SampleCount);

  for(int i=freqCoupure; i<(SampleCount/2); i++)
  {
    if(amplitude[i] < pfParam1){
      amplitude[i] = 0;
      amplitude[SampleCount - i] = 0;
      phase[i] = 0;
      phase[SampleCount - i] = 0; 
    }
  }

  comp(IN, amplitude, phase, SampleCount);

  ifft();

  for(int i=0; i<SampleCount; i++)
  {
    pfOutput2[i] = creal(OUT[i]);
  }

  
  fft_exit();
  
}

/*****************************************************************************/

void 
cleanupPlugin(LADSPA_Handle Instance) {
  free(Instance);
}

/*****************************************************************************/

LADSPA_Descriptor * g_psDescriptor;

/*****************************************************************************/

/* _init() is called automatically when the plugin library is first
   loaded. */
void 
_init() {

  char ** pcPortNames;
  LADSPA_PortDescriptor * piPortDescriptors;
  LADSPA_PortRangeHint * psPortRangeHints;

  g_psDescriptor
    = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));

  if (g_psDescriptor) {

    g_psDescriptor->UniqueID
      = 1912;
    g_psDescriptor->Label
      = strdup("spectralnoiseremover");
    g_psDescriptor->Properties
      = LADSPA_PROPERTY_REALTIME;//HARD_RT_CAPABLE;
    g_psDescriptor->Name 
      = strdup("Spectral Noise Remover");
    g_psDescriptor->Maker
      = strdup("Master Info");
    g_psDescriptor->Copyright
      = strdup("None");
    g_psDescriptor->PortCount
      = 6;
    piPortDescriptors
      = (LADSPA_PortDescriptor *)calloc(6, sizeof(LADSPA_PortDescriptor));
    g_psDescriptor->PortDescriptors
      = (const LADSPA_PortDescriptor *)piPortDescriptors;


    piPortDescriptors[PLUGIN_INPUT1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PLUGIN_INPUT2]
      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PLUGIN_OUTPUT1]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PLUGIN_OUTPUT2]
      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
    piPortDescriptors[PLUGIN_PARAM1]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    piPortDescriptors[PLUGIN_PARAM2]
      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
    pcPortNames
      = (char **)calloc(6, sizeof(char *));
    g_psDescriptor->PortNames 
      = (const char **)pcPortNames;
    pcPortNames[PLUGIN_INPUT1]
      = strdup("Input1");
    pcPortNames[PLUGIN_INPUT2]
      = strdup("Input2");
    pcPortNames[PLUGIN_OUTPUT1]
      = strdup("Output1");
    pcPortNames[PLUGIN_OUTPUT2]
      = strdup("Output2");
    pcPortNames[PLUGIN_PARAM1]
      = strdup("Control 1");
    pcPortNames[PLUGIN_PARAM2]
      = strdup("Control 2");
    psPortRangeHints = ((LADSPA_PortRangeHint *)
			calloc(6, sizeof(LADSPA_PortRangeHint)));
    g_psDescriptor->PortRangeHints
      = (const LADSPA_PortRangeHint *)psPortRangeHints;
    psPortRangeHints[PLUGIN_INPUT1].HintDescriptor
      = 0;
    psPortRangeHints[PLUGIN_INPUT2].HintDescriptor
      = 0;
    psPortRangeHints[PLUGIN_OUTPUT1].HintDescriptor
      = 0;
    psPortRangeHints[PLUGIN_OUTPUT2].HintDescriptor
      = 0;
    psPortRangeHints[PLUGIN_PARAM1].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW |
	LADSPA_HINT_BOUNDED_ABOVE);
    psPortRangeHints[PLUGIN_PARAM1].LowerBound 
      = 0;
    psPortRangeHints[PLUGIN_PARAM1].UpperBound 
      = 1;
    psPortRangeHints[PLUGIN_PARAM2].HintDescriptor
      = (LADSPA_HINT_BOUNDED_BELOW |
	LADSPA_HINT_BOUNDED_ABOVE);
    psPortRangeHints[PLUGIN_PARAM2].LowerBound 
      = 0;
    psPortRangeHints[PLUGIN_PARAM2].UpperBound 
      = 0.5;
    g_psDescriptor->instantiate 
      = instantiatePlugin;
    g_psDescriptor->connect_port 
      = connectPortToPlugin;
    g_psDescriptor->activate
      = NULL;
    g_psDescriptor->run
      = runPlugin;
    g_psDescriptor->run_adding
      = NULL;
    g_psDescriptor->deactivate
      = NULL;
    g_psDescriptor->cleanup
      = cleanupPlugin;
  }
}

/*****************************************************************************/

/* _fini() is called automatically when the library is unloaded. */
void 
_fini() {
  long lIndex;
  if (g_psDescriptor) {
    free((char *)g_psDescriptor->Label);
    free((char *)g_psDescriptor->Name);
    free((char *)g_psDescriptor->Maker);
    free((char *)g_psDescriptor->Copyright);
    free((LADSPA_PortDescriptor *)g_psDescriptor->PortDescriptors);
    for (lIndex = 0; lIndex < g_psDescriptor->PortCount; lIndex++)
      free((char *)(g_psDescriptor->PortNames[lIndex]));
    free((char **)g_psDescriptor->PortNames);
    free((LADSPA_PortRangeHint *)g_psDescriptor->PortRangeHints);
    free(g_psDescriptor);
  }
}
  
/*****************************************************************************/

/* Return a descriptor of the requested plugin type. */
const LADSPA_Descriptor * 
ladspa_descriptor(unsigned long Index) {
  if (Index == 0)
    return g_psDescriptor;
  else
    return NULL;
}

/*****************************************************************************/

/* EOF */
