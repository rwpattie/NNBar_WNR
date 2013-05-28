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
        CXXFLAGS     += $(ROOTCFLAGS) -Iwave_util -Idetector_utils
        LIBS          = $(ROOTLIBS) 
        GLIBS         = $(ROOTGLIBS)
        LIBS         += -lstdc++ -lz -lRMySQL

%: %.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)

%: wave_util/%.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)
	
%: detector_utils/%.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)


fOBJ= fission_foil_analyzer.o wave_util/waveprocessing.o detector_utils/fission_detector.o\
      detector_utils/gastube_detector.o
      

waveviewer: wave_form_viewer.o
	$(CXX) $(CXXFLAGS) -o wave_form_viewer $(LIBS) wave_form_viewer.o -L $(LIBS) $(GLIBS) 

fission: $(fOBJ)
	$(CXX) $(CXXFLAGS) -o fission_foil_analyzer $(LIBS) $(fOBJ) -L $(LIBS) $(GLIBS) 
	
