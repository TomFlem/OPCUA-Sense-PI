BIN=../bin
IDIR =../include
CC=g++
CFLAGS=-I$(IDIR) -pthread -lRTIMULib -Wno-write-strings `pkg-config --cflags MagickWand` -lsense

ODIR=obj
LDIR =../lib

LIBS=-lm

LIST=$(BIN)/server

all:	$(LIST)

_DEPS = server.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = server.o open62541.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(BIN)/%: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	@rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ $(LIST)
