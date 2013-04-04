vpath %.cpp src/
vpath %.hh inc/

ROOTCFLAGS  = $(shell root-config --cflags)
ROOTLIBS = $(shell root-config --libs)   -lNew -lMinuit -lSpectrum
ROOTGLIBS = $(shell root-config --glibs)

CXX = g++
CPPFLAGS = -I inc/
LDFLAGS = -L/usr/local/lib -L/usr/include -L/root/lib

MAIN = unpack
SOURCES = PackagedFile.cpp unpack.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(MAIN)

$(MAIN): $(OBJECTS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(ROOTCFLAGS) -o $@ $(OBJECTS) $(ROOTLIBS) $(ROOTGLIBS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(LDFLAGS) $(ROOTCFLAGS) -c $< -o $@ $(ROOTLIBS) $(ROOTGLIBS)
clean :
	rm -f *.o *~ unpack

.PHONY : all clean
