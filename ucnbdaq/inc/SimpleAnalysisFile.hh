// File: SimpleAnalysisFile.hh
// Name: Leah Broussard
// Date: 2013/4/9
// Purpose: Handles the zeroth level analysis ROOT TTree
//
// Revision History:
// 2013/4/9:  LJB  skeleton class
// 2013/4/10: LJB  added open/create/fill/write functions and analysis 
//                 routines.
 
#ifndef SIMPLE_ANALYSIS_FILE_HH__
#define SIMPLE_ANALYSIS_FILE_HH__

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

//#include <TROOT.h>
#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"

#include "FredDigitizer.hh"
#include "UCNBConfig.hh"
#include "Waveform.hh"

using std::cout;
using std::endl;

class SimpleAnalysisFile
{

private:
  TTree* t;
  bool createmode;
  int privatevariable;
  
public:
  SimpleAnalysisFile();
  ~SimpleAnalysisFile();
  void Close();
  bool Open(std::string path, std::string name);
  bool Open(std::string filename);
  bool Open(int filenum);
  bool Create(std::string path, std::string name);
  bool Create(std::string filename);
  bool Create(int filenum);
  void AnalyzePackets(Waveform& WaveList);
  UShort_t CalcMax(std::vector<Int_t> data);
  UShort_t CalcMin(std::vector<Int_t> data);
  UShort_t CalcZero(std::vector<Int_t> data);
  UShort_t CalcPed(std::vector<Int_t> data);
  void FillTree();
  void Write();
  ULong64_t first_time;  
  ULong64_t global_time;
  Int_t     packet_time_s;
  Int_t     packet_time_us;
  Int_t     packet_time_l;
  Int_t     board;
  Int_t     last;    //TTree requires Int_t for array index
  UShort_t  adc[5000];
  UShort_t  max;
  UShort_t  min;
  UShort_t  zero;
  UShort_t  ped;
  UShort_t  channel;
  TFile* RootFile;   // may as well be public
  TTree* RootTree;
};

#endif // SIMPLE_ANALYSIS_FILE_HH__
