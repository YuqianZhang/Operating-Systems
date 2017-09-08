#!/bin/bash

#example ./gradeit.sh dir1 dir2 logfile

DIR1=$1
DIR2=$2
LOG=${3:-${DIR2}/LOG}

DARGS=         # nothing
DARGS="-q --speed-large-files"         # the big files are killing us --> out of memory / fork refused etc

FILES="in18_4 in60_8 in1K4_16 in10K3_32"
#FILES="in18_4 in60_8"

ALGOS="N  r  f  s  c  X  a  Y"

declare -ai counters
declare -i x=0
for s in ${ALGOS}; do
        let counters[$x]=0
        let x=$x+1
done

echo "gradeit.sh ${DIR1} ${DIR2} ${LOG}" > ${LOG}

echo "                 ${ALGOS}"

for F in ${FILES}; do
    OUTLINE=`printf "%-15s" "${F}"`
    x=0
    for A in ${ALGOS}; do 
	OUTF="out_${F}_${A}_OPFS"
        if [[ ! -e ${DIR1}/${OUTF} ]]; then
            echo "${DIR1}/${OUTF} does not exist" >> ${LOG}
            OUTLINE=`printf "%s  o" "${OUTLINE}"`
            continue;
        fi;
        if [[ ! -e ${DIR2}/${OUTF} ]]; then
            echo "${DIR2}/${OUTF} does not exist" >> ${LOG}
            OUTLINE=`printf "%s  o" "${OUTLINE}"`
            continue;
        fi;
	
#       echo "diff -b ${DARGS} ${DIR1}/${OUTF} ${DIR2}/${OUTF}"
        DIFF=`diff -b ${DARGS} ${DIR1}/${OUTF} ${DIR2}/${OUTF}`
        if [[ "${DIFF}" == "" ]]; then
            OUTLINE=`printf "%s  ." "${OUTLINE}"`
            let counters[$x]=`expr ${counters[$x]} + 1`
        else
            echo "diff -b ${DARGS} ${DIR1}/${OUTF} ${DIR2}/${OUTF} failed" >> ${LOG}
            SUMX=`egrep "^SUM" ${DIR1}/${OUTF}`
            SUMY=`egrep "^SUM" ${DIR2}/${OUTF}`
            echo "   DIR1-SUM ==> ${SUMX}" >> ${LOG}
            echo "   DIR2-SUM ==> ${SUMY}" >> ${LOG}
            OUTLINE=`printf "%s  x" "${OUTLINE}"`
        fi
	let x=$x+1
    done
    echo "${OUTLINE}"
done


OUTLINE=`printf "%-15s" "SUM"`
x=0
for A in ${ALGOS}; do 
    OUTLINE=`printf "%s %2d" "${OUTLINE}" "${counters[$x]}"`
    let x=$x+1
done
echo "${OUTLINE}"

