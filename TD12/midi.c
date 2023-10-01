#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "midifile.h"

void read_midi(char * midifile, int transpose, int channel);

int main (int argc, char *argv[])
{	
	int transpose = 0;
	if(argc == 4){
		transpose = atoi(argv[2]);
	}
	int channel = atoi(argv[3]);
	read_midi(argv[1], transpose, channel);

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

	if(octave != -1){
		printf("Note: %s et octave: %d\n", note, octave);
	} else {
		printf("Note: %s", note);
	}
}


void read_midi(char * midifile, int transpose, int newChannel)
{
  MidiFileEvent_t event;
  unsigned char *data;
  MidiFile_t md = MidiFile_load(midifile);

  event = MidiFile_getFirstEvent(md);

  int mat[12][12];
  int i, j;
  for(i=0; i<12; i++) {
      for(j=0;j<12;j++) {
         mat[i][j] = 0;
      }
   }



  int noteBefore = -1;
  /* boucle principale */
  while(event)
    {
      // A completer : 
      if(MidiFileEvent_isNoteStartEvent(event)){
        // Channel
        int channel = MidiFileNoteStartEvent_getChannel(event);
        if(channel != 10){
          // affichage du nom des notes
          float start = MidiFile_getTimeFromTick(md, MidiFileEvent_getTick(event));
          int note = MidiFileNoteStartEvent_getNote(event);
          print_note(note%12, note/12);
		  printf("%d\n", noteBefore);
		  if(noteBefore != -1){
		  	mat[noteBefore][note%12]++;
		  }

          noteBefore = note%12;
          // affichage de la dur�e des notes
          MidiFileEvent_t note_end = MidiFileNoteStartEvent_getNoteEndEvent(event);
          float end = MidiFile_getTimeFromTick(md, MidiFileEvent_getTick(note_end));
          //printf("Duration : %fs\n", end - start);

          //// change pitch tone
          //MidiFileNoteStartEvent_setChannel(event, newChannel);
          //MidiFileNoteStartEvent_setNote(event, note + transpose);
		  //// change end pitch tone
          //MidiFileNoteEndEvent_setNote(note_end, note + transpose);
		  //MidiFileNoteEndEvent_setChannel(note_end, newChannel);
		  //// change channel
          //puts("\t");
        }
      }
      event = MidiFileEvent_getNextEventInFile(event);
    }

	int sum [12];

	for(int i= 0; i<12; i++){
		int s =0;
		for(int j=0; j<12 ; j++){
			printf("%d  " , mat[i][j]);
			s += mat[i][j];
		}
		sum[i] = s; 
		printf("\n");
	}


	printf("to generation------------------------\n");
	noteBefore = -1;

	md = MidiFile_load(midifile);
	event = MidiFile_getFirstEvent(md);
	srand(time(NULL));
	while(event)
	{
		if(MidiFileEvent_isNoteStartEvent(event)){
        // Channel
        int channel = MidiFileNoteStartEvent_getChannel(event);
        if(channel != 10){
          int note = MidiFileNoteStartEvent_getNote(event);
		  int octave = note/12;
          MidiFileEvent_t note_end = MidiFileNoteStartEvent_getNoteEndEvent(event);
		  int newNote;
		  if(noteBefore != -1){
			int random = rand()%(sum[note%12]-1) +1;
			int s = 0; 
			for(int i=0; i<12; i++){
				//printf("\t%d\n", mat[noteBefore][i]);
				s += mat[noteBefore][i];
				if(random <= s){
					newNote = i;
				}
			}
			newNote = newNote + (octave*12);
		  } else {
			newNote = note;
		  }
		  noteBefore = note;
          print_note(newNote%12, newNote/12);
		
          MidiFileNoteStartEvent_setNote(event, newNote);
          MidiFileNoteEndEvent_setNote(note_end, newNote);
        }
      }
      event = MidiFileEvent_getNextEventInFile(event);
	}

	MidiFile_save(md, "newMIDI.mid");
}
