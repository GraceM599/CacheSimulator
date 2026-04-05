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
    int tagsize;
    int numLines;
    int numSetsExp;
    
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
void setConfig(cacheConfig& cfg, std::string filename) {
    ifstream configFile(filename);
    if (!configFile.is_open()) {
        cout << "ERROR: config.txt failed to open\n";
        return;
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
    if(cfg.fullyAssoc == 'y' || cfg.fullyAssoc == 'Y'){
        cfg.directMapped = 'n';
    }
    getline(configFile, line);
    cfg.setSizeExp = stoi(line);
    getline(configFile, line);
    cfg.lru_char = line[0];
    getline(configFile, line);
    cfg.filename = line;

    int lru=0;

    cfg.numLinesExp = cfg.cacheSizeExp - cfg.lineSizeExp;
    //numLines = 2^numLinesExp

    if ((cfg.fullyAssoc=='y')||(cfg.fullyAssoc=='Y'))
        cfg.setSizeExp=cfg.numLinesExp;
    else
    {
        if ((cfg.directMapped=='y')||(cfg.directMapped=='Y'))
            cfg.setSizeExp=0;
        else {
            if ((cfg.setSizeExp>4)||(cfg.setSizeExp<1))
            {
                cout << "Try again. It's your responsibility to enter numbers that make sense" << endl;
                return;
            }
        }
    }

    if ((cfg.lru_char=='l')||(cfg.lru_char=='L'))
        lru=1;

    cfg.numSetsExp = cfg.numLinesExp - cfg.setSizeExp; //set field size
    //zero for fully associative
    cfg.tagsize = 32 - cfg.numSetsExp - cfg.lineSizeExp;
    cfg.numLines = pow(2, cfg.numLinesExp);

}

std::vector<int> runL1Simulation(cacheConfig cfg) {
    vector<vector<int> > cache(cfg.numLines);
    //cache line -> {tag, set, access #}
    for (int i=0; i<cfg.numLines; i++)
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
        int tag = getTag(addr, cfg.tagsize);
        int set;
        if (!cfg.numSetsExp)
            //if numSetsExp=0, then number of sets = 1 (2^0=1), and it is fully associative
                //there is only one set
                    set = 0;
        else
            set = getSet(addr, cfg.tagsize, cfg.numSetsExp);
        //check for hit or miss
        if (checkCache(set, cfg.setSizeExp, cache, tag, counter, cfg.lru_char))
        {
            numhits++;
        }

        getline(newfile, bytes);
        counter++;
    }
    return {numhits, counter};
}

std::vector<int> runL2Simulation(cacheConfig cfg, cacheConfig cfg2) {
    vector<vector<int> > cache1(cfg.numLines);
    vector<vector<int>> cache2(cfg2.numLines);
    //cache line -> {tag, set, access #}
    for (int i=0; i<cfg.numLines; i++)
    {
        //each line has three parameters: tag, set, access time
        //set all to -1 to start
        cache1[i] = vector<int>(2);
        cache1[i][0]=-1; //tag
        cache1[i][1]=-1; //access counter
    }
    for (int i = 0; i < cfg2.numLines; ++i){
        cache2[i] = vector<int>(2);
        cache2[i][0]=-1; //tag
        cache2[i][1]=-1; //access counter
    }


    ifstream newfile(cfg.filename);
    string ls, addr, bytes;
    int counter = 0;
    bool hit;
    int numhitsL1 = 0;
    int numhitsL2 = 0;
    while (!newfile.eof())
    {

        getline(newfile, ls,' ');
        getline(newfile, addr, ' ');
        int tag = getTag(addr, cfg.tagsize);
        int set;
        int set2;
        if (!cfg.numSetsExp)
            //if numSetsExp=0, then number of sets = 1 (2^0=1), and it is fully associative
                //there is only one set
                    set = 0;
        else
            set = getSet(addr, cfg.tagsize, cfg.numSetsExp);
        //check for hit or miss
        if (checkCache(set, cfg.setSizeExp, cache1, tag, counter, cfg.lru_char))
        {
            numhitsL1++;
        }
        else{
            int tag2 = getTag(addr, cfg2.tagsize);
            //std::cout << "Checking second cache" << std::endl;
            if (!cfg2.numSetsExp)

                        set2 = 0;
            else
                set2 = getSet(addr, cfg2.tagsize, cfg2.numSetsExp);
            //check for hit or miss
            if (checkCache(set2, cfg2.setSizeExp, cache2, tag2, counter, cfg2.lru_char))
            {
                numhitsL2++;
            }
        }

        getline(newfile, bytes);
        counter++;
    }
    return {numhitsL1, numhitsL2, counter};
}

std::vector<int> runTimingSimulation(cacheConfig cfg, cacheConfig cfg2){
}
int main() {

    cout << "Welcome. This is a rudimentary cache simulator." << endl;

    cacheConfig cfg;
    setConfig(cfg, "../config.txt");
    cacheConfig cfg2;
    setConfig(cfg2, "../config2.txt");
    auto results = runL1Simulation(cfg);

    float hitrate = (float) results[0]/(float) results[1];
    cout << "Hits " << results[0] << " accesses " << results[1] << " hit rate " << hitrate << endl;

    cout << "2 Level Cache -----\n";
    results = runL2Simulation(cfg, cfg2);
    float hitrateL1 = (float) (results[0])/(float) results[2];
    float hitrateL2 = (float) (results[1])/(float) results[2];
    float hitrateCombined = (float) (results[0]+results[1])/(float) results[2];
    cout << "Total Hits: " << results[0] + results[1];
    cout << " \nL1 hits: " << results[0] << " L2 hits: " << results[1];
    cout << " \nhit rate L1: " << hitrateL1;
    cout << " \nhit rate L2: " << hitrateL2;
    cout << " \nhit rate combined: " << hitrateCombined;

    return 0;
}
