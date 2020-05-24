//
// rfc1155.cxx
//
// Code automatically generated by asnparse.
//

#ifdef P_USE_PRAGMA
#pragma implementation "rfc1155.h"
#endif

#include <ptlib.h>
#include <ptclib/rfc1155.h>

#define new PNEW


#ifdef P_SNMP

//
// ObjectName
//

PRFC1155_ObjectName::PRFC1155_ObjectName(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_ObjectId(tag, tagClass)
{
}


PObject * PRFC1155_ObjectName::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_ObjectName::Class()), PInvalidCast);
#endif
  return new PRFC1155_ObjectName(*this);
}


//
// ObjectSyntax
//

PRFC1155_ObjectSyntax::PRFC1155_ObjectSyntax(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 2, false
//#ifndef PASN_NOPRINTON
//    ,(const PASN_Names *)Names_PRFC1155_ObjectSyntax,0
//#endif
)
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ObjectSyntax::operator PRFC1155_SimpleSyntax &() const
#else
PRFC1155_ObjectSyntax::operator PRFC1155_SimpleSyntax &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_SimpleSyntax), PInvalidCast);
#endif
  return *(PRFC1155_SimpleSyntax *)choice;
}


PRFC1155_ObjectSyntax::operator const PRFC1155_SimpleSyntax &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_SimpleSyntax), PInvalidCast);
#endif
  return *(PRFC1155_SimpleSyntax *)choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ObjectSyntax::operator PRFC1155_ApplicationSyntax &() const
#else
PRFC1155_ObjectSyntax::operator PRFC1155_ApplicationSyntax &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_ApplicationSyntax), PInvalidCast);
#endif
  return *(PRFC1155_ApplicationSyntax *)choice;
}


PRFC1155_ObjectSyntax::operator const PRFC1155_ApplicationSyntax &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_ApplicationSyntax), PInvalidCast);
#endif
  return *(PRFC1155_ApplicationSyntax *)choice;
}


bool PRFC1155_ObjectSyntax::CreateObject()
{
  choice = new PRFC1155_SimpleSyntax(m_tag, m_tagClass);
  if (((PASN_Choice*)choice)->CreateObject())
    return true;
  delete choice;

  choice = new PRFC1155_ApplicationSyntax(m_tag, m_tagClass);
  if (((PASN_Choice*)choice)->CreateObject())
    return true;
  delete choice;

  choice = NULL;
  return false;
}


PObject * PRFC1155_ObjectSyntax::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_ObjectSyntax::Class()), PInvalidCast);
#endif
  return new PRFC1155_ObjectSyntax(*this);
}



#ifndef PASN_NOPRINTON
const static PASN_Names Names_PRFC1155_SimpleSyntax[]={
      {"number",2}
     ,{"string",4}
     ,{"object",6}
     ,{"empty",5}
};
#endif
//
// SimpleSyntax
//

PRFC1155_SimpleSyntax::PRFC1155_SimpleSyntax(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 4, false
#ifndef PASN_NOPRINTON
    ,(const PASN_Names *)Names_PRFC1155_SimpleSyntax,4
#endif
)
{
}


bool PRFC1155_SimpleSyntax::CreateObject()
{
  switch (m_tag) {
    case e_number :
      choice = new PASN_Integer();
      return true;
    case e_string :
      choice = new PASN_OctetString();
      return true;
    case e_object :
      choice = new PASN_ObjectId();
      return true;
    case e_empty :
      choice = new PASN_Null();
      return true;
  }

  choice = NULL;
  return false;
}


PObject * PRFC1155_SimpleSyntax::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_SimpleSyntax::Class()), PInvalidCast);
#endif
  return new PRFC1155_SimpleSyntax(*this);
}



#ifndef PASN_NOPRINTON
const static PASN_Names Names_PRFC1155_ApplicationSyntax[]={
      {"counter",1}
     ,{"gauge",2}
     ,{"ticks",3}
     ,{"arbitrary",4}
};
#endif
//
// ApplicationSyntax
//

PRFC1155_ApplicationSyntax::PRFC1155_ApplicationSyntax(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 5, false
#ifndef PASN_NOPRINTON
    ,(const PASN_Names *)Names_PRFC1155_ApplicationSyntax,4
#endif
)
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ApplicationSyntax::operator PRFC1155_NetworkAddress &() const
#else
PRFC1155_ApplicationSyntax::operator PRFC1155_NetworkAddress &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_NetworkAddress), PInvalidCast);
#endif
  return *(PRFC1155_NetworkAddress *)choice;
}


PRFC1155_ApplicationSyntax::operator const PRFC1155_NetworkAddress &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_NetworkAddress), PInvalidCast);
#endif
  return *(PRFC1155_NetworkAddress *)choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ApplicationSyntax::operator PRFC1155_Counter &() const
#else
PRFC1155_ApplicationSyntax::operator PRFC1155_Counter &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_Counter), PInvalidCast);
#endif
  return *(PRFC1155_Counter *)choice;
}


PRFC1155_ApplicationSyntax::operator const PRFC1155_Counter &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_Counter), PInvalidCast);
#endif
  return *(PRFC1155_Counter *)choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ApplicationSyntax::operator PRFC1155_Gauge &() const
#else
PRFC1155_ApplicationSyntax::operator PRFC1155_Gauge &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_Gauge), PInvalidCast);
#endif
  return *(PRFC1155_Gauge *)choice;
}


PRFC1155_ApplicationSyntax::operator const PRFC1155_Gauge &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_Gauge), PInvalidCast);
#endif
  return *(PRFC1155_Gauge *)choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ApplicationSyntax::operator PRFC1155_TimeTicks &() const
#else
PRFC1155_ApplicationSyntax::operator PRFC1155_TimeTicks &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_TimeTicks), PInvalidCast);
#endif
  return *(PRFC1155_TimeTicks *)choice;
}


PRFC1155_ApplicationSyntax::operator const PRFC1155_TimeTicks &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_TimeTicks), PInvalidCast);
#endif
  return *(PRFC1155_TimeTicks *)choice;
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_ApplicationSyntax::operator PRFC1155_Opaque &() const
#else
PRFC1155_ApplicationSyntax::operator PRFC1155_Opaque &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_Opaque), PInvalidCast);
#endif
  return *(PRFC1155_Opaque *)choice;
}


PRFC1155_ApplicationSyntax::operator const PRFC1155_Opaque &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_Opaque), PInvalidCast);
#endif
  return *(PRFC1155_Opaque *)choice;
}


bool PRFC1155_ApplicationSyntax::CreateObject()
{
  switch (m_tag) {
    case e_counter :
      choice = new PRFC1155_Counter();
      return true;
    case e_gauge :
      choice = new PRFC1155_Gauge();
      return true;
    case e_ticks :
      choice = new PRFC1155_TimeTicks();
      return true;
    case e_arbitrary :
      choice = new PRFC1155_Opaque();
      return true;
  }

  choice = new PRFC1155_NetworkAddress(m_tag, m_tagClass);
  if (((PASN_Choice*)choice)->CreateObject())
    return true;
  delete choice;

  choice = NULL;
  return false;
}


PObject * PRFC1155_ApplicationSyntax::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_ApplicationSyntax::Class()), PInvalidCast);
#endif
  return new PRFC1155_ApplicationSyntax(*this);
}



#ifndef PASN_NOPRINTON
const static PASN_Names Names_PRFC1155_NetworkAddress[]={
      {"internet",0}
};
#endif
//
// NetworkAddress
//

PRFC1155_NetworkAddress::PRFC1155_NetworkAddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Choice(tag, tagClass, 1, false
#ifndef PASN_NOPRINTON
    ,(const PASN_Names *)Names_PRFC1155_NetworkAddress,1
#endif
)
{
}


#if defined(__GNUC__) && __GNUC__ <= 2 && __GNUC_MINOR__ < 9
PRFC1155_NetworkAddress::operator PRFC1155_IpAddress &() const
#else
PRFC1155_NetworkAddress::operator PRFC1155_IpAddress &()
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_IpAddress), PInvalidCast);
#endif
  return *(PRFC1155_IpAddress *)choice;
}


PRFC1155_NetworkAddress::operator const PRFC1155_IpAddress &() const
#endif
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(PAssertNULL(choice), PRFC1155_IpAddress), PInvalidCast);
#endif
  return *(PRFC1155_IpAddress *)choice;
}


bool PRFC1155_NetworkAddress::CreateObject()
{
  switch (m_tag) {
    case e_internet :
      choice = new PRFC1155_IpAddress();
      return true;
  }

  choice = NULL;
  return false;
}


PObject * PRFC1155_NetworkAddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_NetworkAddress::Class()), PInvalidCast);
#endif
  return new PRFC1155_NetworkAddress(*this);
}


//
// IpAddress
//

PRFC1155_IpAddress::PRFC1155_IpAddress(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_OctetString(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 4);
}


PRFC1155_IpAddress::PRFC1155_IpAddress(const char * v)
{
  SetValue(v);
}


PRFC1155_IpAddress::PRFC1155_IpAddress(const PString & v)
{
  SetValue(v);
}


PRFC1155_IpAddress::PRFC1155_IpAddress(const PBYTEArray & v)
{
  SetValue(v);
}


PRFC1155_IpAddress & PRFC1155_IpAddress::operator=(const char * v)
{
  SetValue(v);
  return *this;
}


PRFC1155_IpAddress & PRFC1155_IpAddress::operator=(const PString & v)
{
  SetValue(v);
  return *this;
}


PRFC1155_IpAddress & PRFC1155_IpAddress::operator=(const PBYTEArray & v)
{
  SetValue(v);
  return *this;
}


PObject * PRFC1155_IpAddress::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_IpAddress::Class()), PInvalidCast);
#endif
  return new PRFC1155_IpAddress(*this);
}


//
// Counter
//

PRFC1155_Counter::PRFC1155_Counter(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Integer(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 0, 4294967295U);
}


PRFC1155_Counter & PRFC1155_Counter::operator=(int v)
{
  SetValue(v);
  return *this;
}


PRFC1155_Counter & PRFC1155_Counter::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * PRFC1155_Counter::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_Counter::Class()), PInvalidCast);
#endif
  return new PRFC1155_Counter(*this);
}


//
// Gauge
//

PRFC1155_Gauge::PRFC1155_Gauge(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Integer(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 0, 4294967295U);
}


PRFC1155_Gauge & PRFC1155_Gauge::operator=(int v)
{
  SetValue(v);
  return *this;
}


PRFC1155_Gauge & PRFC1155_Gauge::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * PRFC1155_Gauge::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_Gauge::Class()), PInvalidCast);
#endif
  return new PRFC1155_Gauge(*this);
}


//
// TimeTicks
//

PRFC1155_TimeTicks::PRFC1155_TimeTicks(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_Integer(tag, tagClass)
{
  SetConstraints(PASN_Object::FixedConstraint, 0, 4294967295U);
}


PRFC1155_TimeTicks & PRFC1155_TimeTicks::operator=(int v)
{
  SetValue(v);
  return *this;
}


PRFC1155_TimeTicks & PRFC1155_TimeTicks::operator=(unsigned v)
{
  SetValue(v);
  return *this;
}


PObject * PRFC1155_TimeTicks::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_TimeTicks::Class()), PInvalidCast);
#endif
  return new PRFC1155_TimeTicks(*this);
}


//
// Opaque
//

PRFC1155_Opaque::PRFC1155_Opaque(unsigned tag, PASN_Object::TagClass tagClass)
  : PASN_OctetString(tag, tagClass)
{
}


PRFC1155_Opaque::PRFC1155_Opaque(const char * v)
{
  SetValue(v);
}


PRFC1155_Opaque::PRFC1155_Opaque(const PString & v)
{
  SetValue(v);
}


PRFC1155_Opaque::PRFC1155_Opaque(const PBYTEArray & v)
{
  SetValue(v);
}


PRFC1155_Opaque & PRFC1155_Opaque::operator=(const char * v)
{
  SetValue(v);
  return *this;
}


PRFC1155_Opaque & PRFC1155_Opaque::operator=(const PString & v)
{
  SetValue(v);
  return *this;
}


PRFC1155_Opaque & PRFC1155_Opaque::operator=(const PBYTEArray & v)
{
  SetValue(v);
  return *this;
}


PObject * PRFC1155_Opaque::Clone() const
{
#ifndef PASN_LEANANDMEAN
  PAssert(IsClass(PRFC1155_Opaque::Class()), PInvalidCast);
#endif
  return new PRFC1155_Opaque(*this);
}


#endif // if ! H323_DISABLE_PRFC1155


// End of rfc1155.cxx
