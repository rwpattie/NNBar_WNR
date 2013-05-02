#ifndef __WAVEUTIL_HH__
#define __WAVEUTIL_HH__

// ROOT Includes
#include "TROOT.h"
#include <TSQLServer.h>
#include <TSQLRow.h>
#include <TSQLResult.h>

// local includes
#include "../typesdef.h"

// C includes
#include "iostream"
#include "stdlib.h"

//-----------------------------------------------------------
// This analysis class will perform waveform processing tasks
// associated with improving the timing over the leading edge
// discrimination implemented by the fadc, determining average
// waveform shapes for each input channel, and doing other 
// things as needed.
//
// Author : R. W. Pattie Jr.
// Start Date : April 17, 2013
// 
//------------------------------------------------------------

class WaveProcessor {
  
    private :
      
      Int_t NWaves;  // number of waveforms processed
      Wave_Averages_t WAve;
                  
    public :
      
      WaveProcessor();
      ~WaveProcessor();
      //-------------------------------------------------
      // Variables
      Chan_t Chan[8]; 
      //-------------------------------------------------
      // Functions
      void     IncrimentWaveCnt(){NWaves++;}
      Double_t GetAveRiseTime() {return WAve.RiseTime;}
      Double_t GetAveLength() {return WAve.Length;}
      Double_t GetAveWidth() {return WAve.DeltaT;}
      Double_t GetAvePedestal() {return WAve.Pedestal;}
      void     ProcessWave(Fadc_Event eve);
      Int_t    CalculatePreTrigger(Fadc_Event eve,Int_t nc);
      Int_t    TimeToPeak(Fadc_Event eve,Int_t trgsample=0);
      Bool_t   GetThreshold(Int_t nrun,Int_t nchn);
};

#endif