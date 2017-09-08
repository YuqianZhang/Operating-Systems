#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <ctype.h>
using namespace std;

ifstream inputFile;
vector<string> symbolList;
int defCount,relativeAddress,useCount,codeCount;
int baseAddress = 0;
bool syntaxError = false;
bool endofFile = false;
int linenum = 1,lineoffset=1, endoffset=1,endline=1,nextlinenumber=1,nextlineoffset=1,characount=0;
int moduleCounter =0;
int MemoryMapline = 0;
int instrCounter = 0;
map<int, pair<string, int> > checkuseList;

void parseerror(int errcode){
	static string errstr[] = {
		"NUM_EXPECTED",		//number expected
		"SYM_EXPECTED",		//symbol expected
		"ADDR_EXPECTED",	//addressing expected which is I/A/R/E
		"SYM_TOO_LONG",		//symbol name is too long 
		"TOO_MANY_DEF_IN_MODULE",	//>16
		"TOO_MANY_USE_IN_MODULE",	//>16
		"TOO_MANY_INSTR",			//total num_instr exceeds memory size(512)
	};
	printf("Parse Error line %d offset %d: %s\n",endline,endoffset,errstr[errcode].c_str());
}

string getString (){ 
    string s = "";
    char ch;
    
    linenum = nextlinenumber;
    lineoffset = nextlineoffset;
    while (true){
        ch = inputFile.get();
        if (inputFile.eof()){
            break;
        }
        else if (ch=='\t'||ch==' '){
            endline=linenum;
            endoffset = lineoffset;
            lineoffset++;//nextlineoffset
        }
        else if (ch=='\n'){
            endline=linenum;
            endoffset = lineoffset;
            linenum++;//nextlinenumber
            lineoffset=1;//nextlineoffset
            
        }
        else {
            s+=ch;
            endline=linenum;
            endoffset = lineoffset;
            lineoffset++;//nextlineoffset
            break;
        }
    }
    characount =1;
    nextlinenumber = linenum;
    nextlineoffset = lineoffset;
      
    while (true){
        ch = inputFile.get();
        if (inputFile.eof()){
            endline=endline;
            endoffset=endoffset+1;
            break;
        }else if (ch=='\t'||ch==' '){
            endline=linenum;
            endoffset = lineoffset;
            lineoffset+=1;
            nextlineoffset = lineoffset;
            return s; 
        }
        else if (ch=='\n'){
            endline = linenum;
            endoffset = lineoffset;
            lineoffset=1;
            nextlineoffset=lineoffset;
            nextlinenumber=linenum+1;
            return s;
        }
        else{
            s+=ch;
            endline=linenum;
            endoffset = lineoffset;
            lineoffset +=1;
            nextlineoffset= lineoffset;
            characount++;
        }

    }
    return s;
}

int stringTointeger (string s)
{
    int result = 0;
    for (int i = 0; i < s.length(); i++)
            result = result *10 + (s[i] - '0');
    return result;
}

bool isSymbol(string s){//symbols always begin with alpha ,followed by optional alphanumerical 
    if (!isalpha(s[0]))
        return false;
    else{
       for (int i =1;i<s.length();i++) {
        if (!isalpha(s[i])&& !isdigit(s[i]))
            return false;
       }
    }
    return true;
}
//------------------ three functions used in getDeflist ()-----------------------
int getDefcount(string s){
    for (int i=0;i<s.length();i++){
        if (!isdigit(s[i])){//checking the if the string is valid digits 
            endoffset-=characount;//if not, move back charac offset
            parseerror(0);
            syntaxError=true;
            return -1;
        }
    }
    return stringTointeger(s);
}

string getSymbol(){
    string s = getString();
    if (!isSymbol(s)){
        endoffset-=characount;
        parseerror(1);
        syntaxError=true;
    }
    if (s.length()>16){
        endoffset-=characount;
        parseerror(3);
        syntaxError=true;
    }
    return s;
}

string getRelativeaddress(){
    string s = getString();
    if (s==""){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
    }
    if (isSymbol(s)){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
     }
    return s;
}

//------------------//----------------//-------------Symbol class------------//------------------
class Symbol{
public:
    int relativeAddress;
    int absoluteAddress;
    //int definitionCount;
    int moduleNum;
    int moduleCodecount;
    bool reDefined;
    bool isUsed;
    
    Symbol(int relative_Address,int base_Address,int module_Number,int module_Codecount){
        relativeAddress = relative_Address;
        absoluteAddress = base_Address + relativeAddress;
        //definitionCount= definition_Count;
        moduleNum = module_Number;
        moduleCodecount= module_Codecount;
        reDefined = false;
        isUsed = false;
    }
};

map<string, Symbol*> symbolTable;

//------------------ two functions used in getUselist ()-----------------------
int getUsecount(){
    string s = getString();
    if (s==""){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
        return -1;
    }
    if (isSymbol(s)){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
        return -1;
    }
    return stringTointeger(s);
}

string getusecountSymbols(){
    string s = getString();
    if (!isSymbol(s)){
        endoffset-=characount;
        parseerror(1);
        syntaxError=true;
     }
    return s;
}

//------------------ three functions used in getNuminstructions ()-----------------------
int getCodecount(){
    string s = getString();
    if (s==""){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
        return -1;
    }
    if (isSymbol(s)){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
        return -1;
    }
    return stringTointeger(s);
}

string getType(){
    string s = getString();
    if (s!=("I")){
        if(s!=("A")){
            if (s!=("R")){
                if (s!=("E")){
                    endoffset-=characount;
                    parseerror(2);
                    syntaxError=true;
                }
            }
        }	
    }
    return s;
}

string getInstr(){
    string s = getString();
    if (s==""){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
    }
    if (isSymbol(s)){
        endoffset-=characount;
        parseerror(0);
        syntaxError=true;
    }
    return s;
}


void getDeflist(){
	string s = getString();
    if (s=="") {
        endofFile=true;
        return;
    }
    defCount = getDefcount(s);//------------------getDefcount()
    if (syntaxError==true) return;

    if (defCount>16){
        endoffset-=characount;
        parseerror(4);
        syntaxError=true;
        return;
    }
    moduleCounter++;//------------------increment moduleCounter
	while (defCount > 0){
		string symbol =  getSymbol(); //------------------getSymbol()
        if (syntaxError==true){
            break;
        }
		string relative_word_address = getRelativeaddress();// ------------------getRelativeaddress()
        if (syntaxError==true){
            break;
        }
		relativeAddress = stringTointeger(relative_word_address);
        //------------------insert into symbolTable & update there
		if (symbolTable.find(symbol) == symbolTable.end()) {//put these two values in the symbolTable
            symbolTable[symbol] = new Symbol(relativeAddress,baseAddress,moduleCounter,0);
            symbolList.push_back(symbol);
        }
        else { 
            symbolTable[symbol]->reDefined = true; //but still use the first assigned value
        }
		defCount--;
	}
}


void getUselist(){
    if (syntaxError==true) return;
	useCount = getUsecount();//------------------getUsecount()
    if (syntaxError==true){
        return;
    }
	
    if (useCount>16){
        endoffset-=characount;
        parseerror(5);
        syntaxError=true;
        return;
    }
	while (useCount>0){
		getusecountSymbols();//------------------getusecountSymbols()
        if (syntaxError==true) break;
		useCount--;
	}
}


void getNuminstructions(){
    if (syntaxError==true) return;
	codeCount =getCodecount();//------------------getCodecount()
    if (syntaxError==true){
        return;
    }
	
    baseAddress+=codeCount;//------------------increment the baseAddress
    //--------------------------------update the moduleCodecount of each symbol in the symbolList------------------------
    for (vector<string>::iterator i=symbolList.begin() ; i!=symbolList.end(); i++){
        int symbolModuleNum = symbolTable[*i]->moduleNum;
        if (moduleCounter==symbolModuleNum){
            symbolTable[*i]->moduleCodecount = codeCount;
        }
    }

    if (baseAddress > 512){
        endoffset-=characount;
        parseerror(6);
        syntaxError=true;
        return;
    }

    while (codeCount>0) {
        getType();//------------------getType()
        if (syntaxError==true){
        break;
        }
        getInstr();//------------------getInstr()
        if (syntaxError==true){
        break;
        }
        codeCount--;
    }
}

int main(int argc, char * argv[]){
    
    inputFile.open(argv[1]);
	while(true){//pass one begins
        getDeflist();
        if (endofFile==true) break;
        if (syntaxError==true) break;
        getUselist();
        if (syntaxError==true) break;
        getNuminstructions();
        if (syntaxError==true) break;
    } //pass one ends
	inputFile.close();

    if (syntaxError==true){
        return 0;//return 0 if any parse error happens
    }

    for (vector<string>::iterator i=symbolList.begin() ; i!=symbolList.end(); i++) {//check rule 5
        int relative_address = symbolTable[*i]->relativeAddress;
        int max =  symbolTable[*i]->moduleCodecount-1;
        if (relative_address > max) {
            printf("Warning: Module %d: %s too big %d (max=%d) assume zero relative\n",symbolTable[*i]->moduleNum, i->c_str(), relative_address, max);
            symbolTable[*i]->relativeAddress = 0;
            symbolTable[*i]->absoluteAddress -= relative_address;
        }   
    }

    //------------------------------------print the Symbol Table------------------------------------
    cout<<"Symbol Table"<<endl;
    for (vector<string>::iterator i = symbolList.begin() ; i!=symbolList.end(); i++){
        Symbol *p = symbolTable[*i];
        int absoluteaddr = p->absoluteAddress;
        printf("%s=%d", i->c_str(), absoluteaddr);
        if (p->reDefined){
            printf(" Error: This variable is multiple times defined; first value used\n");
        }else{
        cout<<endl;
    	}
    }
    cout<<endl;

	inputFile.open(argv[1]);
    cout<<"Memory Map"<<endl;//pass two begins
    moduleCounter = 0; //clear moduleCounter
    while (inputFile >> defCount){
        moduleCounter++;
        baseAddress = instrCounter;
        
        while (defCount>0) {//read and pass def list
            getString();
            getString();
            defCount--;
        }
        
        inputFile >> useCount;//read use list
        string useSymbol;
        for (int i = 0; i < useCount; i++){
            useSymbol = getString();
            checkuseList[i]=pair<string, int>(useSymbol, 1);
        }
        
        inputFile >> codeCount;//read program text
        int ncodeCount = codeCount;
        while(codeCount>0){
            string type;
            type=getString();
            string instr; 
            instr=getString();
            printf("%03d: ", MemoryMapline++);
            while (instr.length() < 4){
                instr = '0' + instr;
            }
            char occ = instr[0];//------------------fetch the opcode in instr
            string oc;
            oc+=occ;
            int opcode = stringTointeger(oc);
           
            string op; //-----------------fetch the operand in instr
            for (int i = 1; i<4; i++) {
                op += instr[i];
            }
            int operand = stringTointeger(op);

            if (instr.length()>4){//check rule 10 & 11
                if (type=="I"){
                    opcode = 9;
                    operand = 999;
                    printf("%d%03d%s\n",opcode,operand," Error: Illegal immediate value; treated as 9999");
                }else{
                    opcode = 9;
                    operand = 999;
                    printf("%d%03d%s\n",opcode,operand," Error: Illegal opcode; treated as 9999");
                }

            }else{      
                if (type=="I"){//I instruction
                    cout<<instr<<endl;
                }

                if (type=="A"){//A instruction
                    if(operand > 512){//check rule 8
                        operand = 0;
                        printf("%d%03d%s\n",opcode,operand," Error: Absolute address exceeds machine size; zero used");
                    }else{
                        printf("%d%03d\n", opcode, operand);
                    }
                }
  
                if (type=="R"){// R instruction
                    if (operand > ncodeCount){//check rule 9
                        operand =0 +baseAddress;
                        printf("%d%03d%s\n", opcode,operand," Error: Relative address exceeds module size; zero used");
                    }else{
                        printf("%d%03d\n", opcode, operand + baseAddress); 
                    }   
                }
                
                if (type=="E"){//E instruction
                        if (operand >= useCount){//check rule 6
                            printf("%d%03d%s\n",opcode,operand," Error: External address exceeds length of uselist; treated as immediate");
                        }else{
                            string symbol = checkuseList[operand].first;
                            checkuseList[operand].second =  0;
                            if (symbolTable.count(symbol) == 0){//check rule 3
                                operand =0;
                                printf("%d%03d Error: %s is not defined; zero used\n", opcode, operand,symbol.c_str());
                            }
                            else{
                                Symbol *p = symbolTable[symbol];
                                p->isUsed = true;
                                printf("%d%03d\n", opcode, p->absoluteAddress);
                            }
                    }
                }
             }
            instrCounter++;
            codeCount--;
        }

        for (int i = 0; i < useCount; i++){//check rule 7 inside of pass two
            if(checkuseList[i].second == 1){
                printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", moduleCounter, checkuseList[i].first.c_str());
            }
        }
    }
    cout<<endl;

    for (vector<string>::iterator i = symbolList.begin(); i != symbolList.end(); i++){//check rule 4
        Symbol *p = symbolTable[*i];
        if (!p->isUsed)
            printf("Warning: Module %d: %s was defined but never used\n", p->moduleNum, i->c_str());
    }
    cout<<endl;//pass two ends
	inputFile.close();
	return 0;
}