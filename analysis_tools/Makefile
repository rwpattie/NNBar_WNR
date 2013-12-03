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
        CXXFLAGS     += $(ROOTCFLAGS) -Iwave_util -Idetector_utils -I./ -Itrajectory_sim
        LIBS          = $(ROOTLIBS) 
        GLIBS         = $(ROOTGLIBS)
        LIBS         += -lstdc++ -lz -lRMySQL
       
%: %.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)
%: wave_util/%.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)
	
%: detector_utils/%.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)
	
%: trajectory_sim/%.C
	$(CXX) $(CXXFLAGS) $< -o $@  $(LIBS) $(GLIBS)

fOBJ= fission_foil_analyzer.o wave_util/waveprocessing.o detector_utils/fission_detector.o\
      detector_utils/gastube_detector.o
      
wOBJ=  wave_form_math.o wave_form_viewer.o

tOBJ= trajectory_sim/neutron_drift_tube.o
      
trajectory: $(tOBJ)
	$(CXX) $(CXXFLAGS) -o trajsim $(LIBS) $(tOBJ) -L $(LIBS) $(GLIBS) 

waveviewer: $(wOBJ)
	$(CXX) $(CXXFLAGS) -o wave_form_viewer $(LIBS) $(wOBJ) -L $(LIBS) $(GLIBS) 

fission: $(fOBJ)
	$(CXX) $(CXXFLAGS) -o fission_foil_analyzer $(LIBS) $(fOBJ) -L $(LIBS) $(GLIBS) 
	
