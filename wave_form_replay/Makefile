#ROOTFLAGS = $(shell $(ROOTSYS)/bin/root-config --libs --glibs --cflags --ldflags --auxlibs --auxcflags)


ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLIBS      = $(shell root-config --libs)   -lNew -lMinuit -lSpectrum
ROOTGLIBS     = $(shell root-config --glibs)

all: na 

na: newanalyzer_nnbar.cpp
	g++ -L/usr/local/lib -L/usr/include -L/root/lib $(ROOTCFLAGS) -o $@ $< $(ROOTLIBS) $(ROOTGLIBS)
