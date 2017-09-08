#include <iostream>
#include <fstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <vector>
using namespace std;

bool V = false;//detailed execution trace
bool D = false;//result line for each IO
int inputfileIndex;
ifstream inputFile;
int numio = 0;
int count = 0;
int currentTrack = 0;
int topBound;
int downBound;
bool up = true;


class request{
public:
	int request_id;
	int timestamp;
	int track;
	int issue_time;
	int finish_time;
	bool added;
	bool finished;
};

vector<request> IOqueue;


class ioScheduler{
public:
	vector<request>firstQueue, secondQueue;
	virtual int getNext(){};
};


class FIFO : public ioScheduler{
public:
	int getNext(){
		int k = 0;//starts at position 0
		vector<request>::iterator i = IOqueue.begin();

	    while ( i != IOqueue.end()) {
	        if (i->added && !i->finished){
	            k = (int)(i - IOqueue.begin());
	            break;  
		    }
	        i++;
	    }

	    return k;
	}
};


class SSTF : public ioScheduler{
	int getNext(){
		int k = -1;
		int seektime;
		int shortest = 1000000;
		vector<request>::iterator i = IOqueue.begin();
		
		while (i->added){
			if (!i->finished){
				seektime = abs(currentTrack - i->track);
				if (seektime < shortest){
					shortest = seektime;
					k = (int)(i - IOqueue.begin());
				}
			}
			i++;
		}

		if (k==-1){ 
			if (i == IOqueue.begin()){
				k = 0;
			}else{
				k = (int)(i - IOqueue.begin());
			}
		}

		return k;
	}
};


class SCAN : public ioScheduler{//direction is set to go up, if bound then down 
	int getNext(){
		topBound = -1;
		downBound = 1000000;
		bool check = false;
		vector<request>::iterator i = IOqueue.begin();

		while (i->added){
			if (!i->finished){
				check = true;
				if (i->track > topBound){
					topBound = i->track;
				}
				if (i->track < downBound){
					downBound = i->track;
				}
			}
			i++;
		}

		if (!check){//in the beginning, none has been added
			i->added = true;
			return getNext();
		}
		
		int j = -1;
		int shortest = 1000000;

		//go upwards
		if (up && currentTrack <= topBound || !up && currentTrack <=downBound){//if there is no candidate in downwards, go up again
			up = true;
			for (vector<request>::iterator i = IOqueue.begin(); i->added; i++) {
    			if (!i->finished && i->track >= currentTrack) {
        			int temp = i->track - currentTrack;
       				if (temp < shortest) {
           				shortest = temp;
            			j = (int)(i - IOqueue.begin());
        			}
    			}
			}
			return j;
		}else{
			//go downwards
			for (vector<request>::iterator i = IOqueue.begin(); i->added; i++) {
    			if (!i->finished && i->track <= currentTrack) {
        			int temp = currentTrack - i->track;
       				if (temp < shortest) {
           				shortest = temp;
            			j = (int)(i - IOqueue.begin());
        			}
    			}
			}
			up = false;// reset the direction for next instructions
			return j;
		}
	}
};


class CSCAN : public ioScheduler{//direction is set to go up, if bound start with lowest index
	int getNext(){
		topBound = -1;
		downBound = 1000000;
		int lowest;
		bool check = false;
		vector<request>::iterator i = IOqueue.begin();

		while (i->added){
			if (!i->finished){
				check = true;
				if (i->track > topBound){
					topBound = i->track;
				}
				if (i->track < downBound){
					downBound = i->track;
					lowest = (int)(i - IOqueue.begin());//make ready for jump back point
				}
			}
			i++;
		}

		if (!check){//in the beginning, none has been added
			i->added = true;
			return getNext();
		}

		int j = -1;
		int shortest = 1000000;

		if (up && currentTrack <= topBound){
			for (vector<request>::iterator i = IOqueue.begin(); i->added; i++) {
    			if (!i->finished && i->track >= currentTrack) {
        			int temp = i->track - currentTrack;
       				if (temp < shortest) {
           				shortest = temp;
            			j = (int)(i - IOqueue.begin());
        			}
    			}
			}
			return j;
		}else{
			return lowest;//if no more candidates in upwards, ump to the lowest index and start over again
		}
	}
};


class FSCAN : public ioScheduler{
public:
	int getNext(){
		topBound = -1;
		downBound = 1000000;
		bool check = false;
		vector<request>::iterator i = firstQueue.begin();

		while(i != firstQueue.end()) { //if the firstqueue is not empty, look for the top and down bound
	        if (!i->finished) {
	            check = true;
	            if (i->track > topBound){
					topBound = i->track;
				}
				if (i->track < downBound){
					downBound = i->track;
				}
	        }
	        i++;
    	}
    	
    	if(!check){ //if the firstqueue is empty, swap the queue
    		firstQueue.clear();//first clear it make it new
    		firstQueue.swap(secondQueue);//swap what is stored in secondQueue

    		return getNext();
    	}

    	int j = -1;
		int shortest = 1000000;

		if (up && currentTrack <= topBound){//go upwards
			for (vector<request>::iterator i = firstQueue.begin(); i != firstQueue.end(); i++) {
    			if (!i->finished && i->track >= currentTrack) {
        			int temp = i->track - currentTrack;
       				if (temp < shortest) {
           				shortest = temp;
            			j = (int)(i - firstQueue.begin());
        			}
    			}
			}
			return j;
		}else{//go downwards
			for (vector<request>::iterator i = firstQueue.begin(); i != firstQueue.end(); i++) {
    			if (!i->finished && i->track <= currentTrack) {
        			int temp = currentTrack - i->track;
       				if (temp < shortest) {
           				shortest = temp;
            			j = (int)(i - firstQueue.begin());
        			}
    			}
			}
			return j;
		}	
	}
};


//set ioScheduler 
ioScheduler *setSche(string al){
	if (al == "i"){
		return new FIFO();
	}else if (al == "j"){
		return new SSTF();
	}else if (al == "s"){
		return new SCAN();
	}else if (al == "c"){
		return new CSCAN();
	}else if (al == "f"){
		return new FSCAN();
	}
	return NULL;
}


int main(int argc, char**argv){

	//--------------------------reading options and files ----------------------------
	char *svalue = NULL;
  	int c;

  	string algo;

 	while ((c = getopt (argc, argv, "vds:")) != -1){
		switch (c){
			case 's':
				svalue = optarg;//-s<algo>
			break;

			case 'v':
				V = true;//-v
			break;

			case 'd':
				D = true;//-d
			break;

			case '?':
				if (isprint (optopt))
				  fprintf (stderr, "Unknown option `-%c'.\n",optopt);
				else
				  fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;

			default:
				abort ();	
		}
	}

	if (svalue == NULL){
  		cout<<"You must provide ioschedule algorithem!!"<<endl;
	}else {
  		algo = svalue;//--->get algo info
  	}
	inputfileIndex = optind;//get the inputFile index

	//---------------------------set ioSche ---------------------------------
	ioScheduler *ioSche;
	ioSche = setSche (algo);

	//--------------------------read all requests from the file before procession----------------------------
	inputFile.open(argv[inputfileIndex]);
	string line;
	while (getline(inputFile,line)){
		istringstream lineStream(line);
		int timestamp;
		int track;
		if (!(lineStream>>timestamp>>track)){//get instructions
			continue;//skip line starts with #
		}else{
			request req;
			req.request_id = numio;
			numio++;//increment the id num
			req.timestamp = timestamp;
			req.track = track;
			req.added = false;
			req.finished = false;
			IOqueue.push_back(req);
		}
	}
	inputFile.close();

	//--------------------------processing with the queue --------------------------
    int time = 0;
   	int beginIndex;
   	int nextIndex;
   	int waitTime;
   	int requestedTrack;
   	int usedTime;
   	int turnaroundTime;

    int total_time = 0;
    int total_movement = 0;
    int total_turnaround = 0;
    int total_waittime = 0;
    int max_waittime = 0;

    if (D){
	    cout<<"TRACE"<<endl;
	}

    if (algo =="f"){
    	// to get the first request from the queue
    	time = IOqueue.begin()->timestamp;
    	IOqueue.begin()->added = true;
    	ioSche->firstQueue.push_back(* IOqueue.begin());

    	if (D){
	       	printf("%d:%6d add %d\n", time, 0, IOqueue.begin()->track);
	    }

	    while(count < numio){
	    	
	    	nextIndex = ioSche->getNext();
	    	request& r = ioSche->firstQueue[nextIndex];

        	if (r.timestamp > time){
        		time = r.timestamp;
        	}
        	r.issue_time = time;
        	total_time = time;

        	waitTime = time - r.timestamp;// currenttime - requesttime
	        if (waitTime > max_waittime){//max_waittime
	        	max_waittime = waitTime;
	        }
	       	total_waittime += waitTime;//total_waittime

        	requestedTrack = r.track;

        	if (D) {
            	printf("%d:%6d issue %d %d\n", time, r.request_id, requestedTrack, currentTrack);
        	}

        	usedTime = abs(currentTrack - requestedTrack);
        	total_movement += usedTime;
	        total_time += usedTime;
	        turnaroundTime = total_time - r.timestamp;
	        total_turnaround += turnaroundTime;
	        
	        time += usedTime;//increment, update currenttime 

	        for (vector<request>::iterator i = IOqueue.begin(); i != IOqueue.end(); i++) {
	            if (!i->added && i->timestamp <= time) {
	            	i->added = true;
	            	ioSche->secondQueue.push_back(* i);
	                if(D) { 
	                	printf("%d:%6d add %d\n", i->timestamp, i->request_id, i->track); 
	                }      
	            }

	        }

	        r.finished = true;
	        r.finish_time = time;
	        
	        if(D) {
	        	printf("%d:%6d finish %d\n", time, r.request_id, turnaroundTime);
	        }

	        currentTrack = requestedTrack;
	        count++;
	        
	        bool notempty = false;
	        vector<request>::iterator i = ioSche->firstQueue.begin();
	        while (i != ioSche->firstQueue.end()){
	        	if (!i->finished){
	        		notempty = true;
	        	}
	        	i++;
	        }
	        
	        if (!notempty && ioSche->secondQueue.size()==0){
    			IOqueue[count].added = true;
    			ioSche->secondQueue.push_back(IOqueue[count]);
    			if (D){
	       			printf("%d:%6d add %d\n", IOqueue[count].timestamp, IOqueue[count].request_id, IOqueue[count].track); 
	    		}
    		}
	    }

    }else{// for other algos
		// to get the first request from the queue
		beginIndex = ioSche->getNext();
	    request& r = IOqueue[beginIndex];
	   	r.added = true;

	   	if (D){
	   		printf("%d:%6d add %d\n", r.timestamp, beginIndex, r.track);
	   	}
	   	
		while (count < numio) {
		    nextIndex = ioSche->getNext();
	    	request& r = IOqueue[nextIndex];
	    	
	        if (r.timestamp > time){
	        	time = r.timestamp;	
	        }
	        r.issue_time = time;
	        total_time = time;
	       
	        waitTime = time - r.timestamp;// currenttime - requesttime
	        if (waitTime > max_waittime){//max_waittime
	        	max_waittime = waitTime;
	        }
	       	total_waittime += waitTime;//total_waittime

	        requestedTrack = r.track;

	       	if (D) {
	           	printf("%d:%6d issue %d %d\n", time, nextIndex, requestedTrack, currentTrack);
	        }

	        usedTime = abs(currentTrack - requestedTrack);
	        total_movement += usedTime;
	        total_time += usedTime;
	        turnaroundTime = total_time - r.timestamp;
	        total_turnaround += turnaroundTime;
	     
	        time += usedTime;//increment, update currenttime 

	        for (vector<request>::iterator i = IOqueue.begin(); i != IOqueue.end(); i++) {
	        	if (algo=="i") {
	        		if (!i->added) {//don't need to check the timestamp, just add 
		            	i->added = true;
		                if(D) { 
		                	printf("%d:%6d add %d\n", i->timestamp, i->request_id, i->track); 
		                }      
		            }
	        	}else{
		            if (!i->added && i->timestamp <= time) {
		            	i->added = true;
		                if(D) { 
		                	printf("%d:%6d add %d\n", i->timestamp, i->request_id, i->track); 
		                }      
		            }
		        }
	        }

	        r.finished = true;
	        r.finish_time = time;

	        if(D) {
	        	printf("%d:%6d finish %d\n", time, nextIndex, turnaroundTime);	
	        }

	        currentTrack = requestedTrack;//update the currentTrack
	        count++;
		}
	}

	if (V) {
		cout<<"IOREQS INFO"<<endl;
		for (vector<request>::iterator i = IOqueue.begin(); i != IOqueue.end(); i++) {
			printf("%5d:%6d%6d%6d\n", 
				(int)(i-IOqueue.begin()), i->timestamp, i->issue_time, i->finish_time); 
		}

	}

	//--------------------------display resuls --------------------------
	double avg_turnaround = (double)total_turnaround/numio;
	double avg_waittime = (double)total_waittime/numio;

	printf("SUM: %d %d %.2lf %.2lf %d\n", 
			total_time, total_movement, avg_turnaround, avg_waittime, max_waittime);

	//cout<<"reaching the end"<<endl;
	return 0;
}//end of main