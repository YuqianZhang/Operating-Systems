#!/bin/bash


# Author: Hubertus Franke  (frankeh@nyu.edu)
OUTDIR=${1:-.}
shift
LINKER=${*:-../linker}

echo "linker=<$LINKER> outdir=<$OUTDIR>"

INS="`seq 1 19`" 
INPRE="input-"
OUTPRE="out-"

SPID=0   # process pid we are monitoring

##########################  START TIMER ####################

TIMELIMIT=20
TPID=0
MPID=$$
EXIT_NOW=0
SPID_KILLED=0	# notification that the process got killed due to timeout

TimerOn()
{
	SPID_KILLED=0
	# echo "sleep ${TIMELIMIT} ; kill -SIGUSR1 $MPID"
	(sleep ${TIMELIMIT} ; kill -SIGUSR1 $MPID) &
	TPID=$!	
	# echo "timer ${TPID}"
}

TimerOff()
{
	if [[ ${TPID} != 0 ]]; then
		# echo "kill timer ${TPID}"
		kill -9 ${TPID} 2>&1 > /dev/null
		wait ${TPID}
		TPID=0
	fi
}

TimerFires()
{
	# echo "fires"
	ps -p ${SPID} | grep -v "^[ ]*PID" 2>&1 > /dev/null
	if [ $? == 0 ]; then
		start_redirect
		# echo "kill appl ${SPID}"
		kill -9 ${SPID} > /dev/null 2>&1 
		stop_redirect
		SPID_KILLED=1
	fi
	TPID=0
}

GetMeOutOfHere()
{
	echo "INTERRUPTED -> TERMINATING"
	EXIT_NOW=1
	TimerFires
}

start_redirect() 
{ 
	exec 3>&2 ; exec 2> /dev/null 
}
stop_redirect()  
{ 
	exec 2>&3 ; exec 3>&- 
}
check_exit()    
{ 
	[[ ${EXIT_NOW} == 1 ]] && exit 
}

trap TimerFires     SIGUSR1
trap GetMeOutOfHere SIGINT

##########################  END TIMER ####################


############################################################################
#  NO TRACING 
############################################################################

# run with RFILE1 
#ulimit -v 300000   # just limit the processes 

for f in ${INS}; do
	echo "${LINKER} ${INPRE}${f}"
	${LINKER} ${INPRE}${f} > ${OUTDIR}/${OUTPRE}${f} 2>&1 &
	#${LINKER} ${INPRE}${f} > ${OUTDIR}/${OUTPRE}${f} &
	SPID=$!

	start_redirect; TimerOn

	wait ${SPID} 2>&1 > /dev/null
	SPID=0

	TimerOff; stop_redirect
	[[ ${SPID_KILLED} == 1 ]] && echo "      Killed after ${TIMELIMIT}"
	check_exit
done

