#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <bits/stdc++.h>
using namespace std;


int ofs = 0;
ifstream inputFile;
ifstream randFile;
vector<int> randoms;


//random function
int myrandom(int burst) {//[0,size)
	if (ofs == randoms.size()-1){
		ofs = 0;//wrap around
	}
	ofs++;
	return (randoms[ofs] % burst);
}

//bitCompare function
bool bitCompare(bitset<32> b1, bitset<32> b2) {
	for (int i =31; i >= 0; i--) {
		if (b1[i] < b2[i]) {
			return true;
		}else if (b1[i] > b2[i]){
			return false;
		}
	}
	return false;
}


class PTE{
public:
	unsigned int present:1;
	unsigned int modified:1;
	unsigned int referenced:1;
	unsigned int pageout:1;
	unsigned int frameIndex:28;//altogether 32bit

	PTE(){
		present = 0;
		modified = 0;
		referenced = 0;
		pageout = 0;
		frameIndex = 0;
	}
};


class Frame{
public:
	PTE *pte;
	int fid;

	Frame(){
		pte = NULL;
	}
};


class Pager{
public:
	vector<PTE *> *pageTable;
	vector<Frame *> *frameTable;

	void setTables (vector<PTE *> *pt, vector<Frame *> *ft){
		pageTable = pt;
		frameTable = ft;
	}

	virtual Frame *determine_victim_frame() = 0;
	virtual void update(PTE *pte, Frame *f) {};
};


class Random : public Pager { 
	//no queue needed 
public:
	Frame *determine_victim_frame(){
		int i = myrandom(frameTable->size());
		return frameTable->at(i);
	}
};


class FIFO : public Pager { 
	vector<Frame *> queue;
public:
	Frame * determine_victim_frame() {
		Frame *f = queue.front();
		queue.erase(queue.begin());
		return f;
	}
	
	void update(PTE *pte, Frame *f) {
		if (f == NULL) {//valid in the pageTable
			return;
		}
		queue.push_back(f);//push into queue
	}
};


class secondChance : public Pager { 
	vector<Frame *> queue;
public:
	Frame *determine_victim_frame(){
		Frame *f = queue.front();
		queue.erase(queue.begin());
		while (true){
			PTE *p = f->pte;
			if (p->referenced == 0){
				break;
			}
			p->referenced = 0;
			queue.push_back(f);
			f = queue.front();
			queue.erase(queue.begin());
		}
		return f;
	}

	void update(PTE *pte, Frame *f) {
		if (f == NULL) {//valid in the pageTable
			return;
		}
		queue.push_back(f);//push into queue
	}
};


class Clock_pf : public Pager { 
	vector<Frame *> queue;
	int fnum;
	int pointer = 0;
	int move = 0;
public:
	Clock_pf(int fn){
		fnum = fn;
		for (int i = 0;i<fn;i++){
			queue.push_back(NULL);
		}
	}

	Frame *determine_victim_frame(){
		Frame *f = queue[pointer];
		while (true){
			PTE *p = f->pte;
			if (p->referenced ==0){
				break;
			}
			p->referenced = 0;
			pointer++;
			if (pointer >= queue.size()){
				pointer = 0;
			}
			f = queue[pointer];
		}
		queue[pointer] = NULL;//this is the right spot for replacement
		pointer++; //move the pointer to the next
		if (pointer >=queue.size()){
			pointer = 0;
		}
		return f;
	}
	
	void update(PTE *pte, Frame *f) {
		if (f==NULL){//valid in the pageTable
			return;
		}
		if (move < queue.size()){//push into queue
			queue[move] = f;
			move++;
			return;
		}
		//if move >= queue.size(), that means replacement has been implimented, check the pointer
		if (pointer == 0){//move pointer back to where I take it
			queue[fnum-1] = f;
		}else{
			queue[pointer-1] = f;
		}
	}
};


class Aging_pf : public Pager{
	vector <bitset<32> > queue;
public:
	Aging_pf(int fn){
		for (int i = 0;i<fn;i++){
			bitset<32> k;
			queue.push_back(k);
		}
	}

	Frame * determine_victim_frame() {
		Frame *f;
		bitset<32> max;
		max = max.flip();
		int lowest = -1;

		for (int i =0;i<queue.size();i++){
			PTE *p =frameTable->at(i)->pte;
			//shift counter right 1 bit before adding R
			queue[i]>>=1;
			//R is added leftmost
			queue[i][31] = p->referenced;
			if (bitCompare(queue[i],max)){
				max = queue[i];
				lowest = i;
				f = frameTable->at(lowest);
			}
			p->referenced = 0;
		}
		queue[lowest].reset();
		return f;
	}
};


class NRU : public Pager{
	int counter = 0;
public:
	Frame *determine_victim_frame(){
		
		vector <PTE *> class0;
		vector <PTE *> class1;
		vector <PTE *> class2;
		vector <PTE *> class3;
		//if present bit is valid, establish class vectors
		for (int i =0;i<pageTable->size();i++){
			PTE *p = pageTable->at(i);
			if (pageTable->at(i)->present == 1){
				if (p->referenced ==0 && p->modified ==0){
					class0.push_back(p);
				}else if(p->referenced ==0 && p->modified ==1){
					class1.push_back(p);
				}else if(p->referenced ==1 && p->modified ==0){
					class2.push_back(p);
				}else if(p->referenced ==1 && p->modified ==1){
					class3.push_back(p);
				}
			}
		}
		counter++;
		if (counter == 10){//reset all valid page referenced bit
			counter = 0;
			for (int i=0;i<pageTable->size();i++){
				PTE *p = pageTable->at(i);
				if (p->present == 1){
					p->referenced = 0;
				}
			}
		}
		//implement the random replacement 
		Frame *f;
		int rand;
		if (!class0.empty()){
			rand = myrandom(class0.size());
			f = frameTable->at(class0[rand]->frameIndex);
		}else if (!class1.empty()){
			rand = myrandom(class1.size());
			f = frameTable->at(class1[rand]->frameIndex);
		}else if (!class2.empty()){
			rand = myrandom(class2.size());
			f = frameTable->at(class2[rand]->frameIndex);
		}else if (!class3.empty()){
			rand = myrandom(class3.size());
			f = frameTable->at(class3[rand]->frameIndex);
		}
	return f;
	}
};


class Clock_vp : public Pager{
	int pointer = 0;
public:
	Frame * determine_victim_frame() {
		Frame *f;
		while (true){
			PTE *p = pageTable->at(pointer);
			if (p->present ==1 && p->referenced ==0){
				f = frameTable->at(p->frameIndex);
				break;
			}
			p->referenced = 0;
			pointer++;
			if(pointer>=64){
				pointer = 0;
			}
		}
		pointer++;
		if(pointer>=64){
			pointer = 0;
		}
		return f;
	}
};


class Aging_vp : public Pager{
	vector <bitset<32> > queue;
public:
	Aging_vp(){
		for (int i = 0;i<64;i++){
			bitset<32> k;
			queue.push_back(k);
		}
	}

	Frame * determine_victim_frame() {
		Frame *f;
		bitset<32> max;
		max = max.flip();
		int lowest = -1;

		for (int i =0;i<queue.size();i++){
			PTE *p =pageTable->at(i);
			if (p->present == 0){
				continue;
			}
			//shift counter right 1 bit before adding R
			queue[i]>>=1;
			//R is added leftmost
			queue[i][31] = p->referenced;
			if (bitCompare(queue[i],max)){
				max = queue[i];
				lowest = i;
				f = frameTable->at(p->frameIndex);
			}
			p->referenced = 0;
		}
		queue[lowest].reset();
		return f;
	} 
};


//setPager function
Pager *setPager(string al, int fn){
	if (al == "r"){
		return new Random();
	}else if (al == "f"){
		return new FIFO();
	}else if (al == "s"){
		return new secondChance();
	}else if (al == "c"){
		return new Clock_pf(fn);
	}else if (al == "a"){
		return new Aging_pf(fn);
	}else if (al == "N"){
		return new NRU();
	}else if (al == "X"){
		return new Clock_vp();
	}else if (al == "Y"){
		return new Aging_vp();
	}
	return NULL;
}


//getframe function
Frame *get_frame(Pager *mmu,vector<Frame *>ftable){
	for (int i=0;i<ftable.size();i++){
		if (ftable[i]->pte == NULL){
			return ftable[i];
		}
	}
	return mmu->determine_victim_frame();
}




int main(int argc, char**argv){

	//--------------------------reading options and files ----------------------------
	char *avalue = NULL;
  	char *ovalue = NULL;
  	char *fvalue = NULL;
  	int index;
  	int inputfileIndex;
  	int c;

  	string algo;
	string options;
	int num_frames;

 	while ((c = getopt (argc, argv, "a:o:f:")) != -1){
		switch (c){
			case 'a':
				avalue = optarg;//-a<algo>
			break;

			case 'o':
				ovalue = optarg;//-o<options>
			break;

			case 'f':
				fvalue = optarg;//-f<num_frames>
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

	if (avalue == NULL){
  		algo = "r";
	}else {
  		algo = avalue;//--->algo
  	}

  	if (ovalue ==NULL){
		options = "OPFS";
	}else {
		options = ovalue;//--->options
	}

	if (fvalue == NULL){
		num_frames = 32;
	}else {
		num_frames = atoi(fvalue);//--->num_frames
	}

	//----------------------inputFile---------------
	index = optind;
	inputfileIndex = optind;

	//----------------------randomFile----------------
	index++;
	if (index < argc){//if has randomfile 
		randFile.open(argv[index]);//read the randomfile
		string rs;
	while(randFile>>rs){
		randoms.push_back(atoi(rs.c_str()));
	}
	randFile.close();
	}

  	//------------------set PRINT OPTIONS------------
	bool O = false;
	bool P = false;
	bool F = false;
	bool S = false;

	for (int i=0;i<options.size();i++){
		if (options.at(i)=='O'){
			O = true;
		}
		if (options.at(i)=='P'){
			P = true;
		}
		if (options.at(i)=='F'){
			F = true;
		}
		if (options.at(i)=='S'){
			S = true;
		}
	}
  	
  	//-------------------initialize pageTable-------------
  	vector<PTE *> pageTable(64);
  	for (int i = 0;i<pageTable.size();i++){
    	pageTable[i] = new PTE();
  	}

  	//-------------------initialize frameTable------------
  	vector<Frame *> frameTable(num_frames);
  	for (int i =0;i<frameTable.size();i++){
   		frameTable[i] = new Frame();
    	frameTable[i]->fid = i;
  	}

	//------------------ set MMU ---------------
	Pager *mmu;
	mmu = setPager(algo,num_frames);
	mmu->setTables(&pageTable,&frameTable);
  	
	//-------------------------------------reading file------------------------
	//tricky calculation, add 32-bit numbers incrementally to the 64-bit counters
	unsigned int instr_count = 0;
	unsigned int zeros = 0;
	unsigned int ins = 0;
	unsigned int outs = 0;
	unsigned int maps = 0;
	unsigned int unmaps = 0;

	inputFile.open(argv[inputfileIndex]);
	string line;

	while (getline(inputFile,line)){
		istringstream lineStream(line);
		int operation;
		int pte_idx;
		if (!(lineStream>>operation>>pte_idx)){//get instructions
			continue;//skip line starts with #
		}

		if (pte_idx>=64||pte_idx<0){// verify the virtual page index
			cout<<"virtual page index must fall within [0-63]"<<endl;
			return 0;
		}

		if (O){//print out instructions 
			cout<<"==> inst: "<<operation<<" "<<pte_idx<<endl;
		}

		Frame *frame = NULL;
		int frameno;
		if (pageTable[pte_idx]->present == 0){//not valid in pageTable
				frame = get_frame(mmu,frameTable);
				frameno = frame->fid;
				if (frame->pte == NULL){//--->non replacement on virtual pages, just zero it
					if (O){
						printf("%ld: ZERO      %3d\n", instr_count, frameno);
					}
					zeros++;
				}else{//--->replacement decision on virtual pages,unmap it
					PTE *outPage = frame->pte;
					int outPage_id = find(pageTable.begin(),pageTable.end(),outPage) - pageTable.begin();
					//must first unmap it
					if (O){
						printf("%ld: UNMAP %3d %3d\n", instr_count, outPage_id, frameno);
					}
					unmaps++;	
					//optionally pageout
					if (outPage->modified == 1) {//the page is dirty/modified 
		  				outPage->pageout =1;
		  				if (O) {
		  					printf("%ld: OUT   %3d %3d\n", instr_count, outPage_id, frameno);	
		  				}
		  				outs++;
	  				}
	  				//page-in if pageout flag is set, or zero it
		  			if (pageTable[pte_idx]->pageout == 0) {//zero pages on first access
		  				if (O) {	
		  					printf("%ld: ZERO      %3d\n", instr_count, frameno);
		  				}
		  				zeros++;
		  			}else {//if pageout flag is set, page-in
		  				if (O) {
		  					printf("%ld: IN    %3d %3d\n", instr_count, pte_idx, frameno);	
		  				}
		  				ins++;
		  			}
		  			//reset bits of outPage
		  			outPage->present = 0;
		  			outPage->modified = 0;
		  			outPage->referenced = 0;
		  			outPage->frameIndex = 0;
				}

				//--------------------------Mapping--------------------------------
				frame->pte = pageTable[pte_idx];//points to pageTable
				pageTable[pte_idx]->frameIndex = frame->fid;//points to frameTalbe
				if (O){
					printf("%ld: MAP   %3d %3d\n", instr_count, pte_idx, frame->fid);
				}
				maps++;
				pageTable[pte_idx]->present = 1;
		} 

		//now valid in the pageTable, update according to the operation
		if (operation ==0){//operation ---> read
			pageTable[pte_idx]->referenced =1;
		}
		if (operation ==1){//operation ---> write
			pageTable[pte_idx]->referenced =1;
			pageTable[pte_idx]->modified =1;
		}
		mmu->update(pageTable[pte_idx],frame);
		
		instr_count++;
	}//end of while loop

	inputFile.close();
	
	//----------------------------print pageTable-------------------------
	if (P){
		for (int i = 0;i<pageTable.size();i++){
			if(pageTable.at(i)->present == 0){//not valid
				if(pageTable.at(i)->pageout == 1){//swapped out
					cout<<"#";
				}else{//not swapped out
					cout<<"*";
				}
				cout<<" ";
				continue;//go to next
			}
			cout<<i<<":"; // pageTable index
			//R-referenced M-modified S-swapped out
			if(pageTable.at(i)->referenced ==1){
				cout<<"R";
			}else{
				cout<<"-";
			}
			if(pageTable.at(i)->modified ==1){
				cout<<"M";
			}else{
				cout<<"-";
			}
			if(pageTable.at(i)->pageout ==1){
				cout<<"S";
			}else{
				cout<<"-";
			}
			cout<<" ";
		}
		cout<<endl;
	}

	//----------------------------print frameTable-------------------------
	if (F){
		PTE *p;
		for (int i=0;i<frameTable.size();i++){
			p = frameTable.at(i)->pte;
			if (p != NULL){
				cout<<(find(pageTable.begin(),pageTable.end(),p)-pageTable.begin());
			}else{
				cout<<"*";
			}
			cout<<" ";
		}
		cout<<endl;
	}

	//-----------------------------print SUM---------------------------------
	unsigned long long totalcost = 0;
	totalcost+= maps*400+unmaps*400+ins*3000+outs*3000+zeros*150+instr_count*1;

	if (S){
		printf("SUM %d U=%d M=%d I=%d O=%d Z=%d ===> %llu\n",
			instr_count,unmaps,maps,ins,outs,zeros,totalcost);
	}
 
  	//cout<<"now reaching the end"<<endl;
	return 0;
}//end of main

	



