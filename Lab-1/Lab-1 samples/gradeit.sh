#!/bin/bash

DIR1=$1
DIR2=$2
LOG=${3:-${DIR2}/LOG}

INS="`seq 1 19`"

OUTPRE="out-"
INPRE="input-"

DIFFCMD="diff -b -B -E"

############################################################################
#  NO TRACING 
############################################################################

declare -ai counters
declare -i x=0
declare -i correct=0

declare -i COUNT=0
rm -f ${LOG}
rm -fR ./tmp 
mkdir ./tmp

for f in ${INS}; do
	OUTLINE=`printf "%02d" ${f}`
	COUNT=0
	OUTF="${OUTPRE}${f}"
	if [[ ! -e ${DIR1}/${OUTF} ]]; then
		echo "${DIR1}/${OUTF} does not exist" >> ${LOG}
		continue;
	fi;
	if [[ ! -e ${DIR2}/${OUTF} ]]; then
		echo "${DIR2}/${OUTF} does not exist" >> ${LOG}
		continue;
	fi;
	
#       	echo "${DIFFCMD} ${DARGS} ${DIR1}/${OUTF} ${DIR2}/${OUTF}"
	########################################################
	#
	#  first extract WARNINGS and NOT WARNING STUFF

	grep "Warning" ${DIR1}/${OUTF} | sort > ./tmp/ref_warn
	grep "Warning" ${DIR2}/${OUTF} | sort > ./tmp/std_warn

	grep -v "Warning" ${DIR1}/${OUTF} | grep -v "^$" > ./tmp/ref_nowarn
	grep -v "Warning" ${DIR2}/${OUTF} | grep -v "^$" > ./tmp/std_nowarn
		
	#######################################################
	#
	# parse errors there should only be one and its the first one
	# to be printed. That will be caught by the -v diff
	
	# if [[ `wc -l  | cut -d' ' -f 1` == 1 ]]; then echo		
        # DIFF=`${DIFFCMD} ${DARGS} ${DIR1}/${OUTF} ${DIR2}/${OUTF}`

	##################################################################	
        DIFF_W=`${DIFFCMD} ${DARGS} ./tmp/ref_warn   ./tmp/std_warn`
        DIFF_N=`${DIFFCMD} ${DARGS} ./tmp/ref_nowarn ./tmp/std_nowarn`

        if [[ "${DIFF_W}" == "" && "${DIFF_N}" == "" ]]; then 
		COUNT=1
	else
		echo "################### ${INPRE}${f}##################" >> ${LOG}
		echo "###    Warning Diff:    ....." >> ${LOG}
        	${DIFFCMD} ${DARGS} ./tmp/ref_warn   ./tmp/std_warn >> ${LOG}
		echo "###    Code/Error Diff: ....." >> ${LOG}
        	${DIFFCMD} ${DARGS} ./tmp/ref_nowarn ./tmp/std_nowarn >> ${LOG}
		echo "" >> ${LOG}
	fi

	OUTLINE=`printf "%s %1d" "${OUTLINE}" "${COUNT}"`
#	echo `expr ${counters[$x]} + ${COUNT}`
	let counters[$x]=`expr ${counters[$x]}+${COUNT}`
	let x=$x+1
	let correct=$correct+${COUNT}
done

OUTLINE="RES $correct of $x: "
x=0
for s in ${INS}; do 
	OUTLINE=`printf "%s %d" "${OUTLINE}" "${counters[$x]}"`
	let x=$x+1
done
echo "${OUTLINE}"

rm -fR ./tmp 
