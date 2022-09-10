#! /bin/bash

FILE1=test1.xml
FILE2=test2.xml
FILE3=test3.xml

# change this number to adjust the request sent

./client $FILE1 &
./client $FILE2 1000 
