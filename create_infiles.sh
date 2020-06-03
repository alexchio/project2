#!/bin/bash

##check for correct arguments

if [ $# -ne 5 ]; then
	echo "Wrong number of arguments"
	exit 1;
fi

if [ $4 -le 0 ]; then
	echo "Number of files can't be negative number"
	exit 1;
fi

if [ $5 -le 0 ]; then
	echo "Number of records can't be negative number"
	exit 1;
fi


mkdir $3	##create input dir

##variables
day=0
month=0
year=0
id=0
age=0
io=0
virus=0
first=(James John Robert Michael Richard Mary Patricia Jennifer Linda Elizabeth)		##first name array
last=(Smith Johnson Williams Jones Brown Davis Miller Wilson Moore Taylor)			##last name array
enex=(ENTER EXIT)										##enter or exit array

input=$2			##countries file
while IFS= read -r line
do
	mkdir $3/$line	##create countries dir
	i=1
	while [ $i -le $4 ]
	do
		day=$((1 + RANDOM % 29))
		month=$((1 + RANDOM % 11))
		year=$((1919 + RANDOM % 100))
		i=$(( $i + 1 ))
		touch $3/$line/$day-$month-$year
		j=1
		while [ $j -le $5 ]
		do
			id=$(( $id + 1 ))		##record id in ascending order
			io=$((1 + RANDOM % 20))		##95% possibility for enter
			if [ $io -gt 1 ]; then
				io=0
			else
				io=1
			fi
			age=$((1 + RANDOM % 120))	##age range 5-95
			virus=`shuf -n 1 $1`		##random line from virus file
			printf '%s\n' "$id ${enex[$io]} ${first[$RANDOM % 10]} ${last[$RANDOM % 10]} $virus $age" >> $3/$line/$day-$month-$year	##write record
			j=$(( $j + 1 ))
		done
	done

done < "$input"
