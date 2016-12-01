#!/bin/bash

./bits_test.exe s
poop=$?; if (( $poop )); then exit $poop; fi

files=( test_lines.txt almost_a_meelyun_sentences.txt pg29765.txt )
for f in $files; do
	./fnv1a_test.exe test_files/$f
	poop=$?; if (( $poop )); then exit $poop; fi
done
