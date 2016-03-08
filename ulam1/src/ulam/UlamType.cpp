#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sstream>
#include "UlamType.h"
#include "CompilerState.h"
#include "Util.h"

namespace MFM {

#define XY(a,b,c,d) b,

  static const char * utype_string[] = {
#include "UlamType.inc"
  };

#undef XY


#define XY(a,b,c,d) d,

  static const char * utype_primitivecode[] = {
#include "UlamType.inc"
  };

#undef XY


  UlamType::UlamType(const UlamKeyTypeSignature key, CompilerState & state) : m_key(key), m_state(state), m_wordLengthTotal(0), m_wordLengthItem(0), m_max(S32_MIN), m_min(S32_MAX)
  {}

  UlamType * UlamType::getUlamType()
  {
    return this;
  }

  const std::string UlamType::getUlamTypeName()
  {
    return m_key.getUlamKeyTypeSignatureAsString(&m_state);
    // REMINDER!! error due to disappearing string:
    //    return m_key.getUlamKeyTypeSignatureAsString().c_str();
  }

  const std::string UlamType::getUlamTypeNameBrief()
  {
    return m_key.getUlamKeyTypeSignatureNameAndSize(&m_state);
  }

  const std::string UlamType::getUlamTypeNameOnly()
  {
    return m_key.getUlamKeyTypeSignatureName(&m_state);
  }

   UlamKeyTypeSignature UlamType::getUlamKeyTypeSignature()
  {
    return m_key;
  }

  bool UlamType::isNumericType()
  {
    return false;
  }

  bool UlamType::isPrimitiveType()
  {
    return false;
  }

  bool UlamType::cast(UlamValue & val, UTI typidx)
  {
    assert(0);
    //std::cerr << "UlamType (cast) error! " << std::endl;
    return false;
  }

  bool UlamType::castTo32(UlamValue & val, UTI typidx)
  {
    assert(0);
    //std::cerr << "UlamType (cast) error! " << std::endl;
    return false;
  }

  bool UlamType::castTo64(UlamValue & val, UTI typidx)
  {
    assert(0);
    //std::cerr << "UlamType (cast) error! " << std::endl;
    return false;
  }

  FORECAST UlamType::safeCast(UTI typidx)
  {
    // initial tests for completeness and scalars
    if(!isComplete() || !m_state.isComplete(typidx))
      {
	std::ostringstream msg;
	msg << "Casting UNKNOWN sizes; " << getBitSize();
	msg << ", Value Type and size was: " << typidx << "," << m_state.getBitSize(typidx);
	MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), DEBUG);
	return CAST_HAZY;  //includes Navs & Hzy's
      }

    //let packable arrays of same size pass...
    if(!checkArrayCast(typidx))
      return CAST_BAD;

    return CAST_CLEAR;
  } //safeCast

  FORECAST UlamType::explicitlyCastable(UTI typidx)
  {
    return UlamType::safeCast(typidx); //default
  } //explicitlyCastable

  bool UlamType::checkArrayCast(UTI typidx)
  {
    if(isScalar() && m_state.isScalar(typidx))
      return true; //not arrays, ok

    bool bOK = true;
    if((getPackable() != PACKEDLOADABLE) || (m_state.determinePackable(typidx) != PACKEDLOADABLE))
      {
	//for now, limited to refs of same class, not subclasses. XXX
	UTI anyUTI = Nouti;
	AssertBool anyDefined = m_state.anyDefinedUTI(m_key, anyUTI);
	assert(anyDefined);

	std::ostringstream msg;
	msg << "Casting requires UNPACKED array support: ";
	msg << m_state.getUlamTypeNameBriefByIndex(typidx).c_str();
	msg << " TO " ;
	msg << getUlamTypeNameBrief().c_str();
	if((m_state.isARefTypeOfUlamType(typidx, anyUTI) == UTIC_SAME) || (m_state.isARefTypeOfUlamType(anyUTI, typidx) == UTIC_SAME))
	  MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), DEBUG);
	else
	  {
	    MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), ERR);
	    bOK = false;
	  }
      }
    else
      {
	s32 arraysize = getArraySize();
	s32 varraysize = m_state.getArraySize(typidx);
	if(arraysize != varraysize)
	  {
	    std::ostringstream msg;
	    msg << "Casting different Array sizes: ";
	    msg << m_state.getUlamTypeNameBriefByIndex(typidx).c_str();
	    msg << " TO " ;
	    msg << getUlamTypeNameBrief().c_str();
	    MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), ERR);
	    bOK = false;
	  }
	else
	  {
	    std::ostringstream msg;
	    msg << "Casting (nonScalar) Array: ";
	    msg << m_state.getUlamTypeNameBriefByIndex(typidx).c_str();
	    msg << " TO " ;
	    msg << getUlamTypeNameBrief().c_str();
	    MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), DEBUG);
	  }
      }
    return bOK;
  } //checkArrayCast

  bool UlamType::checkReferenceCast(UTI typidx)
  {
    //both complete; typidx is a reference
    UlamKeyTypeSignature key1 = getUlamKeyTypeSignature();
    UlamKeyTypeSignature key2 = m_state.getUlamKeyTypeSignatureByIndex(typidx);
    ALT alt1 = key1.getUlamKeyTypeSignatureReferenceType();
    ALT alt2 = key2.getUlamKeyTypeSignatureReferenceType();

    if(key1.getUlamKeyTypeSignatureNameId() != key2.getUlamKeyTypeSignatureNameId())
      return false;
    if(key1.getUlamKeyTypeSignatureArraySize() != key2.getUlamKeyTypeSignatureArraySize())
      return false;
    if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
      return false;

    //skip rest in the case of array item, continue with usual size fit
    if(alt1 == ALT_ARRAYITEM || alt2 == ALT_ARRAYITEM)
      return true;

    if(key1.getUlamKeyTypeSignatureBitSize() != key2.getUlamKeyTypeSignatureBitSize())
	  return false;
    if(alt1 != ALT_NOT || alt2 == ALT_NOT)
      return false;

    return true; //keys the same, except for reference type
  } //checkReferenceCast

  void UlamType::getDataAsString(const u32 data, char * valstr, char prefix)
  {
    sprintf(valstr,"%s", getUlamTypeName().c_str());
  }

  void UlamType::getDataLongAsString(const u64 data, char * valstr, char prefix)
  {
    sprintf(valstr,"%s", getUlamTypeName().c_str());
  }

  s32 UlamType::getDataAsCs32(const u32 data)
  {
    assert(0);
    return (s32) data;
  }

  u32 UlamType::getDataAsCu32(const u32 data)
  {
    assert(0);
    return data;
  }

  s64 UlamType::getDataAsCs64(const u64 data)
  {
    assert(0);
    return (s64) data;
  }

  u64 UlamType::getDataAsCu64(const u64 data)
  {
    assert(0);
    return data;
  }

  s32 UlamType::bitsizeToConvertTypeTo(ULAMTYPE tobUT)
  {
    return UNKNOWNSIZE; //atom, class, nav, ptr, holder
  }

  //deprecated, i think
  const std::string UlamType::getUlamTypeAsStringForC(bool useref)
  {
    assert(isComplete());
    assert(!useref); //could delete the argument useref.

    s32 len = getTotalBitSize(); //includes arrays
    assert(len >= 0);

    std::ostringstream ctype;
    ctype << "BitField<BitVector<BPA>, ";

    if(isScalar())
      ctype << getUlamTypeVDAsStringForC() << ", ";
    else
      ctype << "VD::BITS, "; //use BITS for arrays

    if(!isScalar() && getPackable() != PACKEDLOADABLE)
      len = getBitSize(); //per item

    if(useref)
      ctype << len << ", POS>"; // if reference ??????????????
    else
      ctype << len << ", " << (BITSPERATOM - len) << ">"; //right-just immediate

    return ctype.str();
  } //getUlamTypeAsStringForC

  const std::string UlamType::getUlamTypeVDAsStringForC()
  {
    assert(0);
    return "VD::NOTDEFINED";
  }

  const std::string UlamType::getUlamTypeMangledType()
  {
    // e.g. parsing overloaded functions, may not be complete.
    std::ostringstream mangled;
    s32 bitsize = getBitSize();
    s32 arraysize = getArraySize();

    if(isReference())
      mangled << "r";

    if(arraysize > 0)
      mangled << ToLeximitedNumber(arraysize);
    else
      mangled << 10;

    if(bitsize > 0)
      mangled << ToLeximitedNumber(bitsize);
    else
      mangled << 10;

    std::string ecode(UlamType::getUlamTypeEnumCodeChar(getUlamTypeEnum()));
    mangled << ToLeximited(ecode).c_str();

    return mangled.str();
  } //getUlamTypeMangledType

  const std::string UlamType::getUlamTypeMangledName()
  {
    // e.g. parsing overloaded functions, may not be complete.
    std::ostringstream mangled;
    mangled << getUlamTypeUPrefix().c_str();
    mangled << getUlamTypeMangledType();
    return mangled.str();
  } //getUlamTypeMangledName

  const std::string UlamType::getUlamTypeUPrefix()
  {
    return "Ut_";
  }

  bool UlamType::needsImmediateType()
  {
    return isComplete();
  }

  const std::string UlamType::getUlamTypeImmediateMangledName()
  {
    std::ostringstream imangled;
    imangled << "Ui_" << getUlamTypeMangledName();
    return  imangled.str();
  }

  const std::string UlamType::getUlamTypeImmediateAutoMangledName()
  {
    assert(needsImmediateType());

    if(isReference())
      {
	assert(0);
	return getUlamTypeImmediateMangledName();
      }

    //same as non-ref except for the 'r'
    std::ostringstream  automn;
    automn << "Ui_";
    automn << getUlamTypeUPrefix().c_str();
    automn << "r";
    automn << getUlamTypeMangledType();
    return automn.str();
  } //getUlamTypeImmediateAutoMangledName

  const std::string UlamType::getLocalStorageTypeAsString()
  {
    std::ostringstream ctype;
    ctype << getUlamTypeImmediateMangledName();
    ctype << "<EC>"; //name of struct w typedef(bf) and storage(bv);
    return ctype.str();
  } //getLocalStorageTypeAsString

  const std::string UlamType::getImmediateModelParameterStorageTypeAsString()
  {
    std::ostringstream mpimangled;
    //substitutes Up_ for Ut_ for model parameter immediate
    mpimangled << "Ui_Up_" << getUlamTypeMangledType();
    return mpimangled.str();
  } //getImmediateModelParameterStorageTypeAsString

  const std::string UlamType::getArrayItemTmpStorageTypeAsString()
  {
    return getTmpStorageTypeAsString(getItemWordSize());
  }

  const std::string UlamType::getTmpStorageTypeAsString()
  {
    return getTmpStorageTypeAsString(getTotalWordSize());
  }

  const std::string UlamType::getTmpStorageTypeAsString(s32 sizebyints)
  {
    std::string ctype;
    switch(sizebyints)
      {
      case 0: //e.g. empty quarks
      case 32:
	ctype = "u32";
	break;
      case 64:
	ctype = "u64";
	break;
      default:
	{
	  //ctype = getTmpStorageTypeAsString(getItemWordSize()); //u32, u64 (inf loop)
	  ctype = "T";
	  //assert(0);
	  //MSG(getNodeLocationAsString().c_str(), "Need UNPACKED ARRAY", INFO);
	}
      };
    return ctype;
  } //getTmpStorageTypeAsString

  const char * UlamType::getUlamTypeAsSingleLowercaseLetter()
  {
    return UlamType::getUlamTypeEnumCodeChar(getUlamTypeEnum());
  } //getUlamTypeAsSingleLowercaseLetter

  const std::string UlamType::castMethodForCodeGen(UTI nodetype)
  {
    std::ostringstream rtnMethod;
    UlamType * nut = m_state.getUlamTypeByIndex(nodetype);
    //base types e.g. Int, Bool, Unary, Foo, Bar..
    u32 sizeByIntBitsToBe = getTotalWordSize();
    u32 sizeByIntBits = nut->getTotalWordSize();

    if(sizeByIntBitsToBe != sizeByIntBits)
      {
	std::ostringstream msg;
	msg << "Casting different word sizes; " << sizeByIntBits;
	msg << ", Value Type and size was: " << nut->getUlamTypeName().c_str();
	msg << ", to be: " << sizeByIntBitsToBe << " for type: ";
	msg << getUlamTypeName().c_str();
	MSG(m_state.getFullLocationAsString(m_state.m_locOfNextLineText).c_str(), msg.str().c_str(), DEBUG);

	//use the larger word size
	if(sizeByIntBitsToBe < sizeByIntBits) //downcast using larger
	  sizeByIntBitsToBe = sizeByIntBits;
	else
	  sizeByIntBits = sizeByIntBitsToBe;
      }
    rtnMethod << "_" << nut->getUlamTypeNameOnly().c_str() << sizeByIntBits << "To";
    rtnMethod << getUlamTypeNameOnly().c_str() << sizeByIntBitsToBe;
    return rtnMethod.str();
  } //castMethodForCodeGen

  void UlamType::genUlamTypeMangledAutoDefinitionForC(File * fp)
  {
    s32 len = getTotalBitSize();
    if(len > (BITSPERATOM - ATOMFIRSTSTATEBITPOS))
      return genUlamTypeMangledUnpackedArrayAutoDefinitionForC(fp);

    m_state.m_currentIndentLevel = 0;
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());
    fp->write(" : public UlamRef<EC>");
    fp->write("\n");
    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    // see UlamClass.h for AutoRefBase
    //constructor for ref (auto)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(T& targ, u32 idx) : UlamRef<EC>(idx, ");
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u, targ, NULL) { }\n"); //effself is null for primitives

    //constructor for chain of autorefs (e.g. memberselect with array item)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("(const UlamRef<EC>& arg, u32 idx) : UlamRef<EC>(arg, idx, ");
    fp->write_decimal_unsigned(len); //includes arraysize
    fp->write("u, NULL) { }\n"); //effself is null for primitives

    //calls slow AutoRefBase read method
    genUlamTypeReadDefinitionForC(fp);

    //calls slow AutoRefBase write method
    genUlamTypeWriteDefinitionForC(fp);

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledAutoDefinitionForC

  void UlamType::genUlamTypeMangledImmediateDefinitionForC(File * fp)
  {
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  up;

    m_state.indent(fp);
    fp->write("typedef ");
    fp->write(getUlamTypeAsStringForC(isReference()).c_str()); //e.g. BitVector
    fp->write(" ");
    fp->write(mangledName.c_str());
    fp->write(";\n");
  }

  const char * UlamType::getUlamTypeEnumCodeChar(ULAMTYPE etype)
  {
    return utype_primitivecode[etype]; //static method
  }

  const char * UlamType::getUlamTypeEnumAsString(ULAMTYPE etype)
  {
    return utype_string[etype]; //static method
  }

  ULAMTYPE UlamType::getEnumFromUlamTypeString(const char * typestr)
  {
    ULAMTYPE rtnUT = Nav;
    for(u32 i = 0; i < LASTTYPE; i++)
      {
	if(strcmp(utype_string[i], typestr) == 0)
	  {
	    rtnUT = (ULAMTYPE) i;
	    break;
	  }
      }
    return rtnUT; //static method
  } //getEnumFromUlamTypeString

  ULAMCLASSTYPE UlamType::getUlamClass()
  {
    return UC_NOTACLASS;
  }

  bool UlamType::isScalar()
  {
    return (m_key.getUlamKeyTypeSignatureArraySize() == NONARRAYSIZE);
  }

  bool UlamType::isCustomArray()
  {
    return false;
  }

  s32 UlamType::getArraySize()
  {
    return m_key.getUlamKeyTypeSignatureArraySize(); //could be negative "uknown", or scalar
  }

  s32 UlamType::getBitSize()
  {
    return m_key.getUlamKeyTypeSignatureBitSize(); //could be negative "unknown"
  }

  u32 UlamType::getTotalBitSize()
  {
    s32 arraysize = getArraySize();
    arraysize = (arraysize > NONARRAYSIZE ? arraysize : 1);

    s32 bitsize = getBitSize();
    bitsize = (bitsize != UNKNOWNSIZE ? bitsize : 0);
    return bitsize * arraysize; // >= 0
  }

  ALT UlamType::getReferenceType()
  {
    return m_key.getUlamKeyTypeSignatureReferenceType();
  }

  bool UlamType::isReference()
  {
    return m_key.getUlamKeyTypeSignatureReferenceType() != ALT_NOT;
  }

  bool UlamType::isHolder()
  {
    return false;
  }

  bool UlamType::isComplete()
  {
    return !(m_key.getUlamKeyTypeSignatureBitSize() <= UNKNOWNSIZE || getArraySize() == UNKNOWNSIZE);
  }

  ULAMTYPECOMPARERESULTS UlamType::compare(UTI u1, UTI u2, CompilerState& state)  //static
  {
    assert((u1 != Nouti) && (u2 != Nouti));

    if(u1 == u2) return UTIC_SAME; //short-circuit

    if((u1 == Nav) || (u2 == Nav)) return UTIC_NOTSAME;

    if((u1 == Hzy) || (u2 == Hzy)) return UTIC_DONTKNOW;

    UlamType * ut1 = state.getUlamTypeByIndex(u1);
    UlamType * ut2 = state.getUlamTypeByIndex(u2);
    ULAMCLASSTYPE ct1 = ut1->getUlamClass();
    ULAMCLASSTYPE ct2 = ut2->getUlamClass();
    UlamKeyTypeSignature key1 = ut1->getUlamKeyTypeSignature();
    UlamKeyTypeSignature key2 = ut2->getUlamKeyTypeSignature();

    // Given Class Arguments: we may end up with different sizes given different
    // argument values which may or may not be known while parsing!
    // t.f. classes are much more like the primitive ulamtypes now.
    // Was The Case: classes with unknown bitsizes are essentially as complete
    // as they can be during parse time; and will have the same UTIs.
    if(!ut1->isComplete())
      {
	if(ct1 == UC_NOTACLASS || ut1->getArraySize() == UNKNOWNSIZE)
	  return UTIC_DONTKNOW;

	//class with known arraysize(scalar or o.w.); no more Nav ids.
	if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
	  return UTIC_DONTKNOW;
      }

    if(!ut2->isComplete())
      {
	if(ct2 == UC_NOTACLASS || ut2->getArraySize() == UNKNOWNSIZE)
	  return UTIC_DONTKNOW;

	//class with known arraysize(scalar or o.w.); no more Nav ids.
	if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
	  return UTIC_DONTKNOW;
      }

    //both complete!
    //assert both key and ptr are either both equal or not equal; not different ('!^' eq '==')
    assert((key1 == key2) == (ut1 == ut2));
    return (ut1 == ut2) ? UTIC_SAME : UTIC_NOTSAME;
  } //compare (static)

  ULAMTYPECOMPARERESULTS UlamType::compareWithWildArrayItemReferenceType(UTI u1, UTI u2, CompilerState& state)  //static
  {
    assert((u1 != Nouti) && (u2 != Nouti));

    if(u1 == u2) return UTIC_SAME; //short-circuit

    if((u1 == Nav) || (u2 == Nav)) return UTIC_NOTSAME;

    if((u1 == Hzy) || (u2 == Hzy)) return UTIC_DONTKNOW;

    UlamType * ut1 = state.getUlamTypeByIndex(u1);
    UlamType * ut2 = state.getUlamTypeByIndex(u2);
    ULAMCLASSTYPE ct1 = ut1->getUlamClass();
    ULAMCLASSTYPE ct2 = ut2->getUlamClass();
    UlamKeyTypeSignature key1 = ut1->getUlamKeyTypeSignature();
    UlamKeyTypeSignature key2 = ut2->getUlamKeyTypeSignature();

    // Given Class Arguments: we may end up with different sizes given different
    // argument values which may or may not be known while parsing!
    // t.f. classes are much more like the primitive ulamtypes now.
    // Was The Case: classes with unknown bitsizes are essentially as complete
    // as they can be during parse time; and will have the same UTIs.
    if(!ut1->isComplete())
      {
	if(ct1 == UC_NOTACLASS || ut1->getArraySize() == UNKNOWNSIZE)
	  return UTIC_DONTKNOW;

	//class with known arraysize(scalar or o.w.); no more Nav ids.
	if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
	  return UTIC_DONTKNOW;
      }

    if(!ut2->isComplete())
      {
	if(ct2 == UC_NOTACLASS || ut2->getArraySize() == UNKNOWNSIZE)
	  return UTIC_DONTKNOW;

	//class with known arraysize(scalar or o.w.); no more Nav ids.
	if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
	  return UTIC_DONTKNOW;
      }

    //both complete!
    //keys may have different reference types in the case of array items
    if(key1.getUlamKeyTypeSignatureNameId() != key2.getUlamKeyTypeSignatureNameId())
       return UTIC_NOTSAME;

    if(key1.getUlamKeyTypeSignatureArraySize() != key2.getUlamKeyTypeSignatureArraySize())
      return UTIC_NOTSAME;

    if(key1.getUlamKeyTypeSignatureBitSize() != key2.getUlamKeyTypeSignatureBitSize())
      return UTIC_NOTSAME;

    if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
      return UTIC_NOTSAME; //?

    ALT alt1 = key1.getUlamKeyTypeSignatureReferenceType();
    ALT alt2 = key2.getUlamKeyTypeSignatureReferenceType();
    if(alt1 != alt2)
      {
	if(alt1 == ALT_ARRAYITEM || alt2 == ALT_ARRAYITEM)
	  return UTIC_SAME;
	else
	  return UTIC_NOTSAME;
      }
    return UTIC_SAME;
  } //compareWithWildArrayItemReferenceType (static)

  ULAMTYPECOMPARERESULTS UlamType::compareWithWildReferenceType(UTI u1, UTI u2, CompilerState& state)  //static
  {
    assert((u1 != Nouti) && (u2 != Nouti));

    if(u1 == u2) return UTIC_SAME; //short-circuit

    if((u1 == Nav) || (u2 == Nav)) return UTIC_NOTSAME;

    if((u1 == Hzy) || (u2 == Hzy)) return UTIC_DONTKNOW;

    UlamType * ut1 = state.getUlamTypeByIndex(u1);
    UlamType * ut2 = state.getUlamTypeByIndex(u2);
    ULAMCLASSTYPE ct1 = ut1->getUlamClass();
    ULAMCLASSTYPE ct2 = ut2->getUlamClass();
    UlamKeyTypeSignature key1 = ut1->getUlamKeyTypeSignature();
    UlamKeyTypeSignature key2 = ut2->getUlamKeyTypeSignature();

    // Given Class Arguments: we may end up with different sizes given different
    // argument values which may or may not be known while parsing!
    // t.f. classes are much more like the primitive ulamtypes now.
    // Was The Case: classes with unknown bitsizes are essentially as complete
    // as they can be during parse time; and will have the same UTIs.
    if(!ut1->isComplete())
      {
	if(ct1 == UC_NOTACLASS || ut1->getArraySize() == UNKNOWNSIZE)
	  return UTIC_DONTKNOW;

	//class with known arraysize(scalar or o.w.); no more Nav ids.
	if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
	  return UTIC_DONTKNOW;
      }

    if(!ut2->isComplete())
      {
	if(ct2 == UC_NOTACLASS || ut2->getArraySize() == UNKNOWNSIZE)
	  return UTIC_DONTKNOW;

	//class with known arraysize(scalar or o.w.); no more Nav ids.
	if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
	  return UTIC_DONTKNOW;
      }

    //both complete!
    //keys may have different reference types: (e.g. atomref = atom, atom = atomref)
    if(key1.getUlamKeyTypeSignatureNameId() != key2.getUlamKeyTypeSignatureNameId())
       return UTIC_NOTSAME;

    if(key1.getUlamKeyTypeSignatureArraySize() != key2.getUlamKeyTypeSignatureArraySize())
      return UTIC_NOTSAME;

    if(key1.getUlamKeyTypeSignatureBitSize() != key2.getUlamKeyTypeSignatureBitSize())
      return UTIC_NOTSAME;

    //if(key1.getUlamKeyTypeSignatureClassInstanceIdx() != key2.getUlamKeyTypeSignatureClassInstanceIdx())
    // return UTIC_NOTSAME; //?

    ALT alt1 = key1.getUlamKeyTypeSignatureReferenceType();
    ALT alt2 = key2.getUlamKeyTypeSignatureReferenceType();
    if(alt1 != alt2)
      {
	return UTIC_SAME; //wild
      }
    return UTIC_SAME;
  } //compareWithWildReferenceType (static)

  ULAMTYPECOMPARERESULTS UlamType::compareForArgumentMatching(UTI u1, UTI u2, CompilerState& state)  //static
  {
    return UlamType::compareWithWildArrayItemReferenceType(u1, u2, state);
  }

  ULAMTYPECOMPARERESULTS UlamType::compareForMakingCastingNode(UTI u1, UTI u2, CompilerState& state)  //static
  {
    return UlamType::compareWithWildArrayItemReferenceType(u1, u2, state);
  }

  ULAMTYPECOMPARERESULTS UlamType::compareForUlamValueAssignment(UTI u1, UTI u2, CompilerState& state)  //static
  {
    return UlamType::compareWithWildReferenceType(u1, u2, state);
  }

  u32 UlamType::getTotalWordSize()
  {
    assert(isComplete());
    return m_wordLengthTotal; //e.g. 0, 32, 64, 96
  }

  u32 UlamType::getItemWordSize()
  {
    assert(isComplete());
    return m_wordLengthItem; //e.g. 0, 32, 64, 96
  }

  void UlamType::setTotalWordSize(u32 tw)
  {
    m_wordLengthTotal = tw; //e.g. 32, 64, 96
  }

  void UlamType::setItemWordSize(u32 iw)
  {
    m_wordLengthItem = iw; //e.g. 32, 64, 96
  }

  bool UlamType::isMinMaxAllowed()
  {
    return isScalar(); //minof/maxof allowed in ulam
  }

  u64 UlamType::getMax()
  {
    return m_max;
  }

  s64 UlamType::getMin()
  {
    return m_min;
  }

  u64 UlamType::getMax(UlamValue& rtnUV, UTI uti)
  {
    u32 wordsize = getTotalWordSize();
    if(wordsize <= MAXBITSPERINT)
      rtnUV = UlamValue::makeImmediate(uti, (u32) m_max, m_state);
    else if(wordsize <= MAXBITSPERLONG)
      rtnUV = UlamValue::makeImmediateLong(uti, m_max, m_state);
    return m_max;
  } //getMax (UlamValue)

  s64 UlamType::getMin(UlamValue& rtnUV, UTI uti)
  {
    u32 wordsize = getTotalWordSize();
    if(wordsize <= MAXBITSPERINT)
      rtnUV = UlamValue::makeImmediate(uti, (s32) m_min, m_state);
    else if(wordsize <= MAXBITSPERLONG)
      rtnUV = UlamValue::makeImmediateLong(uti, (s64) m_min, m_state);
    return m_min;
  } //getMin (UlamValue)

  PACKFIT UlamType::getPackable()
  {
    PACKFIT rtn = UNPACKED; //was false == 0
    u32 len = getTotalBitSize(); //could be 0, e.g. 'unknown'

    //scalars are considered packable (arraysize == NONARRAYSIZE); Atoms and Ptrs are NOT.
    if(len <= MAXBITSPERLONG)
      rtn = PACKEDLOADABLE;
    else
      if(len <= MAXSTATEBITS)
	rtn = PACKED;
    return rtn;
  } //getPackable

  const std::string UlamType::readMethodForCodeGen()
  {
    std::string method;
    u32 sizeByIntBits = getTotalWordSize();
    switch(sizeByIntBits)
      {
      case 0: //e.g. empty quarks
      case 32:
	method = "Read";
	break;
      case 64:
	method = "ReadLong";
	break;
      default:
	method = "ReadUnpacked"; //TBD
	//MSG(getNodeLocationAsString().c_str(), "Need UNPACKED ARRAY", INFO);
	assert(0);
      };
    return method;
  } //readMethodForCodeGen

  const std::string UlamType::writeMethodForCodeGen()
  {
    std::string method;
    u32 sizeByIntBits = getTotalWordSize();
    switch(sizeByIntBits)
      {
      case 0: //e.g. empty quarks
      case 32:
	method = "Write";
	break;
      case 64:
	method = "WriteLong";
	break;
      default:
	method = "WriteUnpacked"; //TBD
	//MSG(getNodeLocationAsString().c_str(), "Need UNPACKED ARRAY", INFO);
	assert(0);
      };
    return method;
  } //writeMethodForCodeGen

  const std::string UlamType::readArrayItemMethodForCodeGen()
  {
    std::string method;
    s32 sizeByIntBits = getItemWordSize();
    switch(sizeByIntBits)
      {
      case 0: //e.g. empty quarks
      case 32:
	method = "Read"; //ReadArray
	break;
      case 64:
	method = "ReadLong"; //ReadArrayLong
	break;
      default:
	method = "ReadUnpacked"; //TBD ReadArrayUnpacked
	//assert(0);
      };
    return method;
  } //readArrayItemMethodForCodeGen()

  const std::string UlamType::writeArrayItemMethodForCodeGen()
  {
    std::string method;
    s32 sizeByIntBits = getItemWordSize();
    switch(sizeByIntBits)
      {
      case 0: //e.g. empty quarks
      case 32:
	method = "Write"; //WriteArray
	break;
      case 64:
	method = "WriteLong"; //WriteArrayLong
	break;
      default:
	method = "WriteUnpacked"; //TBD WriteArrayUnpacked
      };
    return method;
  } //writeArrayItemMethodForCodeGen()

  //generates immediates with local storage
  void UlamType::genUlamTypeMangledDefinitionForC(File * fp)
  {
    u32 len = getTotalBitSize(); //could be 0, includes arrays
    if(len > (BITSPERATOM - ATOMFIRSTSTATEBITPOS))
      return genUlamTypeMangledUnpackedArrayDefinitionForC(fp); //no auto, just immediate

    m_state.m_currentIndentLevel = 0;
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    fp->write(" : public ");
    //let's see if gcc can make use of these extra template args for immediate primitives
    fp->write("UlamRefFixed");
    fp->write("<EC, ");
    fp->write_decimal_unsigned(BITSPERATOM - ATOMFIRSTSTATEBITPOS - len); //relative offset
    fp->write("u, ");
    fp->write_decimal_unsigned(len);
    fp->write("u>\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC, "); //BITSPERATOM
    fp->write_decimal_unsigned(BITSPERATOM - ATOMFIRSTSTATEBITPOS - len); //relative offset
    fp->write(", ");
    fp->write_decimal(len);
    fp->write("u> Up_Us;\n");

    //storage here (as an atom)
    m_state.indent(fp);
    fp->write("T m_stg;  //storage here!\n\n");

    //read BV method
    genUlamTypeReadDefinitionForC(fp);

    //write BV method
    genUlamTypeWriteDefinitionForC(fp);

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() : ");
    fp->write("Up_Us");
    fp->write("(m_stg, NULL), ");
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //u32
    fp->write(" d) : ");
    fp->write("Up_Us");
    fp->write("(m_stg, NULL), ");
    fp->write("m_stg(T::ATOM_UNDEFINED_TYPE) { ");
    fp->write("Up_Us::Write(d); }\n");

    //copy constructor here (return by value)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(mangledName.c_str()); //u32
    fp->write("& other) : ");
    fp->write("Up_Us");
    fp->write("(m_stg, NULL), ");
    fp->write("m_stg(other.m_stg) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledDefinitionForC

  void UlamType::genUlamTypeReadDefinitionForC(File * fp)
  {
    //scalar and entire PACKEDLOADABLE array handled by base class read method
    if(!isScalar())
      {
	//reads an element of array;
	//2nd argument generated for compatibility with underlying method
	m_state.indent(fp);
	fp->write("const ");
	fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
	fp->write(" readArrayItem(");
	fp->write("const u32 index, const u32 itemlen) const { return ");
	fp->write("UlamRef<EC>(");
	fp->write("*this, index * itemlen, "); //rel offset
	fp->write("itemlen, NULL)"); //itemlen, primitive effself
	fp->write(".Read(); }\n");
      }
  } //genUlamTypeReadDefinitionForC

  void UlamType::genUlamTypeWriteDefinitionForC(File * fp)
  {
    //scalar and entire PACKEDLOADABLE array handled by base class write method
    if(!isScalar())
      {
	// writes an element of array
	//3rd argument generated for compatibility with underlying method
	m_state.indent(fp);
	fp->write("void writeArrayItem(const ");
	fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
	fp->write(" v, const u32 index, const u32 itemlen) { ");
	fp->write("UlamRef<EC>(");
	fp->write("*this, index * itemlen, "); //rel offset
	fp->write("itemlen, NULL)"); //itemlen, primitive effself
	fp->write(".Write(v); }\n");
      }
  } //genUlamTypeWriteDefinitionForC

  void UlamType::genUlamTypeMangledUnpackedArrayAutoDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    const std::string automangledName = getUlamTypeImmediateAutoMangledName();
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << automangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 itemlen = getBitSize();
    u32 arraysize = getArraySize();

    UlamKeyTypeSignature baseKey(m_key.m_typeNameId, itemlen);
    UTI scalarUTI = m_state.makeUlamType(baseKey, getUlamTypeEnum());
    UlamType * scalarut = m_state.getUlamTypeByIndex(scalarUTI);

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC> class ");
    fp->write(mangledName.c_str());
    fp->write("; //forward \n\n");

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(automangledName.c_str());

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC, "); //BITSPERATOM
    fp->write_decimal(BITSPERATOM - ATOMFIRSTSTATEBITPOS - itemlen); //right-justified, relative
    fp->write(", ");
    fp->write_decimal(itemlen);
    fp->write("u> Up_Us;\n"); //per item

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("T (& m_stgarrayref)[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u];  //ref to BIG storage!\n\n");

    //copy constructor here
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("( ");
    fp->write(automangledName.c_str());
    fp->write("<EC>& r) : m_stgarrayref(r.m_stgarrayref) { }\n");

    //constructor here (used by tmpVars)
    m_state.indent(fp);
    fp->write(automangledName.c_str());
    fp->write("( ");
    fp->write(mangledName.c_str());
    fp->write("<EC>& d) : m_stgarrayref(d.getStorageRef()) { }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(automangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) { return ");
    fp->write("Up_Us(getRef(index), NULL).");
    fp->write(scalarut->readMethodForCodeGen().c_str());
    fp->write("(); }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 itemlen) { ");
    fp->write("Up_Us(getRef(index), NULL).");
    fp->write(scalarut->writeMethodForCodeGen().c_str());
    fp->write("(v);");
    fp->write(" }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write(" getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write(" getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarrayref[index].GetBits(); }\n");

    //Unpacked, an item T&
    m_state.indent(fp);
    fp->write("T& ");
    fp->write(" getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarrayref[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write(" getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index + BPA - T::ATOM_FIRST_STATE_BIT - ");
    fp->write_decimal_unsigned(itemlen); //right-justified, relative per item
    fp->write("u); }\n");

    //Unpacked, position within each item T
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write(" getPosOffset(");
    fp->write(" ) const { return ");
    fp->write("(BPA - T::ATOM_FIRST_STATE_BIT - ");
    fp->write_decimal_unsigned(itemlen); //right-justified, relative per item
    fp->write("u); }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedArrayAutoDefinitionForC

  void UlamType::genUlamTypeMangledUnpackedArrayDefinitionForC(File * fp)
  {
    m_state.m_currentIndentLevel = 0;
    const std::string mangledName = getUlamTypeImmediateMangledName();
    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    u32 itemlen = getBitSize();
    u32 arraysize = getArraySize();

    UlamKeyTypeSignature baseKey(m_key.m_typeNameId, itemlen);
    UTI scalarUTI = m_state.makeUlamType(baseKey, getUlamTypeEnum());
    UlamType * scalarut = m_state.getUlamTypeByIndex(scalarUTI);

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");

    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC, "); //BITSPERATOM
    fp->write_decimal(BITSPERATOM - ATOMFIRSTSTATEBITPOS - itemlen); //right-justified, relative
    fp->write(", ");
    fp->write_decimal(itemlen);
    fp->write("u> Up_Us;\n"); //one item

    //Unpacked, storage reference T (&) [N]
    m_state.indent(fp);
    fp->write("typedef T TARR[");
    fp->write_decimal_unsigned(arraysize);
    fp->write("];\n");

    //storage here (an array of T's)
    m_state.indent(fp);
    fp->write("TARR m_stgarr;  //BIG storage here!\n\n");

    //default constructor (used by local vars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() {");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) ");
    fp->write("m_stgarr[j].SetUndefinedImpl();"); //T::ATOM_UNDEFINED_TYPE
    fp->write(" }\n");

    //constructor here (used by const tmpVars)
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("(const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //u32
    fp->write(" d) { ");
    fp->write("for(u32 j = 0; j < ");
    fp->write_decimal_unsigned(arraysize);
    fp->write("u; j++) {");
    fp->write("m_stgarr[j].SetUndefinedImpl(); "); //T::ATOM_UNDEFINED_TYPE
    fp->write("writeArrayItem(d, j, ");
    fp->write_decimal_unsigned(itemlen); //right-justified per item
    fp->write("u); } }\n");

    //default destructor (for completeness)
    m_state.indent(fp);
    fp->write("~");
    fp->write(mangledName.c_str());
    fp->write("() {}\n");

    //Unpacked Read Array Item
    m_state.indent(fp);
    fp->write("const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" readArrayItem(");
    fp->write("const u32 index, const u32 itemlen) { return ");
    fp->write("Up_Us(getRef(index), NULL).");
    fp->write(scalarut->readMethodForCodeGen().c_str());
    fp->write("(); }\n");

    //Unpacked Write Array Item
    m_state.indent(fp);
    fp->write("void writeArrayItem(const ");
    fp->write(getArrayItemTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" v, const u32 index, const u32 itemlen) { ");
    fp->write("Up_Us(getRef(index), NULL).");
    fp->write(scalarut->writeMethodForCodeGen().c_str());
    fp->write("(v);");
    fp->write(" }\n");

    //Unpacked, an item T
    m_state.indent(fp);
    fp->write("BitVector<BPA>& ");
    fp->write(" getBits(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T const
    m_state.indent(fp);
    fp->write("const BitVector<BPA>& ");
    fp->write(" getBits(");
    fp->write("const u32 index) const { return ");
    fp->write("m_stgarr[index].GetBits(); }\n");

    //Unpacked, an item T&
    m_state.indent(fp);
    fp->write("T& ");
    fp->write(" getRef(");
    fp->write("const u32 index) { return ");
    fp->write("m_stgarr[index]; }\n");

    //Unpacked, position within whole
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write(" getPosOffset(");
    fp->write("const u32 index) const { return ");
    fp->write("(BPA * index + BPA - T::ATOM_FIRST_STATE_BIT - ");
    fp->write_decimal_unsigned(itemlen); //right-justified, relative per item
    fp->write("u); }\n");

    //Unpacked, position within each item T
    m_state.indent(fp);
    fp->write("const u32 ");
    fp->write(" getPosOffset(");
    fp->write(" ) const { return ");
    fp->write("(BPA - T::ATOM_FIRST_STATE_BIT - ");
    fp->write_decimal_unsigned(itemlen); //right-justified, relative per item
    fp->write("u); }\n");

    m_state.indent(fp);
    fp->write("TARR& getStorageRef(");
    fp->write(") { return ");
    fp->write("m_stgarr; }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledUnpackedArrayDefinitionForC

  void UlamType::genUlamTypeMangledImmediateModelParameterDefinitionForC(File * fp)
  {
    assert(isScalar());

    const std::string mangledName = getImmediateModelParameterStorageTypeAsString();

    std::ostringstream  ud;
    ud << "Ud_" << mangledName; //d for define (p used for atomicparametrictype)
    std::string udstr = ud.str();

    m_state.m_currentIndentLevel = 0;

    m_state.indent(fp);
    fp->write("#ifndef ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("#define ");
    fp->write(udstr.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("namespace MFM{\n");
    fp->write("\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("template<class EC>\n");
    m_state.indent(fp);
    fp->write("struct ");
    fp->write(mangledName.c_str());
    fp->write("\n");

    m_state.indent(fp);
    fp->write("{\n");

    m_state.m_currentIndentLevel++;

    m_state.indent(fp);
    fp->write("// immediate model parameter definition:\n");

    //typedef atomic parameter type inside struct
    m_state.indent(fp);
    fp->write("typedef typename EC::ATOM_CONFIG AC;\n");
    m_state.indent(fp);
    fp->write("typedef typename AC::ATOM_TYPE T;\n");
    m_state.indent(fp);
    fp->write("enum { BPA = AC::BITS_PER_ATOM };\n");
    fp->write("\n");

    s32 len = getTotalBitSize();

    m_state.indent(fp);
    fp->write("typedef UlamRefFixed");
    fp->write("<EC, "); //BITSPERATOM
    fp->write_decimal(BITSPERATOM - ATOMFIRSTSTATEBITPOS - len); //right-justified, relative
    fp->write(", ");
    fp->write_decimal(len);
    fp->write("u > Up_Us;\n");

    //reference to storage in atom
    m_state.indent(fp);
    fp->write("T* m_stgPtr;  //ptr to storage here!\n");

    // constructor with args
    m_state.indent(fp);
    fp->write(mangledName.c_str());
    fp->write("() : m_stgPtr(NULL) { }\n");

    m_state.indent(fp);
    fp->write("const ");
    fp->write(getTmpStorageTypeAsString().c_str()); //s32 or u32
    fp->write(" read() const { MFM_API_ASSERT_NONNULL(m_stgPtr); return Up_Us(*m_stgPtr, NULL).");
    fp->write(readMethodForCodeGen().c_str());
    fp->write("(); }\n");

    m_state.indent(fp);
    fp->write("void init(T& realStg) { m_stgPtr = &realStg; }\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("};\n");

    m_state.m_currentIndentLevel--;
    m_state.indent(fp);
    fp->write("} //MFM\n");

    m_state.indent(fp);
    fp->write("#endif /*");
    fp->write(udstr.c_str());
    fp->write(" */\n\n");
  } //genUlamTypeMangledImmediateModelParameterDefinitionForC

  bool UlamType::genUlamTypeDefaultQuarkConstant(File * fp, u32& dqref)
  {
    return false; //only true for quarks in UlamTypeClass
  }

} //end MFM
