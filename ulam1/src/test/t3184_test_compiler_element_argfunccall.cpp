#include "TestCase_EndToEndCompiler.h"

namespace MFM {

  BEGINTESTCASECOMPILER(t3184_test_compiler_element_argfunccall)
  {
    std::string GetAnswerKey()
    {
      return std::string("Ue_Foo { Int(4) m_i(3);  Bool(1) m_b(false);  Int(32) test() {  m_b true cast = m_b ( ( 7 cast 7 cast )max )check = m_i cast return } }\nExit status: 3");
    }
    
    std::string PresetTest(FileManagerString * fms)
    {
      bool rtn1 = fms->add("Foo.ulam","ulam 1; element Foo { Int(4) m_i; Bool m_b; Bool check(Bool b) { if(b) m_i = 4; else m_i = 3; return b; } Bool max(Int a, Int b){ return (Bool) (a - b); } Int test() { m_b = true; m_b = check( max(7,7) ); return m_i; } }\n"); 

      if(rtn1)
	return std::string("Foo.ulam");
      
      return std::string("");
    }
  }
  
  ENDTESTCASECOMPILER(t3184_test_compiler_element_argfunccall)
  
} //end MFM

