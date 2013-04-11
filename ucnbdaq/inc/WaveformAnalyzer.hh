// File: WaveformAnalyzer.hh
// Name: Leah Broussard
// Date: 2013/4/11
// Purpose: Analyzes waveforms for energy and timing
//
// Revision History:
// 2013/4/11: LJB  Shaping waveforms with poly4 fit

#ifndef WAVEFORM_ANALYZER_HH__
#define WAVEFORM_ANALYZER_HH__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include "SimpleAnalysisFile.hh"
#include "TGraph.h"

#define FITPOLY 5       //order+1 of fit polynomial

using std::cout;
using std::endl;

class WaveformAnalyzer
{
private:
  bool dodraw;

public:
  SimpleAnalysisFile SAFile;
  TGraph* g;
  TF1* f;
  TH1D* h;

  inline WaveformAnalyzer(); 
  inline ~WaveformAnalyzer();
  void Open(int filenum) {SAFile.Open(filenum);}
  void Close() {SAFile.Close();}
  inline bool IsOpen() {return SAFile.IsOpen();}
  void GetEntry(int ev);
  void Plot(int ev);
  void Plot();
  void FindMinLoc(int& min, int& mini);
  void LocalFit(int start, int window);
  void LocalFitZero(int window);
  void LocalFitMin(int window);
  double FindAmplitudeFast();
  void BuildSpectrumFast();
};

#endif // WAVEFORM_ANALYZER_HH__
