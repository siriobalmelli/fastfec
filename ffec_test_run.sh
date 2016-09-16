#!/bin/bash  

#fec_ratio 1.02 -> 1.2 in 2% increments
#original_sz 500k, 1M, 2M, 5M, 10M, 40M, 80M, 100M, 150M, 200M, 500M

#empty results folder from any previous runs
rm -rf ffec_results && mkdir ffec_results

#bash doesn't do loops with non-integers, generate list with awk
#iterate through with bash

ratios=$(awk 'BEGIN{for(i=1.02;i<1.22;i+=0.02)print i}')
sizes=(500000 1000000 2000000 5000000 10000000 40000000 80000000 100000000 200000000 500000000)

for i in $ratios
do
	echo "fec ratio: $i"
	for j in ${sizes[@]}
		do
			echo "size is ${j}"
			for k in {1..10}; do
				name=$(echo "${i}_${j}")
				./ffec_test.exe -f $i -o ${j} >> ffec_results/result_${name}.txt_${k} & 
			done
			wait
			cat ffec_results/result_${name}.txt_* > ffec_results/result_${name}.txt 
			rm ffec_results/result_${name}.txt_*  
		done
	
done


#remove the total_avgs.csv from previous runs
rm -f total_avgs.csv

#parse the results in ffec_results/result_*.txt to make averages

FILES=ffec_results/*
for i in $FILES
do
	echo "Processing $i file"
	cat $i | grep "inefficiency=" | cut -d ';' -f 2-3 | grep -Eo '[0-9]\.[0-9]{1,6}' > ffec_results/temp_avgs.txt
	#read the file temp_avgs.txt and add calc avgs
	FILENAME=ffec_results/temp_avgs.txt

	INEFFICIENCY=()
	LOSSTOLLERANCE=()
	INDEX=1;
	while read -r line
	do 
		#pick all inefficieny values from the temp_avgs.txt file
		if (( INDEX % 2 )); then
			INEFFICIENCY+=("$line")	
		else
			LOSSTOLLERANCE+=("$line")
		fi

		INDEX=$((INDEX+1))
	done < $FILENAME

	#sum the array and divide by count	
	sum=$(echo "${INEFFICIENCY[*]}" | sed 's/ /+/g' | bc)
	isumavg=$(echo "$sum / ${#INEFFICIENCY[@]}" | bc -l)

	lsum=$(echo "${LOSSTOLLERANCE[*]}" | sed 's/ /+/g' | bc)
	lsumavg=$(echo "$lsum / ${#LOSSTOLLERANCE[@]}" | bc -l)

	echo "$i: $isumavg, $lsumavg" >> total_avgs.csv
done

cat ffec_results/result_*.txt > ffec_results/total_result.txt
