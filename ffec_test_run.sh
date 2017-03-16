#!/bin/bash  
# Outputs test data so that it can be processed into graphs.
# By way of documentation, this is how to generate the surface plot for inefficiency:
#``` {.octave}
#+	fec_ratio = dlmread("ffec_test_vec_ratio.csv")
#+	sizes = dlmread("ffec_test_vec_size.csv")
#+	inefficiency = dlmread("ffec_test_inefficiency.csv")
#+	surf(fec_ratio, sizes, inefficiency)
#+	set (gca, 'yscale', 'log')
#+	grid("minor", "on")
#+	title("FEC Inefficiency vs. block size and code rate")
#+	xlabel("code rate")
#+	ylabel("block size (Bytes)")
#+	zlabel("inefficiency")
#```
# NOTE that we set the Y axis (sizes) as logarithmic, because our size datapoints
#+	increase by x2 each time.

ITERS=100
if [[ "$1" = "-i" ]]; then
	ITERS=$2
	echo "running with $ITERS test iterations"
	shift
	shift
fi

RATIO_LIMIT="1.42"
# size is in KB
SIZE_START="50"
SIZE_LIMIT="1000000"

# fec_ratio 1.02 -> 1.2 in 2% increments,
#+	bash doesn't do loops with non-integers, generate list with awk.
ratios=( $(awk 'BEGIN{for(i=1.02;i<'"$RATIO_LIMIT"';i+=0.02)print i}') )
sizes=( $(awk 'BEGIN{for(i=10;i<'"$SIZE_LIMIT"';i *= 2)print i}') )

# write vector CSVs for import into e.g. Octave
echo ${ratios[@]} | tr ' ' ',' >./ffec_test_vec_ratio.csv
echo ${sizes[@]} | tr ' ' ',' >./ffec_test_vec_size.csv

# generate 2d matrices of test results:
#+	columns		: 	ratios
#+	rows		:	sizes
rm -f ./ffec_test_inefficiency.csv
rm -f ./ffec_test_enc_speed.csv
rm -f ./ffec_test_dec_speed.csv
for s in ${sizes[@]}; do
	echo "size $(( $s * 1000 )):"
	INEF_ROW=()
	ENC_ROW=()
	DEC_ROW=()
	for r in ${ratios[@]}; do
		echo " ratio $r:"
		INEF=()
		ENC=()
		DEC=()
		for k in $(seq 1 $ITERS); do
			echo -n "$k, "
			# get program output
			OUT=$(./ffec_test.exe -f $r -o $(( $s * 1000 )))
				poop=$?; if (( $poop )); then exit $poop; fi
			# add to arrays of temporary values
			INEF=( ${INEF[@]} $(echo "$OUT" | sed -rn 's/.*inefficiency=([0-9.]+).*/\1 /p') )
			ENC=( ${ENC[@]} $(echo "$OUT" | sed -rn 's/.*enc=([0-9]+)Mb.*/\1/p') )
			DEC=( ${DEC[@]} $(echo "$OUT" | sed -rn 's/.*dec=([0-9]+)Mb.*/\1/p') )
		done
		echo
		# make sums so that we can compute an average 
		inef_sum=$(echo "${INEF[@]}" | tr ' ' '+' | bc)
		enc_sum=$(echo "${ENC[@]}" | tr ' ' '+' | bc)
		dec_sum=$(echo "${DEC[@]}" | tr ' ' '+' | bc)
		# compute the average, add it to the array
		INEF_ROW=( ${INEF_ROW[@]} $(printf "%.4f" $(echo "$inef_sum / ${#INEF[@]}" | bc -l)) )
		ENC_ROW=( ${ENC_ROW[@]} $(( $enc_sum / ${#ENC[@]} )) )
		DEC_ROW=( ${DEC_ROW[@]} $(( $dec_sum / ${#DEC[@]} )) )
	done
	echo "${INEF_ROW[@]}" | tr ' ' ',' >>./ffec_test_inefficiency.csv
	echo "${ENC_ROW[@]}" | tr ' ' ',' >>./ffec_test_enc_speed.csv
	echo "${DEC_ROW[@]}" | tr ' ' ',' >>./ffec_test_dec_speed.csv
done
