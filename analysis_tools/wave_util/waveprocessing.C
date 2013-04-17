#ifndef __WAVEUTIL_CC__
#define __WAVEUTIL_CC__

#include "waveprocessing.h"

WaveProcessor::WaveProcessor()
{
  this->NWaves=0;
}

WaveProcessor::~WaveProcessor()
{
  
}

void WaveProcessor::ProcessWave(Fadc_Event eve)
{
   Int_t PreTriggerTime = CalculatePreTrigger(eve);
  
}
//---------------------------------------------------------------------
Int_t WaveProcessor::CalculatePreTrigger(Fadc_Event eve)
{
    Int_t precnt  = 0; // counter for the pretrigger samples
    Int_t postcnt = eve.last;
    
    do{
      precnt++;
    }while( eve.adc[precnt] < Chan.uthresh && 
	    eve.adc[precnt] > Chan.lthresh);
    
    do{
      postcnt--;
    }while( eve.adc[precnt] < Chan.uthresh && 
	    eve.adc[precnt] > Chan.lthresh);
    
    
    std::cout << "Pretrigger samples are : "<< precnt << "\t" << 16*(Chan.pre+1) << std::endl;
    std::cout << "Pretrigger samples are : "<< postcnt << "\t" << (Chan.post) << std::endl;
  return precnt; 
}
//---------------------------------------------------------------------
Bool_t WaveProcessor::GetThreshold(Int_t nrun, Int_t nchn)
{
  // This function communes with the MySQL run log to get the channel
  // setting for the run in question.
  
  TSQLResult *res;
  TSQLRow    *row;
  
  if(nchn < 0 || nchn > 7){
      std::cout << "Channel " << nchn << " is not valid !!!! " << std::endl;
      return kFALSE;
  }
  
  TSQLServer *sql = TSQLServer::Connect("mysql://localhost/wnr_run_info",
					getenv("UCNADBUSER"),getenv("UCNADBPASS"));
  
  char query[500];
  sprintf(query,"select upper_threshold,lower_threshold,presamples,postsamples from "
                "channel_info where run_number = %d and "
		" channel = %d",nrun,nchn);
  
  res = (TSQLResult*)sql->Query(query);
  
  if(res->GetRowCount() != 0){
      while((row = (TSQLRow*)res->Next())){
	  Chan.uthresh = atoi(row->GetField(0));
	  Chan.lthresh = atoi(row->GetField(1));
	  Chan.pre     = atoi(row->GetField(2));
	  Chan.post    = atoi(row->GetField(3));
      }
  }
  
  return kTRUE; 
}

#endif
