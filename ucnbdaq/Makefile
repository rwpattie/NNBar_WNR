vpath %.cpp src/
vpath %.hh inc/

ROOTCFLAGS  = $(shell root-config --cflags)
ROOTLIBS = $(shell root-config --libs)   -lNew -lMinuit -lSpectrum
ROOTGLIBS = $(shell root-config --glibs)

CXX = g++
CPPFLAGS = -I inc/
LDFLAGS = -L/usr/local/lib -L/usr/include -L/root/lib

MAIN = SimpleAnalyzer
SOURCES = PackagedFile.cpp FredDigitizer.cpp Waveform.cpp SimpleAnalysisFile.cpp
OBJECTS = $(SOURCES:.cpp=.o) $(MAIN).o
INCLUDES = $(SOURCES:.cpp=.hh) UCNBConfig.hh

all: $(MAIN)

$(MAIN): $(OBJECTS) $(MAIN).cpp
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(ROOTCFLAGS) -o $@ $(OBJECTS) $(ROOTLIBS) $(ROOTGLIBS)

$(MAIN).o : $(MAIN).cpp $(INCLUDES)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(ROOTCFLAGS) -c $< -o $@ $(ROOTLIBS) $(ROOTGLIBS)

SimpleAnalysisFile.o : SimpleAnalysisFile.cpp Waveform.o FADCEvent.hh
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(ROOTCFLAGS) -c $< -o $@ $(ROOTLIBS) $(ROOTGLIBS)

%.o : %.cpp %.hh
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(ROOTCFLAGS) -c $< -o $@ $(ROOTLIBS) $(ROOTGLIBS)
clean :
	rm -f *.o *~ $(MAIN)

.PHONY : all clean
