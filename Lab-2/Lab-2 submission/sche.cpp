#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string.h>

using namespace std;

int inputIndex;
int quantum;
int ofs = 0;
int totalCPU = 0;
int totalTT = 0;
int totalCW = 0;
int totalIO = 0;
int lastEventFT;
double utilizationCPU;
double utilizationIO;
bool verbose = false;
string scheduleType;
ifstream inputFile;
ifstream randFile;
vector<int> randoms;


class Process{
	vector<int> AT_TC_CB_IO;

public:
	int pid;
	int static_priority;
	int dynamic_priority;
	int FT;
	int TT;
	int IT;
	int CW;
	int cpuTime;
	int readyAT;//arriving at ready state time
	int CBRemaintime;//cpu burst remaining time

	void initialize(int id){
		for (int i = 0;i < 4;i++){
			AT_TC_CB_IO.push_back(-1);
		}
		pid = id;
		dynamic_priority = static_priority-1;
		FT = 0;
		TT = 0;
		IT = 0;
		CW = 0;
		cpuTime = 0;
		CBRemaintime = -1;
	}

	void setAT(int AT){
		AT_TC_CB_IO[0] = AT;
	}

	void setTC(int TC){
		AT_TC_CB_IO[1] = TC; 
	}

	void setCB(int CB){
		AT_TC_CB_IO[2] = CB;
	}

	void setIO(int IO){
		AT_TC_CB_IO[3] = IO;
	}

	int getAT(){
		return AT_TC_CB_IO[0];
	}

	int getTC(){
		return AT_TC_CB_IO[1];
	}

	int getCB(){
		return AT_TC_CB_IO[2];
	}

	int getIO(){
		return AT_TC_CB_IO[3];
	}

	int getCPUremainingtime(){
		return (getTC() - cpuTime);
	}
};
vector<Process> processes;


class Event{
public:
	int timestamp;
	int pid;
	int transition;
	int createTime;
	string oldState;
	string newState;

	void initialize (int t, int p, int ts){
		timestamp = t;
		pid = p;
		transition = ts;
	}
};


class DES_Layer {
	vector<Event> eventQueue;

public:
	Event get_event(){
		if (eventQueue.empty()){
			Event e;
			e.initialize(-1,-1,-1);
			return e;
		}else{
			Event e = eventQueue[0];
			eventQueue.erase(eventQueue.begin());
			return e;
		}
	}

	void put_event(Event e){
		int i = 0;
		while(i<eventQueue.size() && e.timestamp >= eventQueue[i].timestamp){
			i++;
		}
		eventQueue.insert(eventQueue.begin()+i,e);
	}

	int get_next_event_time(){
		if(eventQueue.empty())
			return -1;
		return eventQueue.front().timestamp;
	}
};


class Scheduler{	
public:
	virtual void add_process(Process *p,bool quantum) = 0;
	virtual Process *get_next_process() = 0;
};
Scheduler *scheduler;


class FCFS : public Scheduler{
	vector<Process *> runQueue;

public:
	void add_process(Process *p,bool quantum){
		runQueue.push_back(p);
	}

	Process *get_next_process(){
		Process *p;
		if(runQueue.empty()){
			return NULL;
		}else{
			p=runQueue.front();
			//runQueue.pop_front();
			runQueue.erase(runQueue.begin());
		}
		return p;
	}
};


class LCFS : public Scheduler{
	vector<Process *> runQueue;

public:
	void add_process(Process *p,bool quantum){
		runQueue.push_back(p);
	}

	Process *get_next_process(){
		Process *p;
		if (runQueue.empty()){
			return NULL;
		}else{
			p=runQueue.back();
			runQueue.pop_back();
		}
		return p;
	}
};


class SJF :public Scheduler{
	vector<Process *> runQueue;

public:
	void add_process(Process *p,bool quantum){
		runQueue.push_back(p);
	}

	Process *get_next_process(){
		Process *p;
		if (runQueue.empty()){
			return NULL;
		}else{
			int shortestremainTime = runQueue[0]->getCPUremainingtime();
			int shortestIndex = 0;
			for(int i=1;i<runQueue.size();i++){
				if (runQueue[i]->getCPUremainingtime() < shortestremainTime){
					shortestremainTime=runQueue[i]->getCPUremainingtime();
					shortestIndex = i;
				}
			}
			p = runQueue[shortestIndex];
			runQueue.erase(runQueue.begin()+shortestIndex);
		}
		return p;
	}
};


class RR : public Scheduler{
	vector<Process *> runQueue;

public:
	void add_process(Process *p,bool quantum){
		runQueue.push_back(p);
	}

	Process *get_next_process(){
		Process *p;
		if(runQueue.empty()){
			return NULL;
		}else{
			p = runQueue.front();
			//runQueue.pop_front();
			runQueue.erase(runQueue.begin());
		}
		return p;
	}
};


class PRIO: public Scheduler{
	vector<Process *> activerunQueue0;
	vector<Process *> activerunQueue1;
	vector<Process *> activerunQueue2;
	vector<Process *> activerunQueue3;
	vector<Process *> expiredrunQueue0;
	vector<Process *> expiredrunQueue1;
	vector<Process *> expiredrunQueue2;
	vector<Process *> expiredrunQueue3;

public:
	void add_process(Process *p,bool quantum){
		int prio = p->dynamic_priority;
		if (quantum){
			if (prio == 0){
				prio = p->static_priority-1;
				p->dynamic_priority = prio;
				if (prio == 3){
					expiredrunQueue3.push_back(p);
					return;
				}else if (prio == 2){
					expiredrunQueue2.push_back(p);
					return;
				}else if (prio == 1){
					expiredrunQueue1.push_back(p);
					return;
				}else if (prio == 0){
					expiredrunQueue0.push_back(p);
					return;
				}
			}else{
				p->dynamic_priority = prio-1;
				prio--;
			}
		}
		if (prio == 0) {
			activerunQueue0.push_back(p);
		}else if (prio == 1) {
			activerunQueue1.push_back(p);
		}else if (prio == 2) {
			activerunQueue2.push_back(p);
		}else if (prio == 3) {
			activerunQueue3.push_back(p);
		}
	}

	Process *get_next_process(){
		if (activerunQueue0.empty() && activerunQueue1.empty() && activerunQueue2.empty() && activerunQueue3.empty() && 
			expiredrunQueue0.empty() && expiredrunQueue1.empty() && expiredrunQueue2.empty() && expiredrunQueue3.empty())
			return NULL;
		if (activerunQueue0.empty() && activerunQueue1.empty() && activerunQueue2.empty() && activerunQueue3.empty()) {
			activerunQueue0.swap(expiredrunQueue0);
			activerunQueue1.swap(expiredrunQueue1);
			activerunQueue2.swap(expiredrunQueue2);
			activerunQueue3.swap(expiredrunQueue3);
		}
		Process *p;// 3 is the highest priority queue
		if (!activerunQueue3.empty()) {
			p = activerunQueue3.front();
			activerunQueue3.erase(activerunQueue3.begin());// pop_front
		}else if(!activerunQueue2.empty()) {
			p = activerunQueue2.front();
			activerunQueue2.erase(activerunQueue2.begin());//pop_front
		}else if(!activerunQueue1.empty()) {
			p = activerunQueue1.front();
			activerunQueue1.erase(activerunQueue1.begin());//pop_front
		}else if (!activerunQueue0.empty()) {
			p = activerunQueue0.front();
			activerunQueue0.erase(activerunQueue0.begin());//pop_front
		}
		return p;
	}
};


int myrandom(int burst) {
	if (ofs == randoms.size()){
		ofs = 0;//wrap around
	}else{
		ofs++;
	}
	return 1 + (randoms[ofs] % burst);
}

void simulation(DES_Layer deslayer){
	int timestamp;
	int beeninoldState;
	int burst;
	int lastIO;
	bool call_scheduler = false;
	Event currentEvent = deslayer.get_event();
	Process *current_running_process = NULL;
	Process *process_blocked = NULL;
	/*
	READY = 1;
	RUNNG = 2;
	BLOCK = 3;
	PREEMPT = 4;
	DONE = 5;*/
	
	while (currentEvent.timestamp !=-1){
		timestamp = currentEvent.timestamp;//set timestamp
		int i = currentEvent.pid;//get the event pid 

		switch(currentEvent.transition){
			//-----------------------------TANSFER TO READY-------------------
			case 1:{
				currentEvent.newState = "READY";
				beeninoldState = timestamp-currentEvent.createTime;
				if (process_blocked == &processes[i]) {
					process_blocked = NULL;//if it was waiting on the list, remove
				}
				processes[i].readyAT = timestamp;
				scheduler->add_process(&processes[i],false);
				call_scheduler = true;
				if (verbose) {
					cout << timestamp<<" "<< i <<" "<< beeninoldState<< ": " << currentEvent.oldState<<" -> "<< currentEvent.newState<<endl;
				}
			}
			break;
			//-----------------------------TANSFER TO RUN--------------------
			case 2:{
				currentEvent.newState = "RUNNG";
				beeninoldState = timestamp-processes[i].readyAT;
				current_running_process = &processes[i];//add it to the current_running_process queue
				if (processes[i].CBRemaintime > 0) {//previous CPU burst not exhausted, no new one generated 
					burst = processes[i].CBRemaintime;
				}else {
					burst= myrandom(processes[i].getCB());
				}
				if (processes[i].getCPUremainingtime() < burst){//reduce burst time
					burst = processes[i].getCPUremainingtime();
				}
				processes[i].CW += beeninoldState;
				if (verbose) {
					cout<< timestamp<<" "<<i<<" "<< beeninoldState <<": "<< currentEvent.oldState <<" -> "<<currentEvent.newState;
					cout <<" cb="<<burst<<" rem="<<processes[i].getCPUremainingtime()<< " prio="<< processes[i].dynamic_priority <<endl;
				}
				if (burst > quantum){
					processes[i].cpuTime += quantum;
					processes[i].CBRemaintime = burst - quantum;
					Event e;
					e.createTime = timestamp;
					e.initialize(timestamp+quantum,i,4);//PREEMPT,4
					e.oldState="RUNNG";
					deslayer.put_event(e);
				}else if ((processes[i].cpuTime + burst) < processes[i].getTC()){
					processes[i].cpuTime += burst;
					processes[i].CBRemaintime=-1;
					Event e;
					e.initialize(timestamp+burst,i,3);//BLOCK,3
					e.createTime=timestamp;
					e.oldState = "RUNNG";
					deslayer.put_event(e);
				}else if((processes[i].cpuTime + burst) >= processes[i].getTC()){//finished 
					processes[i].cpuTime+= burst;
					processes[i].CBRemaintime=-1;
					Event e;
					e.initialize(timestamp+burst,i,5);//DONE,5
					e.createTime = timestamp;
					e.oldState="RUNNG";
					deslayer.put_event(e);
				}
			}
			break;
			//-----------------------------TANSFER TO BLOCK-------------------
			case 3:{
				currentEvent.newState = "BLOCK";
				beeninoldState = timestamp-currentEvent.createTime;
				current_running_process = NULL;//remove currect process from running queue
				int ioburst = myrandom(processes[i].getIO());
				processes[i].IT += ioburst;
				Event et;
				et.initialize(timestamp+ioburst,i,1);//READY 1
				et.createTime = timestamp;
				et.oldState = "BLOCK";
				deslayer.put_event(et);
				processes[i].dynamic_priority = processes[i].static_priority-1;//when a processes returns from I/O, reset dynamic_priority
				if (process_blocked != NULL) {//previously been in block state 
					if ((timestamp + ioburst)-lastIO > 0){//add larger one to totalIO
						process_blocked = &processes[i];
						totalIO += (ioburst +timestamp)-lastIO;
						lastIO = ioburst + timestamp;
					}
				}else {//first time to block state
					process_blocked = &processes[i];
					totalIO += ioburst;
					lastIO = ioburst + timestamp;	
				}
				call_scheduler = true;
				if (verbose) {
					cout <<timestamp<<" "<<i<< " "<< beeninoldState << ": " << currentEvent.oldState <<" -> "<< currentEvent.newState;
					cout << "  ib="<<ioburst<<" rem="<< processes[i].getCPUremainingtime()<< endl;
				}
			}
			break;
			//-----------------------------TANSFER TO PREEMPT----------------------
			case 4:{
				currentEvent.newState = "READY";
				beeninoldState = timestamp-currentEvent.createTime;
				current_running_process = NULL;
				if (verbose) {
					cout <<timestamp<<" "<<i<< " "<< beeninoldState << ": " << currentEvent.oldState <<" -> "<< currentEvent.newState;
					cout << "  cb="<<processes[i].CBRemaintime<<" rem="<<processes[i].getCPUremainingtime()<< " prio="<<processes[i].dynamic_priority<<endl;
				}
				scheduler->add_process(&processes[i],true);
				processes[i].readyAT = timestamp;
				call_scheduler = true;
			}
			break;
			//-----------------------------TANSFER TO DONE------------------------
			case 5:{
				currentEvent.newState = "Done";
				beeninoldState = timestamp-currentEvent.createTime;
				processes[i].FT = timestamp;
				processes[i].TT = processes[i].FT - processes[i].getAT();
				current_running_process = NULL;
				lastEventFT = timestamp;
				call_scheduler = true;
				if (verbose) {
					cout << timestamp<<" "<<i<<" "<< beeninoldState << ": " << currentEvent.newState<<endl;
				}
			}
			break;
		}

		if (call_scheduler){
			if (deslayer.get_next_event_time() == timestamp){
				currentEvent = deslayer.get_event();
				continue;
			}
			call_scheduler = false;
			if (current_running_process == NULL){
				current_running_process = scheduler->get_next_process();
				if (current_running_process == NULL){
					currentEvent = deslayer.get_event();
					continue;
				}
				Event e;
				e.initialize(timestamp,current_running_process->pid,2);//RUNNG,2
				e.createTime = timestamp;
				e.oldState = "READY";
				deslayer.put_event(e);
				current_running_process = NULL;
			}
		}//end if

	currentEvent = deslayer.get_event();
	}//end while loop
}//end of simulate	


int main(int argc, char*argv[]){
	//--------------------reading from -v and -s -----------------------
	inputIndex =1;
	if (argv[inputIndex][1] == 'v'){// to implement the verbose info
		verbose = true;
		inputIndex = 2;
	}
	//fetch the Schueduler info
	if(argv[inputIndex][2] == 'F'){//FCFS Scheduler
		scheduleType = "FCFS";
		scheduler = new FCFS();
		quantum = 100000;
	}else if (argv[inputIndex][2] == 'L'){//LCFS Scheduler
		scheduleType ="LCFS";
		scheduler = new LCFS();
		quantum = 100000;
	}else if (argv[inputIndex][2] == 'S'){//Shortest Job Scheduler
		scheduleType = "SJF";
		scheduler = new SJF();
		quantum = 100000;
	}else if (argv[inputIndex][2] == 'R'){//Round Robin Scheduler
		scheduleType = "RR";
		scheduler = new RR();
		int len = strlen(argv[inputIndex]);
		string quantumInput;
		for (int k =3;k<len;k++){
			quantumInput+=argv[inputIndex][k];
		}
		quantum = atoi(quantumInput.c_str());
		scheduleType += " " + quantumInput;
	}else if (argv[inputIndex][2] == 'P'){//Priority Scheduler
		scheduleType = "PRIO";
		scheduler = new PRIO();
		int len = strlen(argv[inputIndex]);
		string quantumInput;
		for (int k =3;k<len;k++){
			quantumInput+=argv[inputIndex][k];
		}
		quantum = atoi(quantumInput.c_str());
		scheduleType += " " + quantumInput;
	}

	//-----------------------read randFile-------------------------
	inputIndex+=2;
	randFile.open(argv[inputIndex]);
	string rs;
	while(randFile>>rs){
		randoms.push_back(atoi(rs.c_str()));
	}
	randFile.close();

	//-----------------------read inputFile-------------------------
	inputIndex--;
	inputFile.open(argv[inputIndex]);
	string s;
	int i = 0;
	while (inputFile>>s){
		Process p;
		p.static_priority = myrandom(4);
		p.initialize(i);
		p.setAT(atoi(s.c_str()));
		inputFile>>s;
		p.setTC(atoi(s.c_str()));
		inputFile>>s;
		p.setCB(atoi(s.c_str()));
		inputFile>>s;
		p.setIO(atoi(s.c_str()));
		processes.push_back(p);
		i++;
	}
	inputFile.close();

	DES_Layer deslayer;//initialize deslayer
	for (int i = 0;i < processes.size();i++){
		Event e;
		e.initialize(processes[i].getAT(),i, 1);//READY
		e.oldState = "CREATED";
		e.createTime = processes[i].getAT();
		deslayer.put_event(e);
	}
	
	simulation(deslayer);//simulate deslayer
	//----------------------print --------------------------
	cout<<scheduleType<<endl;
	for(int i=0;i<processes.size();i++){
		totalCPU += processes[i].getTC();
		totalTT += processes[i].TT;
		totalCW += processes[i].CW;
		printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
			i,processes[i].getAT(),processes[i].getTC(),processes[i].getCB(),processes[i].getIO(),
			processes[i].static_priority,processes[i].FT, processes[i].TT,processes[i].IT,processes[i].CW);
	}

	utilizationCPU = totalCPU/(double)lastEventFT;
	utilizationIO = totalIO/(double)lastEventFT;

	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
		lastEventFT, (100.0*utilizationCPU), (100.0*utilizationIO), 
		(totalTT/(double)processes.size()), (totalCW/(double)processes.size()), 
		(processes.size()*100/(double)lastEventFT)); 	


	return 0;	
}
//end of main

	


