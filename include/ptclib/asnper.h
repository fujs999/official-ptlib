/*
 * asnper.h
 *
 * Abstract Syntax Notation Encoding Rules classes
 *
 * Portable Tools Library
 *
 */

#ifdef P_INCLUDE_PER


/** Class for ASN Packed Encoding Rules stream.
*/
class PPER_Stream : public PASN_Stream
{
    PCLASSINFO(PPER_Stream, PASN_Stream);
  public:
    PPER_Stream(int aligned = true);
    PPER_Stream(const uint8_t * buf, PINDEX size, bool aligned = true);
    PPER_Stream(const PBYTEArray & bytes, bool aligned = true);

    PPER_Stream & operator=(const PBYTEArray & bytes);

    unsigned GetBitsLeft() const;

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

    bool IsAligned() const { return aligned; }

    bool SingleBitDecode();
    void SingleBitEncode(bool value);

    bool MultiBitDecode(unsigned nBits, unsigned & value);
    void MultiBitEncode(unsigned value, unsigned nBits);

    bool SmallUnsignedDecode(unsigned & value);
    void SmallUnsignedEncode(unsigned value);

    bool LengthDecode(unsigned lower, unsigned upper, unsigned & len);
    void LengthEncode(unsigned len, unsigned lower, unsigned upper);

    bool UnsignedDecode(unsigned lower, unsigned upper, unsigned & value);
    void UnsignedEncode(int value, unsigned lower, unsigned upper);

    void AnyTypeEncode(const PASN_Object * value);

  protected:
    bool aligned;
};

#endif


// End Of File ///////////////////////////////////////////////////////////////
