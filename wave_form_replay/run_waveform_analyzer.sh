#! /bin/bash

fname=file_list_10_2013.txt

exec<$fname
value = 0
while read line
do
   value=`expr $value + 1`;
   echo "$value  $line";
   ./na $line
done
