#!/bin/bash  

echo "Results will be in ffec/all.csv and ffec/total_avgs.csv" 

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


#remove the total_avgs.csv and all.csv from previous runs
rm -f total_avgs.csv
rm -f all.csv

#parse the results in ffec_results/result_*.txt to make averages

FILES=ffec_results/*
for i in $FILES
do
	echo "Processing $i file"
	cat $i | grep "inefficiency=" | cut -d ';' -f 1-2 | grep -Eo '[0-9]\.[0-9]{1,6}' > ffec_results/temp_avgs.txt
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

	echo "$i, $isumavg, $lsumavg" >> total_avgs.csv

	#make a new file with only loss tolerance and ineffficiency in it
	
	if [ "${#INEFFICIENCY[@]}" == "${#LOSSTOLLERANCE[@]}" ]; then 
		echo "Sizes are equal"
	fi

	ext=".csv"
	IFS='.' read -a arr <<< $i
	filename=${arr[0]}${arr[1]}${ext}
	echo "\# $filename" > $filename
	for ((i=0; i<=${#INEFFICIENCY[@]}; i++)); do printf '%s,%s\n' "${INEFFICIENCY[i]}" "${LOSSTOLLERANCE[i]}"; done >> $filename
done

cat ffec_results/result_*.txt > ffec_results/total_result.txt
cat ffec_results/result_*.csv > all.csv

