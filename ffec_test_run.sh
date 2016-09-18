#!/bin/bash  

ITERS=20
if [[ "$1" = "-i" ]]; then
	ITERS=$2
	echo "running with $ITERS test iterations"
	shift
	shift
fi

SINGLE_THREADED=
if [[ "$1" = "-s" ]]; then
	echo "running single-threaded (stats mode)"
	SINGLE_THREADED=1
	shift
fi
	
echo "Results will be in total_avgs.csv" 
#empty results folder from any previous runs
DIR="ffec_results"
rm -rf $DIR && mkdir $DIR
#remove the total_avgs.csv and all.csv from previous runs
rm -f total_avgs.csv
rm -f all.csv


#fec_ratio 1.02 -> 1.2 in 2% increments
#original_sz 500k, 1M, 2M, 5M, 10M, 40M, 80M, 100M, 150M, 200M, 500M

#bash doesn't do loops with non-integers, generate list with awk
#iterate through with bash
ratios=$(awk 'BEGIN{for(i=1.02;i<1.22;i+=0.02)print i}')
sizes=(500000 1000000 2000000 5000000 10000000 40000000 80000000 100000000 200000000 500000000)

#	$1 == RATIO
#	$2 == SIZE
run_tests()
{
	echo "size is ${2}"
	if [[ -n $SINGLE_THREADED ]]; then 
		for k in $(seq 1 $ITERS); do
			name=$(echo "${1}_${2}")
			./ffec_test.exe -f $1 -o $2 >> $DIR/result_${name}.txt 
		done
	else
		for k in {1..$ITERS}; do
			name=$(echo "${1}_${2}")
			./ffec_test.exe -f $1 -o $2 >> $DIR/result_${name}.txt_$2 & 
		done
		wait
		cat $DIR/result_${name}.txt_* > $DIR/result_${name}.txt 
		rm $DIR/result_${name}.txt_*  
	fi
}

# run the actual tests
for i in $ratios
do
	echo "fec ratio: $i"
	for j in ${sizes[@]}; do
		run_tests $i $j
	done
done

TEMPS=
for i in $DIR/*; do
	INEFFICIENCY=( $(sed -rn 's/.*inefficiency=([0-9.]+).*/\1 /p' "$i") )
	LOSSTOLLERANCE=( $(sed -rn 's/.*loss tolerance=([0-9.]+)%.*/\1 /p' "$i") )

	#sum the array and divide by count	
	sum=$(echo "${INEFFICIENCY[*]}" | sed 's/ /+/g' | bc)
	isumavg=$(echo "$sum / ${#INEFFICIENCY[@]}" | bc -l)

	lsum=$(echo "${LOSSTOLLERANCE[*]}" | sed 's/ /+/g' | bc)
	lsumavg=$(echo "$lsum / ${#LOSSTOLLERANCE[@]}" | bc -l)

	params=( $(echo "$i" | sed -rn 's/.*result_([0-9.]+)_([0-9]+).txt/\1 \2/p') )

	printf "%.2lf,%ld,%.4lf,%.2lf\n" \
		${params[0]} ${params[1]} $isumavg $lsumavg >> total_avgs.csv

	# remove the txt file once done
	rm "$i"
done
