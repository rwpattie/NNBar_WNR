ObjSuf        = o
SrcSuf        = C
ExeSuf        =
DllSuf        = so
EVENTLIB      = $(EVENTO)
OutPutOpt     = -o
OFFICIAL      =UCNA_Official_Replay

EXE = analyzer

ROOTCFLAGS    = $(shell root-config --cflags)
ROOTLIBS      = $(shell root-config --libs)   -lNew -lMinuit -lSpectrum
ROOTGLIBS     = $(shell root-config --glibs)


MYOS := $(subst -,,$(shell uname))
# Linux with egcs       

        CXX           = gcc
        CXXFLAGS      = -O3 -Wall -fPIC -g -Wl,--no-as-needed
        LD            = gcc
        LDFLAGS       = -g
        SOFLAGS       = -shared
        CXXFLAGS     += $(ROOTCFLAGS) 
        LIBS          = $(ROOTLIBS) 
        GLIBS         = $(ROOTGLIBS)
        LIBS         += -lstdc++ -lz -lRMySQL

%: %.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)

waveviewer: wave_form_viewer.o
	$(CXX) $(CXXFLAGS) -o wave_form_viewer.o $(LIBS) -L $(LIBS) $(GLIBS)  
