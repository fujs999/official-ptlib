/*
 * contain.cxx
 *
 * Container Classes
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#include <ptlib.h>
#include <ctype.h>

#include <ostream>
#include <limits>
#include <math.h>

#ifdef __NUCLEUS_PLUS__
extern "C" int vsprintf(char *, const char *, va_list);
#endif

#if P_REGEX
  #include <regex.h>
#else
  #include "regex/regex.h"
#endif

#if P_HAS_ICONV
  #include <iconv.h>
#endif // P_HAS_ICONV


#if !P_USE_INLINES
#include "ptlib/contain.inl"
#endif


PDEFINE_POOL_ALLOCATOR(PContainerReference);


#define new PNEW
#undef  __CLASS__
#define __CLASS__ GetClass()


///////////////////////////////////////////////////////////////////////////////

PContainer::PContainer(PINDEX initialSize)
{
  reference = new PContainerReference(initialSize);
  PAssert(reference != NULL, POutOfMemory);
}


PContainer::PContainer(int, const PContainer * cont)
{
  if (cont == this)
    return;

  PAssert(cont != NULL, PInvalidParameter);
  PAssert2(cont->reference != NULL, cont->GetClass(), "Clone of deleted container");

  reference = new PContainerReference(*cont->reference);
  PAssert(reference != NULL, POutOfMemory);
}


PContainer::PContainer(const PContainer & cont)
{
  if (&cont == this)
    return;

  PAssert2(cont.reference != NULL, cont.GetClass(), "Copy of deleted container");

  ++cont.reference->count;
  reference = cont.reference;  // copy the reference pointer
}


PContainer::PContainer(PContainerReference & ref)
  : reference(&ref)
{
}


void PContainer::AssignContents(const PContainer & cont)
{
  if(cont.reference == NULL){
    PAssertAlways("container reference is null");
    return;
  }
  if(cont.GetClass() == NULL){
    PAssertAlways("container class is null");
    return;
  }

  if (reference == cont.reference)
    return;

  if (--reference->count == 0) {
    DestroyContents();
    DestroyReference();
  }

  PAssert(++cont.reference->count > 1, "Assignment of container that was deleted");
  reference = cont.reference;
}


void PContainer::Destruct()
{
  if (reference != NULL) {
    if (--reference->count <= 0) {
      DestroyContents();
      DestroyReference();
    }
    reference = NULL;
  }
}


void PContainer::DestroyReference()
{
  delete reference;
  reference = NULL;
}


PBoolean PContainer::SetMinSize(PINDEX minSize)
{
#if PINDEX_SIGNED
  PASSERTINDEX(minSize);
  if (minSize < 0)
    minSize = 0;
#endif
  if (minSize < GetSize())
    minSize = GetSize();
  return SetSize(minSize);
}


PBoolean PContainer::MakeUnique()
{
  if (IsUnique())
    return true;

  PContainerReference * oldReference = reference;
  reference = new PContainerReference(*reference);
  --oldReference->count;

  return false;
}


///////////////////////////////////////////////////////////////////////////////

#if PMEMORY_CHECK

  #define PAbstractArrayAllocate(s)     (char *)PMemoryHeap::Allocate(s, __FILE__, __LINE__, "PAbstractArrayData")
  #define PAbstractArrayReallocate(p,s)         PMemoryHeap::Reallocate(p, s, __FILE__, __LINE__, "PAbstractArrayData")
  #define PAbstractArrayDeallocate(p)           PMemoryHeap::Deallocate(p, "PAbstractArrayData")

#else // PMEMORY_CHECK

  #define PAbstractArrayAllocate(s)     (char *)malloc(s)
  #define PAbstractArrayReallocate(p,s)         realloc(p, s)
  #define PAbstractArrayDeallocate(p)           free(p)

#endif // PMEMORY_CHECK


PAbstractArray::PAbstractArray(PINDEX elementSizeInBytes, PINDEX initialSize)
  : PContainer(initialSize)
{
  elementSize = elementSizeInBytes;
  PAssert(elementSize != 0, PInvalidParameter);

  if (GetSize() == 0)
    theArray = NULL;
  else {
    theArray = PAbstractArrayAllocate(GetSize() * elementSize);
    PAssert(theArray != NULL, POutOfMemory);
    memset(theArray, 0, GetSize() * elementSize);
  }

  allocatedDynamically = true;
}


PAbstractArray::PAbstractArray(PINDEX elementSizeInBytes,
                               const void *buffer,
                               PINDEX bufferSizeInElements,
                               PBoolean dynamicAllocation)
  : PContainer(bufferSizeInElements)
{
  elementSize = elementSizeInBytes;
  PAssert(elementSize != 0, PInvalidParameter);

  allocatedDynamically = dynamicAllocation;

  if (GetSize() == 0)
    theArray = NULL;
  else if (dynamicAllocation) {
    PINDEX sizebytes = elementSize*GetSize();
    theArray = PAbstractArrayAllocate(sizebytes);
    PAssert(theArray != NULL, POutOfMemory);
    memcpy(theArray, PAssertNULL(buffer), sizebytes);
  }
  else
    theArray = (char *)buffer;
}


PAbstractArray::PAbstractArray(PContainerReference & reference, PINDEX elementSizeInBytes)
  : PContainer(reference)
  , elementSize(elementSizeInBytes)
  , theArray(NULL)
  , allocatedDynamically(false)
{
}


void PAbstractArray::DestroyContents()
{
  if (theArray != NULL) {
    if (allocatedDynamically)
      PAbstractArrayDeallocate(theArray);
    theArray = NULL;
  }
}


void PAbstractArray::CopyContents(const PAbstractArray & array)
{
  elementSize = array.elementSize;
  theArray = array.theArray;
  allocatedDynamically = array.allocatedDynamically;

  if (reference->constObject)
    MakeUnique();
}


void PAbstractArray::CloneContents(const PAbstractArray * array)
{
  elementSize = array->elementSize;
  PINDEX sizebytes = elementSize*GetSize();
  char * newArray = PAbstractArrayAllocate(sizebytes);
  if (newArray == NULL)
    reference->size = 0;
  else
    memcpy(newArray, array->theArray, sizebytes);
  theArray = newArray;
  allocatedDynamically = true;
}


void PAbstractArray::PrintOn(ostream & strm) const
{
  char separator = strm.fill();
  int width = (int)strm.width();
  for (PINDEX  i = 0; i < GetSize(); i++) {
    if (i > 0 && separator != '\0' && !isdigit(separator))
      strm << separator;
    strm.width(width);
    PrintElementOn(strm, i);
  }
  if (separator == '\n')
    strm << '\n';
}


void PAbstractArray::ReadFrom(istream & strm)
{
  PINDEX i = 0;
  while (strm.good()) {
    ReadElementFrom(strm, i);
    if (!strm.fail())
      i++;
  }
  SetSize(i);
}


PObject::Comparison PAbstractArray::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PAbstractArray), PInvalidCast);
  const PAbstractArray & other = (const PAbstractArray &)obj;

  char * otherArray = other.theArray;
  if (theArray == otherArray)
    return EqualTo;

  if (elementSize < other.elementSize)
    return LessThan;

  if (elementSize > other.elementSize)
    return GreaterThan;

  PINDEX thisSize = GetSize();
  PINDEX otherSize = other.GetSize();

  if (thisSize < otherSize)
    return LessThan;

  if (thisSize > otherSize)
    return GreaterThan;

  if (thisSize == 0)
    return EqualTo;

  int retval = memcmp(theArray, otherArray, elementSize*thisSize);
  if (retval < 0)
    return LessThan;
  if (retval > 0)
    return GreaterThan;
  return EqualTo;
}


PBoolean PAbstractArray::SetSize(PINDEX newSize)
{
  return InternalSetSize(newSize, false);
}


PBoolean PAbstractArray::InternalSetSize(PINDEX newSize, PBoolean force)
{
#if PINDEX_SIGNED
  if (newSize < 0)
    return false;
#endif

  PINDEX newsizebytes = elementSize*newSize;
  PINDEX oldsizebytes = elementSize*GetSize();

  if (!force && (newsizebytes == oldsizebytes))
    return true;

  char * newArray;

  if (!IsUnique()) {

    if (newsizebytes == 0)
      newArray = NULL;
    else {
      if ((newArray = PAbstractArrayAllocate(newsizebytes)) == NULL)
        return false;
  
      allocatedDynamically = true;

      if (theArray != NULL)
        memcpy(newArray, theArray, PMIN(oldsizebytes, newsizebytes));
    }

    --reference->count;
    reference = new PContainerReference(newSize);

  } else {

    if (theArray != NULL) {
      if (newsizebytes == 0) {
        if (allocatedDynamically)
          PAbstractArrayDeallocate(theArray);
        newArray = NULL;
      }
      else {
        if ((newArray = PAbstractArrayAllocate(newsizebytes)) == NULL)
          return false;
        memcpy(newArray, theArray, PMIN(newsizebytes, oldsizebytes));
        if (allocatedDynamically)
          PAbstractArrayDeallocate(theArray);
        allocatedDynamically = true;
      }
    }
    else if (newsizebytes != 0) {
      if ((newArray = PAbstractArrayAllocate(newsizebytes)) == NULL)
        return false;
    }
    else
      newArray = NULL;

    reference->size = newSize;
  }

  if (newsizebytes > oldsizebytes)
    memset(newArray+oldsizebytes, 0, newsizebytes-oldsizebytes);

  theArray = newArray;
  return true;
}

void PAbstractArray::Attach(const void *buffer, PINDEX bufferSize)
{
  if (allocatedDynamically && theArray != NULL)
    PAbstractArrayDeallocate(theArray);

  theArray = (char *)buffer;
  reference->size = bufferSize;
  allocatedDynamically = false;
}


void * PAbstractArray::GetPointer(PINDEX minSize)
{
  PAssert(SetMinSize(minSize), POutOfMemory);
  return theArray;
}


PINDEX PAbstractArray::GetLength() const
{
  return elementSize*GetSize();
}


PBoolean PAbstractArray::Concatenate(const PAbstractArray & array)
{
  if (!allocatedDynamically || array.elementSize != elementSize)
    return false;

  PINDEX oldLen = GetSize();
  PINDEX addLen = array.GetSize();

  if (!SetSize(oldLen + addLen))
    return false;

  memcpy(theArray+oldLen*elementSize, array.theArray, addLen*elementSize);
  return true;
}


void PAbstractArray::PrintElementOn(ostream & /*stream*/, PINDEX /*index*/) const
{
}


void PAbstractArray::ReadElementFrom(istream & /*stream*/, PINDEX /*index*/)
{
}


///////////////////////////////////////////////////////////////////////////////

void PCharArray::PrintOn(ostream & strm) const
{
  PINDEX width = (int)strm.width();
  if (width > GetSize())
    width -= GetSize();
  else
    width = 0;

  PBoolean left = (strm.flags()&ios::adjustfield) == ios::left;
  if (left)
    strm.write(theArray, GetSize());

  while (width-- > 0)
    strm << (char)strm.fill();

  if (!left)
    strm.write(theArray, GetSize());
}


void PCharArray::ReadFrom(istream &strm)
{
  PINDEX size = 0;
  SetSize(size+100);

  while (strm.good()) {
    strm >> theArray[size++];
    if (size >= GetSize())
      SetSize(size+100);
  }

  SetSize(size);
}


void PBYTEArray::PrintOn(ostream & strm) const
{
  int line_width = (int)strm.width();
  if (line_width == 0)
    line_width = 16;
  strm.width(0);

  int indent = (int)strm.precision();

  int val_width = ((strm.flags()&ios::basefield) == ios::hex) ? 2 : 3;

  ios::fmtflags oldFlags = strm.flags();
  if (strm.fill() == '0')
    strm.setf(ios::right, ios::adjustfield);

  PINDEX i = 0;
  while (i < GetSize()) {
    if (i > 0)
      strm << '\n';
    int j;
    for (j = 0; j < indent; j++)
      strm << ' ';
    for (j = 0; j < line_width; j++) {
      if (indent >= 0 && j == line_width/2)
        strm << ' ';
      if (i+j < GetSize())
        strm << setw(val_width) << (theArray[i+j]&0xff);
      else {
        for (int k = 0; k < val_width; k++)
          strm << ' ';
      }
      if (indent >= 0)
        strm << ' ';
    }
    if ((strm.flags()&ios::floatfield) != ios::fixed) {
      strm << "  ";
      for (j = 0; j < line_width; j++) {
        if (i+j < GetSize()) {
          unsigned val = theArray[i+j]&0xff;
          if (isprint(val))
            strm << (char)val;
          else
            strm << '.';
        }
      }
    }
    i += line_width;
  }

  strm.flags(oldFlags);
}


void PBYTEArray::ReadFrom(istream &strm)
{
  PINDEX size = 0;
  SetSize(size+100);

  while (strm.good()) {
    unsigned v;
    strm >> v;
    theArray[size] = (BYTE)v;
    if (!strm.fail()) {
      size++;
      if (size >= GetSize())
        SetSize(size+100);
    }
  }

  SetSize(size);
}


void PHexDump::PrintOn(ostream & strm) const
{
  char oldFill = strm.fill('0');
  std::streamsize oldPrec = strm.precision();
  ios::fmtflags oldFlags = strm.setf(ios::hex, ios::basefield);

  if (m_compact) {
    strm.precision(0);
    strm.setf(ios::fixed, ios::floatfield);
    strm.width(GetSize());
  }

  PBYTEArray::PrintOn(strm);

  strm.flags(oldFlags);
  strm.precision(oldPrec);
  strm.fill(oldFill);
}


///////////////////////////////////////////////////////////////////////////////

PBitArray::PBitArray(PINDEX initialSize)
  : PBYTEArray((initialSize+7)>>3)
{
}


PBitArray::PBitArray(const void * buffer,
                     PINDEX length,
                     PBoolean dynamic)
  : PBYTEArray((const BYTE *)buffer, (length+7)>>3, dynamic)
{
}


PObject * PBitArray::Clone() const
{
  return new PBitArray(*this);
}


PINDEX PBitArray::GetSize() const
{
  return PBYTEArray::GetSize()<<3;
}


PBoolean PBitArray::SetSize(PINDEX newSize)
{
  return PBYTEArray::SetSize((newSize+7)>>3);
}


PBoolean PBitArray::SetAt(PINDEX index, PBoolean val)
{
  if (!SetMinSize(index+1))
    return false;

  if (val)
    theArray[index>>3] |= (1 << (index&7));
  else
    theArray[index>>3] &= ~(1 << (index&7));
  return true;
}


PBoolean PBitArray::GetAt(PINDEX index) const
{
  PASSERTINDEX(index);
  if (index >= GetSize())
    return false;

  return (theArray[index>>3]&(1 << (index&7))) != 0;
}


void PBitArray::Attach(const void * buffer, PINDEX bufferSize)
{
  PBYTEArray::Attach((const BYTE *)buffer, (bufferSize+7)>>3);
}


BYTE * PBitArray::GetPointer(PINDEX minSize)
{
  return PBYTEArray::GetPointer((minSize+7)>>3);
}


PBoolean PBitArray::Concatenate(const PBitArray & array)
{
  return PAbstractArray::Concatenate(array);
}


///////////////////////////////////////////////////////////////////////////////

PString::PString()
{
}


PString::PString(const PString & str)
  : std::string(str)
{
}


PString::PString(const PCharArray & buf)
  : std::string(buf)
{
}


PString::PString(const PBYTEArray & buf)
{
  PINDEX bufSize = buf.GetSize();
  if (bufSize > 0) {
    if (buf[bufSize-1] == '\0')
      --bufSize;
    memcpy(GetPointerAndSetLength(bufSize), buf, bufSize);
  }
}


PString::PString(const std::string & str)
  : std::string(str)
{
}


PString::PString(char c)
  : std::string(1, c)
{
}


const PString & PString::Empty()
{
  static int EmptyStringMemory[(sizeof(PConstString)+sizeof(int)-1)/sizeof(int)];
#undef new
  static PConstString const * EmptyString = new (EmptyStringMemory) PConstString("");
#define new PNEW
  return *EmptyString;
}


PString::PString(const char * cstr)
  : std::string(cstr != NULL ? cstr : "")
{
}


PString::PString(const wchar_t * wstr)
{
  if (wstr == NULL)
    MakeEmpty();
  else
    InternalFromWChar(wstr, wcslen(wstr));
}

PString::PString(const wchar_t * wstr, PINDEX len)
{
  InternalFromWChar(wstr, len);
}


PString::PString(const PWCharArray & wstr)
{
  PINDEX size = wstr.GetSize();
  if (size > 0 && wstr[size-1] == 0) // Stip off trailing NULL if present
    size--;
  InternalFromWChar(wstr, size);
}


PString::PString(const char * cstr, PINDEX len)
  : std::string(cstr, len)
{
}


static int TranslateHex(char x)
{
  if (x >= 'a')
    return x - 'a' + 10;

  if (x >= 'A')
    return x - 'A' + '\x0a';

  return x - '0';
}


static const unsigned char PStringEscapeCode[]  = {  'a',  'b',  'f',  'n',  'r',  't',  'v' };
static const unsigned char PStringEscapeValue[] = { '\a', '\b', '\f', '\n', '\r', '\t', '\v' };

static void TranslateEscapes(const char * & src, char * dst)
{
  bool hadLeadingQuote = *src == '"';
  if (hadLeadingQuote)
    src++;

  while (*src != '\0') {
    int c = *src++ & 0xff;
    if (c == '"' && hadLeadingQuote) {
      *dst = '\0'; // Trailing '"' and remaining string is ignored
      break;
    }

    if (c == '\\') {
      c = *src++ & 0xff;
      for (PINDEX i = 0; i < PARRAYSIZE(PStringEscapeCode); i++) {
        if (c == PStringEscapeCode[i])
          c = PStringEscapeValue[i];
      }

      if (c == 'x' && isxdigit(*src & 0xff)) {
        c = TranslateHex(*src++);
        if (isxdigit(*src & 0xff))
          c = (c << 4) + TranslateHex(*src++);
      }
      else if (c >= '0' && c <= '7') {
        int count = c <= '3' ? 3 : 2;
        src--;
        c = 0;
        do {
          c = (c << 3) + *src++ - '0';
        } while (--count > 0 && *src >= '0' && *src <= '7');
      }
    }

    *dst++ = (char)c;
  }
}


PString::PString(ConversionType type, const char * str, ...)
{
  switch (type) {
    case Pascal :
      if (*str != '\0') {
        PINDEX length = *str & 0xff;
        memcpy(GetPointerAndSetLength(length), str+1, length);
      }
      break;

    case Basic :
      if (str[0] != '\0' && str[1] != '\0') {
        PINDEX length = (str[0] & 0xff) | ((str[1] & 0xff) << 8);
        memcpy(GetPointerAndSetLength(length), str+2, length);
      }
      break;

    case Literal :
      reserve(strlen(str)+1);
      TranslateEscapes(str, const_cast<char *>(data()));
      MakeMinimumSize();
      break;

    case Printf : {
      va_list args;
      va_start(args, str);
      vsprintf(str, args);
      va_end(args);
      break;
    }

    default :
      PAssertAlways(PInvalidParameter);
  }
}


template <typename T> void p_unsigned2string(T value, unsigned base, std::string & str)
{
  if (value >= base)
    p_unsigned2string<T>((T)(value/base), base, str);
  value %= base;
  str += (char)(value < 10 ? (value + '0') : (value + 'A'-10));
}


template <typename S, typename U> void p_signed2string(S value, unsigned base, std::string & str)
{
  if (value >= 0)
    p_unsigned2string<U>(value, base, str);
  else {
    str += '-';
    p_unsigned2string<U>(-value, base, str);
  }
}


PString::PString(short n)
{
  p_signed2string<signed int, unsigned>(n, 10, *this);
}


PString::PString(unsigned short n)
{
  p_unsigned2string<unsigned int>(n, 10, *this);
}


PString::PString(int n)
{
  p_signed2string<signed int, unsigned>(n, 10, *this);
}


PString::PString(unsigned int n)
{
  p_unsigned2string<unsigned int>(n, 10, *this);
}


PString::PString(long n)
{
  p_signed2string<signed long, unsigned long>(n, 10, *this);
}


PString::PString(unsigned long n)
{
  p_unsigned2string<unsigned long>(n, 10, *this);
}


#ifdef HAVE_LONG_LONG_INT
PString::PString(long long n)
{
  p_signed2string<signed long long, unsigned long long>(n, 10, *this);
}
#endif


#ifdef HAVE_UNSIGNED_LONG_LONG_INT
PString::PString(unsigned long long n)
{
  p_unsigned2string<unsigned long long>(n, 10, *this);
}
#endif


static const char siTable[][2] = { "f", "p", "n", "u", "m", "", "k", "M", "G", "T", "E" };
static const size_t siCount = sizeof(siTable)/2;
static const size_t siZero = siCount/2;

static void InternalConvertScaleSI(PInt64 value, unsigned precision, std::string & str)
{
  // Scale it according to SI multipliers
  if (value > -1000 && value < 1000)
    return p_signed2string<PInt64, PUInt64>(value, 10, str);

  if (precision > 21)
    precision = 21;

  PInt64 absValue = value;
  if (absValue < 0) {
    absValue = -absValue;
    ++precision;
  }

  PINDEX length = 0;
  PInt64 multiplier = 1;
  for (size_t i = siZero+1; i < siCount; ++i) {
    multiplier *= 1000;
    if (absValue < multiplier*1000) {
      p_signed2string<PInt64, PUInt64>(value/multiplier, 10, str);
      precision -= length;
      if (precision > 0 && absValue%multiplier != 0) {
        str += '.';
        do {
          multiplier /= 10;
          str += (absValue/multiplier)%10 + '0';
        } while (--precision > 0 && absValue%multiplier != 0);
      }
      str += siTable[i][0];
      break;
    }
  }
}

template <typename S, typename U>
  void p_convert(PString::ConversionType type, S value, unsigned param, std::string & str)
{
#define GetClass() NULL
  switch (type) {
    case PString::Signed :
      PAssert(param != 1 && param <= 36, PInvalidParameter);
      return p_signed2string<S, U>(value, param, str);

    case PString::Unsigned :
      PAssert(param != 1 && param <= 36, PInvalidParameter);
      return p_unsigned2string<U>(value, param, str);

    case PString::ScaleSI :
      return InternalConvertScaleSI(value, param, str);

    default :
      break;
  }

  PAssertAlways(PInvalidParameter);
#undef GetClass
}

#define PSTRING_CONV_CTOR(paramType, signedType, unsignedType) \
PString::PString(ConversionType type, paramType value, unsigned param) \
{ \
  p_convert<signedType, unsignedType>(type, value, param, *this); \
}

PSTRING_CONV_CTOR(unsigned char,  char,   unsigned char);
PSTRING_CONV_CTOR(         short, short,  unsigned short);
PSTRING_CONV_CTOR(unsigned short, short,  unsigned short);
PSTRING_CONV_CTOR(         int,   int,    unsigned int);
PSTRING_CONV_CTOR(unsigned int,   int,    unsigned int);
PSTRING_CONV_CTOR(         long,  long,   unsigned long);
PSTRING_CONV_CTOR(unsigned long,  long,   unsigned long);
#ifdef HAVE_LONG_LONG_INT
PSTRING_CONV_CTOR(         long long, long long, unsigned long long);
#endif
#ifdef HAVE_UNSIGNED_LONG_LONG_INT
PSTRING_CONV_CTOR(unsigned long long, long long, unsigned long long);
#endif


PString::PString(ConversionType type, double value, unsigned places)
{
  switch (type) {
    case Decimal :
      sprintf("%0.*f", (int)places, value);
      break;

    case Exponent :
      sprintf("%0.*e", (int)places, value);
      break;

    case ScaleSI :
      if (value == 0)
        sprintf("%0.*f", (int)places, value);
      else {
        // Scale it according to SI multipliers
        double multiplier = 1e-15;
        double absValue = fabs(value);
        size_t i;
        for (i = 0; i < siCount-1; ++i) {
          double nextMultiplier = multiplier * 1000;
          if (absValue < nextMultiplier)
            break;
          multiplier = nextMultiplier;
        }
        value /= multiplier;
        // Want places to be significant figures
        if (places >= 3 && value >= 100)
          places -= 3;
        else if (places >= 2 && value >= 10)
          places -= 2;
        else if (places >= 1)
          --places;
        sprintf("%0.*f%s", (int)places, value, siTable[i]);
      }
      break;

    default :
      PAssertAlways(PInvalidParameter);
      MakeEmpty();
  }
}


PString & PString::operator=(short n)
{
  clear();
  p_signed2string<signed int, unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(unsigned short n)
{
  clear();
  p_unsigned2string<unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(int n)
{
  clear();
  p_signed2string<signed int, unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(unsigned int n)
{
  clear();
  p_unsigned2string<unsigned int>(n, 10, *this);
  return *this;
}


PString & PString::operator=(long n)
{
  clear();
  p_signed2string<signed long,  unsigned long>(n, 10, *this);
  return *this;
}


PString & PString::operator=(unsigned long n)
{
  clear();
  p_unsigned2string<unsigned long>(n, 10, *this);
  return *this;
}


#ifdef HAVE_LONG_LONG_INT
PString & PString::operator=(long long n)
{
  clear();
  p_signed2string<signed long long, unsigned long long>(n, 10, *this);
  return *this;
}
#endif


#ifdef HAVE_UNSIGNED_LONG_LONG_INT
PString & PString::operator=(unsigned long long n)
{
  clear();
  p_unsigned2string<unsigned long long>(n, 10, *this);
  return *this;
}
#endif


PString & PString::MakeEmpty()
{
  clear();
  return *this;
}


PObject * PString::Clone() const
{
  return new PString(*this);
}


void PString::PrintOn(ostream &strm) const
{
  strm << c_str();
}


void PString::ReadFrom(istream &strm)
{
  std::getline(strm, *this);
}


PObject::Comparison PString::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PString), PInvalidCast);
  return InternalCompare(0, P_MAX_INDEX, ((const PString &)obj).c_str());
}


PINDEX PString::HashFunction() const
{
  /* Hash function from "Data Structures and Algorithm Analysis in C++" by
     Mark Allen Weiss, with limit of only executing over at most the first and
     last characters to increase speed when dealing with large strings. */

  PINDEX length = GetLength(); // Use virtual function so PStringStream recalculates length
  switch (length) {
    case 0:
      return 0;
    case 1:
      return tolower(at(0) & 0xff) % 127;
  }

  static const PINDEX MaxCount = 18; // Make sure big enough to cover whole PGloballyUniqueID::AsString()
  PINDEX count = std::min(length / 2, MaxCount);
  unsigned hash = 0;
  PINDEX i;
  for (i = 0; i < count; i++)
    hash = (hash << 5) ^ tolower(at(i) & 0xff) ^ hash;
  for (i = length - count - 1; i < length; i++)
    hash = (hash << 5) ^ tolower(at(i) & 0xff) ^ hash;
  return hash % 127;
}


PString PString::operator+(const char * cstr) const
{
  if (!cstr)
    return *this;

  PString str = *this;
  str += cstr;
  return str;
}


PString PString::operator+(char c) const
{
  PString str = *this;
  str += c;
  return str;
}


PString & PString::operator+=(const char * cstr)
{
  if (!cstr)
    return *this;

  std::string::operator+=(cstr);
  return *this;
}


PString & PString::operator+=(char ch)
{
  std::string::operator+=(ch);
  return *this;
}


PString PString::operator&(const char * cstr) const
{
  PString str;
  str &= cstr;
  return str;
}


PString PString::operator&(char c) const
{
  PString str;
  str &= c;
  return str;
}


PString & PString::operator&=(const char * cstr)
{
  if (cstr == NULL)
    return *this;

  PINDEX alen = strlen(cstr);
  if (alen == 0)
    return *this;

  PINDEX olen = GetLength();
  if (olen > 0 && at(olen-1) != ' ' && *cstr != ' ')
    std::string::operator+=(' ');
  std::string::operator+=(cstr);
  return *this;
}


PString & PString::operator&=(char ch)
{
  PINDEX olen = GetLength();
  if (olen > 0 && at(olen-1) != ' ' && ch != ' ')
    std::string::operator+=(' ');
  std::string::operator+=(ch);
  return *this;
}


PString & PString::Delete(PINDEX start, PINDEX len)
{
  std::string::erase(start, len);
  return *this;
}


PString PString::operator()(PINDEX start, PINDEX end) const
{
#if PINDEX_SIGNED
  if (end < 0 || start < 0)
    return Empty();
#endif

  if (end < start)
    return Empty();

  PINDEX len = GetLength();
  if (start > len)
    return Empty();

  if (end >= len) {
    if (start == 0)
      return *this;
    end = len-1;
  }

  return PString(substr(start, end - start + 1));
}


PString PString::Left(PINDEX len) const
{
  if (len <= 0)
    return Empty();

  if (len >= GetLength())
    return *this;

  return PString(substr(0, len));
}


PString PString::Right(PINDEX len) const
{
  if (len <= 0)
    return Empty();

  PINDEX srclen = GetLength();
  if (len >= srclen)
    return *this;

  return PString(substr(srclen - len, len));
}


PString PString::Mid(PINDEX start, PINDEX len) const
{
#if PINDEX_SIGNED
  if (len <= 0 || start < 0)
#else
  if (len == 0)
#endif
    return Empty();

  if (len == P_MAX_INDEX || start+len < start) // If open ended or check for wraparound
    return operator()(start, P_MAX_INDEX);
  else
    return operator()(start, start+len-1);
}

PString PString::Ellipsis(PINDEX maxLength, PINDEX fromEnd) const
{
  if (GetLength() <= maxLength)
    return *this;

  maxLength -= 3;
  if (fromEnd > maxLength)
    fromEnd = maxLength;

  return Left(maxLength-fromEnd) + "..." + Right(fromEnd);
}


bool PString::operator*=(const char * cstr) const
{
  if (cstr == NULL)
    return IsEmpty();

  return strcasecmp(c_str(), cstr) == 0;
}


PObject::Comparison PString::NumCompare(const PString & str, PINDEX count, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0 || count < 0)
    return LessThan;
#endif

  PINDEX len = str.GetLength();
  if (count > len)
    count = len;
  return InternalCompare(offset, count, str);
}


PObject::Comparison PString::NumCompare(const char * cstr, PINDEX count, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0 || count < 0)
    return LessThan;
#endif

  PINDEX len = ::strlen(cstr);
  if (count > len)
    count = len;
  return InternalCompare(offset, count, cstr);
}


PObject::Comparison PString::InternalCompare(PINDEX offset, PINDEX length, const char * cstr) const
{
#if PINDEX_SIGNED
  if (offset < 0 || length < 0)
    return LessThan;
#endif

  if (offset == 0 && c_str() == cstr)
    return EqualTo;

  const char * s1 = c_str()+offset;
  const char * s2 = cstr != NULL ? cstr : "";
  return Compare2(length != P_MAX_INDEX ? internal_strncmp(s1, s2, length) : internal_strcmp(s1, s2), 0);
}


int PString::internal_strcmp(const char * s1, const char *s2) const
{
  return strcmp(s1, s2);
}


int PString::internal_strncmp(const char * s1, const char *s2, size_t n) const
{
  return strncmp(s1, s2, n);
}


PINDEX PString::Find(char ch, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  PINDEX len = GetLength();
  while (offset < len) {
    if (InternalCompare(offset, 1, &ch) == EqualTo)
      return offset;
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::Find(const char * cstr, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cstr == NULL || *cstr == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  PINDEX clen = strlen(cstr);
  if (clen > len)
    return P_MAX_INDEX;

  if (offset > len - clen)
    return P_MAX_INDEX;

  if (len - clen < 10) {
    while (offset+clen <= len) {
      if (InternalCompare(offset, clen, cstr) == EqualTo)
        return offset;
      offset++;
    }
    return P_MAX_INDEX;
  }

  int strSum = 0;
  int cstrSum = 0;
  for (PINDEX i = 0; i < clen; i++) {
    strSum += toupper(at(offset+i) & 0xff);
    cstrSum += toupper(cstr[i] & 0xff);
  }

  // search for a matching substring
  while (offset+clen < len) {
    if (strSum == cstrSum && InternalCompare(offset, clen, cstr) == EqualTo)
      return offset;
    strSum += toupper(at(offset+clen) & 0xff);
    strSum -= toupper(at(offset) & 0xff);
    offset++;
  }

  return P_MAX_INDEX;
}


PINDEX PString::FindLast(char ch, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  PINDEX len = GetLength();
  if (len == 0)
    return P_MAX_INDEX;

  if (offset >= len)
    offset = len-1;

  while (InternalCompare(offset, 1, &ch) != EqualTo) {
    if (offset == 0)
      return P_MAX_INDEX;
    offset--;
  }

  return offset;
}


PINDEX PString::FindLast(const char * cstr, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cstr == NULL || *cstr == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  PINDEX clen = strlen(cstr);
  if (clen > len)
    return P_MAX_INDEX;

  if (offset > len - clen)
    offset = len - clen;

  int strSum = 0;
  int cstrSum = 0;
  for (PINDEX i = 0; i < clen; i++) {
    strSum += toupper(at(offset+i) & 0xff);
    cstrSum += toupper(cstr[i] & 0xff);
  }

  // search for a matching substring
  while (strSum != cstrSum || InternalCompare(offset, clen, cstr) != EqualTo) {
    if (offset == 0)
      return P_MAX_INDEX;
    --offset;
    strSum += toupper(at(offset) & 0xff);
    strSum -= toupper(at(offset+clen) & 0xff);
  }

  return offset;
}


PINDEX PString::FindOneOf(const char * cset, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cset == NULL || *cset == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  while (offset < len) {
    const char * p = cset;
    while (*p != '\0') {
      if (InternalCompare(offset, 1, p) == EqualTo)
        return offset;
      p++;
    }
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::FindSpan(const char * cset, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  if (cset == NULL || *cset == '\0')
    return P_MAX_INDEX;

  PINDEX len = GetLength();
  while (offset < len) {
    const char * p = cset;
    while (InternalCompare(offset, 1, p) != EqualTo) {
      if (*++p == '\0')
        return offset;
    }
    offset++;
  }
  return P_MAX_INDEX;
}


PINDEX PString::FindRegEx(const PRegularExpression & regex, PINDEX offset) const
{
#if PINDEX_SIGNED
  if (offset < 0)
    return P_MAX_INDEX;
#endif

  PINDEX pos = 0;
  PINDEX len = 0;
  if (FindRegEx(regex, pos, len, offset))
    return pos;

  return P_MAX_INDEX;
}


PBoolean PString::FindRegEx(const PRegularExpression & regex,
                        PINDEX & pos,
                        PINDEX & len,
                        PINDEX offset,
                        PINDEX maxPos) const
{
#if PINDEX_SIGNED
  if (offset < 0 || maxPos < 0)
    return false;
#endif

  PINDEX olen = GetLength();
  if (offset > olen)
    return false;

  if (offset == olen) {
    if (!regex.Execute("", pos, len))
      return false;
  }
  else {
    if (!regex.Execute(c_str() + offset, pos, len))
      return false;
  }

  pos += offset;
  if (pos+len > maxPos)
    return false;

  return true;
}

PBoolean PString::MatchesRegEx(const PRegularExpression & regex) const
{
  PINDEX pos = 0;
  PINDEX len = 0;

  if (!regex.Execute(c_str(), pos, len))
    return false;

  return (pos == 0) && (len == GetLength());
}

PString & PString::Replace(const PString & target, const PString & subs, PBoolean all, PINDEX offset)
{
#if PINDEX_SIGNED
  if (offset < 0)
    return *this;
#endif
    
  MakeUnique();

  PINDEX tlen = target.GetLength();
  PINDEX slen = subs.GetLength();
  do {
    PINDEX pos = Find(target, offset);
    if (pos == P_MAX_INDEX)
      return *this;
    Splice(subs, pos, tlen);
    offset = pos + slen;
  } while (all);

  return *this;
}


PString & PString::Splice(const char * cstr, PINDEX pos, PINDEX len)
{
#if PINDEX_SIGNED
  if (len < 0 || pos < 0)
    return *this;
#endif

  if (pos >= GetLength())
    return operator+=(cstr);

  if (len > 0)
    erase(pos, len);
  insert(pos, cstr);
  return *this;
}


bool PString::InternalSplit(const PString & delimiter, PString & before, PString & after, SplitOptions_Bits options) const
{
  PINDEX pos = Find(delimiter);

  if (pos != P_MAX_INDEX) {
    before = (options&SplitTrimBefore) ? Left(pos).Trim() : Left(pos);
    pos += delimiter.GetLength();
    after = (options&SplitTrimAfter) ? Mid(pos).Trim() : Mid(pos);
  }
  else {
    if (!(options&(SplitDefaultToBefore|SplitDefaultToAfter)))
      return false;

    if (options&SplitDefaultToBefore)
      before = (options&SplitTrimBefore) ? Trim() : *this;

    if (options&SplitDefaultToAfter)
      after = (options&SplitTrimBefore) ? Trim() : *this;
  }

  if (before.IsEmpty() && (options&SplitBeforeNonEmpty))
    return false;

  if (after.IsEmpty() && (options&SplitAfterNonEmpty))
    return false;

  return true;
}


PStringArray PString::Tokenise(const char * separators, PBoolean onePerSeparator) const
{
  PStringArray tokens;
  
  if (separators == NULL || IsEmpty())  // No tokens
    return tokens;
    
  PINDEX token = 0;
  PINDEX p1 = 0;
  PINDEX p2 = FindOneOf(separators);

  if (p2 == 0) {
    if (onePerSeparator) { // first character is a token separator
      tokens[token] = Empty();
      token++;                        // make first string in array empty
      p1 = 1;
      p2 = FindOneOf(separators, 1);
    }
    else {
      do {
        p1 = p2 + 1;
      } while ((p2 = FindOneOf(separators, p1)) == p1);
    }
  }

  while (p2 != P_MAX_INDEX) {
    if (p2 > p1)
      tokens[token] = operator()(p1, p2-1);
    else
      tokens[token] = Empty();
    token++;

    // Get next separator. If not one token per separator then continue
    // around loop to skip over all the consecutive separators.
    do {
      p1 = p2 + 1;
    } while ((p2 = FindOneOf(separators, p1)) == p1 && !onePerSeparator);
  }

  tokens[token] = operator()(p1, P_MAX_INDEX);

  return tokens;
}


PStringArray PString::Lines() const
{
  PStringArray lines;
  
  if (IsEmpty())
    return lines;
    
  PINDEX line = 0;
  PINDEX p1 = 0;
  PINDEX p2;
  while ((p2 = FindOneOf("\r\n", p1)) != P_MAX_INDEX) {
    lines[line++] = operator()(p1, p2-1);
    p1 = p2 + 1;
    if (at(p2) == '\r' && at(p1) == '\n') // CR LF pair
      p1++;
  }
  if (p1 < GetLength())
    lines[line] = operator()(p1, P_MAX_INDEX);
  return lines;
}


PString PString::LeftTrim() const
{
  if (IsEmpty())
    return PString::Empty();

  const char * lpos = c_str();
  while (isspace(*lpos & 0xff))
    lpos++;
  return PString(lpos);
}


PString PString::RightTrim() const
{
  if (IsEmpty())
    return PString::Empty();

  const char * rpos = c_str()+GetLength()-1;
  if (!isspace(*rpos & 0xff))
    return *this;

  while (isspace(*rpos & 0xff)) {
    if (rpos == c_str())
      return Empty();
    rpos--;
  }

  // make Apple & Tornado gnu compiler happy
  PString retval(c_str(), rpos - c_str() + 1);
  return retval;
}


PString PString::Trim() const
{
  if (IsEmpty())
    return PString::Empty();

  const char * lpos = c_str();
  while (isspace(*lpos & 0xff))
    lpos++;
  if (*lpos == '\0')
    return Empty();

  const char * rpos = c_str()+GetLength()-1;
  if (!isspace(*rpos & 0xff)) {
    if (lpos == c_str())
      return *this;
    else
      return PString(lpos);
  }

  while (isspace(*rpos & 0xff))
    rpos--;
  return PString(lpos, rpos - lpos + 1);
}


PString PString::ToLower() const
{
  PString newStr;
  for (char *cpos = newStr.GetPointerAndSetLength(length()); *cpos != '\0'; cpos++) {
    if (isupper(*cpos & 0xff))
      *cpos = (char)tolower(*cpos & 0xff);
  }
  return newStr;
}


PString PString::ToUpper() const
{
  PString newStr;
  for (char *cpos = newStr.GetPointerAndSetLength(length()); *cpos != '\0'; cpos++) {
    if (islower(*cpos & 0xff))
      *cpos = (char)toupper(*cpos & 0xff);
  }
  return newStr;
}


long PString::AsInteger(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtol(c_str(), &dummy, base);
}


DWORD PString::AsUnsigned(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoul(c_str(), &dummy, base);
}


int64_t PString::AsInteger64(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoll(c_str(), &dummy, base);
}


uint64_t PString::AsUnsigned64(unsigned base) const
{
  PAssert(base != 1 && base <= 36, PInvalidParameter);
  char * dummy;
  return strtoull(c_str(), &dummy, base);
}


double PString::AsReal() const
{
#ifndef __HAS_NO_FLOAT
  char * dummy;
  return strtod(c_str(), &dummy);
#else
  return 0.0;
#endif
}



#if P_HAS_ICONV
  namespace {
    struct PCharsetConverter : PObject
    {
      iconv_t m_cd;
      char  * m_inPtr;
      size_t  m_inLen;
      char  * m_outPtr;
      size_t  m_outLen;

      PCharsetConverter(
        const char * from,
        const char * to,
        const void * inPtr,
        size_t inLen,
        void * outPtr,
        size_t outLen
      )
        : m_inPtr((char *)inPtr)
        , m_inLen(inLen)
        , m_outPtr((char *)outPtr)
        , m_outLen(outLen)
      {
        m_cd = iconv_open(to, from);
        PAssert(m_cd != (iconv_t)-1, PInvalidParameter);
      }

      ~PCharsetConverter()
      {
        if (m_cd != (iconv_t)-1)
          iconv_close(m_cd);
      }

      int Convert()
      {
        return iconv(m_cd, &m_inPtr, &m_inLen, &m_outPtr, &m_outLen) != (size_t)-1 ? 0 : errno;
      }

      void EmitOutput(const void * data, size_t len)
      {
        if (len <= m_outLen) {
          memcpy(m_outPtr, data, len);
          m_outPtr += len;
          m_outLen -= len;
        }
      }
    };
  };
#endif // P_HAS_ICONV


PWCharArray PString::AsWide() const
{
  PWCharArray wide(1); // Null terminated empty string

  if (IsEmpty())
    return wide;

#if P_HAS_ICONV

  if (!PAssert(wide.SetSize(GetLength()), POutOfMemory))
    return wide;

  static const wchar_t BadChar = 0xFFFD;
  PCharsetConverter cvt("UTF-8", "WCHAR_T", GetPointer(), GetLength(), wide.GetPointer(), wide.GetSize()*sizeof(wchar_t));
  for (;;) {
    int err = cvt.Convert();
    switch (err) {
      case EILSEQ :
        do {
          ++cvt.m_inPtr;
          --cvt.m_inLen;
        } while (cvt.m_inLen > 0 && (*cvt.m_inPtr & 0xc0) == 0x80);
        cvt.EmitOutput(&BadChar, sizeof(BadChar));
        break;

      case EINVAL :
        cvt.EmitOutput(&BadChar, sizeof(BadChar));
        // Do next case

      case 0 :
        wide.SetSize(wide.GetSize() - cvt.m_outLen/sizeof(wchar_t) + 1);
        return wide;

      default :
        PAssertAlways(PSTRSTRM("Could not convert UTF-8 to wchar_t string: error=" << err << " - " << strerror(err)));
        return PWCharArray(1);
    }
  }

#elif defined(_WIN32)

  PINDEX len = MultiByteToWideChar(CP_UTF8, 0, c_str(), GetLength(), NULL, 0);
  if (PAssert(len > 0, PSTRSTRM("MultiByteToWideChar failed with error " << ::GetLastError())) &&
      PAssert(wide.SetSize(len+1), POutOfMemory))
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, c_str(), GetLength(), wide.GetPointer(), wide.GetSize());

#else // _WIN32 || _POSIX_VERSION

  if (PAssert(wide.SetSize(GetSize()), POutOfMemory)) { // Will be at least this big
    PINDEX count = 0;
    PINDEX i = 0;
    PINDEX length = GetLength()+1; // Include the trailing '\0'
    while (i < length) {
      int c = at(i);
      if ((c&0x80) == 0)
        wide[count++] = (BYTE)at(i++);
      else if ((c&0xe0) == 0xc0) {
        if (i < length-1)
          wide[count++] = (WORD)(((at(i  )&0x1f)<<6)|
                                  (at(i+1)&0x3f));
        i += 2;
      }
      else if ((c&0xf0) == 0xe0) {
        if (i < length-2)
          wide[count++] = (WORD)(((at(i  )&0x0f)<<12)|
                                 ((at(i+1)&0x3f)<< 6)|
                                  (at(i+2)&0x3f));
        i += 3;
      }
      else {
        if ((c&0xf8) == 0xf0)
          i += 4;
        else if ((c&0xfc) == 0xf8)
          i += 5;
        else
          i += 6;
        if (i <= length)
          wide[count++] = 0xfffd;
      }
    }

    wide.SetSize(count+1);  // Final size
  }
#endif // _WIN32 || _POSIX_VERSION

  return wide;
}


void PString::InternalFromWChar(const wchar_t * wstr, PINDEX len)
{
  if (wstr == NULL || len <= 0) {
    MakeEmpty();
    return;
  }

#if P_HAS_ICONV

  static const char BadChar[] = { '\xef', '\xbf', '\xbd' }; // 0xFFFD
  PINDEX max_length = len*6;
  PCharsetConverter cvt("WCHAR_T", "UTF-8", wstr, len*sizeof(wchar_t), GetPointerAndSetLength(max_length), max_length);
  for (;;) {
    int err = cvt.Convert();
    switch (err) {
      case EILSEQ :
        cvt.m_inPtr += sizeof(wchar_t);
        cvt.m_inLen -= sizeof(wchar_t);
        cvt.EmitOutput(BadChar, sizeof(BadChar));
        break;

      case EINVAL :
        cvt.EmitOutput(BadChar, sizeof(BadChar));
        // Do next case

      case 0 :
        resize(max_length - cvt.m_outLen);
        return;

      default :
        PAssertAlways(PSTRSTRM("Could not convert wchar_t string to UTF-8: error=" << err << " - " << strerror(err)));
        MakeEmpty();
        return;
    }
  }

#elif defined(_WIN32)

  int outLen = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL);
  if (PAssert(outLen > 0, PSTRSTRM("Could not convert wchar_t to UTF-8: errno=" << ::GetLastError())))
    WideCharToMultiByte(CP_UTF8, 0, wstr, len, GetPointerAndSetLength(outLen), GetSize(), NULL, NULL);

#else // _WIN32 || _POSIX_VERSION

  PINDEX i;
  PINDEX length = 0;
  const wchar_t * ptr;
  for (ptr = wstr, i = 0; i < len; i++) {
    unsigned v = *ptr++;
    if (v < 0x80)
      length++;
    else if (v < 0x800)
      length += 2;
    else if (v < 0x10000)
      length += 3;
    else if (v < 0x200000)
      length += 4;
    else if (v < 0x4000000)
      length += 5;
    else if (v < 0x80000000)
      length += 6;
  }

  char * out = GetPointerAndSetLength(length);
  for (ptr = wstr, i = 0; i < len; i++) {
    unsigned v = *ptr++;
    if (v > 0 && v < 0x80) // Allow for an embedded zero
      *out++ = (char)v;
    else if (v < 0x800) {
      *out++ = (char)(0xc0|(v>>6  ));
      *out++ = (char)(0x80|(v&0x3f));
    }
    else if (v < 0x10000) {
      *out++ = (char)(0xe0|( v>>12     ));
      *out++ = (char)(0x80|((v>>6)&0x3f));
      *out++ = (char)(0x80|( v    &0x3f));
    }
    else if (v < 0x200000) {
      *out++ = (char)(0xf0|( v>>18      ));
      *out++ = (char)(0x80|((v>>12)&0x3f));
      *out++ = (char)(0x80|((v>> 6)&0x3f));
      *out++ = (char)(0x80|( v     &0x3f));
    }
    else if (v < 0x4000000) {
      *out++ = (char)(0xf8|( v>>24      ));
      *out++ = (char)(0x80|((v>>18)&0x3f));
      *out++ = (char)(0x80|((v>>12)&0x3f));
      *out++ = (char)(0x80|((v>> 6)&0x3f));
      *out++ = (char)(0x80|( v     &0x3f));
    }
    else if (v < 0x80000000) {
      *out++ = (char)(0xfc+( v>>30)      );
      *out++ = (char)(0x80+((v>>24)&0x3f));
      *out++ = (char)(0x80+((v>>18)&0x3f));
      *out++ = (char)(0x80+((v>>12)&0x3f));
      *out++ = (char)(0x80+((v>> 6)&0x3f));
      *out++ = (char)(0x80+( v     &0x3f));
    }
    // Ignore
  }

  *out = '\0';
#endif // _WIN32 || _POSIX_VERSION
}



PBYTEArray PString::ToPascal() const
{
  PINDEX len = GetLength();
  PAssert(len < 256, "Cannot convert to PASCAL string");
  BYTE buf[256];
  buf[0] = (BYTE)len;
  memcpy(&buf[1], c_str(), len);
  return PBYTEArray(buf, len);
}


PString PString::ToLiteral() const
{
  PString str('"');
  for (const char * p = c_str(); *p != '\0'; p++) {
    if (*p == '"')
      str += "\\\"";
    else if (*p == '\\')
      str += "\\\\";
    else if (isprint(*p & 0xff))
      str += *p;
    else {
      PINDEX i;
      for (i = 0; i < PARRAYSIZE(PStringEscapeValue); i++) {
        if (*p == PStringEscapeValue[i]) {
          str += PString('\\') + (char)PStringEscapeCode[i];
          break;
        }
      }
      if (i >= PARRAYSIZE(PStringEscapeValue))
        str.sprintf("\\%03o", *p & 0xff);
    }
  }
  return str + '"';
}


PString PString::FromLiteral(PINDEX & offset) const
{
  if (offset >= GetLength())
    return PString::Empty();

  PString str;
  const char * cstr = c_str() + offset;
  TranslateEscapes(cstr, str.GetPointerAndSetLength(GetLength()-offset));
  str.MakeMinimumSize();
  offset = cstr - c_str();

  return str;
}


PString & PString::sprintf(const char * fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  return vsprintf(fmt, args);
}

#if defined(__GNUC__) || defined(__SUNPRO_CC)
#define _vsnprintf vsnprintf
#endif

PString & PString::vsprintf(const char * fmt, va_list arg)
{
  PINDEX len = GetLength();
#ifdef P_VXWORKS
  // The library provided with tornado 2.0 does not have the implementation
  // for vsnprintf
  // as workaround, just use a array size of 2000
  resize(::vsprintf(GetPointerAndSetLength(len+2000), fmt, arg));
#else
  int providedSpace = 0;
  int requiredSpace;
  do {
    providedSpace += 1000;
    requiredSpace = _vsnprintf(GetPointerAndSetLength(providedSpace+len), providedSpace, fmt, arg);
  } while (requiredSpace == -1 || requiredSpace >= providedSpace);
  resize(len + requiredSpace);
#endif // P_VXWORKS

  return *this;
}


PString psprintf(const char * fmt, ...)
{
  PString str;
  va_list args;
  va_start(args, fmt);
  return str.vsprintf(fmt, args);
}


PString pvsprintf(const char * fmt, va_list arg)
{
  PString str;
  return str.vsprintf(fmt, arg);
}


PBoolean PString::MakeMinimumSize(PINDEX newLength)
{
  if (newLength == 0)
    newLength = strlen(c_str());
  resize(newLength);
  return true;
}


char * PString::GetPointerAndSetLength(PINDEX len)
{
  resize(len);
  return const_cast<char *>(data());
}


///////////////////////////////////////////////////////////////////////////////

PObject * PCaselessString::Clone() const
{
  return new PCaselessString(*this);
}


int PCaselessString::internal_strcmp(const char * s1, const char *s2) const
{
  return strcasecmp(s1, s2);
}


int PCaselessString::internal_strncmp(const char * s1, const char *s2, size_t n) const
{
  return strncasecmp(s1, s2, n);
}


///////////////////////////////////////////////////////////////////////////////

PStringArray::PStringArray(PINDEX count, char const * const * strarr, PBoolean caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  if (count == P_MAX_INDEX) {
    count = 0;
    while (strarr[count] != NULL)
      count++;
  }

  SetSize(count);
  for (PINDEX i = 0; i < count; i++) {
    PString * newString;
    if (caseless)
      newString = new PCaselessString(strarr[i]);
    else
      newString = new PString(strarr[i]);
    SetAt(i, newString);
  }
}


PStringArray::PStringArray(const PString & str)
{
  SetSize(1);
  (*theArray)[0] = str.CloneAs<PString>();
}


PStringArray::PStringArray(const char * cstr)
{
  SetSize(1);
  (*theArray)[0] = new PString(cstr);
}


PStringArray::PStringArray(const PStringList & list)
{
  SetSize(list.GetSize());
  PINDEX count = 0;
  for (PStringList::const_iterator it = list.begin(); it != list.end(); ++it)
    (*theArray)[count++] = it->CloneAs<PString>();
}


PStringArray::PStringArray(const PSortedStringList & list)
{
  SetSize(list.GetSize());
  for (PINDEX i = 0; i < list.GetSize(); i++)
    (*theArray)[i] = list[i].CloneAs<PString>();
}


PStringArray::PStringArray(const PStringSet & set)
{
  SetSize(set.GetSize());
  PINDEX count = 0;
  for (PStringSet::const_iterator it = set.begin(); it != set.end(); ++it)
    (*theArray)[count++] = it->CloneAs<PString>();
}


PStringArray & PStringArray::operator+=(const PStringArray & v)
{
  PINDEX i;
  for (i = 0; i < v.GetSize(); i++)
    AppendString(v[i]);

  return *this;
}


void PStringArray::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PString PStringArray::operator[](PINDEX index) const
{
  PASSERTINDEX(index);
  if (index < GetSize() && (*theArray)[index] != NULL)
    return *(PString *)(*theArray)[index];
  return PString::Empty();
}


PString & PStringArray::operator[](PINDEX index)
{
  PASSERTINDEX(index);
  PAssert(SetMinSize(index+1), POutOfMemory);
  if ((*theArray)[index] == NULL)
    (*theArray)[index] = new PString;
  return *(PString *)(*theArray)[index];
}


static void strcpy_with_increment(char * & strPtr, const PString & str)
{
  PINDEX len = str.GetLength()+1;
  memcpy(strPtr, (const char *)str, len);
  strPtr += len;
}

char ** PStringArray::ToCharArray(PCharArray * storage) const
{
  PINDEX i;

  PINDEX mySize = GetSize();
  PINDEX storageSize = (mySize+1)*sizeof(char *);
  for (i = 0; i < mySize; i++)
    storageSize += (*this)[i].GetLength()+1;

  char ** storagePtr;
  if (storage != NULL)
    storagePtr = (char **)storage->GetPointer(storageSize);
  else
    storagePtr = (char **)malloc(storageSize);

  if (storagePtr == NULL)
    return NULL;

  char * strPtr = (char *)&storagePtr[mySize+1];

  for (i = 0; i < mySize; i++) {
    storagePtr[i] = strPtr;
    strcpy_with_increment(strPtr, (*this)[i]);
  }

  storagePtr[i] = NULL;

  return storagePtr;
}


PString PStringArray::ToString(char separator) const
{
  PStringStream str;

  for (PINDEX i = 0; i < GetSize(); ++i) {
    if (i > 0)
      str << separator;
    str << (*this)[i];
  }

  return str;
}


///////////////////////////////////////////////////////////////////////////////

PStringList::PStringList(PINDEX count, char const * const * strarr, PBoolean caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count; i++) {
    PString * newString;
    if (caseless)
      newString = new PCaselessString(strarr[i]);
    else
      newString = new PString(strarr[i]);
    Append(newString);
  }
}


PStringList::PStringList(const PString & str)
{
  Append(str.Clone());
}


PStringList::PStringList(const char * cstr)
{
  AppendString(cstr);
}


PStringList::PStringList(const PStringArray & array)
{
  for (PINDEX i = 0; i < array.GetSize(); i++)
    Append(array[i].Clone());
}


PStringList::PStringList(const PSortedStringList & list)
{
  for (PSortedStringList::const_iterator it = list.begin(); it != list.end(); ++it)
    Append(it->Clone());
}


PStringList::PStringList(const PStringSet & set)
{
  for (PStringSet::const_iterator it = set.begin(); it != set.end(); ++it)
    Append(it->Clone());
}


PStringList & PStringList::operator += (const PStringList & v)
{
  for (PStringList::const_iterator i = v.begin(); i != v.end(); i++)
    AppendString(*i);

  return *this;
}


void PStringList::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


///////////////////////////////////////////////////////////////////////////////

PSortedStringList::PSortedStringList(PINDEX count,
                                     char const * const * strarr,
                                     PBoolean caseless)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count; i++) {
    PString * newString;
    if (caseless)
      newString = new PCaselessString(strarr[i]);
    else
      newString = new PString(strarr[i]);
    Append(newString);
  }
}


PSortedStringList::PSortedStringList(const PString & str)
{
  Append(str.Clone());
}


PSortedStringList::PSortedStringList(const char * cstr)
{
  AppendString(cstr);
}


PSortedStringList::PSortedStringList(const PStringArray & array)
{
  for (PINDEX i = 0; i < array.GetSize(); i++)
    Append(array[i].Clone());
}


PSortedStringList::PSortedStringList(const PStringList & list)
{
  for (PStringList::const_iterator it = list.begin(); it != list.end(); ++it)
    Append(it->Clone());
}


PSortedStringList::PSortedStringList(const PStringSet & set)
{
  for (PStringSet::const_iterator it = set.begin(); it != set.end(); ++it)
    Append(it->Clone());
}


void PSortedStringList::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    AppendString(str);
  }
}


PINDEX PSortedStringList::GetNextStringsIndex(const PString & str) const
{
  PINDEX len = str.GetLength();
  PSortedListElement * element;
  PINDEX index = InternalStringSelect(str, len, m_info->m_root, element);

  if (index != 0) {
    PSortedListElement * prev;
    while ((prev = m_info->Predecessor(element)) != &m_info->nil && ((PString *)prev->m_data)->NumCompare(str, len) >= EqualTo) {
      element = prev;
      index--;
    }
  }

  return index;
}


PINDEX PSortedStringList::InternalStringSelect(const char * str,
                                               PINDEX len,
                                               PSortedListElement * thisElement,
                                               PSortedListElement * & selectedElement) const
{
  if (thisElement == &m_info->nil)
    return 0;

  switch (((PString *)thisElement->m_data)->NumCompare(str, len)) {
    case PObject::LessThan :
    {
      PINDEX index = InternalStringSelect(str, len, thisElement->m_right, selectedElement);
      return thisElement->m_left->m_subTreeSize + index + 1;
    }

    case PObject::GreaterThan :
      return InternalStringSelect(str, len, thisElement->m_left, selectedElement);

    default :
      selectedElement = thisElement;
      return thisElement->m_left->m_subTreeSize;
  }
}


///////////////////////////////////////////////////////////////////////////////

PStringSet::PStringSet(PINDEX count, char const * const * strarr, PBoolean caseless)
  : BaseClass(true)
{
  if (count == 0)
    return;

  if (PAssertNULL(strarr) == NULL)
    return;

  for (PINDEX i = 0; i < count; i++) {
    if (caseless)
      Include(PCaselessString(strarr[i]));
    else
      Include(PString(strarr[i]));
  }
}


PStringSet::PStringSet(const PString & str)
  : BaseClass(true)
{
  Include(str);
}


PStringSet::PStringSet(const char * cstr)
  : BaseClass(true)
{
  Include(cstr);
}


PStringSet::PStringSet(const PStringArray & strArray)
  : BaseClass(true)
{
  for (PINDEX i = 0; i < strArray.GetSize(); ++i)
    Include(strArray[i]);
}


PStringSet::PStringSet(const PStringList & strList)
  : BaseClass(true)
{
  for (PStringList::const_iterator it = strList.begin(); it != strList.end(); ++it)
    Include(*it);
}


void PStringSet::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    Include(str);
  }
}


///////////////////////////////////////////////////////////////////////////////

POrdinalToString::POrdinalToString(PINDEX count, const Initialiser * init)
{
  while (count-- > 0) {
    SetAt(init->key, init->value);
    init++;
  }
}


void POrdinalToString::ReadFrom(istream & strm)
{
  while (strm.good()) {
    POrdinalKey key;
    char equal;
    PString str;
    strm >> key >> ws >> equal >> str;
    if (equal != '=')
      SetAt(key, PString::Empty());
    else
      SetAt(key, str.Mid(equal+1));
  }
}


///////////////////////////////////////////////////////////////////////////////

PStringToOrdinal::PStringToOrdinal(PINDEX count,
                                   const Initialiser * init,
                                   PBoolean caseless)
{
  while (count-- > 0) {
    if (caseless)
      SetAt(PCaselessString(init->key), init->value);
    else
      SetAt(init->key, init->value);
    init++;
  }
}


PStringToOrdinal::PStringToOrdinal(PINDEX count,
                                   const POrdinalToString::Initialiser * init,
                                   PBoolean caseless)
{
  while (count-- > 0) {
    if (caseless)
      SetAt(PCaselessString(init->value), init->key);
    else
      SetAt(init->value, init->key);
    init++;
  }
}


void PStringToOrdinal::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    PINDEX equal = str.FindLast('=');
    if (equal == P_MAX_INDEX)
      SetAt(str, 0);
    else
      SetAt(str.Left(equal), str.Mid(equal+1).AsInteger());
  }
}


///////////////////////////////////////////////////////////////////////////////

PStringToString::PStringToString(PINDEX count,
                                 const Initialiser * init,
                                 PBoolean caselessKeys,
                                 PBoolean caselessValues)
{
  while (count-- > 0) {
    if (caselessValues)
      if (caselessKeys)
        SetAt(PCaselessString(init->key), PCaselessString(init->value));
      else
        SetAt(init->key, PCaselessString(init->value));
    else
      if (caselessKeys)
        SetAt(PCaselessString(init->key), init->value);
      else
        SetAt(init->key, init->value);
    init++;
  }
}


void PStringToString::ReadFrom(istream & strm)
{
  while (strm.good()) {
    PString str;
    strm >> str;
    if (str.IsEmpty())
      continue;

    PString key, value;
    str.Split('=', key, value, PString::SplitDefaultToBefore);

    PString * ptr = GetAt(key);
    if (ptr != NULL)
      *ptr += '\n' + value;
    else
      SetAt(key, value);
  }
}


void PStringToString::FromString(const PString & str)
{
  RemoveAll();

  PStringStream strm(str);
  strm >> *this;
}


void PStringToString::Merge(const PStringToString & other, MergeAction action)
{
  for (const_iterator it = other.begin(); it != other.end(); ++it) {
    PString * str = GetAt(it->first);
    if (str == NULL || action == e_MergeOverwrite)
      SetAt(it->first, it->second);
    else if (action == e_MergeAppend)
      *str += '\n' + it->second;
  }
}


char ** PStringToString::ToCharArray(bool withEqualSign, PCharArray * storage) const
{
  const_iterator it;

  PINDEX mySize = GetSize();
  PINDEX numPointers = mySize*(withEqualSign ? 1 : 2) + 1;
  PINDEX storageSize = numPointers*sizeof(char *);
  for (it = begin(); it != end(); ++it)
    storageSize += it->first.GetLength()+1 + it->second.GetLength()+1;

  char ** storagePtr;
  if (storage != NULL)
    storagePtr = (char **)storage->GetPointer(storageSize);
  else
    storagePtr = (char **)malloc(storageSize);

  if (storagePtr == NULL)
    return NULL;

  char * strPtr = (char *)&storagePtr[numPointers];
  PINDEX strIndex = 0;

  for (it = begin(); it != end(); ++it) {
    storagePtr[strIndex++] = strPtr;
    if (withEqualSign)
      strcpy_with_increment(strPtr, it->first + '=' + it->second);
    else {
      strcpy_with_increment(strPtr, it->first);
      storagePtr[strIndex++] = strPtr;
      strcpy_with_increment(strPtr, it->second);
    }
  }

  storagePtr[strIndex] = NULL;

  return storagePtr;
}


///////////////////////////////////////////////////////////////////////////////


PString PStringOptions::GetString(const PCaselessString & key, const char * dflt) const
{
  PString * str = PStringToString::GetAt(key);
  return str != NULL ? *str : PString(dflt);
}


bool PStringOptions::GetBoolean(const PCaselessString & key, bool dflt) const
{
  PString * str = PStringToString::GetAt(key);
  if (str == NULL)
    return dflt;

  if (str->IsEmpty() || str->AsUnsigned() != 0)
    return true;

  static char const * const synonymsForTrue[] = { "true", "yes", "enabled" };
  for (PINDEX i = 0; i < PARRAYSIZE(synonymsForTrue); ++i) {
    if (PConstCaselessString(synonymsForTrue[i]).NumCompare(*str) == EqualTo)
      return true;
  }

  return false;
}


long PStringOptions::GetInteger(const PCaselessString & key, long dflt) const
{
  PString * str = PStringToString::GetAt(key);
  return str != NULL ? str->AsInteger() : dflt;
}


void PStringOptions::SetInteger(const PCaselessString & key, long value)
{
  PStringToString::SetAt(key, PString(PString::Signed, value));
}


double PStringOptions::GetReal(const PCaselessString & key, double dflt) const
{
  PString * str = PStringToString::GetAt(key);
  return str != NULL ? str->AsReal() : dflt;
}


void PStringOptions::SetReal(const PCaselessString & key, double value, int decimals)
{
  PStringToString::SetAt(key, PString(decimals < 0 ? PString::Exponent : PString::Decimal, value, decimals));
}


///////////////////////////////////////////////////////////////////////////////

PRegularExpression::PRegularExpression()
  : m_compileOptions(IgnoreCase)
  , m_compiledRegex(NULL)
  , m_lastError(NotCompiled)
{
}


PRegularExpression::PRegularExpression(const PString & pattern, CompileOptions options)
  : m_pattern(pattern)
  , m_compileOptions(options)
  , m_compiledRegex(NULL)
{
  InternalCompile(true);
}


PRegularExpression::PRegularExpression(const char * pattern, CompileOptions options)
  : m_pattern(pattern)
  , m_compileOptions(options)
  , m_compiledRegex(NULL)
{
  InternalCompile(true);
}


PRegularExpression::PRegularExpression(const PRegularExpression & from)
  : m_pattern(from.m_pattern)
  , m_compileOptions(from.m_compileOptions)
  , m_compiledRegex(NULL)
{
  InternalCompile(true);
}


PRegularExpression & PRegularExpression::operator=(const PRegularExpression & from)
{
  if (&from != this) {
    m_pattern = from.m_pattern;
    m_compileOptions = from.m_compileOptions;
    InternalCompile(true);
  }

  return *this;
}


PRegularExpression::~PRegularExpression()
{
  InternalClean();
}


void PRegularExpression::InternalClean()
{
  if (m_compiledRegex != NULL) {
    regfree((regex_t*)m_compiledRegex);
    free(m_compiledRegex);
    m_compiledRegex = NULL;
  }
}


void PRegularExpression::PrintOn(ostream &strm) const
{
  strm << m_pattern;
}


PString PRegularExpression::GetErrorText() const
{
  char str[256];
  regerror(m_lastError, (regex_t*)m_compiledRegex, str, sizeof(str));
  return str;
}


bool PRegularExpression::Compile(const PString & pattern, CompileOptions options)
{
  m_pattern = pattern;
  m_compileOptions = options;
  return InternalCompile(false);
}


bool PRegularExpression::Compile(const char * pattern, CompileOptions options)
{
  m_pattern = pattern;
  m_compileOptions = options;
  return InternalCompile(false);
}


bool PRegularExpression::InternalCompile(bool assertOnFail)
{
  InternalClean();

  if (m_pattern.IsEmpty()) {
    m_lastError = assertOnFail ? NotCompiled : BadPattern;
    return false;
  }

  m_compiledRegex = malloc(sizeof(regex_t));
  m_lastError = (ErrorCodes)regcomp((regex_t*)m_compiledRegex, m_pattern, m_compileOptions);
  if (m_lastError == NoError)
    return true;

  InternalClean();

  if (assertOnFail)
    PAssertAlways(PSTRSTRM("Regular expression " << m_pattern.ToLiteral() << " failed to compile: " << GetErrorText()));

  return false;
}


bool PRegularExpression::Execute(const PString & str, PINDEX & start, ExecOptions options) const
{
  PINDEX dummy;
  return Execute((const char *)str, start, dummy, options);
}


bool PRegularExpression::Execute(const PString & str, PINDEX & start, PINDEX & len, ExecOptions options) const
{
  return Execute((const char *)str, start, len, options);
}


bool PRegularExpression::Execute(const char * cstr, PINDEX & start, ExecOptions options) const
{
  PINDEX dummy;
  return Execute(cstr, start, dummy, options);
}


bool PRegularExpression::Execute(const char * cstr, PINDEX & start, PINDEX & len, ExecOptions options) const
{
  if (m_compiledRegex == NULL)
    m_lastError = NotCompiled;

  if (m_lastError != NoError && m_lastError != NoMatch)
    return false;

  regmatch_t match;

  m_lastError = (ErrorCodes)regexec((regex_t*)m_compiledRegex, cstr, 1, &match, options);
  if (m_lastError != NoError)
    return false;

  start = match.rm_so;
  len = match.rm_eo - start;
  return true;
}


bool PRegularExpression::Execute(const PString & str, PIntArray & starts, ExecOptions options) const
{
  PIntArray dummy;
  return Execute((const char *)str, starts, dummy, options);
}


bool PRegularExpression::Execute(const PString & str,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 ExecOptions options) const
{
  return Execute((const char *)str, starts, ends, options);
}


bool PRegularExpression::Execute(const char * cstr, PIntArray & starts, ExecOptions options) const
{
  PIntArray dummy;
  return Execute(cstr, starts, dummy, options);
}


bool PRegularExpression::Execute(const char * cstr,
                                 PIntArray & starts,
                                 PIntArray & ends,
                                 ExecOptions options) const
{
  if (m_compiledRegex == NULL) {
    m_lastError = NotCompiled;
    return false;
  }

  PINDEX count = starts.GetSize();
  if (count == 0) {
    starts.SetSize(1);
    count = 1;
  }
  ends.SetSize(count);

  regmatch_t * matches = new regmatch_t[count];

  m_lastError = (ErrorCodes)::regexec((regex_t*)m_compiledRegex, cstr, count, matches, options);
  if (m_lastError == NoError) {
    for (PINDEX i = 0; i < count; i++) {
      starts[i] = matches[i].rm_so;
      ends[i] = matches[i].rm_eo;
    }
  }

  delete [] matches;

  return m_lastError == NoError;
}


bool PRegularExpression::Execute(const char * cstr, PStringArray & substring, ExecOptions options) const
{
  if (m_compiledRegex == NULL) {
    m_lastError = NotCompiled;
    return false;
  }

  PINDEX count = substring.GetSize();
  if (count == 0) {
    substring.SetSize(1);
    count = 1;
  }

  regmatch_t * matches = new regmatch_t[count];

  m_lastError = (ErrorCodes)::regexec((regex_t*)m_compiledRegex, cstr, count, matches, options);
  if (m_lastError == NoError) {
    for (PINDEX i = 0; i < count; i++)
      substring[i] = PString(cstr+matches[i].rm_so, matches[i].rm_eo-matches[i].rm_so);
  }

  delete [] matches;

  return m_lastError == NoError;
}


PString PRegularExpression::EscapeString(const PString & str)
{
  PString translated = str;

  PINDEX lastPos = 0;
  PINDEX nextPos;
  while ((nextPos = translated.FindOneOf("\\^$+?*.[]()|{}", lastPos)) != P_MAX_INDEX) {
    translated.Splice("\\", nextPos);
    lastPos = nextPos+2;
  }

  return translated;
}


///////////////////////////////////////////////////////////////////////////////

void PPrintEnum(std::ostream & strm, int value, int begin, int end, char const * const * names)
{
  if (value < begin || value >= end)
    strm << '<' << value << '>';
  else
    strm << names[value-begin];
}


int PReadEnum(std::istream & strm, int begin, int end, char const * const * names, bool matchCase)
{
  char name[100]; // If someone has an enumeration longer than this, it deserves to fail!
  strm >> ws;
  strm.get(name, sizeof(name), ' ');
  if (strm.fail() || strm.bad())
    return end;

  size_t len = strlen(name);
  int match = end;
  for (int value = begin; value < end; ++value) {
    const char * cmp = names[value-begin];

    if ((matchCase ? strcmp(name, cmp) : strcasecmp(name, cmp)) == 0) // Exact match
      return value;

    if ((matchCase ? strncmp(name, cmp, len) : strncasecmp(name, cmp, len)) == 0) {
      if (match < end) {
        match = end; // Not unique for the length
        break;
      }
      match = value;
    }
  }

  if (match < end)
    return match;

  do {
    strm.putback(name[--len]);
  } while (len > 0);

  strm.clear();
  strm.setstate(ios::failbit);

  return end;
}


int PParseEnum(const char * str, int begin, int end, char const * const * names, bool matchCase)
{
  std::stringstream strm;
  strm.str(str);
  return PReadEnum(strm, begin, end, names, matchCase);
}


void PPrintBitwiseEnum(std::ostream & strm, unsigned bits, char const * const * names)
{
  if (bits == 0) {
    strm << *names;
    return;
  }

  ++names;

  std::streamsize width = strm.width();
  if (width > 0) {
    std::streamsize len = 0;
    unsigned bit = 1;
    const char * const * name = names;
    while (*name != NULL) {
      if (bits & bit) {
        if (len > 0)
          ++len;
        len += strlen(*name);
      }
      bit <<= 1;
      ++name;
    }
    if (width > len)
      width -= len;
    else
      width = 0;
    strm.width(0);
  }

  if (width > 0 && (strm.flags()&ios::left) == 0)
    strm << setw(width) << " ";

  bool needSpace = false;
  for (unsigned bit = 1; *names != NULL; bit <<= 1, ++names) {
    if (bits & bit) {
      if (needSpace)
        strm << ' ';
      else
        needSpace = true;
      strm << *names;
    }
  }

  if (width > 0 && (strm.flags()&ios::right) == 0)
    strm << setw(width) << " ";
}


unsigned PReadBitwiseEnum(std::istream & strm, char const * const * names, bool continueOnError)
{
  unsigned bits = 0;
  while ((continueOnError || strm.good()) && !strm.eof()) {
    char name[100]; // If someone has an enumeration longer than this, it deserves to fail!
    strm >> ws;
    strm.get(name, sizeof(name), ' ');
    if (strm.fail() || strm.bad())
      break;

    if (strcmp(name, *names) == 0)
      return 0;

    bool unknown = true;
    for (unsigned i = 1; names[i] != NULL; ++i) {
      if (strcmp(name, names[i]) == 0) {
        bits |= (1 << (i-1));
        unknown = false;
        break;
      }
    }

    if (continueOnError)
      continue;

    if (unknown) {
      size_t i = strlen(name);
      do {
        strm.putback(name[--i]);
      } while (i > 0);

      break;
    }
  }
  return bits;
}


// End Of File ///////////////////////////////////////////////////////////////
