
// https://bitbucket.org/doolin/disambiguator/src/450cd6c85791/src/training.cpp#cl-501

#include <string>
#include <fstream>

#include <cppunit/Portability.h>
#include <cppunit/portability/CppUnitSet.h>
#include <cppunit/extensions/TestFactory.h>
#include <cppunit/TestCase.h>

#include "training.h"

extern "C" {
  #include "strcmp95.h"
}

using std::string;


class RarenamesTest : public CppUnit::TestCase {

public:
  RarenamesTest(std::string name) : CppUnit::TestCase(name) {}


 /**
  * The name_compare function builds a similarity
  * weight.
  */
  void test_rarename() {
    
    CPPUNIT_ASSERT(1 == 1);
  }

};


void test_rarenames() {

  RarenamesTest * rt = new RarenamesTest(std::string("initial test"));
  rt->test_rarename();
  delete rt;
}


#ifdef rarenames_STANDALONE
int
main(int argc, char ** argv) {
  test_rarenames();
  return 0;
}
#endif