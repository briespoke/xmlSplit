#include "main.hpp"

using namespace std;

OFStreamFactory::OFStreamFactory (string _basePath, char _min, char _max, int _numChars){
  current = 0;
  this->basePath = _basePath;
  this->min = _min;
  this->max = _max;
  this->numChars = _numChars;
}
OFStreamFactory::~OFStreamFactory() {}

string OFStreamFactory::getName() {
  int base = max - min + 1;
  int running = current;
  list<char> name;
  do {
    int currentPlace = running % base;
    name.push_front(min + currentPlace);
    running = running - currentPlace;
    running = running / base;
  } while (running != 0);
  
  while (name.size() < numChars) {
    name.push_front(min);
  }
  
  current ++;
  
  string nameString;
  
  for (list<char>::iterator it = name.begin(); it != name.end(); it++) {
    nameString.append(&(*it), 1);
  }
  
  return nameString;
}
string OFStreamFactory::getStreamName() {
  string name = basePath + '/' + getName();
  return name;
}

SplitStream::SplitStream(OFStreamFactory * factory, vector<string> splitTag) {
  this->factory = factory;
  this->splitTag = splitTag;
  inSplitArea = false;
  countLimit = -1;
  lineLimit = -1;
  byteLimit = -1;
}
SplitStream::~SplitStream() {}

void SplitStream::process(istream & input) {
  string accumulator;
  string line;

  lines = bytes = count = 0;

  ofstream output(factory->getStreamName().c_str(), ofstream::out);

  size_t bytesRead;

  size_t tagStart = -1;
  size_t tagEnd = -1;
  size_t offset;
  bool running = true;
  input >> line;
  string current;
  bytes += load(current, line);
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
	  count++;
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
	  count++;
	}
	currentTag.pop_back();
      }
      if (inSplitArea) {
	accumulator.append(current);
	accumulator.append("\n");
	lines ++;
      }
      if (leavingSplitArea) {
	bool shouldWrite = false;
	if (lineLimit != -1 && lines >= lineLimit) {
	  shouldWrite = true;
	}
	if (byteLimit != -1 && bytes >= byteLimit) {
	  shouldWrite = true;
	}
	if (countLimit != -1 && count >= countLimit) {
	  shouldWrite = true;
	}
	if (shouldWrite) {
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

    bytes += load(current, line);
    
    if (input.fail() || input.eof()) {
      if (! accumulator.empty()) {
	for (int i = 0; i < splitTag.size() - 1; i++) {
	  output << '<' << splitTag[i] << '>' << endl;
	}
	output << accumulator;
	for (int i = 0; i < splitTag.size() - 1; i++) {
	  output << "</" << splitTag[i] << '>' << endl;
	}
      }
      running = false;
    }

  }
}

int SplitStream::load(string &output, string &input) {
  int bytes = 0;
  try {
    int a, b;
    getFirstTagPosition(input, a, b);
    if (a > 0) {
      output = input.substr(0, a);
      input.erase(0, a);
      bytes = a;
    } else {
      output = input.substr(a, b + 1);
      input.erase(a, b + 1);
      bytes = b + 1 - a;
    }
  } catch (int e) {
    output = input;
    bytes = input.size();
    input.clear();
  }
  return bytes;
}

void SplitStream::getFirstTagPosition(string input, int & begin, int & end) {
  int tagStart = input.find('<', 0);
  int tagEnd = input.find('>', tagStart + 1);
  
  if (tagEnd == string::npos || tagStart == string::npos) {
    throw 3;
  }
  
  begin = tagStart;
  end = tagEnd;
}

string SplitStream::getTagName(string input, int & tagType) {
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
bool SplitStream::vectorCompare(vector<string> & a, vector<string> & b) {
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

void SplitStream::setLineLimit(int lines) {
  cout << "Set line limit to " << lines << endl;
  this->lineLimit = lines;
}

void SplitStream::setByteLimit(int bytes) {
  cout << "Set byte limit to " << bytes << endl;
  this->byteLimit = bytes;
}

void SplitStream::setCountLimit(int count) {
  cout << "Set count limit to " << count << endl;
  this->countLimit = count;
}


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

int getNumericalArg(const char * argSpec, int argc, char ** argv) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], argSpec) == 0) {
      if (i + 1 < argc) {
	int limit = atoi(argv[i + 1]);
	
	if (limit == 0) {
	  cerr << "Invalid argument (" << argv[i + 1] << ") to " << argSpec << ", aborting." << endl;
	  throw 4;
	}
	i++;
	return limit;
      } else { 
	cerr << "Need more arguments to " << argSpec << ", aborting." << endl;
	throw 5;
      }
    }
  }
  return -1;
}

vector<string> getTagSpec(char * tagSpec) {
  vector<string> tags;
  string tokens(tagSpec);
  istringstream tokenizer(tokens);
  string current;
  while (getline(tokenizer, current, '/')) {
    tags.push_back(current);
  }
  if (tags.size() == 0) {
    throw 7;
  }
  return tags;
}

int main (int argc, char ** argv) {
  vector<string> tagSpec;
  try {
    tagSpec = getTagSpec(argv[argc - 1]);
  } catch (int e) {
    cerr << "Invalid tag specification" << endl;
    return 2;
  }
  OFStreamFactory factory("/Users/callouskitty/xmlSplit", '0', '9', 4);
  SplitStream ss(&factory, tagSpec);

  ss.setLineLimit(getNumericalArg("-l", argc, argv));
  ss.setByteLimit(getNumericalArg("-b", argc, argv));
  ss.setCountLimit(getNumericalArg("-c", argc, argv));

  ss.process(cin);
  
}
