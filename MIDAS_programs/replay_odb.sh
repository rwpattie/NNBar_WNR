#! /bin/bash
#-----------------------------------------------------------------#
# This bash script gets information about the run from the MIDAS
# ODB files and exports it to a text file and a MySQL database.
# 
# Author : R. W. Pattie Jr.
# Date   : April 1, 2013
# System : Kubuntu 12.10
#
# ----------------------------------------------------------
# Important environment variabls :
# 
#  WNR_ODB_DIR = The path to where the odb files are stored
#
#  UCNADBUSER = username for the MySQL database 
#               (ensure this user has write permission)
#
#  UCNADBPASS = Password for the MySQL database
#-----------------------------------------------------------

# set the text file output for this script
RUNLIST=runlist.log
# if the run list exists delete it.
if [ -f $RUNLIST ]
then
  rm $RUNLIST
fi
# create the file
touch $RUNLIST
# save the current odb file
echo 'about to save current odb'
odbedit -e Default -c 'save current.odb'
# set the unique id for the channel_info table to 0
nkey=3310
echo 'about to loop'
# loop over some large number that captures all the runs in question
for (( i=2174; i<2334; i++ ))
do
  # set the odb file 
  ODBFILE=$WNR_ODB_DIR/run0$(($i)).odb
  if [ -f $ODBFILE ]
  then
  # if the odbfile exists then proceed to information retrieval 
    if [ -f input_line.sql ] # check to see of the sql input file exists
    then 
      rm input_line.sql # if so delete it
    fi
  # load the ODBFILE into MIDAS memory  
  echo "loading file $ODBFILE"
  odbedit -c "load $ODBFILE"
  # Get the run number and comments
  Run_number=$( eval "odbedit -e Default -c 'ls \"/Runinfo/Run Number\"'" | awk '{print $3}' )
  COMMENT=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Comment"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  # create a MySQL command to delete the run information currently in the table
  echo "delete from wnr_run_info.run_info where wnr_run_info.run_info.run_number = $Run_number" >> input_line.sql
  # send that command to the database
  mysql -u $UCNADBUSER --password=$UCNADBPASS <  input_line.sql
  # delete the sql input file
  rm input_line.sql
  # Get the start/stop times, and the description of each channel  
  START_TIME=$(odbedit -e Default -c 'ls "/Runinfo/Start time binary"' | awk '{print $4}')
  STOP_TIME=$(odbedit -e Default -c 'ls "/Runinfo/Stop time binary"' | awk '{print $4}')
  CHN0=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 0"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN1=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 1"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN2=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 2"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN3=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 3"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN4=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 4"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN5=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 5"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN6=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 6"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  CHN7=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 7"' | awk '{ s = ""; for (i = 3; i <= NF; i++) s = s $i " "; print s }')
  # calcuate the run length in seconds
  RUNLENGTH=$(($STOP_TIME - $START_TIME)) 
  # send this information to a text file for local debugging
  echo "$Run_number, $START_TIME, $STOP_TIME, $RUNLENGTH, $COMMENT, $CHN0, $CHN1, $CHN2, $CHN3, $CHN4, $CHN5, $CHN6, $CHN7"  >> $RUNLIST
  # create a command to insert the data into the database
  echo "insert into wnr_run_info.run_info (run_number,start_time,stop_time,run_length,comment) values($Run_number,FROM_UNIXTIME($START_TIME),FROM_UNIXTIME($STOP_TIME),$RUNLENGTH,'$COMMENT')" >> input_line.sql
  # send the command to the database
  mysql -u $UCNADBUSER --password=$UCNADBPASS < input_line.sql
  # delete the sql input file
  rm input_line.sql
  #
  echo "delete from wnr_run_info.channel_info where wnr_run_info.channel_info.run_number = $Run_number" >> input_line.sql
  mysql -u $UCNADBUSER --password=$UCNADBPASS <  input_line.sql
  #-------------------------------------------------------------------------------
  # The next part of the code fills the channel_info table of the database.
  # for a simple single board set up the NFADC XX directory can be a constant
  # however if multiple boards are set up a second loop over the boards is 
  # required.
  #-------------------------------------------------------------------------------
  for (( k=0; k<3 ; k++ ))
  do
  NBRD=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/nboard"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  for (( j=0; j<8 ; j++ )) # loop over each channel
  do
  
    # delete the sql temp file
    rm input_line.sql
    
    # Get the trigger mask, upper/lower thresholds, presamples, postsamples, sd_factor, and channel description from the
    # the MIDAS odb files.
    TRMSK=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/Channel '$j'/trigger_mask"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    UTHRSH=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/Channel '$j'/upper_threshold"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    LTHRSH=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/Channel '$j'/lower_threshold"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    PRESMPL=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/Channel '$j'/presamples"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    PSTSMPL=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/Channel '$j'/stretch_samples"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    SDFACTR=$(odbedit -e Default -c 'ls "/Equipment/fADCs/Settings/NFADC 0'$k'/Channel '$j'/sd_factor"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
    COMMENTChl=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Board '$(($k+1))' Channel '$j'"' | awk '{ s = ""; for (i = 5; i <= NF; i++) s = s $i " "; print s }')
    # generate the sql command file to insert these variables to the table
    echo "insert into wnr_run_info.channel_info (run_number,trigger_mask,channel,board,upper_threshold,lower_threshold,presamples,postsamples,sd_factor,comment,nrun) values($Run_number,$TRMSK,$j,$NBRD,$UTHRSH,$LTHRSH,$PRESMPL,$PSTSMPL,$SDFACTR,'$COMMENTChl',$nkey)" >> input_line.sql
    # send the command to the database
    mysql -u $UCNADBUSER --password=$UCNADBPASS < input_line.sql
    # incriment the unique key for the channel_info table.
    nkey=$(($nkey+1))
  done
  done
  
  fi
done

#odbedit -e Default -c 'load current.odb'
