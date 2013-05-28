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
   //Int_t PreTriggerTime = CalculatePreTrigger(eve);
  
}
//---------------------------------------------------------------------
Int_t WaveProcessor::CalculatePreTrigger(Fadc_Event eve,Int_t nc)
{
    Int_t precnt  = 0; // counter for the pretrigger samples
    Int_t postcnt = eve.last;
    
    do {
      precnt++;
    } while( eve.adc[precnt] < Chan[nc].uthresh && 
	     eve.adc[precnt] > Chan[nc].lthresh);
    
    do {
      postcnt--;
    } while( eve.adc[precnt] < Chan[nc].uthresh && 
	     eve.adc[precnt] > Chan[nc].lthresh);
    /*
    std::cout << "Pretrigger samples are : "<< precnt << "\t" << 16*(Chan.pre+1) << std::endl;
    
    std::cout << "Pretrigger samples are : "<< postcnt << "\t" << (Chan.post) << std::endl;
    */
    return precnt; 
}
//---------------------------------------------------------------------
Int_t WaveProcessor::TimeToPeak(Fadc_Event eve,Int_t trgsample)
{
    // This function calculates the number of samples between
    // the trgsample and the maximum adc sample.  By default 
    // trgsample is set to 0 and the delta t from the beginning
    // of the waveform will be return.  If trgsample is set to a
    // nonzero value that is less than the number of samples, then
    // the the number of sample between that point and the maximum
    // will be returned.
    
    if(trgsample > eve.last){
//	std::cout << "Trigger point incorrectly set, returing to"
//		     " default paramater" << std::endl;
	trgsample = 0;
    }
    
    Int_t maxbin    = 0;
    Int_t max_value = 0;
    
    // simple loop over samples to find the maximum
    for(Int_t i = trgsample ; i < eve.last ; i++){
	if(eve.adc[i] > max_value){
	  maxbin = i;
	  max_value = eve.adc[i];
	}
    }
    // return the difference.
    return maxbin - trgsample;
}
//---------------------------------------------------------------------
Bool_t WaveProcessor::GetThreshold(Int_t nrun, Int_t nchn)
{
  // This function communes with the MySQL run log to get the channel
  // setting for the run in question.
  //--------------------------------------------------------------
  TSQLResult *res;  // Create pointers to the mysql results
  TSQLRow    *row;
  //---------------------------------------------------------------
  // Check to see if the channel passed is in the valid range.
  if(nchn < 0 || nchn > 7){
      std::cout << "Channel " << nchn << " is not valid !!!! " << std::endl;
      return kFALSE;
  }
  // Connect to the database.
  TSQLServer *sql = TSQLServer::Connect("mysql://localhost/wnr_run_info",
					getenv("UCNADBUSER"),getenv("UCNADBPASS"));
  // Generate the query
  char query[500];
  
  sprintf(query,"select upper_threshold,lower_threshold,presamples,postsamples from "
                "channel_info where run_number = %d and "
		" channel = %d",nrun,nchn);
  // submit the query to the database
  res = (TSQLResult*)sql->Query(query);
  // process the results
  if(res->GetRowCount() != 0){
      while((row = (TSQLRow*)res->Next())){
	  Chan[nchn].uthresh = atoi(row->GetField(0));
	  Chan[nchn].lthresh = atoi(row->GetField(1));
	  Chan[nchn].pre     = atoi(row->GetField(2));
	  Chan[nchn].post    = atoi(row->GetField(3));
      }
  }
  std::cout << "Channel info" << nchn << std::endl;
  std::cout << "---------------------------------------------"<<std::endl;
  std::cout << "pre-trigger " << Chan[nchn].pre << std::endl;
  
  return kTRUE; 
}

Int_t WaveProcessor::EventWght(Fadc_Event eve,Int_t *x)
{
  
  Bool_t AboveThr = kFALSE;
  Int_t npeak = 0;
  Int_t nch = eve.channel;
  Int_t current = 0;
  
  for(Int_t i = 0 ; i < eve.last ; i++){
      if(eve.adc[i] < Chan[nch].uthresh && eve.adc[i] > Chan[nch].lthresh){
	if(AboveThr){
	  x[npeak] = current;
	  npeak++;
	  current = 0;
	  AboveThr = kFALSE;
	}
      }
      if(eve.adc[i] > Chan[nch].uthresh || eve.adc[i] < Chan[nch].lthresh){
	if(!AboveThr)
	  AboveThr = kTRUE;
	else {
	    if(eve.adc[i] > current && Chan[nch].uthresh < eve.adc[i])
		current = eve.adc[i];
	    else if(eve.adc[i] < current && Chan[nch].lthresh > eve.adc[i])
		current = eve.adc[i];
	}
      }
  }
  //std::cout << "Found npeaks " << npeak << std::endl;
  npeak = 1;
  return npeak;
}

#endif
