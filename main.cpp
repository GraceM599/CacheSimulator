#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;


string getBinary(char  hex)
{
    if (hex=='0')
        return "0000";
    if (hex=='1')
        return "0001";
    if (hex=='2')
        return "0010";
    if (hex=='3')
        return "0011";
    if (hex=='4')
        return "0100";
    if (hex=='5')
        return "0101";
    if (hex=='6')
        return "0110";
    if (hex=='7')
        return "0111";
    if (hex=='8')
        return "1000";
    if (hex=='9')
        return "1001";
    if ((hex=='a')||(hex=='A'))
        return "1010";
    if ((hex=='b')||(hex=='B'))
        return "1011";
    if ((hex=='c')||(hex=='C'))
        return "1100";
    if ((hex=='d')||(hex=='D'))
        return "1101";
    if ((hex=='e')||(hex=='E'))
        return "1110";
    if ((hex=='f')||(hex=='F'))
        return "1111";
    return "";

}

int getTag(string addr, int tagSize)
{
    //takes addr and tagSize and returns the numerical value of tag
    string tagBinary;
    int tag=0;
    string extra;
    int numHex = tagSize/4;
    int numExtra = tagSize % 4;

    int i;
    for  (i=0; i<numHex; i++)
        tagBinary += getBinary(addr[i+2]);

    if (numExtra > 0)
    {
        extra = getBinary(addr[i+2]);
        for (int j = 0; j<numExtra; j++)
            tagBinary+=extra[j];
    }
    int multiplier=1;
    for (i=tagSize-1;i>=0; i--)
    {
        if (tagBinary[i]=='1')
            tag+=multiplier;
        multiplier *= 2;
    }
    return tag;
}
int getSet(string addr, int tagsize, int setsize)
{
    //returns block number
    int set=0;
    string binaryAddress;
    string setBinary;

    for (int i=0; i<8; i++)
        binaryAddress+=getBinary(addr[i+2]);
    for (int i=0; i<setsize; i++)
        setBinary += binaryAddress[tagsize+i];
    //now turn setBinary into decimal
    int multiplier=1;
    for (int i=setsize-1; i>=0; i--)
    {
        if (setBinary[i]=='1')
            set+=multiplier;
        multiplier *=2;
    }
    return set;
}
bool checkCache(int set, int setSizeExp,  vector<vector<int> > &cache, int tag, int counter, int lru)
{
    if (!setSizeExp) //direct mapped
    {
        if (cache[set][0] == tag) {
            cache[set][1]=counter;
            return true;
        }
        else {
            cache[set][0]=tag;
            cache[set][1]=counter;
            return false;
        }
    }
    float setSize = pow(2, setSizeExp);
    int j=set*setSize;
    int emptySpot=-1;
    int smallestCounter=-1;
    int lineToReplace=-1;
    for (int i = 0; i<setSize; i++)
    {

        if (cache[i+j][0]==tag) {
            if (lru)
                cache[i+j][1]=counter;
            return true;
        }
        else if (cache[i+j][0]==-1){
            emptySpot = i+j;
        }
        else if (smallestCounter==-1) {
            smallestCounter = cache[i+j][1];
            lineToReplace=i+j;

        }
        else if(cache[i+j][1]<smallestCounter)
        {
            smallestCounter = cache[i+j][1];
            lineToReplace=i+j;
        }

    }
    //empty spot?
    if (emptySpot!=-1)  //there was an empty spot, fill it
    {
        cache[emptySpot][0]=tag;
        cache[emptySpot][1]=counter;
    }
    else //update entry with lowest counter
    {
        cache[lineToReplace][0]=tag;
        cache[lineToReplace][1]=counter;
    }
    return false;

}

struct cacheConfig {
    int cacheSizeExp;     //exponent for total cache size
    int lineSizeExp;      //exponent for block/line size
    char fullyAssoc;      //'Y'/'N' for fully associative
    char directMapped;    //'Y'/'N' for direct mapped
    int setSizeExp;       //exponent for number of lines per set (if set associative)
////zero for direct mapped (2^0 = 1 line/set), numLinesExp for fully associative (1 set)
    char lru_char;     //'L' for LRU, anything else for FIFO
    std::string filename; //memory trace file
    int numLinesExp;
};

bool isBlank(const string &s) {
    return all_of(s.begin(), s.end(), [](unsigned char c) {
        return isspace(c);
    });
}

void getNextLine(ifstream &f, string &out) {
    string line;
    while (getline(f, line)) {
        //cout << "LINE: " << line << " size=" << line.size() << endl;

        if (!line.empty() && !isBlank(line)) {
            out = line;
            return;
        }
    }

    out = ""; // EOF
}
void setConfig() {

}
int main() {

    cacheConfig cfg;
    ifstream configFile("../config.txt");
    if (!configFile.is_open()) {
        cout << "ERROR: config.txt failed to open\n";
        return 1;
    }
    std::string line;
    getline(configFile, line);
    cfg.cacheSizeExp = stoi(line);
    getline(configFile, line);
    cfg.lineSizeExp = stoi(line); //line size = 2^lineSizeExp, lineSizeExp=size of offset field
    getline(configFile, line);
    cfg.fullyAssoc = line[0];
    getline(configFile, line);
    cfg.directMapped= line[0];
    getline(configFile, line);
    cfg.setSizeExp = stoi(line);
    getline(configFile, line);
    cfg.lru_char = line[0];
    getline(configFile, line);
    cfg.filename = line;

    int lru=0;

    cout << "This is a rudimentary cache simulator." << endl;

    int numLinesExp = cfg.cacheSizeExp - cfg.lineSizeExp;
    //numLines = 2^numLinesExp

    if ((cfg.fullyAssoc=='y')||(cfg.fullyAssoc=='Y'))
        cfg.setSizeExp=numLinesExp;
    else
    {
        if ((cfg.directMapped=='y')||(cfg.directMapped=='Y'))
            cfg.setSizeExp=0;
        else {
            if ((cfg.setSizeExp>4)||(cfg.setSizeExp<1))
            {
                cout << "Try again. It's your responsibility to enter numbers that make sense" << endl;
                return 0;
            }
        }
    }

    if ((cfg.lru_char=='l')||(cfg.lru_char=='L'))
        lru=1;

    int numSetsExp = numLinesExp - cfg.setSizeExp; //set field size
    //zero for fully associative
    int tagsize = 32 - numSetsExp - cfg.lineSizeExp;
    int numLines = pow(2, numLinesExp);
    vector<vector<int> > cache(numLines);
    for (int i=0; i<numLines; i++)
    {
        //each line has three parameters: tag, set, access time
        //set all to -1 to start
        cache[i] = vector<int>(2);
        cache[i][0]=-1; //tag
        cache[i][1]=-1; //access counter
    }

    ifstream newfile(cfg.filename);
    string ls, addr, bytes;
    int counter = 0;
    bool hit;
    int numhits = 0;
    while (!newfile.eof())
    {

        getline(newfile, ls,' ');
        getline(newfile, addr, ' ');
        int tag = getTag(addr, tagsize);
        int set;
        if (!numSetsExp)
            //if numSetsExp=0, then number of sets = 1 (2^0=1), and it is fully associative
            //there is only one set
            set = 0;
        else
            set = getSet(addr, tagsize, numSetsExp);
        //check for hit or miss
        if (checkCache(set, cfg.setSizeExp, cache, tag, counter, lru))
        {
            numhits++;
        }

        getline(newfile, bytes);
        counter++;
    }
    float hitrate = (float) numhits/(float) counter;
    cout << "Hits " << numhits << " accesses " << counter << " hit rate " << hitrate << endl;

    return 0;
}
