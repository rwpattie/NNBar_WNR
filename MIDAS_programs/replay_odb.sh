#! /bin/bash

RUNLIST=runlist.log

if [ -f $RUNLIST ]
then
  rm $RUNLIST
fi

touch $RUNLIST

odbedit -e Default -c 'save current.odb'
nkey=0
for (( i=0; i<700; i++ ))
do
  ODBFILE=$WNR_ODB_DIR/run00$(($i)).odb
  if [ -f $ODBFILE ]
  then
    if [ -f input_line.sql ]
    then 
      rm input_line.sql
    fi
  echo "loading file $ODBFILE"
  odbedit -c "load $ODBFILE"
  Run_number=$( eval "odbedit -e Default -c 'ls \"/Runinfo/Run Number\"'" | awk '{print $3}' )
  COMMENT=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Comment"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  echo "delete from wnr_run_info.run_info where wnr_run_info.run_info.run_number = $Run_number" >> input_line.sql
  mysql -u $UCNADBUSER --password=$UCNADBPASS <  input_line.sql
  rm input_line.sql
  START_TIME=$(odbedit -e Default -c 'ls "/Runinfo/Start time binary"' | awk '{print $4}')
  STOP_TIME=$(odbedit -e Default -c 'ls "/Runinfo/Stop time binary"' | awk '{print $4}')
  
  START_TIMES=$(odbedit -e Default -c 'ls "/Runinfo/Start time"' | awk '{print $4}')
  STOP_TIMES=$(odbedit -e Default -c 'ls "/Runinfo/Stop time"' | awk '{print $4}')
  
  CHN0=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 0"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN1=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 1"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN2=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 2"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN3=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 3"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN4=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 4"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN5=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 5"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN6=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 6"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN7=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 7"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  
  RUNLENGTH=$(($STOP_TIME - $START_TIME)) 
  
  echo "$Run_number, $START_TIME, $STOP_TIME, $RUNLENGTH, $COMMENT, $CHN0, $CHN1, $CHN2, $CHN3, $CHN4, $CHN5, $CHN6, $CHN7"  >> $RUNLIST
  echo "insert into wnr_run_info.run_info (run_number,start_time,stop_time,run_length,comment) values($Run_number,FROM_UNIXTIME($START_TIME),FROM_UNIXTIME($STOP_TIME),$RUNLENGTH,'$COMMENT')" >> input_line.sql
  mysql -u $UCNADBUSER --password=$UCNADBPASS < input_line.sql
  rm input_line.sql
  echo "delete from wnr_run_info.channel_info where wnr_run_info.channel_info.run_number = $Run_number" >> input_line.sql
  mysql -u $UCNADBUSER --password=$UCNADBPASS <  input_line.sql
  
  for (( j=0; j<8 ; j++ ))
  do
    rm input_line.sql
    echo "delete from wnr_run_info.channel_info where wnr_run_info.channel_info.nrun = $nkey" >> input_line.sql
    mysql -u $UCNADBUSER --password=$UCNADBPASS <  input_line.sql
    rm input_line.sql
    
    TRMSK=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 00/Channel '$j'/trigger_mask"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    UTHRSH=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 00/Channel '$j'/upper_threshold"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    LTHRSH=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 00/Channel '$j'/lower_threshold"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    PRESMPL=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 00/Channel '$j'/presamples"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    PSTSMPL=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 00/Channel '$j'/stretch_samples"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    SDFACTR=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 00/Channel '$j'/sd_factor"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    COMMENTChl=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel '$j'"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
    
    echo "insert into wnr_run_info.channel_info (run_number,trigger_mask,channel,upper_threshold,lower_threshold,presamples,postsamples,sd_factor,comment,nrun) values($Run_number,$TRMSK,$j,$UTHRSH,$LTHRSH,$PRESMPL,$PRESMPL,$SDFACTR,'$COMMENTChl',$nkey)" >> input_line.sql
    mysql -u $UCNADBUSER --password=$UCNADBPASS < input_line.sql
    nkey=$(($nkey+1))
  done
  
  fi
done

#odbedit -e Default -c 'load current.odb'
