/*
 * $Id: contain.inl,v 1.24 1994/10/30 11:50:27 robertj Exp $
 *
 * Portable Windows Library
 *
 * PContainer Inline Function Definitions
 *
 * Copyright 1993 Equivalence
 *
 * $Log: contain.inl,v $
 * Revision 1.24  1994/10/30 11:50:27  robertj
 * Split into Object classes and Container classes.
 * Changed mechanism for doing notification callback functions.
 *
 * Revision 1.23  1994/10/23  04:41:45  robertj
 * Added implemtation for PString constructor used by Clone().
 * Added PStringDictionary function.
 *
 * Revision 1.22  1994/07/27  05:58:07  robertj
 * Synchronisation.
 *
 * Revision 1.21  1994/07/25  03:31:00  robertj
 * Fixed missing PINLINEs.
 *
 * Revision 1.20  1994/07/17  10:46:06  robertj
 * Added string container functions for searching.
 *
 * Revision 1.19  1994/07/02  03:03:49  robertj
 * Addition of container searching facilities.
 *
 * Revision 1.18  1994/06/25  11:55:15  robertj
 * Unix version synchronisation.
 *
 * Revision 1.17  1994/04/20  12:17:44  robertj
 * assert stuff
 *
 * Revision 1.16  1994/04/01  14:05:46  robertj
 * Added PString specific containers.
 *
 * Revision 1.15  1994/03/07  07:45:40  robertj
 * Major upgrade
 *
 * Revision 1.14  1994/01/15  02:48:55  robertj
 * Rearranged PString assignment operator for NT portability.
 *
 * Revision 1.13  1994/01/13  08:42:29  robertj
 * Fixed missing copy constuctor and assignment operator for PString.
 *
 * Revision 1.12  1994/01/13  05:33:41  robertj
 * Added contructor to get caseless string from ordinary string.
 *
 * Revision 1.11  1994/01/03  04:42:23  robertj
 * Mass changes to common container classes and interactors etc etc etc.
 *
 * Revision 1.10  1993/12/31  06:48:46  robertj
 * Made inlines optional for debugging purposes.
 * Added PImgIcon class.
 *
 * Revision 1.9  1993/12/24  04:20:52  robertj
 * Mac CFront port.
 *
 * Revision 1.8  1993/12/22  05:54:08  robertj
 * Checked for severe out of memory condition in containers.
 *
 * Revision 1.7  1993/12/16  00:51:46  robertj
 * Made some container functions const.
 *
 * Revision 1.6  1993/12/15  21:10:10  robertj
 * Fixed reference system used by container classes.
 *
 * Revision 1.5  1993/08/27  18:17:47  robertj
 * Fixed bugs in PSortedList default size.
 *
 * Revision 1.4  1993/07/16  14:40:55  robertj
 * Added PString constructor for individual characters.
 * Added string to C style literal format.
 *
 * Revision 1.3  1993/07/14  12:49:16  robertj
 * Fixed RCS keywords.
 *
 */


///////////////////////////////////////////////////////////////////////////////

PINLINE PContainer::~PContainer()
  { Destruct(); }

PINLINE void PContainer::CloneContents(const PContainer *)
  { }

PINLINE void PContainer::CopyContents(const PContainer &)
  { }

PINLINE PINDEX PContainer::GetSize() const
  { return PAssertNULL(reference)->size; }

PINLINE BOOL PContainer::IsEmpty() const
  { return GetSize() == 0; }

PINLINE BOOL PContainer::IsUnique() const
  { return PAssertNULL(reference)->count <= 1; }


///////////////////////////////////////////////////////////////////////////////

PINLINE BOOL PAbstractArray::MakeUnique()
  { return IsUnique() || SetSize(GetSize()); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PString::PString()
  : PCharArray(1) { }

PINLINE PString::PString(const PString & str)
  : PCharArray(str) { }

PINLINE PString::PString(const PString * str)
  : PCharArray(*str) { }

PINLINE PString::PString(const char * cstr)
  : PCharArray(strlen(PAssertNULL(cstr))+1) { strcpy(theArray, cstr); }

PINLINE PString::PString(const char * cstr, PINDEX len)
  : PCharArray(len+1) { memcpy(theArray, PAssertNULL(cstr), len); }

PINLINE PString::PString(char c)
  : PCharArray(2) { *theArray = c; }

PINLINE PObject::Comparison PString::CompareString(const char * cstr) const
  { return (Comparison)strcmp(theArray,PAssertNULL(cstr)); }

PINLINE BOOL PString::MakeMinimumSize()
  { return SetSize(strlen(theArray)+1); }

PINLINE PINDEX PString::GetLength() const
  { return strlen(theArray); }

PINLINE PString & PString::operator=(const PString & str)
  { PCharArray::operator=(str); return *this; }

PINLINE PString PString::operator+(const PString & str) const
  { return operator+((const char *)str); }

PINLINE PString operator+(const char * cstr, const PString & str)
  { return PString(cstr) + str; }
  
PINLINE PString & PString::operator+=(const PString & str)
  { return operator+=((const char *)str); }

PINLINE BOOL PString::operator==(const PObject & obj) const
  { return PObject::operator==(obj); }

PINLINE BOOL PString::operator!=(const PObject & obj) const
  { return PObject::operator!=(obj); }

PINLINE BOOL PString::operator<(const PObject & obj) const
  { return PObject::operator<(obj); }

PINLINE BOOL PString::operator>(const PObject & obj) const
  { return PObject::operator>(obj); }

PINLINE BOOL PString::operator<=(const PObject & obj) const
  { return PObject::operator<=(obj); }

PINLINE BOOL PString::operator>=(const PObject & obj) const
  { return PObject::operator>=(obj); }

PINLINE BOOL PString::operator==(const char * cstr) const
  { return CompareString(cstr) == EqualTo; }

PINLINE BOOL PString::operator!=(const char * cstr) const
  { return CompareString(cstr) != EqualTo; }

PINLINE BOOL PString::operator<(const char * cstr) const
  { return CompareString(cstr) == LessThan; }

PINLINE BOOL PString::operator>(const char * cstr) const
  { return CompareString(cstr) == GreaterThan; }

PINLINE BOOL PString::operator<=(const char * cstr) const
  { return CompareString(cstr) != GreaterThan; }

PINLINE BOOL PString::operator>=(const char * cstr) const
  { return CompareString(cstr) != LessThan; }

PINLINE PString::operator const unsigned char *() const
  { return (const unsigned char *)theArray; }

PINLINE PString & PString::vsprintf(const PString & fmt, va_list args)
  { return vsprintf((const char *)fmt, args); }

PINLINE PString pvsprintf(const PString & fmt, va_list args)
  { return pvsprintf((const char *)fmt, args); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PCaselessString::PCaselessString()
  : PString() { }

PINLINE PCaselessString::PCaselessString(const char * cstr)
  : PString(cstr) { }

PINLINE PCaselessString::PCaselessString(const PString & str)
  : PString(str) { }

PINLINE PCaselessString & PCaselessString::operator=(const PString & str)
  { PString::operator=(str); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PStringStreamBuffer::PStringStreamBuffer(const PStringStreamBuffer & s)
  : string(s.string) { }

PINLINE PStringStreamBuffer &
                  PStringStreamBuffer::operator=(const PStringStreamBuffer & s)
  { string = s.string; return *this; }

PINLINE PStringStreamBuffer::PStringStreamBuffer(PStringStream * str)
  : string(PAssertNULL(str)) { sync(); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PCollection::PCollection(PINDEX initialSize)
  : PContainer(initialSize) { }

PINLINE PCollection::PCollection(const PCollection * c)
  : PContainer(c) { }

PINLINE void PCollection::AllowDeleteObjects(BOOL yes)
  { reference->deleteObjects = yes; }

PINLINE void PCollection::DisallowDeleteObjects()
  { AllowDeleteObjects(FALSE); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PArrayObjects::PArrayObjects(PINDEX initialSize)
  : theArray(PNEW ObjPtrArray(initialSize)) { }

PINLINE void PArrayObjects::CopyContents(const PArrayObjects & array)
  { theArray = array.theArray; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PStringArray::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractList::PAbstractList()
  : info(new ListInfo) { PAssertNULL(info); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PStringList::AppendString(const PString & str)
  { return Append(PNEW PString(str)); }

PINLINE PINDEX PStringList::InsertString(
                                   const PString & before, const PString & str)
  { return Insert(before, PNEW PString(str)); }

PINLINE PINDEX PStringList::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractSortedList::PAbstractSortedList()
  : info(new SortedListInfo) { PAssertNULL(info); }

PINLINE void PSortedListElement::MakeBlack()
  { colour = Black; }

PINLINE void PSortedListElement::MakeRed()
  { colour = Red; }

PINLINE BOOL PSortedListElement::IsBlack()
  { return colour == Black; }

PINLINE BOOL PSortedListElement::IsLeftBlack()
  { return left == NULL || left->colour == Black; }

PINLINE BOOL PSortedListElement::IsRightBlack()
  { return right == NULL || right->colour == Black; }

PINLINE BOOL PSortedListElement::LeftTreeSize()
  { return left != NULL ? left->subTreeSize : 0; }

PINLINE BOOL PSortedListElement::RightTreeSize()
  { return right != NULL ? right->subTreeSize : 0; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PINDEX PSortedStringList::AppendString(const PString & str)
  { return Append(PNEW PString(str)); }

PINLINE PINDEX PSortedStringList::InsertString(
                                   const PString & before, const PString & str)
  { return Insert(before, PNEW PString(str)); }

PINLINE PINDEX PSortedStringList::GetStringsIndex(const PString & str) const
  { return GetValuesIndex(str); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PScalarKey::PScalarKey(PINDEX newKey)
  : theKey(newKey) { }

PINLINE PScalarKey::operator PINDEX() const
  { return theKey; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractSet::PAbstractSet()
  { hashTable->deleteKeys = reference->deleteObjects; }
  
PINLINE BOOL PAbstractSet::Contains(const PObject & key)
  { return hashTable->GetElementAt(key) != NULL; }


PINLINE PStringSet::PStringSet()
  : PAbstractSet() { }

PINLINE PStringSet::PStringSet(const PStringSet * c)
  : PAbstractSet(c) { }

PINLINE PObject * PStringSet::Clone() const
  { return PNEW PStringSet(this); }

PINLINE void PStringSet::Include(const PString & key)
  { Append(PNEW PString(key)); }

PINLINE void PStringSet::Exclude(const PString & key)
  { Remove(&key); }

PINLINE BOOL PStringSet::operator[](const PString & key)
  { return Contains(key); }

PINLINE const PString & PStringSet::GetKeyAt(PINDEX index) const
  { return (const PString &)AbstractGetKeyAt(index); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PAbstractDictionary::PAbstractDictionary()
  { hashTable->deleteKeys = TRUE; }
  
PINLINE PAbstractDictionary::PAbstractDictionary(const PAbstractDictionary * c)
  : PHashTable(c) { }


PINLINE PStringDictionary::PStringDictionary()
  : PAbstractDictionary() { }

PINLINE PStringDictionary::PStringDictionary(const PStringDictionary * c)
  : PAbstractDictionary(c) { }

PINLINE PObject * PStringDictionary::Clone() const
  { return PNEW PStringDictionary(this); }

PINLINE BOOL PStringDictionary::SetAt(const PObject & key, PString str)
  { return PAbstractDictionary::SetAt(key, PNEW PString(str)); }

PINLINE PString & PStringDictionary::operator[](const PString & key) const
  { return (PString &)GetRefAt(key); }

PINLINE const PString & PStringDictionary::GetKeyAt(PINDEX index) const
  { return (const PString &)AbstractGetKeyAt(index); }

PINLINE PString & PStringDictionary::GetDataAt(PINDEX index) const
  { return (PString &)AbstractGetDataAt(index); }

PINLINE BOOL PStringDictionary::SetDataAt(PINDEX index, const PString & str)
  { return PAbstractDictionary::SetDataAt(index, PNEW PString(str)); }


// End Of File ///////////////////////////////////////////////////////////////
