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
  SplitStream(OFStreamFactory * factory, vector<string> splitTag) {
    this->factory = factory;
    this->splitTag = splitTag;
    inSplitArea = false;
  }
  
  ~SplitStream() {}
  
  void process(istream & input) {
    string accumulator;
    string line;

    ofstream output(factory->getStreamName().c_str(), ofstream::out);

    size_t bytesRead;

    size_t tagStart = -1;
    size_t tagEnd = -1;
    size_t offset;
    bool running = true;
    input >> line;
    string current = load(line);
    while (running) {
      try {
	int a, b, tagType;
	bool leavingSplitArea = false;
	getFirstTagPosition(current, a, b);
	string tag = current.substr(a, b - a + 1);
	
	string tagName = getTagName(tag, tagType);

	if (tagType == TAGTYPE_BEGIN || tagType == TAGTYPE_SINGLETON) {
	  currentTag.push_back(tagName);
	  
	  if (vectorCompare(currentTag, splitTag)) {
	    inSplitArea = true;
	  }
	  

	} else if (tagType == TAGTYPE_END && tagName == currentTag[currentTag.size() - 1]) {
	  if (vectorCompare(currentTag, splitTag)) {
	    accumulator.append(current);
	    accumulator.append("\n");
	    inSplitArea = false;
	    leavingSplitArea = true;
	  }
	  currentTag.pop_back();
	}
	for (int i = 0; i < currentTag.size(); i++) {
	}
	
	if (tagType == TAGTYPE_SINGLETON) {
	  if (vectorCompare(currentTag, splitTag)) {
	    inSplitArea = false;
	    leavingSplitArea = true;
	    
	    accumulator.append(current);
	    accumulator.append("\n");
	  }
	  currentTag.pop_back();
	}
	if (inSplitArea) {
	  accumulator.append(current);
	  accumulator.append("\n");
	}
	if (leavingSplitArea) {
	  for (int i = 0; i < currentTag.size(); i++) {
	    output << '<' << currentTag[i] << '>' << endl;
	  }
	  output << accumulator;
	  for (int i = 0; i < currentTag.size(); i++) {
	    output << "</" << currentTag[i] << '>' << endl;
	  }
	  output.close();
	  output.open(factory->getStreamName().c_str(), ofstream::out);
	  accumulator.clear();
	}

      } catch (int e) {
	if (inSplitArea) {
	  accumulator.append(current);
	  accumulator.append("\n");
	}

      }

      if (line.empty()) {
	input >> line;	
      }

      current = load(line);

      if (input.fail() || input.eof()) {
	running = false;
      }

    }
    /*
      while (!accumulator.empty()) {
      output.write(accumulator.c_str(), accumulator.size());
      accumulator.clear();
    }
    */

  }
  string load(string &input) {
    string tmp;
    try {
      int a, b;
      getFirstTagPosition(input, a, b);
      if (a > 0) {
	tmp = input.substr(0, a);
	input.erase(0, a);
      } else {
	tmp = input.substr(a, b + 1);
	input.erase(a, b + 1);
      }
    } catch (int e) {
      tmp = input;
      input.clear();
    }
    return tmp;
  }

  void getFirstTagPosition(string input, int & begin, int & end) {
    int tagStart = input.find('<', 0);
    int tagEnd = input.find('>', tagStart + 1);
    
    if (tagEnd == string::npos || tagStart == string::npos) {
      throw 3;
    }
    
    begin = tagStart;
    end = tagEnd;
  }

  string getTagName(string input, int & tagType) {
    int beginTag = input.find('<', 0);

    if (beginTag != 0) {
      throw 1;
    }
    int firstSpace = input.find(' ', 0);
    int firstSlash = input.find('/', 1);
    int firstBrace = input.find('>', 0);
    if (firstBrace == string::npos) {
      throw 2;
    }
    //This means we are dealing with an end tag.
    if (firstSlash == 1) {
      int wordEnd = INT_MAX;
      if (firstSpace != string::npos) {
	wordEnd = firstSpace;
      }
      if (firstBrace != string::npos && firstBrace < wordEnd) {
	wordEnd = firstBrace;
      }
      tagType = TAGTYPE_END;
      return input.substr(2, wordEnd - 2);
    } else {
      int wordEnd = INT_MAX;
      if (firstSpace != string::npos) {
	wordEnd = firstSpace;
      }
      if (firstBrace != string::npos && firstBrace < wordEnd) {
	wordEnd = firstBrace;
      }
      if (firstSlash != string::npos && firstSlash < wordEnd) {
	wordEnd = firstSlash;
	tagType = TAGTYPE_SINGLETON;
      } else {
	tagType = TAGTYPE_BEGIN;
      }
      return input.substr(1, wordEnd - 1);
    }
  }
  void beingTag(string tagName, map<string, string> attribute) {
  }
  void endTag(string tagName) {
  }
  void text(string textContent) {
  }
  bool vectorCompare(vector<string> & a, vector<string> & b) {
    if (a.size() != b.size()) {
      return false;
    }
    for(int i = 0; i < a.size(); i++) {
      if (a[i] != b[i]) {
	return false;
      }
    }
    return true;
  }
  
private:
  OFStreamFactory * factory;
  vector<string> currentTag;
  vector<string> splitTag;
  bool inSplitArea;
};

int test() {
  // Run unit tests
  OFStreamFactory factory("/Users/callouskitty/xmlSplit", '0', '9', 4);
  
  vector<string> leaf;
  leaf.push_back("root");
  leaf.push_back("leaf");

  SplitStream ss(&factory, leaf);
  int failed = 0;

  try {
    int tagType;
    cout << ss.getTagName("<cow>", tagType) << endl;
    if (tagType != TAGTYPE_BEGIN) {
      failed++;
    }
    cout << ss.getTagName("<cow f='moo'>", tagType) << endl;
    if (tagType != TAGTYPE_BEGIN) {
      failed++;
    }
    cout << ss.getTagName("<cow/>", tagType) << endl;
    if (tagType != TAGTYPE_SINGLETON) {
      failed++;
    }
    cout << ss.getTagName("</cow>", tagType) << endl;
    if (tagType != TAGTYPE_END) {
      failed++;
    }
    cout << ss.getTagName("<cow >", tagType) << endl;
    if (tagType != TAGTYPE_BEGIN) {
      failed++;
    }
  } catch (int e) {
    failed ++;
  }
  int tagType;
  try {
    ss.getTagName("<cow", tagType);
    failed++;
  } catch (int e) {
    cout << "Properly rejected <cow" << endl;
  }
  try {
    ss.getTagName("cow", tagType);
    failed++;
  } catch (int e) {
    cout << "Properly rejected cow" << endl;
  }
  try {
    cout << ss.getTagName("<cow wef wwef ", tagType);
    failed++;
  } catch (int e) {
    cout << "Properly rejected <cow wef wef" << endl;
  }
  try {
    cout << ss.getTagName("<cow fewf='f'", tagType);
    failed++;
  } catch (int e) {
    cout << "Properly rejected <cow fewf='f'" << endl;
  }
  try {
    int a, b;
    ss.getFirstTagPosition("<cow>", a, b);
    printf("%d, %d\n", a, b);
    ss.getFirstTagPosition("    <cow>", a, b);
    printf("%d, %d\n", a, b);
    ss.getFirstTagPosition("<cow>    ", a, b);
    printf("%d, %d\n", a, b);
    ss.getFirstTagPosition("    <cow>     ", a, b);
    printf("%d, %d\n", a, b);
  } catch (int e) {
    failed++;
  }
  try {
    int a, b;
    ss.getFirstTagPosition("cow", a, b);
    failed++;
  } catch (int e) {
    cout << "Properly rejected cow" << endl;
  }
  vector<string> a;
  vector<string> b;
  
  a.push_back("moo");
  b.push_back("moo");
  
  bool testVal = ss.vectorCompare(a, b);
  if (testVal == false) {
    failed++;
  } else {
    cout << "A and B are the same" << endl;
  }
  a.push_back("cow");
  
  testVal = ss.vectorCompare(a, b);

  if (testVal == true) {
    failed++;
  } else {
    cout << "A and B are different" << endl;
  }
  

  return failed;
}

int main (int argc, char ** argv) {
  //printf("Failed: %d\n", test());
  for (int currentArg = 0; currentArg < argc; currentArg++) {
    
  }
  vector<string> leaf;
  leaf.push_back("root");
  leaf.push_back("leaf");
  OFStreamFactory factory("/Users/callouskitty/xmlSplit", '0', '9', 4);
  SplitStream ss(&factory, leaf);
  ss.process(cin);
  
}
