#  -----------------------------------------------------------------------------
# 
#                      GNU General Public Licence
#           
#       This program is free software; you can redistribute it and/or
#       modify it under the terms of the GNU General Public License as
#       published by the Free Software Foundation; either version 2 of
#       the License, or (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS For A PARTICULAR PURPOSE. See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public
#       License along with this program; see the file COPYING. If not,
#       write to the Free Software Foundation, Inc., 59
#       Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  -----------------------------------------------------------------------------
#

#--------------------------------
# GENERAL VARIABLES
#--------------------------------

CC = gcc 
CFLAGS = -O3 -Wall -lm -I ${HOME}/local/include -pg -g -L ${HOME}/local/lib

#--------------------------------
# LIBRARY SOURCES
#--------------------------------

LIB_SRC =  gnuplot_i.c 
LIB_OBJ = $(LIB_SRC:.c=.o)
EXECS = analyse_synthese

all: $(EXECS)

analyse_synthese : $(LIB_OBJ) $(LIB_SRC) analyse_synthese.c
	$(CC) $(CFLAGS) $(LIB_OBJ) analyse_synthese.c -o $(EXECS) -lsndfile  -lvorbis -lvorbisenc -logg -lFLAC -lm -lfftw3

clean : 
	rm -f *.o 

cleanall :
	rm -f *.o *~ 

