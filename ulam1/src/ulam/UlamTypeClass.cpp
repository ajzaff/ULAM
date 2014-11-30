#include <iostream>
#include <sstream>
#include <stdio.h>
#include "UlamTypeClass.h"
#include "UlamValue.h"
#include "CompilerState.h"
#include "Util.h"

namespace MFM {

  static const char * CUSTOMARRAY_GET_MANGLEDNAME = "Uf_4aRef";

  static const char * CUSTOMARRAY_SET_MANGLEDNAME = "Uf_4aSet";


  UlamTypeClass::UlamTypeClass(const UlamKeyTypeSignature key, const UTI uti, ULAMCLASSTYPE type) : UlamType(key, uti), m_class(type), m_customArray(false), m_customArrayType(Nav)
  {
    m_wordLengthTotal = calcWordSize(getTotalBitSize());
    m_wordLengthItem = calcWordSize(getBitSize());
  }


   ULAMTYPE UlamTypeClass::getUlamTypeEnum()
   {
     return Class;
   }


  const std::string UlamTypeClass::getUlamTypeImmediateMangledName(CompilerState * state)
  {
    if(needsImmediateType())
      {
	return UlamType::getUlamTypeImmediateMangledName(state);
      }

    return "T";
  }


  bool UlamTypeClass::needsImmediateType()
  {
    // now allowing right-justified naked quarks
    // no "naked" quarks; all living within an atom somehwere.
    return (m_class == UC_ELEMENT || m_class == UC_INCOMPLETE || getBitSize() == 0 ? false : true);
  }


  const std::string UlamTypeClass::getTmpStorageTypeAsString(CompilerState * state)
  {
    if(m_class == UC_QUARK)
      {
#if 0
	if(!isScalar())
	  return UlamType::getTmpStorageTypeAsString(state); // entire, u32 or u64
	else
	  {
	    if(isCustomArray())
	      return getArrayItemTmpStorageTypeAsString(state);
	    else
	      UlamType::getTmpStorageTypeAsString(state); // u32 or u64
	  }
#endif
	return UlamType::getTmpStorageTypeAsString(state); // entire, u32 or u64
      }
    else if(m_class == UC_ELEMENT)
      {
 	return "T";
      }
    else
      assert(0);
    return ""; //error!
  }


  const std::string UlamTypeClass::getArrayItemTmpStorageTypeAsString(CompilerState * state)
  {
    if(!isScalar())
      return UlamType::getArrayItemTmpStorageTypeAsString(state);
      
    assert(isCustomArray());
    return state->getUlamTypeByIndex(getCustomArrayType())->getTmpStorageTypeAsString(state);
    //return "T";  //Atom, a vcustom array?
  }


  const std::string UlamTypeClass::getImmediateStorageTypeAsString(CompilerState * state)
  {
    std::ostringstream ctype;
    ctype << getUlamTypeImmediateMangledName(state);

    //if(m_class == UC_QUARK)
    if(m_class == UC_QUARK && isScalar())
      ctype << "<CC>";  // not ,POS> because immediates know their position

    return ctype.str();
  } //getImmediateStorageTypeAsString


  const char * UlamTypeClass::getUlamTypeAsSingleLowercaseLetter()
  {
    switch(m_class)
      {
      case UC_ELEMENT:
	return "e";
      case UC_QUARK:
	return "q";
      case UC_INCOMPLETE:
      default:
	assert(0);
      };
    return "?";
  }


  //see SymbolVariableDataMember printPostfix for recursive output
  void UlamTypeClass::getDataAsString(const u32 data, char * valstr, char prefix, CompilerState& state)
  {
    if(prefix == 'z')
      sprintf(valstr,"%d", data);
    else
      sprintf(valstr,"%c%d", prefix, data);
  }


  ULAMCLASSTYPE UlamTypeClass::getUlamClass()
  {
    return m_class;
  }


  void UlamTypeClass::setUlamClass(ULAMCLASSTYPE type)
  {
    m_class = type;
  }


  bool UlamTypeClass::isConstant()
  {
    return false;   //e.g. zero-size quark is not a constant
  }

  bool UlamTypeClass::isScalar()
  {
    return (m_key.getUlamKeyTypeSignatureArraySize() == NONARRAYSIZE);
  }

  bool UlamTypeClass::isCustomArray()
  {
    return m_customArray;
  }


  void UlamTypeClass::setCustomArrayType(UTI type)
  {
    assert(isScalar());
    m_customArray = true;
    m_customArrayType = type;
  }


  UTI UlamTypeClass::getCustomArrayType()
  {
    assert(m_customArray);
    return m_customArrayType;
  }


  s32 UlamTypeClass::getBitSize()
  {
    s32 bitsize = m_key.getUlamKeyTypeSignatureBitSize();
    return bitsize < 0 ? 0 : bitsize; //allow for empty quarks 
  }


  PACKFIT UlamTypeClass::getPackable()
  {
    if(m_class == UC_ELEMENT)
      return PACKED;

    return UlamType::getPackable();  //quarks depend their size
  }


  //quarks are right-justified in an atom space
  const std::string UlamTypeClass::getUlamTypeAsStringForC()
  {
    assert(m_class != UC_INCOMPLETE);
    if(m_class == UC_QUARK)
      {
	return UlamType::getUlamTypeAsStringForC();
      }
   
    return "T"; //for elements
  }


  const std::string UlamTypeClass::getUlamTypeVDAsStringForC()
  {
    return "VD::BITS";  //for quark use bits
  }


  const std::string UlamTypeClass::getUlamTypeUPrefix()
  {
    if(getArraySize() > 0)
      return "Ut_";

    switch(m_class)
      {
      case UC_ELEMENT:
	return "Ue_";
      case UC_QUARK:
	return "Uq_";
      case UC_INCOMPLETE:
	return "U?_";
     default:
	assert(0);
      };
    return "xx";
  }


  const std::string UlamTypeClass::getUlamTypeNameBrief(CompilerState * state)
  {
    return m_key.getUlamKeyTypeSignatureName(state);
  }


  void UlamTypeClass::genUlamTypeMangledDefinitionForC(File * fp, CompilerState * state)
  {
    assert(m_class == UC_QUARK);

    if(!isScalar())
      {
	// immediate quark arrays are treated like immediate primitives, not quarks
	  return UlamType::genUlamTypeMangledDefinitionForC(fp, state);
      }

    //is scalar here..
    state->m_currentIndentLevel = 0;
    const std::string mangledName = getUlamTypeImmediateMangledName(state);
    std::ostringstream  ud;
    ud << "Ud_" << mangledName;  //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    //s32 sizeByIntBits = getTotalWordSize();
    s32 len = getTotalBitSize();

    state->indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");
    
    state->indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");
    
    state->indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    state->m_currentIndentLevel++;

    //forward declaration of quark (before struct!)
    state->indent(fp);
    fp->write("template<class CC, u32 POS> class ");
    fp->write(getUlamTypeMangledName(state).c_str());
    fp->write(";  //forward\n\n");

    state->indent(fp);
    fp->write("template<class CC>\n");

    state->indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    fp->write("\n");
    state->indent(fp);
    fp->write("{\n");

    state->m_currentIndentLevel++;
    
    //typedef atomic parameter type inside struct
    state->indent(fp);
    fp->write("typedef typename CC::ATOM_TYPE T;\n");
    state->indent(fp);
    fp->write("typedef typename CC::PARAM_CONFIG P;\n");
    state->indent(fp);
    fp->write("enum { BPA = P::BITS_PER_ATOM };\n");
    fp->write("\n");

    //quark typedef (right-justified)
    state->indent(fp);
    fp->write("typedef ");
    fp->write(getUlamTypeMangledName(state).c_str());
    fp->write("<CC, ");
    fp->write_decimal(BITSPERATOM - len);
    fp->write("> Us;\n");

    state->indent(fp);
    fp->write("typedef AtomicParameterType<");
    fp->write("CC");  //BITSPERATOM
    fp->write(", ");
    fp->write(getUlamTypeVDAsStringForC().c_str());
    fp->write(", ");
    fp->write_decimal(len);   //include arraysize
    fp->write(", ");
    fp->write_decimal(BITSPERATOM - len); //right-justified    
    fp->write("> ");
    fp->write(" Up_Us;\n");

    //storage here in atom
    state->indent(fp);
    fp->write("T m_stg;  //storage here!\n");

    //default constructor (used by local vars)
    state->indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() : m_stg() { }\n");

    //constructor here (used by const tmpVars)
    state->indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString(state).c_str()); //s32 or u32
    //fp->write(" d) : m_t(d) {}\n");
    fp->write(" d) { write(d); }\n");

    //copy constructor here (used by func call return values)
    state->indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getImmediateStorageTypeAsString(state).c_str()); //s32 or u32
    fp->write("& d) : m_stg(d.m_stg) {}\n");

    //default destructor (for completeness)
    state->indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //read 'entire quark' method
    genUlamTypeReadDefinitionForC(fp, state);

    //write 'entire quark' method
    genUlamTypeWriteDefinitionForC(fp, state);

    // getBits method for scalar
    state->indent(fp);
    fp->write("BitVector<BPA>& getBits() { return m_stg.GetBits(); }\n");

    if(isCustomArray())
      genCustomArrayMangledDefinitionForC(fp, state);

    state->m_currentIndentLevel--;
    state->indent(fp);
    fp->write("};\n");
    
    state->m_currentIndentLevel--;
    state->indent(fp);
    fp->write("} //MFM\n");
    
    state->indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledDefinitionForC


  void UlamTypeClass::genUlamTypeReadDefinitionForC(File * fp, CompilerState * state)
  {
    // arrays are handled separately
    if(!isScalar())
      return UlamType::genUlamTypeReadDefinitionForC(fp,state);

    //not an array
    state->indent(fp);
    fp->write("const ");
    fp->write(getTmpStorageTypeAsString(state).c_str()); //s32 or u32
    fp->write(" read() const { return Up_Us::");
    fp->write(readMethodForCodeGen().c_str());
    fp->write("(m_stg.GetBits() ); }\n");
  } //genUlamTypeReadDefinitionForC


  void UlamTypeClass::genUlamTypeWriteDefinitionForC(File * fp, CompilerState * state)
  {
    // arrays are handled separately
    if(!isScalar())
      return UlamType::genUlamTypeWriteDefinitionForC(fp,state);

    // here, must be scalar
    state->indent(fp);
    fp->write("void write(const ");
    fp->write(getTmpStorageTypeAsString(state).c_str()); //s32 or u32
    fp->write(" v) { Up_Us::");
    fp->write(writeMethodForCodeGen().c_str());
    fp->write("(m_stg.GetBits(), v); }\n");
  } //genUlamTypeWriteDefinitionForC


  void UlamTypeClass::genCustomArrayMangledDefinitionForC(File * fp, CompilerState * state)
  {
    assert(m_class == UC_QUARK);

    assert(isScalar() && isCustomArray());

    state->indent(fp);
    fp->write("//CustomArray accessor methods:\n");
    // reads an element of array
    state->indent(fp);
    fp->write("const "); 
    fp->write(getArrayItemTmpStorageTypeAsString(state).c_str()); //T
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 unitsize) { return Us::"); //const unhappy w first arg
    fp->write(readArrayItemMethodForCodeGen().c_str());
    fp->write("(m_stg, ");
    fp->write(state->getUlamTypeByIndex(Int)->getImmediateStorageTypeAsString(state).c_str());
    fp->write("((const s32)index)).read(); }\n");

    // reads an element of array
    state->indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString(state).c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 unitsize) { Us::");
    fp->write(writeArrayItemMethodForCodeGen().c_str());
    fp->write("(m_stg, ");
    fp->write(state->getUlamTypeByIndex(Int)->getImmediateStorageTypeAsString(state).c_str());
    fp->write("((const s32) index), ");
    fp->write(state->getUlamTypeByIndex(Atom)->getImmediateStorageTypeAsString(state).c_str());
    fp->write("(v)); }\n");
  } //genCustomArrayMangledDefinitionForC


  const std::string UlamTypeClass::readArrayItemMethodForCodeGen()
  {
    if(isCustomArray())
      return CUSTOMARRAY_GET_MANGLEDNAME;
    return UlamType::readArrayItemMethodForCodeGen();
  }


  const std::string UlamTypeClass::writeArrayItemMethodForCodeGen()
  {
    if(isCustomArray())
      return CUSTOMARRAY_SET_MANGLEDNAME;
    return UlamType::writeArrayItemMethodForCodeGen();
  }

} //end MFM