#!/bin/bash

DIR1=$1
DIR2=$2
DARGS=$3

INS="0 1 2 3 4 5 6"
SCHPAR="F L S R2 R5 P2 P5"

for f in ${INS}; do
	for s in ${SCHPAR}; do
		X=`diff ${DARGS} ${DIR1}/output${f}_${s} ${DIR2}/output${f}_${s}`
                [[ "${X}"  != "" ]] && echo "************  outputs${f}_${s}  differ" && echo "${X}"
	done
done

