/*
 * pstring.h
 *
 * Character string class.
 *
 * Portable Tools Library
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
 * The Original Code is Portable Tools Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */


#include <ptlib/bitwise_enum.h> /// Note must be before #ifndef PTLIB_STRING_H

#ifndef PTLIB_STRING_H
#define PTLIB_STRING_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <string>
#include <vector>
#include <ptlib/array.h>


///////////////////////////////////////////////////////////////////////////////
// PString class

class PStringArray;
class PRegularExpression;
class PString;

/**The same as the standard C snprintf(fmt, 1000, ...), but returns a
   PString instead of a const char *.
 */
PString psprintf(
  const char * fmt, ///< printf style format string
  ...
);

/**The same as the standard C vsnprintf(fmt, 1000, va_list arg), but
   returns a PString instead of a const char *.
 */
PString pvsprintf(
  const char * fmt, ///< printf style format string
  va_list arg       ///< Arguments for formatting
);


/// Wide character array, based on standard wchar_t
typedef PBaseArray<wchar_t> PWCharArray;


/**The character string class. It supports a wealth of additional functions
   for string processing and conversion. Operators are provided so that
   strings can virtually be treated as a basic type.

   An important feature of the string class, which is not present in other
   container classes, is that when the string contents is changed, that is
   resized or elements set, the string is "dereferenced", and a duplicate
   made of its contents. That is this instance of the array is disconnected
   from all other references to the string data, if any, and a new string array
   contents created. For example consider the following:
<pre><code>
          PString s1 = "String"; // New array allocated and set to "String"
          PString s2 = s1;       // s2 has pointer to same array as s1
                                 // and reference count is 2 for both
          s1[0] = 's';           // Breaks references into different strings
</code></pre>
   at the end s1 is "string" and s2 is "String" both with reference count of 1.

   The functions that will "break" a reference are <code>SetSize()</code>,
   <code>SetMinSize()</code>, <code>GetPointer()</code>, <code>SetAt()</code> and
   <code>operator[]</code>.

   Note that the array is a '\\0' terminated string as in C strings. Thus the
   memory allocated, and the length of the string may be different values.

   Also note that the PString is inherently an 8 bit string. The character set
   is generally defined as UTF-8, though this is only relevant when converting
   to or from wchar_t or PWideString. The format of a wchar_t based string is
   platform dependent. For Windows this is UTF-16. For GNU base builds
   (typically Linux) this is UCS-4, 32 bit characters.
 */

class PString : public PObject, public std::string
{
  PCLASSINFO(PString, PObject);
public:
  typedef const char * Initialiser;

  /**@name Construction */
  //@{
    /**Construct an empty string. This will have one character in it which is
       the '\\0' character.
     */
    PString();

    /**Create a new reference to the specified string. The string memory is not
       copied, only the pointer to the data.
     */
    PString(
      const PString & str  ///< String to create new reference to.
    );

    /**Create a new reference to the specified buffer. The string memory is not
       copied, only the pointer to the data.
     */
    PString(
      const PCharArray & buf  ///< Buffer to create new reference to.
    );

    /**Create a new string based on the uint8_t array.
       Note it is up to the caller to assure the semantics of a binary object
       such as PBYTEArray are suitable to be used as a string.
     */
    PString(
      const PBYTEArray & buf  ///< Buffer to create new reference to.
    );

    /**Create a new string from the specified std::string
     */
    PString(
      const std::string & str
    );

    /**Create a string from the C string array. This is most commonly used with
       a literal string, eg "hello". A new memory block is allocated of a size
       sufficient to take the length of the string and its terminating
       '\\0' character.
     */
    PString(
      const char * cstr ///< Standard '\\0' terminated C string.
    );

    /**Create a string from the wchar_t string array.
       A new memory block is allocated of a size sufficient to take the length
       of the string and its terminating '\\0' character.
     */
    PString(
      const wchar_t * ustr ///< wchar_t null terminated string.
    );

        /**Create a string from the array. A new memory block is allocated of
           a size equal to <code>len</code> plus one which is sufficient to take
           the string and a terminating '\\0' character.

           Note that this function will allow a string with embedded '\\0'
           characters to be created, but most of the functions here will be unable
           to access characters beyond the first '\\0'. Furthermore, if the
           <code>MakeMinimumSize()</code> function is called, all data beyond that first
           '\\0' character will be lost.
         */
    PString(
      const char * cstr,  ///< Pointer to a string of characters.
      PINDEX len          ///< Length of the string in bytes.
    );

    /**Create a UTF-8 string from the wchar_t array. A new memory block is
       allocated of a size sufficient to take the converted string and a
       terminating '\\0' character.

           Note that this function will allow a string with embedded '\\0'
           characters to be created, but most of the functions here will be unable
           to access characters beyond the first '\\0'. Furthermore, if the
           <code>MakeMinimumSize()</code> function is called, all data beyond that first
           '\\0' character will be lost.
         */
    PString(
      const wchar_t * ustr,  ///< Pointer to a string of wchar_t characters.
      PINDEX len             ///< Length of the string in characters.
    );

    /**Create a UTF-8 string from the wchar_t array. A new memory block is
       allocated of a size sufficient to take the converted string and a
       terminating '\\0' character.

       Note that this function will allow a string with embedded '\\0'
       characters to be created, but most of the functions here will be unable
       to access characters beyond the first '\\0'. Furthermore, if the
       <code>MakeMinimumSize()</code> function is called, all data beyond that first
       '\\0' character will be lost.
     */
    PString(
      const PWCharArray & ustr ///< wchar_t null terminated string.
    );

        /**Create a string from the single character. This is most commonly used
           as a type conversion constructor when a literal character, eg 'A' is
           used in a string expression. A new memory block is allocated of two
           characters to take the char and its terminating '\\0' character.
         */
    PString(
      char ch    ///< Single character to initialise string.
    );

    /**Create a string from the integer type.
       This will create a simple base 10, shortest length conversion of the
       integer (with sign character if appropriate) into the string.
      */
    PString(
      short n   ///< Integer to convert
    );

    /**Create a string from the integer type.
       This will create a simple base 10, shortest length conversion of the
       integer (with sign character if appropriate) into the string.
      */
    PString(
      unsigned short n   ///< Integer to convert
    );

    /**Create a string from the integer type.
       This will create a simple base 10, shortest length conversion of the
       integer (with sign character if appropriate) into the string.
      */
    PString(
      int n   ///< Integer to convert
    );

    /**Create a string from the integer type.
       This will create a simple base 10, shortest length conversion of the
       integer (with sign character if appropriate) into the string.
      */
    PString(
      unsigned int n   ///< Integer to convert
    );

    /**Create a string from the integer type.
       This will create a simple base 10, shortest length conversion of the
       integer (with sign character if appropriate) into the string.
      */
    PString(
      long n   ///< Integer to convert
    );

    /**Create a string from the integer type.
       This will create a simple base 10, shortest length conversion of the
       integer (with sign character if appropriate) into the string.
      */
    PString(
      unsigned long n   ///< Integer to convert
    );

    #ifdef HAVE_LONG_LONG_INT
        /**Create a string from the integer type.
           This will create a simple base 10, shortest length conversion of the
           integer (with sign character if appropriate) into the string.
          */
    PString(
      long long n   ///< Integer to convert
    );
    #endif

    #ifdef HAVE_UNSIGNED_LONG_LONG_INT
        /**Create a string from the integer type.
           This will create a simple base 10, shortest length conversion of the
           integer (with sign character if appropriate) into the string.
          */
    PString(
      unsigned long long n   ///< Integer to convert
    );
    #endif


        /* Type of conversion to make in the conversion constructors.
         */
    enum ConversionType {
      Pascal,   ///< Data is a length byte followed by characters.
      Basic,    ///< Data is two length bytes followed by characters.
      Literal,  ///< Data is C language style string with \\ escape codes.
      Signed,   ///< Convert a signed integer to a string.
      Unsigned, ///< Convert an unsigned integer to a string.
      Decimal,  ///< Convert a real number to a string in decimal format.
      Exponent, ///< Convert a real number to a string in exponent format.
      ScaleSI,  ///< Output scaled to SI units, e.g. k, M, G.
      Printf,   ///< Formatted output, sprintf() style function.
      NumConversionTypes
    };

    /* Contruct a new string converting from the spcified data source into
       a string array.
     */
    PString(
      ConversionType type,  ///< Type of data source for conversion.
      const char * str,    ///< String to convert.
      ...                 ///< Extra parameters for <code>sprintf()</code> call.
    );

    #ifdef HAVE_LONG_LONG_INT
    PString(
      ConversionType type, ///< Type of data source for conversion.
      long long value,     ///< Large integer value to convert.
      unsigned param = 10  /**< For Signed/Unsigned, this is number base
                                to use for the integer conversion.
                                For ScaleSI, this is the number of significant
                                figures to be used, maiximum 4. */
    );
    #endif
    #ifdef HAVE_UNSIGNED_LONG_LONG_INT
    PString(ConversionType type, unsigned long long value, unsigned param = 10);
    #endif

    PString(ConversionType type, unsigned long  value, unsigned param = 10);
    PString(ConversionType type,   signed long  value, unsigned param = 10);
    PString(ConversionType type, unsigned int   value, unsigned param = 10);
    PString(ConversionType type,   signed int   value, unsigned param = 10);
    PString(ConversionType type, unsigned short value, unsigned param = 10);
    PString(ConversionType type,   signed short value, unsigned param = 10);
    PString(ConversionType type, unsigned char  value, unsigned param = 10);
    PString(ConversionType type,   signed char  value, unsigned param = 10);

    /* Contruct a new string converting from the spcified data source into
       a string array.
     */
    PString(
      ConversionType type,  ///< Type of data source for conversion.
      double value,         ///< Floating point value to convert.
      unsigned places       ///< Number of decimals in real number output.
    );

    /**Assign the string to the current object. The current instance then
       becomes another reference to the same string in the <code>str</code>
       parameter.

       @return
       reference to the current PString object.
     */
    PString & operator=(
      const PString & str  ///< New string to assign.
      );

      /**Assign the string to the current object. The current instance then
         becomes another reference to the same string in the <code>str</code>
         parameter.

         @return
         reference to the current PString object.
       */
    PString & operator=(
      const std::string & str  ///< New string to assign.
    );

    /**Assign the C string to the current object. The current instance then
       becomes a unique reference to a copy of the <code>cstr</code> parameter.
       The <code>cstr</code> parameter is typically a literal string, eg:
       <pre><code>
            myStr = "fred";
       </code></pre>
       @return
         reference to the current PString object.
      */
    PString & operator=(
      const char * cstr  ///< C string to assign.
    );

    /**Assign the character to the current object. The current instance then
       becomes a unique reference to a copy of the character parameter. eg:
       <pre><code>
            myStr = 'A';
       </code></pre>
       @return
         reference to the current PString object.
      */
    PString & operator=(
      char ch            ///< Character to assign.
    );

    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      short n   ///< Integer to convert
    );

    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      unsigned short n   ///< Integer to convert
    );

    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      int n   ///< Integer to convert
    );

    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      unsigned int n   ///< Integer to convert
    );

    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      long n   ///< Integer to convert
    );

    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      unsigned long n   ///< Integer to convert
    );

#ifdef HAVE_LONG_LONG_INT
    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      long long n   ///< Integer to convert
    );
#endif

#ifdef HAVE_UNSIGNED_LONG_LONG_INT
    /**Assign a string from the integer type.
        This will create a simple base 10, shortest length conversion of the
        integer (with sign character if appropriate) into the string.
      */
    PString & operator=(
      unsigned long long n   ///< Integer to convert
    );
#endif

    /**Return an empty string.
      */
    static const PString & Empty();
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Make a complete duplicate of the string. Note that the data in the
       array of characters is duplicated as well and the new object is a
       unique reference to that data.
     */
    virtual PObject * Clone() const;

    /**Get the relative rank of the two strings. The system standard function,
       eg strcmp(), is used.

       @return
       comparison of the two objects, <code>EqualTo</code> for same,
       <code>LessThan</code> for <code>obj</code> logically less than the
       object and <code>GreaterThan</code> for <code>obj</code> logically
       greater than the object.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Other PString to compare against.
    ) const;

    /**Output the string to the specified stream.
     */
    virtual void PrintOn(
      ostream & strm  ///< I/O stream to output to.
    ) const;

    /**Input the string from the specified stream. This will read all
       characters until a end of line is reached. The end of line itself is
       <b>not</b> placed in the string, however it <b>is</b> removed from
       the stream.
     */
    virtual void ReadFrom(
      istream & strm  ///< I/O stream to input from.
    );

    /**Calculate a hash value for use in sets and dictionaries.

       The hash function for strings will produce a value based on the sum of
       the first three characters of the string. This is a fairly basic
       function and make no assumptions about the string contents. A user may
       descend from PString and override the hash function if they can take
       advantage of the types of strings being used, eg if all strings start
       with the letter 'A' followed by 'B or 'C' then the current hash function
       will not perform very well.

       @return
       hash value for string.
     */
    virtual PINDEX HashFunction() const;
  //@}

  /**@name Size/Length functions */
  //@{
    /**Set the actual memory block array size to the minimum required to hold
       the current string contents.
       
       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.

       Also, this will ignore the m_length variable and redetermine the length
       of teh string using strlen(), resetting the m_length.

       @return
       true if new memory block successfully re-allocated.
     */
    bool MakeMinimumSize(
      PINDEX newLength = 0  /// New length for string, if zero strlen is used
    );
  //@}

  /**@name Concatenation operators **/
  //@{
    /**Concatenate two strings to produce a third. The original strings are
       not modified, an entirely new unique reference to a string is created.
       
       @return
       new string with concatenation of the object and parameter.
     */
    PString operator+(
      const PString & str   ///< String to concatenate.
    ) const;

    /**Concatenate a C string to a PString to produce a third. The original
       string is not modified, an entirely new unique reference to a string
       is created. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          myStr = aStr + "fred";
</code></pre>

       @return
       new string with concatenation of the object and parameter.
     */
    PString operator+(
      const char * cstr  ///< C string to concatenate.
    ) const;

    /**Concatenate a single character to a PString to produce a third. The
       original string is not modified, an entirely new unique reference to a
       string is created. The <code>ch</code> parameter is typically a
       literal, eg:
<pre><code>
          myStr = aStr + '!';
</code></pre>

       @return
       new string with concatenation of the object and parameter.
     */
    PString operator+(
      char ch   ///< Character to concatenate.
    ) const;

    /**Concatenate a PString to a C string to produce a third. The original
       string is not modified, an entirely new unique reference to a string
       is created. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          myStr = "fred" + aStr;
</code></pre>

       @return
       new string with concatenation of the object and parameter.
     */
    friend PString operator+(
      const char * cstr,    ///< C string to be concatenated to.
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a PString to a single character to produce a third. The
       original string is not modified, an entirely new unique reference to a
       string is created. The <code>ch</code> parameter is typically a literal,
       eg:
<pre><code>
          myStr = '!' + aStr;
</code></pre>

       @return
       new string with concatenation of the object and parameter.
     */
    friend PString operator+(
      char  ch,             ///< Character to be concatenated to.
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a string to another string, modifiying that string.

       @return
       reference to string that was concatenated to.
     */
    PString & operator+=(
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a C string to a PString, modifiying that string. The
       <code>cstr</code> parameter is typically a literal string, eg:
<pre><code>
          myStr += "fred";
</code></pre>

       @return
       reference to string that was concatenated to.
     */
    PString & operator+=(
      const char * cstr  ///< C string to concatenate.
    );

    /**Concatenate a single character to a PString. The <code>ch</code>
       parameter is typically a literal, eg:
<pre><code>
          myStr += '!';
</code></pre>

       @return
       new string with concatenation of the object and parameter.
     */
    PString & operator+=(
      char ch   ///< Character to concatenate.
    );


    /**Concatenate a PString to another PString to produce a third. The original
       string is not modified, an entirely new unique reference to a string
       is created

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       new string with concatenation of the object and parameter.
     */
    PString operator&(
      const PString & str   ///< String to concatenate.
    ) const;

    /**Concatenate a C string to a PString to produce a third. The original
       string is not modified, an entirely new unique reference to a string
       is created. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          myStr = aStr & "fred";
</code></pre>

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       new string with concatenation of the object and parameter.
     */
    PString operator&(
      const char * cstr  ///< C string to concatenate.
    ) const;

    /**Concatenate a single character to a PString to produce a third. The
       original string is not modified, an entirely new unique reference to a
       string is created. The <code>ch</code> parameter is typically a
       literal, eg:
<pre><code>
          myStr = aStr & '!';
</code></pre>

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       new string with concatenation of the object and parameter.
     */
    PString operator&(
      char ch   ///< Character to concatenate.
    ) const;

    /**Concatenate a PString to a C string to produce a third. The original
       string is not modified, an entirely new unique reference to a string
       is created. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          myStr = "fred" & aStr;
</code></pre>

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       new string with concatenation of the object and parameter.
     */
    friend PString operator&(
      const char * cstr,    ///< C string to be concatenated to.
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a PString to a single character to produce a third. The
       original string is not modified, an entirely new unique reference to a
       string is created. The <code>c</code> parameter is typically a literal,
       eg:
<pre><code>
          myStr = '!' & aStr;
</code></pre>

       This function differs from <code>operator+</code> in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       new string with concatenation of the object and parameter.
     */
    friend PString operator&(
      char  ch,              ///< Character to be concatenated to.
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a PString to a PString, modifiying that string.

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       reference to string that was concatenated to.
     */
    PString & operator&=(
      const PString & str   ///< String to concatenate.
    );

    /**Concatenate a C string to a PString, modifiying that string. The
       <code>cstr</code> parameter is typically a literal string, eg:
<pre><code>
          myStr &= "fred";
</code></pre>

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       reference to string that was concatenated to.
     */
    PString & operator&=(
      const char * cstr  ///< C string to concatenate.
    );


    /**Concatenate a character to a PString, modifiying that string. The
       <code>ch</code> parameter is typically a literal string, eg:
<pre><code>
          myStr &= '!';
</code></pre>

       This function differes from operator+ in that it assures there is at
       least one space between the strings. Exactly one space is added if
       there is not a space at the end of the first or beggining of the last
       string.

       @return
       reference to string that was concatenated to.
     */
    PString & operator&=(
      char ch  ///< Character to concatenate.
    );
  //@}


  /**@name Comparison operators */
  //@{
    /**Compare two strings using case insensitive comparison.

       @return
       true if equal.
     */
    bool operator*=(
      const PString & str  ///< PString object to compare against.
    ) const;

    /**Compare two strings using the <code>PObject::Compare()</code> function. This
       is identical to the <code>PObject</code> class function but is necessary due
       to other overloaded versions.

       @return
       true if equal.
     */
    bool operator==(
      const PString & str  ///< PString object to compare against.
    ) const;

    /**Compare two strings using the <code>PObject::Compare()</code> function. This
       is identical to the <code>PObject</code> class function but is necessary due
       to other overloaded versions.

       @return
       true if not equal.
     */
    bool operator!=(
      const PString & str  ///< PString object to compare against.
    ) const;

    /**Compare two strings using the <code>PObject::Compare()</code> function. This
       is identical to the <code>PObject</code> class function but is necessary due
       to other overloaded versions.

       @return
       true if less than.
     */
    bool operator<(
      const PString & str  ///< PString object to compare against.
    ) const;

    /**Compare two strings using the <code>PObject::Compare()</code> function. This
       is identical to the <code>PObject</code> class function but is necessary due
       to other overloaded versions.

       @return
       true if greater than.
     */
    bool operator>(
      const PString & str  ///< PString object to compare against.
    ) const;

    /**Compare two strings using the <code>PObject::Compare()</code> function. This
       is identical to the <code>PObject</code> class function but is necessary due
       to other overloaded versions.

       @return
       true if less than or equal.
     */
    bool operator<=(
      const PString & str  ///< PString object to compare against.
    ) const;

    /**Compare two strings using the <code>PObject::Compare()</code> function. This
       is identical to the <code>PObject</code> class function but is necessary due
       to other overloaded versions.

       @return
       true if greater than or equal.
     */
    bool operator>=(
      const PString & str  ///< PString object to compare against.
    ) const;


    /**Compare a PString to a C string using a case insensitive compare
       function. The <code>cstr</code> parameter is typically a literal string,
       eg:
<pre><code>
          if (myStr *= "fred")
</code></pre>

       @return
       true if equal.
     */
    bool operator*=(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using the <code>Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal string,
       eg:
<pre><code>
          if (myStr == "fred")
</code></pre>

       @return
       true if equal.
     */
    bool operator==(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if (myStr != "fred")
</code></pre>

       @return
       true if not equal.
     */
    bool operator!=(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if (myStr < "fred")
</code></pre>

       @return
       true if less than.
     */
    bool operator<(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if (myStr > "fred")
</code></pre>

       @return
       true if greater than.
     */
    bool operator>(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if (myStr <= "fred")
</code></pre>

       @return
       true if less than or equal.
     */
    bool operator<=(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if (myStr >= "fred")
</code></pre>

       @return
       true if greater than or equal.
     */
    bool operator>=(
      const char * cstr  ///< C string to compare against.
    ) const;

    /**Compare a PString to a C string using a case insensitive compare
       function. The <code>cstr</code> parameter is typically a literal string,
       eg:
<pre><code>
          if ("fred" *= myStr)
</code></pre>

       @return
       true if equal.
     */
    friend bool operator*=(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a PString to a C string using the <code>Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal string,
       eg:
<pre><code>
          if ("fred" == myStr)
</code></pre>

       @return
       true if equal.
     */
    friend bool operator==(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if ("fred" != myStr)
</code></pre>

       @return
       true if not equal.
     */
    friend bool operator!=(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if ("fred" < myStr)
</code></pre>

       @return
       true if less than.
     */
    friend bool operator<(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if ("fred" > myStr)
</code></pre>

       @return
       true if greater than.
     */
    friend bool operator>(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if ("fred" <= myStr)
</code></pre>

       @return
       true if less than or equal.
     */
    friend bool operator<=(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a PString to a C string using the <code>PObject::Compare()</code>
       function. The <code>cstr</code> parameter is typically a literal
       string, eg:
<pre><code>
          if ("fred" >= myStr)
</code></pre>

       @return
       true if greater than or equal.
     */
    friend bool operator>=(
      const char * cstr,  ///< C string to compare against.
      const PString & str ///< String to compare against
    );

    /**Compare a string against a substring of the object.
       This will compare at most <code>count</code> characters of the string, starting at
       the specified <code>offset</code>, against that many characters of the <code>str</code>
       parameter. If <code>count</code> greater than the length of the <code>str</code> parameter
       then the actual length of <code>str</code> is used. If <code>count</code> and the length of
       <code>str</code> are greater than the length of the string remaining from the
       <code>offset</code> then false is returned.

       @return
       true if str is a substring of .
     */
    Comparison NumCompare(
      const PString & str,        ///< PString object to compare against.
      PINDEX count = P_MAX_INDEX, ///< Number of chacracters in str to compare
      PINDEX offset = 0           ///< Offset into string to compare
    ) const;

    /**Compare a string against a substring of the object.
       This will compare at most <code>count</code> characters of the string, starting at
       the specified <code>offset</code>, against that many characters of the <code>str</code>
       parameter. If <code>count</code> greater than the length of the <code>str</code> parameter
       then the actual length of <code>str</code> is used. If <code>count</code> and the length of
       <code>str</code> are greater than the length of the string remaining from the
       <code>offset</code> then false is returned.

       @return
       true if str is a substring of .
     */
    Comparison NumCompare(
      const char * cstr,          ///< C string object to compare against.
      PINDEX count = P_MAX_INDEX, ///< Number of chacracters in str to compare
      PINDEX offset = 0           ///< Offset into string to compare
    ) const;
  //@}


  /**@name Search & replace functions */
  //@{
    /** Locate the position within the string of the character. */
    PINDEX Find(
      char ch,              ///< Character to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /** Locate the position within the string of the substring. */
    PINDEX Find(
      const PString & str,  ///< String to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /* Locate the position within the string of the character or substring. The
       search will begin at the character offset provided.
       
       If <code>offset</code> is beyond the length of the string, then the
       function will always return P_MAX_INDEX.
       
       The matching will be for identical character or string. If a search
       ignoring case is required then the string should be converted to a
       PCaselessString before the search is made.

       @return
       position of character or substring in the string, or P_MAX_INDEX if the
       character or substring is not in the string.
     */
    PINDEX Find(
      const char * cstr,    ///< C string to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /** Locate the position of the last matching character. */
    PINDEX FindLast(
      char ch,                     ///< Character to search for in string.
      PINDEX offset = P_MAX_INDEX  ///< Offset into string to begin search.
    ) const;

    /** Locate the position of the last matching substring. */
    PINDEX FindLast(
      const PString & str,         ///< String to search for in string.
      PINDEX offset = P_MAX_INDEX  ///< Offset into string to begin search.
    ) const;

    /**Locate the position of the last matching substring.
       Locate the position within the string of the last matching character or
       substring. The search will begin at the character offset provided,
       moving backward through the string.

       If <code>offset</code> is beyond the length of the string, then the
       search begins at the end of the string. If <code>offset</code> is zero
       then the function always returns P_MAX_INDEX.

       The matching will be for identical character or string. If a search
       ignoring case is required then the string should be converted to a
       PCaselessString before the search is made.

       @return
       position of character or substring in the string, or P_MAX_INDEX if the
       character or substring is not in the string.
     */
    PINDEX FindLast(
      const char * cstr,           ///< C string to search for in string.
      PINDEX offset = P_MAX_INDEX  ///< Offset into string to begin search.
    ) const;

    /** Locate the position of one of the characters in the set. */
    PINDEX FindOneOf(
      const PString & set,  ///< String of characters to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /**Locate the position of one of the characters in the set.
       The search will begin at the character offset provided.

       If <code>offset</code> is beyond the length of the string, then the
       function will always return P_MAX_INDEX.
       
       The matching will be for identical character or string. If a search
       ignoring case is required then the string should be converted to a
       PCaselessString before the search is made.

       @return
       position of character in the string, or P_MAX_INDEX if no characters
       from the set are in the string.
     */
    PINDEX FindOneOf(
      const char * cset,    ///< C string of characters to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /** Locate the position of character not in the set. */
    PINDEX FindSpan(
      const PString & set,  ///< String of characters to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /**Locate the position of character not in the set.
       The search will begin at the character offset provided.

       If <code>offset</code> is beyond the length of the string, or every character
       in the string is a member of the set, then the function will always
       return P_MAX_INDEX.
       
       The matching will be for identical character or string. If a search
       ignoring case is required then the string should be converted to a
       PCaselessString before the search is made.

       @return
       position of character not in the set, or P_MAX_INDEX if all characters
       from the string are in the set.
     */
    PINDEX FindSpan(
      const char * cset,    ///< C string of characters to search for in string.
      PINDEX offset = 0     ///< Offset into string to begin search.
    ) const;

    /**Locate the position within the string of one of the regular expression.
       The search will begin at the character offset provided.

       If <code>offset</code> is beyond the length of the string, then the
       function will always return P_MAX_INDEX.
       
       @return
       position of regular expression in the string, or P_MAX_INDEX if no
       characters from the set are in the string.
     */
    PINDEX FindRegEx(
      const PRegularExpression & regex, ///< regular expression to find
      PINDEX offset = 0                 ///< Offset into string to begin search.
    ) const;

    /**Locate the position within the string of one of the regular expression.
       The search will begin at the character offset provided.

       If <code>offset</code> is beyond the length of the string, then the
       function will always return P_MAX_INDEX.
       
       @return
       position of regular expression in the string, or P_MAX_INDEX if no
       characters from the set are in the string.
     */
    bool FindRegEx(
      const PRegularExpression & regex, ///< regular expression to find
      PINDEX & pos,                     ///< Position of matched expression
      PINDEX & len,                     ///< Length of matched expression
      PINDEX offset = 0,                ///< Offset into string to begin search.
      PINDEX maxPos = P_MAX_INDEX       ///< Maximum offset into string
    ) const;


    /**Return true if the entire string matches the regular expression
     */
    bool MatchesRegEx(
      const PRegularExpression & regex ///< regular expression to match
    ) const;

    /**Locate the substring within the string and replace it with the specifed
       substring. The search will begin at the character offset provided.

       If <code>offset</code> is beyond the length of the string, then the
       function will do nothing.

       The matching will be for identical character or string. If a search
       ignoring case is required then the string should be converted to a
       PCaselessString before the search is made.
     */
    PString & Replace(
      const PString & target,   ///< Text to be removed.
      const PString & subs,     ///< String to be inserted into the gaps created
      bool all = false,         ///< Replace all occurrences of target text.
      PINDEX offset = 0         ///< Offset into string to begin search.
    );

    /**Splice the string into the current string at the specified position. The
       specified number of bytes are removed from the string.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
     */
    PString & Splice(
      const PString & str,  ///< Substring to insert.
      PINDEX pos,           ///< Position in string to insert the substring.
      PINDEX len = 0        ///< Length of section to remove.
    );

    /**Splice the string into the current string at the specified position. The
       specified number of bytes are removed from the string.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
     */
    PString & Splice(
      const char * cstr,    ///< Substring to insert.
      PINDEX pos,           ///< Position in string to insert the substring.
      PINDEX len = 0        ///< Length of section to remove.
    );

    /**Remove the substring from the string.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
     */
    PString & Delete(
      PINDEX start,   ///< Position in string to remove.
      PINDEX len      ///< Number of characters to delete.
    );
  //@}


  /**@name Sub-string functions */
  //@{
    /**Extract a portion of the string into a new string. The original string
       is not changed and a new unique reference to a string is returned.
       
       The substring is returned inclusive of the characters at the
       <code>start</code> and <code>end</code> positions.
       
       If the <code>end</code> position is greater than the length of the
       string then all characters from the <code>start</code> up to the end of
       the string are returned.

       If <code>start</code> is greater than the length of the string or
       <code>end</code> is before <code>start</code> then an empty string is
       returned.

       @return
       substring of the source string.
     */
    PString operator()(
      PINDEX start,  ///< Starting position of the substring.
      PINDEX end     ///< Ending position of the substring.
    ) const;

    /**Extract a portion of the string into a new string. The original string
       is not changed and a new unique reference to a string is returned.
       
       A substring from the beginning of the string for the number of
       characters specified is extracted.
       
       If <code>len</code> is greater than the length of the string then all
       characters to the end of the string are returned.

       If <code>len</code> is zero then an empty string is returned.

       @return
       substring of the source string.
     */
    PString Left(
      PINDEX len   ///< Number of characters to extract.
    ) const;

    /**Extract a portion of the string into a new string. The original string
       is not changed and a new unique reference to a string is returned.

       A substring from the end of the string for the number of characters
       specified is extracted.
       
       If <code>len</code> is greater than the length of the string then all
       characters to the beginning of the string are returned.

       If <code>len</code> is zero then an empty string is returned.

       @return
       substring of the source string.
     */
    PString Right(
      PINDEX len   ///< Number of characters to extract.
    ) const;

    /**Extract a portion of the string into a new string. The original string
       is not changed and a new unique reference to a string is returned.
       
       A substring from the <code>start</code> position for the number of
       characters specified is extracted.
       
       If <code>len</code> is greater than the length of the string from the
       <code>start</code> position then all characters to the end of the
       string are returned.

       If <code>start</code> is greater than the length of the string or
       <code>len</code> is zero then an empty string is returned.

       @return
       substring of the source string.
     */
    PString Mid(
      PINDEX start,             ///< Starting position of the substring.
      PINDEX len = P_MAX_INDEX  ///< Number of characters to extract.
    ) const;

    /** Return a string up to a maximum length, substituting "..." to too long.
      */
    PString Ellipsis(
      PINDEX maxLength,   ///< Maxmimum length of resulting string
      PINDEX fromEnd = 0  ///< Number of characters at end of string adter "..."
    ) const;

    /**Create a string consisting of all characters from the source string
       except all spaces at the beginning of the string. The original string
       is not changed and a new unique reference to a string is returned.
       
       @return
       string with leading spaces removed.
     */
    PString LeftTrim() const;

    /**Create a string consisting of all characters from the source string
       except all spaces at the end of the string. The original string is not
       changed and a new unique reference to a string is returned.
       
       @return
       string with trailing spaces removed.
     */
    PString RightTrim() const;

    /**Create a string consisting of all characters from the source string
       except all spaces at the beginning and end of the string. The original
       string is not changed and a new unique reference to a string is
       returned.
       
       @return
       string with leading and trailing spaces removed.
     */
    PString Trim() const;


    /**Create a string consisting of all characters from the source string
       with all upper case letters converted to lower case. The original
       string is not changed and a new unique reference to a string is
       returned.
       
       @return
       string with upper case converted to lower case.
     */
    PString ToLower() const;

    /**Create a string consisting of all characters from the source string
       with all lower case letters converted to upper case. The original
       string is not changed and a new unique reference to a string is
       returned.
       
       @return
       string with lower case converted to upper case.
     */
    PString ToUpper() const;


    /// Options for PString::Split() function.
    P_DECLARE_BITWISE_ENUM_EX(
      SplitOptions,6,
      (
        SplitOnly,
        SplitTrimBefore,
        SplitTrimAfter,
        SplitDefaultToBefore,
        SplitDefaultToAfter,
        SplitBeforeNonEmpty,
        SplitAfterNonEmpty
      ),
      SplitTrim = SplitTrimBefore|SplitTrimAfter,
      SplitNonEmpty = SplitBeforeNonEmpty|SplitAfterNonEmpty
    );
    /** Split the string into two substrings around delimiter.

        @return
        If the delimiter not present and \p options does not contain
        SplitDefaultToBefore or SplitDefaultToAfter, false is returned.

        If SplitBeforeNonEmpty or SplitAfterNonEmpty is present, then false
        is returned if the respective result, after the optional trim, is an
        empty string.
      */
    __inline bool Split(
      const PString & delimiter,  ///< Delimiter around which tom plit the substrings
      PString & before,           ///< Substring before delimiter
      PString & after,            ///< Substring after delimiter
      SplitOptions options = SplitTrim  ///< Options for how to split the string
      ) const
    { return InternalSplit(delimiter, before, after, options); }

    // The insanity below is all to maintain backward compatibility with the previous API
    __inline bool Split(char delimiter, PString & before, PString & after, SplitOptions options) const              { return InternalSplit(delimiter, before, after, options); }
    __inline bool Split(char delimiter, PString & before, PString & after, SplitOptions_Bits options) const         { return InternalSplit(delimiter, before, after, options); }
    __inline bool Split(const char * delimiter, PString & before, PString & after, SplitOptions options) const      { return InternalSplit(delimiter, before, after, options); }
    __inline bool Split(const char * delimiter, PString & before, PString & after, SplitOptions_Bits options) const { return InternalSplit(delimiter, before, after, options); }
    __inline bool Split(char delimiter, PString & before, PString & after, bool trim) const                         { return InternalSplit(delimiter, before, after, trim ? SplitTrim : SplitOnly); }


    /** Split the string into an array of substrings. */
    PStringArray Tokenise(
      const PString & separators,
        ///< A string for the set of separator characters that delimit tokens.
      bool onePerSeparator = true
        ///< Flag for if there are empty tokens between consecutive separators.
    ) const;
    /**Split the string into an array of substrings.
       Divide the string into an array of substrings delimited by characters
       from the specified set.
       
       There are two options for the tokenisation, the first is where the
       <code>onePerSeparator</code> is true. This form will produce a token
       for each delimiter found in the set. Thus the string ",two,three,,five"
       would be split into 5 substrings; "", "two", "three", "" and "five".
       
       The second form where <code>onePerSeparator</code> is false is used
       where consecutive delimiters do not constitute a empty token. In this
       case the string "  a list  of words  " would be split into 5 substrings;
       "a", "list", "of", "words" and "".

       There is an important distinction when there are delimiters at the
       beginning or end of the source string. In the first case there will be
       empty strings at the end of the array. In the second case delimeters
       at the beginning of the source string are ignored and if there are one
       or more trailing delimeters, they will yeild a single empty string at
       the end of the array.

       @return
       an array of substring for each token in the string.
     */
    PStringArray Tokenise(
      const char * cseparators,
        ///< A C string for the set of separator characters that delimit tokens.
      bool onePerSeparator = true
        ///< Flag for if there are empty tokens between consecutive separators.
    ) const;

    /**Split the string into individual lines. The line delimiters may be a
       carriage return ('\\r'), a line feed ('\\n') or a carriage return and
       line feed pair ("\\r\\n"). A line feed and carriage return pair ("\\n\\r")
       would yield a blank line. between the characters.

       The <code>Tokenise()</code> function should not be used to split a string
       into lines as a #"\\r\\n"# pair consitutes a single line
       ending. The <code>Tokenise()</code> function would produce a blank line in
       between them.

       @return
       string array with a substring for each line in the string.
     */
    PStringArray Lines() const;
  //@}

  /**@name Conversion functions */
  //@{
    /**Concatenate a formatted output to the string. This is identical to the
       standard C library <code>sprintf()</code> function, but appends its
       output to the string.
       
       This function makes the assumption that there is less the 1000
       characters of formatted output. The function will assert if this occurs.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
       
       @return
       reference to the current string object.
     */
    PString & sprintf(
      const char * cfmt,   ///< C string for output format.
      ...                  ///< Extra parameters for <code>sprintf()</code> call.
    );

    /**Produce formatted output as a string. This is identical to the standard
       C library <code>sprintf()</code> function, but sends its output to a
       <code>PString</code>.

       This function makes the assumption that there is less the 1000
       characters of formatted output. The function will assert if this occurs.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
       
       @return
       reference to the current string object.
     */
    friend PString psprintf(
      const char * cfmt,   ///< C string for output format.
      ...                  ///< Extra parameters for <code>sprintf()</code> call.
    );

    /** Concatenate a formatted output to the string. */
    PString & vsprintf(
      const PString & fmt, ///< String for output format.
      va_list args         ///< Extra parameters for <code>sprintf()</code> call.
    );
    /**Concatenate a formatted output to the string. This is identical to the
       standard C library <code>vsprintf()</code> function, but appends its
       output to the string.

       This function makes the assumption that there is less the 1000
       characters of formatted output. The function will assert if this occurs.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
       
       @return
       reference to the current string object.
     */
    PString & vsprintf(
      const char * cfmt,   ///< C string for output format.
      va_list args         ///< Extra parameters for <code>sprintf()</code> call.
    );

    /** Produce formatted output as a string. */
    friend PString pvsprintf(
      const char * cfmt,   ///< C string for output format.
      va_list args         ///< Extra parameters for <code>sprintf()</code> call.
    );
    /**Produce formatted output as a string. This is identical to the standard
       C library <code>vsprintf()</code> function, but sends its output to a
       <code>PString</code>.

       This function makes the assumption that there is less the 1000
       characters of formatted output. The function will assert if this occurs.

       Note that this function will break the current instance from multiple
       references to the string. A new string buffer is allocated and the data
       from the old string buffer copied to it.
       
       @return
       reference to the current string object.
     */
    friend PString pvsprintf(
      const PString & fmt, ///< String for output format.
      va_list args         ///< Extra parameters for <code>sprintf()</code> call.
    );


    /**Convert the string to any type that can be converted using a stream.
       @return value for the string.
     */
    template <typename T> __inline T As(T t = T()) const;

    /**Convert the string to an integer value using the specified number base.
       All characters up to the first illegal character for the number base are
       converted. Case is not significant for bases greater than 10.

       The number base may only be zero, or from 2 to 36 and the function will
       assert if it is not in this range. If zero the base is determined by the
       prefix characters, e.g. 0x.

       This function uses the standard C library <code>strtol()</code> function.

       @return
       integer value for the string.
     */
    long AsInteger(
      unsigned base = 0    ///< Number base to convert the string in.
    ) const;
    /**Convert the string to an integer value using the specified number base.
       All characters up to the first illegal character for the number base are
       converted. Case is not significant for bases greater than 10.

       The number base may only be zero, or from 2 to 36 and the function will
       assert if it is not in this range. If zero the base is determined by the
       prefix characters, e.g. 0x.

       This function uses the standard C library <code>strtoul()</code> function.

       @return
       integer value for the string.
     */
    uint32_t AsUnsigned(
      unsigned base = 0    ///< Number base to convert the string in.
    ) const;
    /**Convert the string to an integer value using the specified number base.
       All characters up to the first illegal character for the number base are
       converted. Case is not significant for bases greater than 10.

       The number base may only be zero, or from 2 to 36 and the function will
       assert if it is not in this range. If zero the base is determined by the
       prefix characters, e.g. 0x.

       This function uses the standard C library <code>strtoll()</code> function.

       @return
       integer value for the string.
     */
    int64_t AsInteger64(
      unsigned base = 0    ///< Number base to convert the string in.
    ) const;

    __inline int64_t AsInt64(unsigned base = 0) const { return AsInteger(base); } // Backward compatibility

    /**Convert the string to an integer value using the specified number base.
       All characters up to the first illegal character for the number base are
       converted. Case is not significant for bases greater than 10.

       The number base may only be zero, or from 2 to 36 and the function will
       assert if it is not in this range. If zero the base is determined by the
       prefix characters, e.g. 0x.

       This function uses the standard C library <code>strtoull()</code> function.

       @return
       integer value for the string.
     */
    uint64_t AsUnsigned64(
      unsigned base = 0    ///< Number base to convert the string in.
    ) const;

    /**Convert the string to a floating point number. This number may be in
       decimal or exponential form. All characters up to the first illegal
       character for a floting point number are converted.

       This function uses the standard C library <code>strtod()</code>
       function.

       @return
       floating point value for the string.
     */
    double AsReal() const;
     
    /**Convert UTF-8 string to native wide character string.
       Note the resultant PWCharArray will have the trailing null included.
       For Windows this is UTF-16, for GNU C (Linux) it is UCS-4.
      */
    PWCharArray AsWide() const;

    P_DEPRECATED PWCharArray AsUCS2() const { return AsWide(); }

    /**Convert a standard null terminated string to a "pascal" style string.
       This consists of a songle byte for the length of the string and then
       the string characters following it.
       
       This function will assert if the string is greater than 255 characters
       in length.

       @return
       byte array containing the "pascal" style string.
     */
    PBYTEArray ToPascal() const;

    /**Convert the string to C literal string format. This will convert non
       printable characters to the \\nnn form or for standard control characters
       such as line feed, to \\n form. Any '"' characters are also escaped with
       a \\ character and the entire string is enclosed in '"' characters.
       
       @return
       string converted to a C language literal form.
     */
    PString ToLiteral() const;

    /** Parse a C literal string format as for PString::ToLiteral().
        The \p offset is the initial position is \p str and will be updated
        to the final position in the string for the literal. If the string
        starts with an opening quote, this will be the one past final
        enclosing quote. If no opening quote is present this will be
        PString::GetLength().
      */
    PString FromLiteral(
      PINDEX & offset
    ) const;

    /**Get a pointer to the buffer and set the length of the string.
       Note the caller may require the actual length to be less than the len
       parameter, in which case the MakeMinimumSize() function should be
       called immediately afterward.

       Example 1:
       <CODE>
          PString str;
          memcpy(str.GetPointerAndSetLength(len), someData, len);
       </CODE>

       Example 2:
       <CODE>
          PString str;
          str.SetMinSize(1000);
          str.MakeMinimumSize(SomeFunction(str.GetPointerAndSetLength(0), str.GetSize()));
       </CODE>

       Note the old GetPointer() is deliberately made private to assure the
       new semantics are used.
      */
    char * GetPointerAndSetLength(
      PINDEX len    /// New Length
    );
  //@}

  /**@name Backward compatibility functions */
  //@{
    P_DEPRECATED PINDEX GetSize() const { return capacity(); }
    P_DEPRECATED bool SetSize(PINDEX newSize) { reserve(newSize); return true; }
    P_DEPRECATED bool SetMinSize(PINDEX newSize) { if (capacity() >= (size_t)newSize) reserve(newSize); return true; }
    PINDEX GetLength() const { return length(); }
    bool IsEmpty() const { return empty(); }
    PString & MakeEmpty() { clear(); return *this; }
    P_DEPRECATED bool MakeUnique() { return true; }
    const char * GetPointer(PINDEX = 0) const { return data(); }
    char GetAt(PINDEX idx) const { return at(idx); }
    operator const unsigned char *() const { return (const unsigned char *)c_str(); }
    operator const char *() const { return c_str(); }
    char operator[](PINDEX index) const { return at(index); }
    char & operator[](PINDEX index) { return at(index); }
  //@}


  protected:
    void InternalFromWChar(
      const wchar_t * ptr,
      PINDEX len
    );
    virtual Comparison InternalCompare(
      PINDEX offset,      // Offset into string to compare.
      PINDEX length,      // Number of characters to compare.
      const char * cstr   // C string to compare against.
    ) const;
    virtual int internal_strcmp(const char * s1, const char *s2) const;
    virtual int internal_strncmp(const char * s1, const char *s2, size_t n) const;

    bool InternalSplit(
      const PString & delimiter,  // Delimiter around which tom plit the substrings
      PString & before,           // Substring before delimiter
      PString & after,            // Substring after delimiter
      SplitOptions_Bits options   // options
    ) const;
};


inline ostream & operator<<(ostream & stream, const PString & string)
{
  string.PrintOn(stream);
  return stream;
}


class PWideString : public PWCharArray {
  PCLASSINFO(PWideString, PWCharArray);

  public:
  typedef const wchar_t * Initialiser;

    PWideString() { }
    PWideString(const PWCharArray & arr) : PWCharArray(arr) { }
    PWideString(const PString     & str) : PWCharArray(str.AsWide()) { }
    PWideString(const char        * str);
    PWideString & operator=(const PWideString & str) { PWCharArray::operator=(str); return *this; }
    PWideString & operator=(const PString     & str) { PWCharArray::operator=(str.AsWide()); return *this; }
    PWideString & operator=(const std::string & str);
    PWideString & operator=(const char        * str);
    friend inline ostream & operator<<(ostream & stream, const PWideString & string) { return stream << PString(string); }

    PINDEX GetLength() const { return GetSize() - 1; }

  protected:
    PWideString(PContainerReference & reference_) : PWCharArray(reference_) { }
};

#ifdef UNICODE
  typedef PWideString PVarString;
#else
  typedef PString PVarString;
#endif


//////////////////////////////////////////////////////////////////////////////

/**This class is a variation of a string that ignores case. Thus in all
   standard comparison (==, < etc) and search
   (<code>Find()</code> etc) functions the case of the characters and strings is
   ignored.
   
   The characters in the string still maintain their case. Only the comparison
   operations are affected. So printing etc will still display the string as
   entered.
 */
class PCaselessString : public PString
{
  PCLASSINFO(PCaselessString, PString);

  public:
    /**Create a new, empty, caseless string.
     */
    PCaselessString();

    /**Create a new caseless string, initialising it to the characters in the
       C string provided.
     */
    PCaselessString(
      const char * cstr   ///< C string to initialise the caseless string from.
    );

    /**Create a caseless string, with a reference to the characters in the
       normal <code>PString</code> provided. A PCaselessString may also be provided
       to this constructor.
     */
    PCaselessString(
      const PString & str  ///< String to initialise the caseless string from.
    );


    /**Create a caseless string from a std::string
     */
    PCaselessString(
      const std::string & str  ///< String to initialise the caseless string from.
      ) : PString(str)
    { }

    /**Assign the string to the current object. The current instance then
       becomes another reference to the same string in the <code>str</code>
       parameter.
       
       @return
       reference to the current PString object.
     */
    PCaselessString & operator=(
      const PString & str  ///< New string to assign.
    );

    /**Assign the string to the current object. The current instance then
       becomes another reference to the same string in the <code>str</code>
       parameter.
       
       @return
       reference to the current PString object.
     */
    PCaselessString & operator=(
      const std::string & str  ///< New string to assign.
    ) { return operator=(str.c_str()); }

    /**Assign the C string to the current object. The current instance then
       becomes a unique reference to a copy of the <code>cstr</code> parameter.
       The <code>cstr</code> parameter is typically a literal string, eg:
<pre><code>
          myStr = "fred";
</code></pre>
       @return
       reference to the current PString object.
     */
    PCaselessString & operator=(
      const char * cstr  ///< C string to assign.
    );

    /**Assign the character to the current object. The current instance then
       becomes a unique reference to a copy of the character parameter. eg:
<pre><code>
          myStr = 'A';
</code></pre>
       @return
       reference to the current PString object.
     */
    PCaselessString & operator=(
      char ch            ///< Character to assign.
    );


  // Overrides from class PObject
    /**Make a complete duplicate of the string. Note that the data in the
       array of characters is duplicated as well and the new object is a
       unique reference to that data.
     */
    virtual PObject * Clone() const;

  protected:
  // Overrides from class PString
    virtual int internal_strcmp(const char * s1, const char *s2) const;
    virtual int internal_strncmp(const char * s1, const char *s2, size_t n) const;
};


//////////////////////////////////////////////////////////////////////////////

// For backward compatibility
typedef const PString         PConstString;
typedef const PCaselessString PConstCaselessString;


#ifdef _WIN32
// Now have PConstString can have these definitions
__inline PWideString::PWideString(const char * str) : PWCharArray(PConstString(str).AsWide()) { }
__inline PWideString & PWideString::operator=(const std::string & str) { PWCharArray::operator=(PConstString(str.c_str()).AsWide()); return *this; }
__inline PWideString & PWideString::operator=(const char        * str) { PWCharArray::operator=(PConstString(str).AsWide()); return *this; }
#endif


//////////////////////////////////////////////////////////////////////////////

/**This class is a standard C++ stream class descendent for reading or writing
   streamed data to or from a <code>PString</code> class.
   
   All of the standard stream I/O operators, manipulators etc will operate on
   the PStringStream class.
 */
class PStringStream : public PObject, public std::stringstream
{
  PCLASSINFO(PStringStream, PObject);

public:
  /**Create a new, empty, string stream. Data may be output to this stream,
     but attempts to input from it will return end of file.

     The internal string is continually grown as required during output.
   */
  PStringStream() { }
  PStringStream(const PString & str) : std::stringstream(str) { }
  PStringStream(const char * cstr) : std::stringstream(cstr) { }

  PStringStream & operator=(const PStringStream & s) { str(s.str()); return *this; }
  PStringStream & operator=(const PString & s) { str(s); return *this; }
  PStringStream & operator=(const std::string & s) { str(s); return *this; }
  PStringStream & operator=(const char * s) { str(s); return *this; }
  PStringStream & operator=(char c) { str(std::string(1, c)); return *this; }

  __inline void clear() { std::stringstream::clear(); }

  void PrintOn(ostream & strm) const { strm << str(); }

  // Backward compatibility
  PString MakeEmpty() { str(std::string()); return *this; }
  operator PString() const { return str(); }
  PINDEX GetLength() const { return str().length(); }
  PString operator()(PINDEX start, PINDEX finish) const { return PString(str())(start, finish); }
  bool IsEmpty() const { return rdbuf()->pubseekoff(0, ios_base::cur, ios_base::out) == 0; }
  PStringStream & Replace(const PString & target, const PString & subs, bool all = false, PINDEX offset = 0) { PString s = str(); s.Replace(target, subs, all, offset); str(s); return *this; }
  PStringStream & Splice(const PString & insert, PINDEX pos, PINDEX len = 0) { PString s = str(); s.Splice(insert, pos, len); str(s); return *this; }
  PStringStream & Delete(PINDEX start, PINDEX len) { PString s = str(); s.Delete(start, len); str(s); return *this; }
};


__inline PString PSTRSTRM_support(const ostream & s) { return dynamic_cast<const std::stringstream &>(s).str(); }

/**Output a stream expression to a string parameter.
   This allows a string parameter to a function use a stream expression to
   generate that string. For example, given the function:
<pre><code>
      void DoSomething(const PString & str);
</code></pre>
   you could make a call to that function as:
<pre><code>
      DoSomething(PSTRSTRM("Fred used" << number << " of \"" << item << '"'));
</code></pre>
 */
#define PSTRSTRM(arg) PSTRSTRM_support(std::stringstream() << std::flush << arg << std::flush)


// Definition for template PString memeber function, needs to be after PStringStream
template <typename T> __inline T PString::As(T t) const
{
  PStringStream strm(*this);
  strm >> t;
  return t;
}


class PStringList;
class PSortedStringList;
class PStringSet;

/**This is an array collection class of <code>PString</code> objects. It has all the
   usual functions for a collection, with the object types set to
   <code>PString</code> pointers.
   
   In addition some addition functions are added that take a const
   <code>PString</code> reference instead of a pointer as most standard collection
   functions do. This is more convenient for when string expressions are used
   as parameters to function in the collection.
*/
class PStringArray : public PArray<PString>
{
    typedef PArray<PString> BaseClass;
    PCLASSINFO(PStringArray, BaseClass);

  protected:
    inline PStringArray(int dummy, const PStringArray * c)
      : BaseClass(dummy, c)
    {
    }

  public:
  /**@name Construction */
  //@{
    /// Create an empty PString array.
    __inline PStringArray(PINDEX initialSize = 0)
      : BaseClass(initialSize) { }

    __inline PStringArray(const BaseClass & other)
      : BaseClass(other) { }

    /**Create a PStringArray from the array of C strings. If count is
       P_MAX_INDEX then strarr is assumed to point to an array of strings
       where the last pointer is NULL.
     */
    PStringArray(
      PINDEX count,                 ///< Count of strings in array
      char const * const * strarr,  ///< Array of C strings
      bool caseless = false         ///< New strings are to be PCaselessStrings
    );
    /**Create a PStringArray of length one from the single string.
     */
    PStringArray(
      const PString & str  ///< Single string to convert to an array of one.
    );
    /**Create a PStringArray of length one from the single string.
     */
    PStringArray(
      const char * cstr  ///< Single string to convert to an array of one.
    );
    /**Create a PStringArray from the list of strings.
     */
    PStringArray(
      const PStringList & list  ///< List of strings to convert to array.
    );
    /**Create a PStringArray from the sorted list strings.
     */
    PStringArray(
      const PSortedStringList & list  ///< List of strings to convert to array.
    );
    /**Create a PStringArray from the string set.
    */
    PStringArray(
      const PStringSet & set  ///< Set of strings to convert to array.
    );

    /**
      * Create a PStringArray from a vector of PStrings
      */
    PStringArray(
      const std::vector<PString> & vec
    )
    {
      for (std::vector<PString>::const_iterator r = vec.begin(); r != vec.end(); ++r)
        AppendString(*r);
    }

    /**
      * Create a PStringArray from a vector of std::string
      */
    PStringArray(
      const std::vector<std::string> & vec
    )
    {
      for (std::vector<std::string>::const_iterator r = vec.begin(); r != vec.end(); ++r)
        AppendString(PString(*r));
    }

    /**
      * Create a PStringArray from an STL container
      */
    template <typename stlContainer>
    static PStringArray container(
      const stlContainer & vec
    )
    {
      PStringArray list;
      for (typename stlContainer::const_iterator r = vec.begin(); r != vec.end(); ++r)
        list.AppendString(PString(*r));
      return list;
    }

  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads '\\n' separated strings until
       <code>!strm.good()</code>.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );

    virtual PObject * Clone() const
    {
      return new PStringArray(0, this);
    }
  //@}

  /**@name New functions for class */
  //@{
    /**As for <code>GetValuesIndex()</code> but takes a PString argument so that
       literals will be automatically converted.

       @return
       Index of string in array or P_MAX_INDEX if not found.
     */
    PINDEX GetStringsIndex(
      const PString & str ///< String to search for index of
    ) const;

    PString operator[](
      PINDEX index  ///< Index position in the collection of the object.
    ) const;

    /**Retrieve a reference  to the object in the array. If there was not an
       object at that ordinal position or the index was beyond the size of the
       array then the function will create a new one.

       @return
       reference to the object at <code>index</code> position.
     */
    PString & operator[](
      PINDEX index  ///< Index position in the collection of the object.
    );

    /** Append a string to the array
     */
    PINDEX AppendString(
      const PString & str ///< String to append.
    );
    void push_back(const PString & str) { AppendString(str); }

    /**Concatenate a PString or PStringArray to the array

       @return
       The PStringArray with the new items appended
     */
    PStringArray & operator +=(const PStringArray & array);
    PStringArray & operator +=(const PString & str) { AppendString(str);  return *this; }
    PStringArray & operator +=(const char * cstr)   { AppendString(cstr); return *this; }


    /**Create a new PStringArray, and add PString or PStringArray to it
       a new PStringArray

       @return
       A new PStringArray with the additional elements(s)
     */
    PStringArray operator + (const PStringArray & array);
    PStringArray operator + (const PString & str);

    /**Create an array of C strings.
       If storage is NULL then this returns a single pointer that must be
       disposed of using free(). Note that each of the strings are part of the
       same memory allocation so only one free() is required.

       If storage is not null then that is used to allocate the memory.
      */
    char ** ToCharArray(
      PCharArray * storage = NULL
    ) const;

    /**Create a PString concatenation of all array memebers.
      */
    PString ToString(
      char separator = ' '
    ) const;
  //@}
};


/**This is a list collection class of <code>PString</code> objects. It has all the
   usual functions for a collection, with the object types set to
   <code>PString</code> pointers.
   
   In addition some addition functions are added that take a const
   <code>PString</code> reference instead of a pointer as most standard collection
   functions do. This is more convenient for when string expressions are used
   as parameters to function in the collection.
 */
class PStringList : public PList<PString>
{
    typedef PList<PString> BaseClass;
    PCLASSINFO(PStringList, BaseClass);

  protected:
    PStringList(int dummy, const PStringList * c)
      : BaseClass(dummy, c) { }

  public:
  /**@name Construction */
  //@{
    /**Create an empty PStringList.
     */
    __inline PStringList()
      : BaseClass() { }

    __inline PStringList(const BaseClass & other)
      : BaseClass(other) { }

    /**Create a PStringList from the array of C strings.
     */
    PStringList(
      PINDEX count,                 ///< Count of strings in array
      char const * const * strarr,  ///< Array of C strings
      bool caseless = false         ///< New strings are to be PCaselessStrings
    );
    /**Create a PStringList of length one from the single string.
     */
    PStringList(
      const PString & str  ///< Single string to convert to a list of one.
    );
    /**Create a PStringList of length one from the single string.
     */
    PStringList(
      const char * cstr  ///< Single string to convert to a list of one.
    );
    /**Create a PStringList from the array of strings.
     */
    PStringList(
      const PStringArray & array  ///< Array of strings to convert to list
    );
    /**Create a PStringList from the sorted list of strings.
     */
    PStringList(
      const PSortedStringList & list  ///< List of strings to convert to list.
    );
    /**Create a PStringList from the string set.
    */
    PStringList(
      const PStringSet & set  ///< Set of strings to convert to array.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads '\\n' separated strings until
       <code>!strm.good()</code>.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );

    virtual PObject * Clone() const
    {
      return new PStringList(0, this);
    }
  //@}

  /**@name Operations */
  //@{
    /** Append a string to the list.
     */
    PINDEX AppendString(
      const PString & str ///< String to append.
    );

    /** Insert a string into the list.
     */
    PINDEX InsertString(
      const PString & before,   ///< String to insert before.
      const PString & str       ///< String to insert.
    );

    /** Get the index of the string with the specified value.
      A linear search of list is performed to find the string value.
     */
    PINDEX GetStringsIndex(
      const PString & str   ///< String value to search for.
    ) const;

    /**Concatenate a PString or PStringArray to the list

       @return
       The PStringArray with the new items appended
     */
    PStringList & operator +=(const PStringList & list);
    PStringList & operator +=(const PString & str) { AppendString(str);  return *this; }
    PStringList & operator +=(const char * cstr)   { AppendString(cstr); return *this; }


    /**Create a new PStringList, and add PString or PStringList to it
       a new PStringList

       @return
       A new PStringList with the additional elements(s)
     */
    PStringList operator + (const PStringList & array);
    PStringList operator + (const PString & str);

    /**
      * Create a PStringArray from an STL container
      */
    template <typename stlContainer>
    static PStringList container(
      const stlContainer & vec
    )
    {
      PStringList list;
      for (typename stlContainer::const_iterator r = vec.begin(); r != vec.end(); ++r)
        list.AppendString(PString(*r));
      return list;
    }
  //@}
};


/**This is a sorted list collection class of <code>PString</code> objects. It has all
   the usual functions for a collection, with the object types set to
   <code>PString</code> pointers.
   
   In addition some addition functions are added that take a const
   <code>PString</code> reference instead of a pointer as most standard collection
   functions do. This is more convenient for when string expressions are used
   as parameters to function in the collection.

   See the PAbstractSortedList and PSortedList classes for more information.
 */
class PSortedStringList : public PSortedList<PString>
{
    typedef PSortedList<PString> BaseClass;
    PCLASSINFO(PSortedStringList, BaseClass);

  protected:
    PSortedStringList(int dummy, const PSortedStringList * c)
      : BaseClass(dummy, c) { }

  public:
  /**@name Construction */
  //@{
    /**Create an empty PStringList.
     */
    __inline PSortedStringList()
      : BaseClass() { }

    __inline PSortedStringList(const BaseClass & other)
      : BaseClass(other) { }

    /**Create a PStringArray from the array of C strings.
     */
    PSortedStringList(
      PINDEX count,                 ///< Count of strings in array
      char const * const * strarr,  ///< Array of C strings
      bool caseless = false         ///< New strings are to be PCaselessStrings
    );
    /**Create a PSortedStringList of length one from the single string.
     */
    PSortedStringList(
      const PString & str  ///< Single string to convert to a list of one.
    );
    /**Create a PSortedStringList of length one from the single string.
     */
    PSortedStringList(
      const char * cstr  ///< Single string to convert to a list of one.
    );
    /**Create a PSortedStringList from the array of strings.
     */
    PSortedStringList(
      const PStringArray & array  ///< Array of strings to convert to list
    );
    /**Create a PSortedStringList from the list of strings.
     */
    PSortedStringList(
      const PStringList & list  ///< List of strings to convert to list.
    );
    /**Create a PStringList from the string set.
    */
    PSortedStringList(
      const PStringSet & set  ///< Set of strings to convert to array.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads '\\n' separated strings until
       <code>!strm.good()</code>.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );
  //@}

  /**@name Operations */
  //@{
    /** Add a string to the list.
        This will place the string in the correct position in the sorted list.
     */
    PINDEX AppendString(
      const PString & str ///< String to append.
    );

    /** Get the index of the string with the specified value.
      A binary search of tree is performed to find the string value.
     */
    PINDEX GetStringsIndex(
      const PString & str   ///< String value to search for.
    ) const;

    /** Get the index of the next string after specified value.
      A binary search of tree is performed to find the string greater than
      or equal to the specified string value.
     */
    PINDEX GetNextStringsIndex(
      const PString & str   ///< String value to search for.
    ) const;
  //@}

  protected:
    PINDEX InternalStringSelect(
      const char * str,
      PINDEX len,
      PSortedListElement * thisElement,
      PSortedListElement * & lastElement
    ) const;
};


/**This is a set collection class of <code>PString</code> objects. It has all the
   usual functions for a collection, with the object types set to
   <code>PString</code> pointers.

   In addition some addition functions are added that take a const
   <code>PString</code> reference instead of a pointer as most standard collection
   functions do. This is more convenient for when string expressions are used
   as parameters to function in the collection.

   Unlike the normal sets, this will delete the PStrings removed from it. This
   complements the automatic creation of new PString objects when literals or
   expressions are used.
 */
class PStringSet : public PSet<PString>
{
    typedef PSet<PString> BaseClass;
    PCLASSINFO(PStringSet, BaseClass);

  protected:
    PStringSet(int dummy, const PStringSet * c)
      : BaseClass(dummy, c) { }

  public:
  /**@name Construction */
  //@{
    /**Create a PStringSet.
     */
    PStringSet(bool initialDeleteObjects = true)
      : BaseClass(initialDeleteObjects) { }

    PStringSet(const BaseClass & other)
      : BaseClass(other) { }

    /**Create a PStringSet from the array of C strings.
     */
    PStringSet(
      PINDEX count,                 ///< Count of strings in array
      char const * const * strarr,  ///< Array of C strings
      bool caseless = false         ///< New strings are to be PCaselessStrings
    );

    /**Create a PStringSet containing the single string.
     */
    PStringSet(
      const PString & str  ///< Single string to convert to a set of one.
    );
    /**Create a PStringSet containing the single string.
     */
    PStringSet(
      const char * cstr  ///< Single string to convert to a set of one.
    );
    /**Create a PStringSet containing the strings.
     */
    PStringSet(
      const PStringArray & strArray  ///< String to convert to a set.
    );
    /**Create a PStringSet containing the strings.
     */
    PStringSet(
      const PStringList & strList  ///< String to convert to a set.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads '\\n' separated strings until
       <code>!strm.good()</code>.
     */
    virtual void ReadFrom(
      istream &strm   ///< Stream to read the objects contents from.
    );

    virtual PObject * Clone() const
    {
      return PNEW PStringSet(0, this);
    }
  //@}

  /**@name Operations */
  //@{
    /** Include the spcified string value into the set. */
    void Include(
      const PString & key ///< String value to add to set.
    );
    /** Include the spcified string value into the set. */
    PStringSet & operator+=(
      const PString & key ///< String value to add to set.
    );
    /** Exclude the spcified string value from the set. */
    void Exclude(
      const PString & key ///< String value to remove from set.
    );
    /** Exclude the spcified string value from the set. */
    PStringSet & operator-=(
      const PString & key ///< String value to remove from set.
    );
  //@}
};


/**This template class maps the PAbstractDictionary to a specific key type and
   a <code>PString</code> data type. The functions in this class primarily do all the
   appropriate casting of types.

   Note that if templates are not used the <code>PDECLARE_STRING_DICTIONARY</code>
   macro will simulate the template instantiation.
 */
template <class K> class PStringDictionary : public PDictionary<K, PString>
{
  typedef PDictionary<K, PString> BaseClass;
  PCLASSINFO(PStringDictionary, BaseClass);

  public:
  /**@name Construction */
  //@{
    /**Create a new, empty, dictionary.

       Note that by default, objects placed into the dictionary will be
       deleted when removed or when all references to the dictionary are
       destroyed.
     */
    PStringDictionary()
      : BaseClass() { }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Make a complete duplicate of the dictionary. Note that all objects in
       the array are also cloned, so this will make a complete copy of the
       dictionary.
     */
    virtual PObject * Clone() const
      { return PNEW PStringDictionary(0, this); }
  //@}

  /**@name New functions for class */
  //@{
    /**Get the string contained in the dictionary at the <code>key</code>
       position. The hash table is used to locate the data quickly via the
       hash function provided by the key.

       The last key/data pair is remembered by the class so that subseqent
       access is very fast.

       This function returns the <code>dflt</code> value if there is no data
       at the key position.

       @return
       reference to the object indexed by the key.
     */
    PString operator()(const K & key, const char * dflt = NULL) const
    {
      PString * str = this->GetAt(key);
      return str != NULL ? *str : PString(dflt);
    }

    /**Add a new object to the collection. If the objects value is already in
       the dictionary then the object is overrides the previous value. If the
       AllowDeleteObjects option is set then the old object is also deleted.

       The object is placed in the an ordinal position dependent on the keys
       hash function. Subsequent searches use the has function to speed access
       to the data item.

       @return
       true if the object was successfully added.
     */
    virtual bool SetAt(
      const K & key,       // Key for position in dictionary to add object.
      const PString & str  // New string value to put into the dictionary.
    ) { return this->AbstractSetAt(key, PNEW PString(str)) != NULL; }
  //@}

  protected:
    PStringDictionary(int dummy, const PStringDictionary * c)
      : BaseClass(dummy, c) { }
};


/**Begin declaration of a dictionary of strings class.
   This macro is used to declare a descendent of PAbstractList class,
   customised for a particular key type <b>K</b> and data object type
   <code>PString</code>.

   If the compilation is using templates then this macro produces a descendent
   of the <code>PStringDictionary</code> template class. If templates are not being
   used then the macro defines a set of inline functions to do all casting of
   types. The resultant classes have an identical set of functions in either
   case.

   See the <code>PStringDictionary</code> and <code>PAbstractDictionary</code> classes for
   more information.
 */
#define PDECLARE_STRING_DICTIONARY(cls, K) \
  PDECLARE_CLASS(cls, PStringDictionary<K>) \
  protected: \
    cls(int dummy, const cls * c) \
      : PStringDictionary<K>(dummy, c) { } \
  public: \
    cls() \
      : PStringDictionary<K>() { } \
    virtual PObject * Clone() const \
      { return PNEW cls(0, this); } \


/**Declare a dictionary of strings class.
   This macro is used to declare a descendent of PAbstractDictionary class,
   customised for a particular key type <b>K</b> and data object type
   <code>PString</code>. This macro closes the class declaration off so no additional
   members can be added.

   If the compilation is using templates then this macro produces a typedef
   of the <code>PStringDictionary</code> template class.

   See the <code>PStringDictionary</code> class and <code>PDECLARE_STRING_DICTIONARY</code>
   macro for more information.
 */
#define PSTRING_DICTIONARY(cls, K) typedef PStringDictionary<K> cls


/**This is a dictionary collection class of <code>PString</code> objects, keyed by an
   ordinal value. It has all the usual functions for a collection, with the
   object types set to <code>PString</code> pointers. The class could be considered
   like a sparse array of strings.

   In addition some addition functions are added that take a const
   <code>PString</code> reference instead of a pointer as most standard collection
   functions do. This is more convenient for when string expressions are used
   as parameters to function in the collection.
 */
#ifdef DOC_PLUS_PLUS
class POrdinalToString : public PStringDictionary {
#endif
PDECLARE_STRING_DICTIONARY(POrdinalToString, POrdinalKey);
  public:
  /**@name Construction */
  //@{
    /// Structure for static array initialiser for class.
    struct Initialiser {
      /// Ordinal key for string.
      PINDEX key;
      /// String value for ordinal.
      const char * value;
    };
    /** Initialise the ordinal dictionary of strings from the static array.
     */
    POrdinalToString(
      PINDEX count,                ///< Count of strings in initialiser array
      const Initialiser * init     ///< Array of Initialiser structures
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads '\\n' separated strings until
       <code>!strm.good()</code>.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );
  //@}
};

/**This is a dictionary collection class of ordinals keyed by
   <code>PString</code> objects. It has all the usual functions for a collection,
   with the object types set to <code>POrdinalKey</code> pointers.

   In addition some addition functions are added that take a const
   <code>POrdinalKey</code> reference or a simple <code>PINDEX</code> instead of a pointer
   as most standard collection functions do. This is more convenient for when
   integer expressions are used as parameters to function in the collection.

   See the PAbstractDicionary and POrdinalDictionary classes for more information.
 */
class PStringToOrdinal : public POrdinalDictionary<PString>
{
  public:
  /**@name Construction */
  //@{
    PStringToOrdinal() {}

    /// Structure for static array initialiser for class.
    struct Initialiser {
      /// String key for ordinal.
      const char * key;
      /// Ordinal value for string.
      PINDEX value;
    };
    /** Initialise the string dictionary of ordinals from the static array.
     */
    PStringToOrdinal(
      PINDEX count,                ///< Count of strings in initialiser array
      const Initialiser * init,    ///< Array of Initialiser structures
      bool caseless = false        ///< New keys are to be PCaselessStrings
    );
    /** Initialise the string dictionary of ordinals from the static array.
    */
    PStringToOrdinal(
      PINDEX count,                ///< Count of strings in initialiser array
      const POrdinalToString::Initialiser * init,    ///< Array of Initialiser structures
      bool caseless = false        ///< New keys are to be PCaselessStrings
    );
    //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads '\\n' separated strings until
       <code>!strm.good()</code>.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );
  //@}
};


/**This is a dictionary collection class of <code>PString</code> objects, keyed by
   another string. It has all the usual functions for a collection, with the
   object types set to <code>PString</code> pointers.

   In addition some addition functions are added that take a const
   <code>PString</code> reference instead of a pointer as most standard collection
   functions do. This is more convenient for when string expressions are used
   as parameters to function in the collection.
 */
#ifdef DOC_PLUS_PLUS
class PStringToString : public PStringDictionary {
#endif
PDECLARE_STRING_DICTIONARY(PStringToString, PString);
  public:
  /**@name Construction */
  //@{
    /// Structure for static array initialiser for class.
    struct Initialiser {
      /// String key for string.
      const char * key;
      /// String value for string.
      const char * value;
    };
    /** Initialise the string dictionary of strings from the static array.
     */
    PStringToString(
      PINDEX count,                ///< Count of strings in initialiser array
      const Initialiser * init,    ///< Array of Initialiser structures
      bool caselessKeys = false,   ///< New keys are to be PCaselessStrings
      bool caselessValues = false  ///< New values are to be PCaselessStrings
    );

    /** Initialise the string dictionary from the string.
        The string is expected to be of the form "key=value\\nkey=value".
      */
    explicit PStringToString(
      const PString & str  ///< String to read dictionary from
    ) { FromString(str); }
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Input the contents of the object from the stream. This is
       primarily used by the standard <code>operator>></code> function.

       The default behaviour reads input of the form "key=value\\nkey=value"
       until <code>!strm.good()</code>.

       If there are more than one line with the same key they dictionary entry
       is is appended to with '\n' separator.
     */
    virtual void ReadFrom(
      istream &strm   // Stream to read the objects contents from.
    );
  //@}

  /**@name New functions for class */
  //@{
    /// Actions to take when doing a PStringToString::Merge()
    enum MergeAction
    {
      e_MergeOverwrite, ///< Overwrite destination keys value
      e_MergeIgnore,    ///< Ignore new value, leave desstination as is
      e_MergeAppend     ///< Append to existing key value, separated by '\n'
    };
    /**Merge string dictionary into this dictionary.
       If \p unique is true, then if the merge value for a key is the same
      */
    void Merge(
      const PStringToString & merge, ///< Strings to merge
      MergeAction action
    );

    /**Create an array of C strings.
       If withEqualSign is true then array is GetSize()+1 strings of the form
       key=value. If false then the array is GetSize()*2+1 strings where
       consecutive pointers are the key and option respecitively of each
       entry in the dictionary.

       If storage is NULL then this returns a single pointer that must be
       disposed of using free(). Note that each of the strings are part of the
       same memory allocation so only one free() is required.

       If storage is not null then that is used to allocate the memory.
      */
    char ** ToCharArray(
      bool withEqualSign,
      PCharArray * storage = NULL
    ) const;

    /**Set dictionary from string.
       The string is expected to be of the form "key=value\\nkey=value".
       If there are more than one line with the same key they dictionary entry
       is is appended to with '\n' separator.
      */
    void FromString(
      const PString & str  ///< String to read dictionary from
    );
  //@}
};


/** Specialised version of PStringToString to contain a dictionary of
    options/attributes.

    This assures that the keys are caseless and has some access functions for
    bool/int types for ease of access with default values.
  */
class PStringOptions : public PStringToString 
{
  public:
    PStringOptions() { }
    explicit PStringOptions(const PString & str) : PStringToString(str) { }
    PStringOptions(const PStringToString & other) : PStringToString(other) { }
    PStringOptions & operator=(const PStringToString & other) { PStringToString::operator=(other); return *this; }

    /// Determine if the specified key is present.
    bool Contains(const char *              key   ) const { PConstCaselessString k(key); return PStringToString::Contains(k); }
    bool Contains(const std::string     &   key   ) const { return PStringToString::Contains(PCaselessString(key)); }
    bool Contains(const PString         &   key   ) const { return PStringToString::Contains(PCaselessString(key)); }
    bool Contains(const PCaselessString &   key   ) const { return PStringToString::Contains(key); }
    bool Contains(const PCaselessString & (*key)()) const { return PStringToString::Contains(key()); }

    // Overide default PStringToString::SetAt() to make sure the key is caseless
    PString * GetAt(const char *              key   ) const { PConstCaselessString k(key); return PStringToString::GetAt(k); }
    PString * GetAt(const std::string     &   key   ) const { return PStringToString::GetAt(PCaselessString(key)); }
    PString * GetAt(const PString         &   key   ) const { return PStringToString::GetAt(PCaselessString(key)); }
    PString * GetAt(const PCaselessString &   key   ) const { return PStringToString::GetAt(key); }
    PString * GetAt(const PCaselessString & (*key)()) const { return PStringToString::GetAt(key()); }

    // Overide default PStringToString::SetAt() to make sure the key is caseless
    bool SetAt(const char *              key,    const PString & data) { PConstCaselessString k(key); return SetAt(k, data); }
    bool SetAt(const std::string     &   key,    const PString & data) { return SetAt(PCaselessString(key), data); }
    bool SetAt(const PString         &   key,    const PString & data) { return SetAt(PCaselessString(key), data); }
    bool SetAt(const PCaselessString &   key,    const PString & data) { MakeUnique(); return PStringToString::SetAt(key, data); }
    bool SetAt(const PCaselessString & (*key)(), const PString & data) { return SetAt(key(), data); }

    // Overide default PStringToString::RemoveAt() to make sure the key is caseless
    PString * RemoveAt(const char *              key)    { PConstCaselessString k(key); return RemoveAt(k); }
    PString * RemoveAt(const std::string     &   key)    { return RemoveAt(PCaselessString(key)); }
    PString * RemoveAt(const PString         &   key)    { return RemoveAt(PCaselessString(key)); }
    PString * RemoveAt(const PCaselessString &   key)    { MakeUnique(); return PStringToString::RemoveAt(key); }
    PString * RemoveAt(const PCaselessString & (*key)()) { return RemoveAt(key()); }

    /// Get an option value.
    PString GetString(const char *              key,    const char * dflt = NULL) const { PConstCaselessString k(key); return GetString(k, dflt); }
    PString GetString(const std::string     &   key,    const char * dflt = NULL) const { return GetString(PCaselessString(key), dflt); }
    PString GetString(const PString         &   key,    const char * dflt = NULL) const { return GetString(PCaselessString(key), dflt); }
    PString GetString(const PCaselessString &   key,    const char * dflt = NULL) const;
    PString GetString(const PCaselessString & (*key)(), const char * dflt = NULL) const { return GetString(key(), dflt); }

    /// Set the option value.
    bool SetString(const char *              key,    const PString & value) { return SetAt(key, value); }
    bool SetString(const std::string     &   key,    const PString & value) { return SetAt(key, value); }
    bool SetString(const PString         &   key,    const PString & value) { return SetAt(key, value); }
    bool SetString(const PCaselessString &   key,    const PString & value) { return SetAt(key, value); }
    bool SetString(const PCaselessString & (*key)(), const PString & value) { return SetAt(key, value); }

    /// Get the option value as a boolean.
    bool GetBoolean(const char *              key,    bool dflt = false) const { PConstCaselessString k(key); return GetBoolean(k, dflt); }
    bool GetBoolean(const std::string     &   key,    bool dflt = false) const { return GetBoolean(PCaselessString(key), dflt); }
    bool GetBoolean(const PString         &   key,    bool dflt = false) const { return GetBoolean(PCaselessString(key), dflt); }
    bool GetBoolean(const PCaselessString &   key,    bool dflt = false) const;
    bool GetBoolean(const PCaselessString & (*key)(), bool dflt = false) const { return GetBoolean(key(), dflt); }

    /// Set the option value as a boolean.
    void SetBoolean(const char *              key,    bool value) { PConstCaselessString k(key); SetBoolean(k, value); }
    void SetBoolean(const std::string     &   key,    bool value) { SetBoolean(PCaselessString(key), value); }
    void SetBoolean(const PString         &   key,    bool value) { SetBoolean(PCaselessString(key), value); }
    void SetBoolean(const PCaselessString &   key,    bool value) { SetAt(key, value ? "true" : "false"); }
    void SetBoolean(const PCaselessString & (*key)(), bool value) { SetBoolean(key(), value); }

    /// Get the option value as an integer.
    long GetInteger(const char *              key,    long dflt = 0) const { PConstCaselessString k(key); return GetInteger(k, dflt); }
    long GetInteger(const std::string     &   key,    long dflt = 0) const { return GetInteger(PCaselessString(key), dflt); }
    long GetInteger(const PString         &   key,    long dflt = 0) const { return GetInteger(PCaselessString(key), dflt); }
    long GetInteger(const PCaselessString &   key,    long dflt = 0) const;
    long GetInteger(const PCaselessString & (*key)(), long dflt = 0) const { return GetInteger(key(), dflt); }

    /// Set an integer value for the particular MIME info field.
    void SetInteger(const char *              key,    long value) { PConstCaselessString k(key); SetInteger(k, value); }
    void SetInteger(const std::string     &   key,    long value) { SetInteger(PCaselessString(key), value); }
    void SetInteger(const PString         &   key,    long value) { SetInteger(PCaselessString(key), value); }
    void SetInteger(const PCaselessString &   key,    long value);
    void SetInteger(const PCaselessString & (*key)(), long value) { SetInteger(key(), value); }

    /// Get the option value as an enum.
    template<typename E> E GetEnum(const char *              key,    E dflt) const { PConstCaselessString k(key); return GetEnum(k, dflt); }
    template<typename E> E GetEnum(const std::string     &   key,    E dflt) const { return GetEnum(PCaselessString(key), dflt); }
    template<typename E> E GetEnum(const PString         &   key,    E dflt) const { return GetEnum(PCaselessString(key), dflt); }
    template<typename E> E GetEnum(const PCaselessString &   key,    E dflt) const { return (E)GetInteger(key, dflt); }
    template<typename E> E GetEnum(const PCaselessString & (*key)(), E dflt) const { return GetEnum(key(), dflt); }

    /// Set an enum value for the particular MIME info field.
    template<typename E> void SetEnum(const char *              key,    E value) { PConstCaselessString k(key); SetInteger(k, value); }
    template<typename E> void SetEnum(const std::string     &   key,    E value) { SetInteger(PCaselessString(key), value); }
    template<typename E> void SetEnum(const PString         &   key,    E value) { SetInteger(PCaselessString(key), value); }
    template<typename E> void SetEnum(const PCaselessString &   key,    E value) { SetInteger(key, value); }
    template<typename E> void SetEnum(const PCaselessString & (*key)(), E value) { SetInteger(key(), value); }

    /// Get the option value as a floating point real.
    double GetReal(const char *              key,    double dflt = 0) const { PConstCaselessString k(key); return GetReal(k, dflt); }
    double GetReal(const std::string     &   key,    double dflt = 0) const { return GetReal(PCaselessString(key), dflt); }
    double GetReal(const PString         &   key,    double dflt = 0) const { return GetReal(PCaselessString(key), dflt); }
    double GetReal(const PCaselessString &   key,    double dflt = 0) const;
    double GetReal(const PCaselessString & (*key)(), double dflt = 0) const { return GetReal(key(), dflt); }

    /// Set a floating point real value for the particular MIME info field.
    void SetReal(const char *              key,    double value, int decimals) { PConstCaselessString k(key); SetReal(k, value, decimals); }
    void SetReal(const std::string     &   key,    double value, int decimals) { SetReal(PCaselessString(key), value, decimals); }
    void SetReal(const PString         &   key,    double value, int decimals) { SetReal(PCaselessString(key), value, decimals); }
    void SetReal(const PCaselessString &   key,    double value, int decimals);
    void SetReal(const PCaselessString & (*key)(), double value, int decimals) { SetReal(key(), value, decimals); }

    /// Get the option value as any type.
    template<typename T> T GetVar(const char *              key,    const T & dflt) const { PConstCaselessString k(key); return GetVar<T>(k, dflt); }
    template<typename T> T GetVar(const std::string     &   key,    const T & dflt) const { return GetVar<T>(PCaselessString(key), dflt); }
    template<typename T> T GetVar(const PString         &   key,    const T & dflt) const { return GetVar<T>(PCaselessString(key), dflt); }
    template<typename T> T GetVar(const PCaselessString &   key,    const T & dflt) const { PStringStream s(GetString(key)); T v; s>>v; return s.fail() || s.bad() ? dflt : v; }
    template<typename T> T GetVar(const PCaselessString & (*key)(), const T & dflt) const { return GetVar<T>(key(), dflt); }

    template<typename T> bool SetVar(const char *              key,    const T & value) { PConstCaselessString k(key); return SetVar<T>(k, value); }
    template<typename T> bool SetVar(const std::string     &   key,    const T & value) { return SetVar<T>(PCaselessString(key), value); }
    template<typename T> bool SetVar(const PString         &   key,    const T & value) { return SetVar<T>(PCaselessString(key), value); }
    template<typename T> bool SetVar(const PCaselessString &   key,    const T & value) { return SetString(key, PSTRSTRM(value)); }
    template<typename T> bool SetVar(const PCaselessString & (*key)(), const T & value) { return SetVar<T>(key(), value); }

    /// Determine of the option exists.
    __inline bool Has(const char            * key)      const { return Contains(key); }
    __inline bool Has(const std::string     & key)      const { return Contains(key); }
    __inline bool Has(const PString         & key)      const { return Contains(key); }
    __inline bool Has(const PCaselessString & key)      const { return Contains(key); }
    __inline bool Has(const PCaselessString & (*key)()) const { return Contains(key); }

    /// Get the option value.
    __inline PString Get(const char *              key,    const char * dflt = NULL) const { return GetString(key, dflt); }
    __inline PString Get(const PString         &   key,    const char * dflt = NULL) const { return GetString(key, dflt); }
    __inline PString Get(const std::string     &   key,    const char * dflt = NULL) const { return GetString(key, dflt); }
    __inline PString Get(const PCaselessString &   key,    const char * dflt = NULL) const { return GetString(key, dflt); }
    __inline PString Get(const PCaselessString & (*key)(), const char * dflt = NULL) const { return GetString(key, dflt); }
    __inline PString Get(const char *              key,    const std::string & dflt) const { return GetString(key, dflt.c_str()); }
    __inline PString Get(const std::string     &   key,    const std::string & dflt) const { return GetString(key, dflt.c_str()); }
    __inline PString Get(const PString         &   key,    const std::string & dflt) const { return GetString(key, dflt.c_str()); }
    __inline PString Get(const PCaselessString &   key,    const std::string & dflt) const { return GetString(key, dflt.c_str()); }
    __inline PString Get(const PCaselessString & (*key)(), const std::string & dflt) const { return GetString(key, dflt.c_str()); }
    __inline PString Get(const char *              key,    const PString     & dflt) const { return GetString(key, dflt); }
    __inline PString Get(const std::string     &   key,    const PString     & dflt) const { return GetString(key, dflt); }
    __inline PString Get(const PString         &   key,    const PString     & dflt) const { return GetString(key, dflt); }
    __inline PString Get(const PCaselessString &   key,    const PString     & dflt) const { return GetString(key, dflt); }
    __inline PString Get(const PCaselessString & (*key)(), const PString     & dflt) const { return GetString(key, dflt); }

    /// Set the option value.
    __inline bool Set(const char *              key,    const PString & value) { return SetAt(key, value); }
    __inline bool Set(const std::string     &   key,    const PString & value) { return SetAt(key, value); }
    __inline bool Set(const PString         &   key,    const PString & value) { return SetAt(key, value); }
    __inline bool Set(const PCaselessString &   key,    const PString & value) { return SetAt(key, value); }
    __inline bool Set(const PCaselessString & (*key)(), const PString & value) { return SetAt(key, value); }

    /// Remove option value
    __inline void Remove(const char *              key)    { RemoveAt(key); }
    __inline void Remove(const std::string     &   key)    { RemoveAt(key); }
    __inline void Remove(const PString         &   key)    { RemoveAt(key); }
    __inline void Remove(const PCaselessString &   key)    { RemoveAt(key); }
    __inline void Remove(const PCaselessString & (*key)()) { RemoveAt(key); }
};


/**A class representing a regular expression that may be used for locating
   patterns in strings. The regular expression string is "compiled" into a
   form that is more efficient during the matching. This compiled form
   exists for the lifetime of the PRegularExpression instance.
 */
class PRegularExpression : public PObject
{
    PCLASSINFO(PRegularExpression, PObject);
  public:
  /**@name Constructors & destructors */
  //@{
    /// Flags for compiler options.
    P_DECLARE_BITWISE_ENUM(
      CompileOptions, 3,
      (
        Simple,         ///< No options, simple regex
        Extended,       ///< Use extended regular expressions
        IgnoreCase,     ///< Ignore case in search.
        AnchorNewLine   /**< If this bit is set, then anchors do not match at
                              newline characters in the string. If not set,
                              then anchors do match at newlines. */
      )
    );

    /// Flags for execution options.
    P_DECLARE_BITWISE_ENUM(
      ExecOptions, 2,
      (
        Normal,             ///< Normal execution
        NotBeginningOfLine, /**< If this bit is set, then the beginning-of-line
                                 operator doesn't match the beginning of the string
                                 (presumably because it's not the beginning of a
                                 line). If not set, then the beginning-of-line
                                 operator does match the beginning of the string. */
        NotEndofLine        /**< Like <code>NotBeginningOfLine</code>, except for
                                 the end-of-line.  */
      )
    );

    /// Create a new, empty, regular expression
    PRegularExpression();

    /** Create and compile a new regular expression pattern.
     */
    PRegularExpression(
      const PString & pattern,            ///< Pattern to compile
      CompileOptions options = IgnoreCase ///< Pattern match options
    );

    /** Create and compile a new regular expression pattern.
     */
    PRegularExpression(
      const char * cpattern,              ///< Pattern to compile
      CompileOptions options = IgnoreCase ///< Pattern match options
    );

    /**
      * Copy a regular expression
      */
    PRegularExpression(
      const PRegularExpression &
    );

    /**
      * Assign a regular expression
      */
    PRegularExpression & operator =(
      const PRegularExpression &
    );

    /// Release storage for the compiled regular expression.
    ~PRegularExpression();
  //@}


  /**@name Overrides from class PObject */
  //@{
    /**Output the regular expression to the specified stream.
     */
    virtual void PrintOn(
      ostream & strm  ///< I/O stream to output to.
    ) const;
  //@}

  /**@name Status functions */
  //@{
    /// Error codes.
    enum ErrorCodes {
      /// Success.
      NoError = 0,    
      /// Didn't find a match (for regexec).
      NoMatch,      

      // POSIX regcomp return error codes.  (In the order listed in the standard.)
      /// Invalid pattern.
      BadPattern,  
      /// Not implemented.
      CollateError,  
      /// Invalid character class name.
      BadClassType,  
      /// Trailing backslash.
      BadEscape,    
      /// Invalid back reference.
      BadSubReg,
      /// Unmatched left bracket.
      UnmatchedBracket, 
      /// Parenthesis imbalance.
      UnmatchedParen,
      /// Unmatched <b>\\</b>.
      UnmatchedBrace,
      /// Invalid contents of <b>\\</b>.
      BadBR,        
      /// Invalid range end.
      RangeError,  
      /// Ran out of memory.
      OutOfMemory,
      /// No preceding re for repetition op.
      BadRepitition,

      /* Error codes we've added.  */
      /// Premature end.
      PrematureEnd,
      /// Compiled pattern bigger than 2^16 bytes.
      TooBig,
      /// Unmatched ) or \\); not returned from regcomp.
      UnmatchedRParen,
      /// Miscellaneous error
      NotCompiled
    };

    /**Get the error code for the last Compile() or Execute() operation.

       @return
       Error code.
     */
    ErrorCodes GetErrorCode() const { return m_lastError; }

    /**Get the text description for the error of the last Compile() or
       Execute() operation.

       @return
       Error text string.
     */
    PString GetErrorText() const;

    /** Return the string which represents the pattern matched by the regular expression. */
    const PString & GetPattern() const { return m_pattern; }
  //@}

  /**@name Compile & Execute functions */
  //@{
    /** Compiler pattern. */
    bool Compile(
      const PString & pattern,            ///< Pattern to compile
      CompileOptions options = IgnoreCase ///< Pattern match options
    );
    /**Compiler pattern.
       The pattern is compiled into an internal format to speed subsequent
       execution of the pattern match algorithm.

       @return
       true if successfully compiled.
     */
    bool Compile(
      const char * cpattern,              ///< Pattern to compile
      CompileOptions options = IgnoreCase ///< Pattern match options
    );


    /** Execute regular expression */
    bool Execute(
      const PString & str,          ///< Source string to search
      PINDEX & start,               ///< First match location
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /** Execute regular expression */
    bool Execute(
      const PString & str,          ///< Source string to search
      PINDEX & start,               ///< First match location
      PINDEX & len,                 ///< Length of match
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /** Execute regular expression */
    bool Execute(
      const char * cstr,            ///< Source string to search
      PINDEX & start,               ///< First match location
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /** Execute regular expression */
    bool Execute(
      const char * cstr,            ///< Source string to search
      PINDEX & start,               ///< First match location
      PINDEX & len,                 ///< Length of match
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /** Execute regular expression */
    bool Execute(
      const PString & str,          ///< Source string to search
      PIntArray & starts,           ///< Array of match locations
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /** Execute regular expression */
    bool Execute(
      const PString & str,          ///< Source string to search
      PIntArray & starts,           ///< Array of match locations
      PIntArray & ends,             ///< Array of match ends
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /** Execute regular expression */
    bool Execute(
      const char * cstr,            ///< Source string to search
      PIntArray & starts,           ///< Array of match locations
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /**Execute regular expression.
       Execute the pattern match algorithm using the previously compiled
       pattern.

       The \p starts and \p ends arrays indicate the start and end positions
       of the matched substrings in \p cstr. The 0th member is filled in to
       indicate what substring was matched by the entire regular expression.
       The remaining members report what substring was matched by parenthesized
       subexpressions within the regular expression.

       The caller should set the size of the \p starts array before calling to
       indicated how many subexpressions are expected. If the \p starts array
       is empty, it will be set to one entry.

       @return true if successfully compiled.
     */
    bool Execute(
      const char * cstr,            ///< Source string to search
      PIntArray & starts,           ///< Array of match locations
      PIntArray & ends,             ///< Array of match ends
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
    /**Execute regular expression.
       Execute the pattern match algorithm using the previously compiled
       pattern.

       The \p substring arrays is filled with the matched substrings out of
       \p cstr. The 0th member is filled in to indicate what substring was
       matched by the entire regular expression. The remaining members report
       what substring was matched by parenthesized subexpressions within the
       regular expression.

       The caller should set the size of the \p substring array before calling
       to indicated how many subexpressions are expected. If the \p substring
       array is empty, it will be set to one entry.

       @return true if successfully compiled.
     */
    bool Execute(
      const char * cstr,            ///< Source string to search
      PStringArray & substring,     ///< Array of matched substrings
      ExecOptions options = Normal  ///< Pattern match options
    ) const;
  //@}

  /**@name Miscellaneous functions */
  //@{
    /**Escape all characters in the <code>str</code> parameter that have a
       special meaning within a regular expression.

       @return
       String with additional escape ('\\') characters.
     */
    static PString EscapeString(
      const PString & str     ///< String to add esacpes to.
    );
  //@}

  protected:
    bool InternalCompile(bool assertOnFail);
    void InternalClean();

    PString            m_pattern;
    CompileOptions     m_compileOptions;
    void             * m_compiledRegex;
    mutable ErrorCodes m_lastError;
};


#endif // PTLIB_STRING_H


// End Of File ///////////////////////////////////////////////////////////////
