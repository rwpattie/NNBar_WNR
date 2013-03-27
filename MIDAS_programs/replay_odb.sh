#! /bin/bash

RUNLIST=runlist.log

if [ -f $RUNLIST ]
then
  rm $RUNLIST
fi

touch $RUNLIST

odbedit -e Default -c 'save current.odb'

for (( i=0; i<700; i++ ))
do
  ODBFILE=$WNR_ODB_DIR/run00$(($i)).odb
  if [ -f $ODBFILE ]
  then
  echo "loading file $ODBFILE"
  odbedit -c "load $ODBFILE"
  Run_number=$( eval "odbedit -e Default -c 'ls \"/Runinfo/Run Number\"'" | awk '{print $3}' )
  COMMENT=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Comment"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  START_TIME=$(odbedit -e Default -c 'ls "/Runinfo/Start time binary"' | awk '{print $4}')
  STOP_TIME=$(odbedit -e Default -c 'ls "/Runinfo/Stop time binary"' | awk '{print $4}')

  CHN0=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 0"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN1=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 1"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN2=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 2"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN3=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 3"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN4=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 4"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN5=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 5"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN6=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 6"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  CHN7=$(odbedit -e Default -c 'ls "/Experiment/Run Parameters/Channel 7"' | awk '{ s = ""; for (i = 2; i <= NF; i++) s = s $i " "; print s }')
  

  RUNLENGTH=$(($STOP_TIME - $START_TIME)) 
  
  echo "$Run_number, $START_TIME, $STOP_TIME, $RUNLENGTH, $COMMENT, $CHN0, $CHN1, $CHN2, $CHN3, $CHN4, $CHN5, $CHN6, $CHN7"  >> $RUNLIST
  
  fi
done

#odbedit -e Default -c 'load current.odb'
