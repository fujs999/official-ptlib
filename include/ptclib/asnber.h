/*
 * asnper.h
 *
 * Abstract Syntax Notation Encoding Rules classes
 *
 * Portable Tools Library
 *
 */

#ifdef P_INCLUDE_BER

/** Class for ASN basic Encoding Rules stream.
*/
class PBER_Stream : public PASN_Stream
{
    PCLASSINFO(PBER_Stream, PASN_Stream);
  public:
    PBER_Stream();
    PBER_Stream(const PBYTEArray & bytes);
    PBER_Stream(const uint8_t * buf, PINDEX size);

    PBER_Stream & operator=(const PBYTEArray & bytes);

    virtual bool Read(PChannel & chan);
    virtual bool Write(PChannel & chan);

    virtual bool NullDecode(PASN_Null &);
    virtual void NullEncode(const PASN_Null &);
    virtual bool BooleanDecode(PASN_Boolean &);
    virtual void BooleanEncode(const PASN_Boolean &);
    virtual bool IntegerDecode(PASN_Integer &);
    virtual void IntegerEncode(const PASN_Integer &);
    virtual bool EnumerationDecode(PASN_Enumeration &);
    virtual void EnumerationEncode(const PASN_Enumeration &);
    virtual bool RealDecode(PASN_Real &);
    virtual void RealEncode(const PASN_Real &);
    virtual bool ObjectIdDecode(PASN_ObjectId &);
    virtual void ObjectIdEncode(const PASN_ObjectId &);
    virtual bool BitStringDecode(PASN_BitString &);
    virtual void BitStringEncode(const PASN_BitString &);
    virtual bool OctetStringDecode(PASN_OctetString &);
    virtual void OctetStringEncode(const PASN_OctetString &);
    virtual bool ConstrainedStringDecode(PASN_ConstrainedString &);
    virtual void ConstrainedStringEncode(const PASN_ConstrainedString &);
    virtual bool BMPStringDecode(PASN_BMPString &);
    virtual void BMPStringEncode(const PASN_BMPString &);
    virtual bool ChoiceDecode(PASN_Choice &);
    virtual void ChoiceEncode(const PASN_Choice &);
    virtual bool ArrayDecode(PASN_Array &);
    virtual void ArrayEncode(const PASN_Array &);
    virtual bool SequencePreambleDecode(PASN_Sequence &);
    virtual void SequencePreambleEncode(const PASN_Sequence &);
    virtual bool SequenceKnownDecode(PASN_Sequence &, PINDEX, PASN_Object &);
    virtual void SequenceKnownEncode(const PASN_Sequence &, PINDEX, const PASN_Object &);
    virtual bool SequenceUnknownDecode(PASN_Sequence &);
    virtual void SequenceUnknownEncode(const PASN_Sequence &);

    virtual PASN_Object * CreateObject(unsigned tag,
                                       PASN_Object::TagClass tagClass,
                                       bool primitive) const;

    bool HeaderDecode(unsigned & tagVal,
                      PASN_Object::TagClass & tagClass,
                      bool & primitive,
                      unsigned & len);
    bool HeaderDecode(PASN_Object & obj, unsigned & len);
    void HeaderEncode(const PASN_Object & obj);
};


#endif


// End Of File ///////////////////////////////////////////////////////////////
