#include <iostream>
#include <string>
#include <list>
#include <map>
#include <fstream>
#include <cstdio>
#include <sstream>
#include <climits>
#include <exception>
#include <vector>

using namespace std;

#define BUFFER_SIZE 100000

#define TAGTYPE_BEGIN 0
#define TAGTYPE_END 1
#define TAGTYPE_SINGLETON 2

class OFStreamFactory {
public:
  OFStreamFactory(string _basePath, char _min, char _max, int _numChars);

  virtual ~OFStreamFactory();

  string getName();
  string getStreamName();
private:
  int current;
  string basePath;
  char min;
  char max;
  int numChars;
};

class SplitStream {
public:
  SplitStream(OFStreamFactory * factory, vector<string> splitTag);
  
  virtual ~SplitStream();
  
  void process(istream & input);

  string load(string &input);

  void getFirstTagPosition(string input, int & begin, int & end);

  string getTagName(string input, int & tagType);

  bool vectorCompare(vector<string> & a, vector<string> & b);
  
private:
  OFStreamFactory * factory;
  vector<string> currentTag;
  vector<string> splitTag;
  bool inSplitArea;
};

