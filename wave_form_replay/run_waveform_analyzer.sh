#! /bin/bash

fname=file_list.txt

exec<$fname
value = 0
while read line
do
   value=`expr $value + 1`;
   echo "$value  $line";
   ./na $line
done
