/*
 * pssl.h
 *
 * Secure Sockets Layer channel interface class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
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
 * Contributor(s): ______________________________________.
 */

#ifndef PTLIB_PSSL_H
#define PTLIB_PSSL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#if P_SSL

#include <ptlib/sockets.h>


struct ssl_st;
struct ssl_ctx_st;
struct x509_st;
struct X509_name_st;
struct evp_pkey_st;
struct evp_cipher_ctx_st;
struct dh_st;
struct aes_key_st;
struct SHAstate_st;
struct bio_method_st;
struct bio_st;


enum PSSLFileTypes {
  PSSLFileTypePEM,
  PSSLFileTypeASN1,
  PSSLFileTypeDEFAULT
};


typedef PNotifierTemplate<bool> PSSLPasswordNotifier;
#define PDECLARE_SSLPasswordNotifier(cls, fn) PDECLARE_NOTIFIER2(PString, cls, fn, bool)


/**Private key for SSL.
   This class embodies a common environment for all private keys used by the
   PSSLContext and PSSLChannel classes.
  */
class PSSLPrivateKey : public PObject
{
  PCLASSINFO(PSSLPrivateKey, PObject);
  public:
    /**Create an empty private key.
      */
    PSSLPrivateKey();

    /**Create a new RSA private key.
      */
    PSSLPrivateKey(
      unsigned modulus,   ///< Number of bits
      void (*callback)(int,int,void *) = NULL,  ///< Progress callback function
      void *cb_arg = NULL                       ///< Argument passed to callback
    );

    /**Create a new private key given the file.
       The type of the private key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PSSLPrivateKey(
      const PFilePath & keyFile,  ///< Private key file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Create private key from the binary ASN1 DER encoded data specified.
      */
    PSSLPrivateKey(
      const BYTE * keyData,   ///< Private key data
      PINDEX keySize          ///< Size of private key data
    );

    /**Create private key from the binary ASN1 DER encoded data specified.
      */
    PSSLPrivateKey(
      const PBYTEArray & keyData  ///< Private key data
    );

    /**Create a copy of the private key.
      */
    PSSLPrivateKey(
      const PSSLPrivateKey & privKey
    );
    PSSLPrivateKey(
      evp_pkey_st * privKey,
      bool duplicate = true
    );

    /**Create a copy of the private key.
      */
    PSSLPrivateKey & operator=(
      const PSSLPrivateKey & privKay
    );
    PSSLPrivateKey & operator=(
      evp_pkey_st * privKay
    );

    /**Destroy and release storage for private key.
      */
    ~PSSLPrivateKey();

    /**Get internal OpenSSL private key structure.
      */
    operator evp_pkey_st *() const { return m_pkey; }

    /**Set internal OpenSSL private key structure.
      */
    void Attach(evp_pkey_st * key);

    /**Create a new private key.
     */
    PBoolean Create(
      unsigned modulus,   ///< Number of bits
      void (*callback)(int,int,void *) = NULL,  ///< Progress callback function
      void *cb_arg = NULL                       ///< Argument passed to callback
    );

    /**Return true if is a valid private key.
     */
    bool IsValid() const { return m_pkey != NULL; }

    /**Set the certificate as binary ASN1 DER encoded data.
      */
    bool SetData(
      const PBYTEArray & data
    );

    /**Get the certificate as binary ASN1 DER encoded data.
      */
    PBYTEArray GetData() const;

    /**Get the certificate as ASN1 DER base64 encoded data.
      */
    PString AsString() const;

    /**Set the certificate from ASN1 DER base64 encoded data.
      */
    bool Parse(
      const PString & keyStr
    );

    /**Load private key from file.
       The type of the private key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Load(
      const PFilePath & keyFile,  ///< Private key file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT,  ///< Type of file to read
      const PSSLPasswordNotifier & notifier = PSSLPasswordNotifier()  ///< Call back for asking of password
    );

    /**Save private key to file.
       The type of the private key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Save(
      const PFilePath & keyFile,  ///< Private key file
      PBoolean append = false,        ///< Append to file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to write
    );


  protected:
    void FreePrivateKey();
    evp_pkey_st * m_pkey;
};


/**Certificate for SSL.
   This class embodies a common environment for all certificates used by the
   PSSLContext and PSSLChannel classes.
  */
class PSSLCertificate : public PObject
{
    PCLASSINFO(PSSLCertificate, PObject);
  public:
    /**Create an empty certificate.
      */
    PSSLCertificate();

    /**Create a new certificate given the file.
       The type of the certificate key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PSSLCertificate(
      const PFilePath & certFile, ///< Certificate file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Create certificate from the binary ASN1 DER encoded data specified.
      */
    PSSLCertificate(
      const BYTE * certData,  ///< Certificate data
      PINDEX certSize        ///< Size of certificate data
    );

    /**Create certificate from the binary ASN1 DER encoded data specified.
      */
    PSSLCertificate(
      const PBYTEArray & certData  ///< Certificate data
    );

    /**Create certificate from the ASN1 DER base64 encoded data specified.
      */
    PSSLCertificate(
      const PString & certString  ///< Certificate data as string
    );

    /**Create a copy of the certificate.
      */
    PSSLCertificate(
      const PSSLCertificate & cert
    );
    PSSLCertificate(
      x509_st * cert,
      bool duplicate = true
    );

    /**Create a copy of the certificate.
      */
    PSSLCertificate & operator=(
      const PSSLCertificate & cert
    );
    PSSLCertificate & operator=(
      x509_st * cert
    );

    /**Destroy and release storage for certificate.
      */
    ~PSSLCertificate();

    /**Get internal OpenSSL X509 structure.
      */
    operator x509_st *() const { return m_certificate; }

    /**Set internal OpenSSL X509 structure.
      */
    void Attach(x509_st * cert);

    /**Return true if is a valid certificate.
     */
    bool IsValid() const { return m_certificate != NULL; }

    
    /**Create a new root certificate.
       The subject name is a string of the form "/name=value/name=value" where
       name is a short name for the field and value is a string value for the
       field for example:
          "/C=ZA/SP=Western Cape/L=Cape Town/O=Thawte Consulting cc"
          "/OU=Certification Services Division/CN=Thawte Server CA"
          "/Email=server-certs@thawte.com"
     */
    bool CreateRoot(
      const PString & subject,    ///< Subject name for certificate
      const PSSLPrivateKey & key, ///< Key to sign certificate with
      const char * digest = NULL, ///< Digest algorithm to use, default is "sha-1"
      unsigned version = 0        ///< Certificate version, default latest
    );

    /**Set the certificate as binary ASN1 DER encoded data.
      */
    bool SetData(
      const PBYTEArray & data
    );

    /**Get the certificate as binary ASN1 DER encoded data.
      */
    PBYTEArray GetData() const;

    /**Get the certificate as ASN1 DER base64 encoded data.
      */
    PString AsString() const;

    /**Set the certificate from ASN1 DER base64 encoded data.
      */
    bool Parse(
      const PString & certStr
    );

    /**Load certificate from file.
       The type of the certificate key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Load(
      const PFilePath & certFile, ///< Certificate file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Save certificate to file.
       The type of the certificate key can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Save(
      const PFilePath & keyFile,  ///< Certificate key file
      PBoolean append = false,        ///< Append to file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to write
    );

    class X509_Name : public PObject {
        PCLASSINFO(X509_Name, PObject);
      public:
        X509_Name(X509_name_st * name = NULL) : m_name(name) { }

        Comparison Compare(const PObject & other) const;
        void PrintOn(ostream & strm) const;

        bool IsValid() const { return m_name != NULL; }

        PString GetCommonName() const;
        PString GetNID(int id) const;
        PString AsString(
          int indent = -1  // Negative means single line
        ) const;

      protected:
        X509_name_st * m_name;
    };

    /**Get certificate issuer name.
      */
    bool GetIssuerName(X509_Name & name) const;

    /**Get certificate subject name.
      */
    bool GetSubjectName(X509_Name & name) const;
    PString GetSubjectName() const;

    /**Get certificate alternate subject name.
      */
    PString GetSubjectAltName() const;

    P_DECLARE_BITWISE_ENUM(CheckHostFlags, 5, (
      CheckHostNormalRules,
      CheckHostAlwaysUseSubject,
      CheckHostNoWildcards,
      CheckHostNoPartialWildcards,
      CheckHostMultiLabelWildcards,
      CheckHostSingleLabelDomains
    ));

    /**Check the host name against the certificate.
      */
    bool CheckHostName(
      const PString & hostname,
      CheckHostFlags flags = CheckHostNormalRules
    );

    virtual void PrintOn(ostream & strm) const { strm << GetSubjectName(); }

    typedef std::list<x509_st *> X509_Chain;
    const X509_Chain & GetChain() const { return m_chain; }

  protected:
    void FreeCertificate();
    x509_st * m_certificate;
    X509_Chain m_chain;
};


/** Create a "fingerprint" for SSL certificate.
  */
class PSSLCertificateFingerprint : public PObject
{
    PCLASSINFO(PSSLCertificateFingerprint, PObject);
  public:
    P_DECLARE_ENUM(HashType,
      HashMd5,
      HashSha1,
      HashSha256,
      HashSha512
    );

    /// Create empty fingerprint.
    PSSLCertificateFingerprint();

    /** Create fingerprint by parsing string in format: "type fingerprint"
     */
    PSSLCertificateFingerprint(
      const PString& inStr    ///< Fingerprint string.
    );

    /** Create fingerprint with specified type from certificate.
     */
    PSSLCertificateFingerprint(
      HashType type,                      ///< Hasing algorithm to use
      const PSSLCertificate& certificate  ///< Certificate to fingerprint.
    );

    Comparison Compare(const PObject & other) const;

    bool IsValid() const;

    bool MatchForCertificate(
      const PSSLCertificate& cert
    ) const;

    PString AsString() const;
    bool FromString(const PString & str);

    HashType GetHash() const { return m_hashAlogorithm; }
    void SetHash(HashType hash) { m_hashAlogorithm = hash; }

  private:
    HashType m_hashAlogorithm;
    PString  m_fingerprint;
};


/**Diffie-Hellman key exchange conteext.
   This class embodies a set of Diffie Helman key exchange parameters and
   context.
  */
class PSSLDiffieHellman : public PObject
{
  PCLASSINFO(PSSLDiffieHellman, PObject);
  public:
    /**Create an empty set of Diffie-Hellman parameters.
      */
    PSSLDiffieHellman();

    /**Create a new set of Diffie-Hellman parameters given the file.
       The type of the file can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PSSLDiffieHellman(
      const PFilePath & dhFile, ///< Diffie-Hellman parameters file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Create a set of Diffie-Hellman parameters.
       If \p pubKey is NULL, it is automatically generated.

       Note, all three buffers (if not NULL) must point to numBits/8 bytes.
      */
    PSSLDiffieHellman(
      PINDEX numBits,            ///< Number of bits
      const BYTE * pData,        ///< Modulus data (numBits/8 bytes)
      const BYTE * gData,        ///< Generator data (numBits/8 bytes)
      const BYTE * pubKey = NULL ///< Public key data (numBits/8 bytes)
    );

    /**Create a set of Diffie-Hellman parameters.
       If \p pubKey is empty, it is automatically generated.
      */
    PSSLDiffieHellman(
      const PBYTEArray & pData,   ///< Modulus data
      const PBYTEArray & gData,   ///< Generator data
      const PBYTEArray & pubKey = PBYTEArray() ///< Public key data
    );

    /**Create a copy of the Diffie-Hellman parameters.
      */
    PSSLDiffieHellman(
      const PSSLDiffieHellman & dh
    );

    /**Create a copy of the Diffie-Hellman parameters.
      */
    PSSLDiffieHellman & operator=(
      const PSSLDiffieHellman & dh
    );

    /**Destroy and release storage for Diffie-Hellman parameters.
      */
    ~PSSLDiffieHellman();

    /**Return true if is a valid Diffie-Hellman context.
     */
    bool IsValid() const { return m_dh != NULL; }

    /**Get internal OpenSSL DH structure.
      */
    operator dh_st *() const { return m_dh; }

    /**Load Diffie-Hellman parameters from file.
       The type of the file can be specified explicitly, or if
       PSSLFileTypeDEFAULT it will be determined from the file extension,
       ".pem" is a text file, anything else eg ".der" is a binary ASN1 file.
      */
    PBoolean Load(
      const PFilePath & dhFile, ///< Diffie-Hellman parameters file
      PSSLFileTypes fileType = PSSLFileTypeDEFAULT  ///< Type of file to read
    );

    /**Get number of bits being used.
      */
    PINDEX GetNumBits() const;

    /**Get the P value
      */
    PBYTEArray GetModulus() const;

    /**Get the G value
      */
    PBYTEArray GetGenerator() const;

    /**Get the "half-key" value
      */
    PBYTEArray GetHalfKey() const;

    /**Compute the session key, geven other half-key
      */
    bool ComputeSessionKey(const PBYTEArray & otherHalf);

    /**Get the session key value
      */
    const PBYTEArray & GetSessionKey() const { return m_sessionKey; }

  protected:
    bool Construct(const BYTE * pData, PINDEX pSize,
                   const BYTE * gData, PINDEX gSize,
                   const BYTE * kData, PINDEX kSize);

    dh_st    * m_dh;
    PBYTEArray m_sessionKey;
};


/// AES encryption scheme
class PAESContext : public PObject
{
  PCLASSINFO(PAESContext, PObject);
  public:
    PAESContext();
    PAESContext(bool encrypt, const void * key, PINDEX numBits);
    ~PAESContext();

    void SetEncrypt(const void * key, PINDEX numBits);
    void SetDecrypt(const void * key, PINDEX numBits);

    void Encrypt(const void * in, void * out);
    void Decrypt(const void * in, void * out);

  protected:
    aes_key_st * m_key;
};


/// Encryption/decryption context
class PSSLCipherContext : public PObject, PNonCopyable
{
    PCLASSINFO(PSSLCipherContext, PObject);
  public:
    PSSLCipherContext(
      bool encrypt
    );
    
    ~PSSLCipherContext();

    /**Get internal OpenSSL cipher context structure.
      */
    operator evp_cipher_ctx_st *() const { return m_context; }

    /// Indicate we are encrypting data
    bool IsEncrypt() const;

    /**Get selected algorithm
      */
    PString GetAlgorithm() const;

    /** Set encryption/decryption algorithm.
        The \p 
      */
    bool SetAlgorithm(
      const PString & name  ///< Name or OID for the algorithm.
    );

    /** Set encryption/decryption key.
      */
    bool SetKey(const PBYTEArray & key) { return SetKey(key, key.GetSize()); }
    bool SetKey(const BYTE * keyPtr, PINDEX keyLen);

    /** Set encryption/decryption initial vector.
      */
    bool SetIV(const PBYTEArray & iv) { return SetIV(iv, iv.GetSize()); }
    bool SetIV(const BYTE * ivPtr, PINDEX ivLen);

    enum PadMode {
      NoPadding,
      PadPKCS,
      PadLoosePKCS,
      PadCipherStealing
    };

    /**Set padding mode.
       If NoPadding, then buffers supplied to Process() must be exact multiple
       of the block size.
      */
    bool SetPadding(PadMode pad);

    /**Get padding mode.
       If NoPadding, then buffers supplied to Process() must be exact multiple
       of the block size.
      */
    PadMode GetPadding() const { return m_padMode; }

    /** Encrypt/Decrypt a block of data.
      */
    bool Process(
      const PBYTEArray & in,  ///< Data to be encrypted
      PBYTEArray & out        ///< Encrypted data
    );
    bool Process(
      const BYTE * inPtr,     ///< Data to be encrypted
      PINDEX inLen,           ///< Length of data to be encrypted
      BYTE * outPtr,          ///< Encrypted data
      PINDEX & outLen,        ///< Max output space on input, then actual output data size
      bool partial = false    ///< Partial data, more to come
    );

    /** Get the cipher key length
      */
    PINDEX GetKeyLength() const;

    /** Get the cipher initial vector length
      */
    PINDEX GetIVLength() const;

    /** Get the cipher block size
      */
    PINDEX GetBlockSize() const;

    /** Calculate the rounded up size for encrypted data
      */
    PINDEX GetBlockedDataSize(PINDEX size) const;

  protected:
    PadMode             m_padMode;
    bool                m_encrypt;
    evp_cipher_ctx_st * m_context;

    // ciphertext stealing code based on a OpenSSL patch by An-Cheng Huang
    unsigned char m_pad_buf[32];        // Saved partial block of input data
    unsigned char m_pad_final_buf[32];  // Last processed block of output data
    int           m_pad_buf_len;        // Number of bytes in m_pad_buf
    bool          m_pad_final_used;     // Indicates whether the final buffer is used
    bool UpdateCTS(unsigned char *out, int *outl, const unsigned char *in, int inl);
    bool EncryptFinalCTS(unsigned char *out, int *outl);
    bool DecryptFinalCTS(unsigned char *out, int *outl);

    // Relaxed decryption that doesn't verify contents of the padding in the last decrypted block
    bool UpdateLoose(unsigned char *out, int *outl, const unsigned char *in, int inl);
    bool DecryptUpdateLoose(unsigned char *out, int *outl, const unsigned char *in, int inl);
    bool DecryptFinalLoose(unsigned char *out, int *outl);
};


/// SHA1 digest scheme
class PSHA1Context : public PObject, PNonCopyable
{
  PCLASSINFO(PSHA1Context, PObject);
  public:
    PSHA1Context();
    ~PSHA1Context();

    enum { BlockSize = 64 };

    void Update(const void * data, PINDEX length);
    void Update(const PString & str) { Update((const char *)str, str.GetLength()); }

    typedef BYTE Digest[20];
    void Finalise(Digest result);

    static void Process(const void * data, PINDEX length, Digest result);
    static void Process(const PString & str, Digest result) { Process((const char *)str, str.GetLength(), result); }

  protected:
    SHAstate_st * m_context;
};


class PSSLContext;

/** Encapsulation of the infromation to handle certificates with SSL.
 */
class PSSLCertificateInfo
{
public:
  ///  Construction
  PSSLCertificateInfo(
    bool withDefaults = false
  );
  PSSLCertificateInfo(
    const PString & authority,    ///< Certificate Authority directory, file or data
    const PString & certificate,  ///< Local certificate filename or data
    const PString & privateKey,   ///< Private key filename or data for above local certificate
    bool autoCreate = false       ///< If certificate/privateKey are file paths, and do not exist, then create.
  );

  virtual ~PSSLCertificateInfo() { }

  PSSLCertificateInfo(const PSSLCertificateInfo & info) { SetSSLCredentials(info); }
  PSSLCertificateInfo & operator=(const PSSLCertificateInfo & info) { SetSSLCredentials(info); return *this; }

  /** Apply the SSL certificates/key for SSL based calls, e.g. sips or h323s
      This function loads the certificates and keys for use by a OpalListener
      or OpalTransport on the \p endpoint parameter. It allows for embedded
      certificates and keys, while the default behaviour loads the
      certificates and keys from files pointed to by member variables.

      Note that a listener must have a cert/key and may have CA directory/list
      for bi-directional authentication. A transport should have the CA
      directory/list set, and if missing then no server authentication is
      performed. Similarly if a transport may have an optional cert/key for
      bi-directional authentication.
    */
  virtual bool ApplySSLCredentials(
    PSSLContext & context,    ///< Context on which to set certificates
    bool create = true        ///< Create self signed cert/key if required & enabled
  ) const;

  /**Get the Certificate Authority. This may be:
      filenames (';' separated),
      a directory of certificate files,
      direct certificate data,
      '*' for system defined default.
    */
  PString GetSSLCertificateAuthority() const;

  /**Set the Certificate Authority. This may be:
      filenames (';' separated),
      a directory of certificate files,
      direct certificate data.
    */
  void SetSSLCertificateAuthority(const PString & ca);

  /**Get the default local certificate filename or data
    */
  PString GetSSLCertificate() const;

  /**Set the default local certificate filename or data
    */
  void SetSSLCertificate(const PString & cert);

  /**Get the default local private key filename or data
    */
  PString GetSSLPrivateKey() const;

  /**Set the default local private key filename or data
    */
  void SetSSLPrivateKey(const PString & key);

  /**Set flag to auto-create a self signed root certificate and private key.
   */
  void SetSSLAutoCreateCertificate(bool yes);

  /**Get flag to auto-create a self signed root certificate and private key.
   */
  bool GetSSLAutoCreateCertificate() const;

  /**Set all the certificate info. */
  void SetSSLCredentials(
    const PString & authority,    ///< Certificate Authority directory, file or data
    const PString & certificate,  ///< Local certificate filename or data
    const PString & privateKey,   ///< Private key filename or data for above local certificate
    bool autoCreate = false       ///< If certificate/privateKey are file paths, and do not exist, then create.
  );
  void SetSSLCredentials(
    const PSSLCertificateInfo & info  ///< Other certificate info to copy from
  );

  /**Get all the certificate info. */
  void GetSSLCredentials(
    PString & authority,    ///< Certificate Authority directory, file or data
    PString & certificate,  ///< Local certificate filename or data
    PString & privateKey,   ///< Private key filename or data for above local certificate
    bool & autoCreate       ///< If certificate/privateKey are file paths, and do not exist, then create.
  ) const;

  ///  Indicate if there is certficate info available.
  bool HasSSLCertificates() const;

  P_DEPRECATED PString GetSSLCertificateAuthorityFiles() const { return GetSSLCertificateAuthority(); }
  P_DEPRECATED void SetSSLCertificateAuthorityFiles(const PString & ca) { SetSSLCertificateAuthority(ca); }
  P_DEPRECATED PString GetSSLCertificateFile() const { return GetSSLCertificate();  }
  P_DEPRECATED void SetSSLCertificateFile(const PString & cert) { SetSSLCertificate(cert); }
  P_DEPRECATED PString GetSSLPrivateKeyFile() const { return GetSSLPrivateKey(); }
  P_DEPRECATED void SetSSLPrivateKeyFile(const PString & key) { SetSSLPrivateKey(key); }

protected:
  PString m_sslCertificateAuthority;
  PString m_sslCertificate;
  PString m_sslPrivateKey;
  bool    m_sslAutoCreateCertificate;

private:
  mutable PCriticalSection m_sslInfoMutex;
};


/**Context for SSL channels.
   This class embodies a common environment for all connections made via SSL
   using the PSSLChannel class. It includes such things as the version of SSL
   and certificates, CA's etc.
  */
class PSSLContext : public PObject, PNonCopyable
{
    PCLASSINFO(PSSLContext, PObject);
  public:
    P_DECLARE_TRACED_ENUM(Method,
      SSLv3,
      TLSv1,
      TLSv1_1,
      TLSv1_2,
      TLSv1_3,
      DTLSv1,   // Note DTLS should always be greater than TLS
      DTLSv1_2,
      DTLSv1_2_v1_0
    );

    static const Method HighestTLS = TLSv1_3;
    static const Method HighestDTLS = DTLSv1_2_v1_0;

    /**Create a new context for SSL channels.
       An optional session ID may be provided in the context. This is used
       to identify sessions across multiple channels in this context. The
       session ID is a completely arbitrary block of data. If sessionId is
       non NULL and idSize is zero, then sessionId is assumed to be a pointer
       to a C string.
       The default SSL method is TLSv1
      */
    PSSLContext(
      const void * sessionId = NULL,  ///< Pointer to session ID
      PINDEX idSize = 0               ///< Size of session ID
    );
    PSSLContext(
      Method method,                  ///< SSL connection method
      const void * sessionId = NULL,  ///< Pointer to session ID
      PINDEX idSize = 0               ///< Size of session ID
    );

    /**Clean up the SSL context.
      */
    ~PSSLContext();

    /**Get the internal SSL context structure.
      */
    operator ssl_ctx_st *() const { return m_context; }

    /**Set the locations for CA certificates used to verify peer certificates.
      */
    bool SetVerifyLocations(
      const PFilePath & caFile, ///< File for CA certificates
      const PDirectory & caDir  ///< Directory for CA certificates
    );

    /**Set the locations for CA certificates used to verify peer certificates.
      */
    bool SetVerifyDirectory(
      const PDirectory & caDir  ///< Directory for CA certificates
    );

    /**Set the locations for CA certificates used to verify peer certificates.
      */
    bool SetVerifyFile(
      const PFilePath & caFile  ///< File for CA certificates
    );

    /**Set the locations for CA certificates used to verify peer certificates.
      */
    bool SetVerifySystemDefault();

    /**Set the CA certificate used to verify peer certificates.
      */
    bool SetVerifyCertificate(
      const PSSLCertificate & cert
    );

    P_DECLARE_ENUM(VerifyMode,
      VerifyNone,
      VerifyPeer,
      VerifyPeerMandatory
    );

    /**Set certificate verification mode for connection.
      */
    void SetVerifyMode(
      VerifyMode mode,    ///< New verification mode
      unsigned depth = 9  ///< Verification depth (max number of certs in chain)
    );

    /**Set certificate verification mode for connection.
      */
    VerifyMode GetVerifyMode() const;

    /**Set the CA certificate(s) to send to client from server.
      */
    bool AddClientCA(
      const PSSLCertificate & certificate
    );
    bool AddClientCA(
      const PList<PSSLCertificate> & certificates
    );

    /**Use the certificate specified.
      */
    bool UseCertificate(
      const PSSLCertificate & certificate
    );

    /**Use the private key specified.
      */
    bool UsePrivateKey(
      const PSSLPrivateKey & key
    );

    /**Use the Diffie-Hellman parameters specified.
      */
    bool UseDiffieHellman(
      const PSSLDiffieHellman & dh
    );

    /**Set the available ciphers to those listed.
      */
    bool SetCipherList(
      const PString & ciphers   ///< List of cipher names.
    );

    /**Set the credentials for the context.
      */
    bool SetCredentials(
      const PSSLCertificateInfo & info,  ///< Informaation about certificates to use.
      bool create = false                ///< Create self signed cert/key if required & enabled
    );
    bool SetCredentials(
      const PString & authority,    ///< Certificate Authority directory, file or data
      const PString & certificate,  ///< Local certificate file or data
      const PString & privateKey,   ///< Private key file or data for local certificate
      bool create = false           ///< If certificate/provateKey are file paths and do not exist, then create.
    );

    /// Set the notifier for when SSL needs to get a password to unlock a private key.
    void SetPasswordNotifier(
      const PSSLPasswordNotifier & notifier   ///< Notifier to be called
    );

    /// Set TLS extension
    bool SetExtension(
      const char * extension
    );

    Method GetMethod() const { return m_method; }

  protected:
    void Construct(const void * sessionId, PINDEX idSize);

    Method       m_method;
    ssl_ctx_st * m_context;
    PSSLPasswordNotifier m_passwordNotifier;
};


/**This class will start a secure SSL based channel.
  */
class PSSLChannel : public PIndirectChannel
{
  PCLASSINFO(PSSLChannel, PIndirectChannel)
  public:
    /**Create a new channel given the context.
       If no context is given a default one is created.
      */
    PSSLChannel(
      PSSLContext * context = NULL,   ///< Context for SSL channel
      PBoolean autoDeleteContext = false  ///< Flag for context to be automatically deleted.
    );
    PSSLChannel(
      PSSLContext & context           ///< Context for SSL channel
    );

    /**Close and clear the SSL channel.
      */
    ~PSSLChannel();

    // Overrides from PChannel
    virtual PBoolean Read(void * buf, PINDEX len);
    virtual PBoolean Write(const void * buf, PINDEX len);
    virtual PBoolean Close();
    virtual PBoolean Shutdown(ShutdownValue);
    virtual PString GetErrorText(ErrorGroup group = NumErrorGroups) const;
    virtual PBoolean ConvertOSError(P_INT_PTR libcReturnValue, ErrorGroup group = LastGeneralError);

    // New functions
    /**Accept a new inbound connection (server).
       This version expects that the indirect channel has already been opened
       using Open() beforehand.
      */
    PBoolean Accept();

    /**Accept a new inbound connection (server).
      */
    PBoolean Accept(
      PChannel & channel  ///< Channel to attach to.
    );

    /**Accept a new inbound connection (server).
      */
    PBoolean Accept(
      PChannel * channel,     ///< Channel to attach to.
      PBoolean autoDelete = true  ///< Flag for if channel should be automatically deleted.
    );


    /**Connect to remote server.
       This version expects that the indirect channel has already been opened
       using Open() beforehand.
      */
    PBoolean Connect();

    /**Connect to remote server.
      */
    PBoolean Connect(
      PChannel & channel  ///< Channel to attach to.
    );

    /**Connect to remote server.
      */
    PBoolean Connect(
      PChannel * channel,     ///< Channel to attach to.
      PBoolean autoDelete = true  ///< Flag for if channel should be automatically deleted.
    );

    /**Set the CA certificate(s) to send to client from server.
      */
    bool AddClientCA(
      const PSSLCertificate & certificate
    );
    bool AddClientCA(
      const PList<PSSLCertificate> & certificates
    );

    /**Use the certificate specified.
      */
    PBoolean UseCertificate(
      const PSSLCertificate & certificate
    );

    /**Use the private key file specified.
      */
    PBoolean UsePrivateKey(
      const PSSLPrivateKey & key
    );

    /**Get the available ciphers.
      */
    PString GetCipherList() const;

    typedef PSSLContext::VerifyMode VerifyMode;

    struct VerifyInfo
    {
      VerifyInfo(bool ok, const PSSLCertificate & cert, int err) : m_ok(ok), m_peerCertificate(cert), m_errorCode(err) { }
      bool m_ok;
      PSSLCertificate m_peerCertificate;
      int m_errorCode;
    };
    typedef PNotifierTemplate<VerifyInfo &> VerifyNotifier;
    #define PDECLARE_SSLVerifyNotifier(cls, fn) PDECLARE_NOTIFIER2(PSSLChannel, cls, fn, PSSLChannel::VerifyInfo &)
    #define PCREATE_SSLVerifyNotifier(fn) PCREATE_NOTIFIER2(fn, PSSLChannel::VerifyInfo &)

    /**Set certificate verification mode for connection.
      */
    void SetVerifyMode(
      VerifyMode mode,
      const VerifyNotifier & notifier = VerifyNotifier()
    );

    /** Call back for certificate verification.
        Default calls m_verifyNotifier if not NULL.
      */
    virtual void OnVerify(
      VerifyInfo & info
    );

    /**Get the peer certificate, if there is one.
       If SetVerifyMode() has been called with VerifyPeer then this will
       return true if the remote does not offer a certificate. If set to
       VerifyPeerMandatory, then it will return false. In both cases it will
       return false if the certificate is offered but cannot be authenticated.
      */
    bool GetPeerCertificate(
      PSSLCertificate & certificate,
      PString * error = NULL
    );

    /**Set the Server Name Indication TLS extension.
      */
    bool SetServerNameIndication(
      const PString & name   ///< For client, this is the server we are conneting to
    );

    /**Check the host name against the certificate.
       Note if SetVerifyMode() is set to VerifyNone, this always returns true.
      */
    bool CheckHostName(
      const PString & hostname,
      PSSLCertificate::CheckHostFlags flags = PSSLCertificate::CheckHostNormalRules
    );

    PSSLContext * GetContext() const { return m_context; }

    /**Get the internal SSL context structure.
      */
    operator ssl_st *() const { return m_ssl; }


  protected:
    void Construct(PSSLContext * ctx, PBoolean autoDel);
    virtual bool InternalAccept();
    virtual bool InternalConnect();

  protected:
    static int  BioRead(bio_st * bio, char * buf, int len);
    static int  BioWrite(bio_st * bio, const char * buf, int len);
    static long BioControl(bio_st * bio, int cmd, long num, void * ptr);
    static int  BioClose(bio_st * bio);

    virtual int  BioRead(char * buf, int len);
    virtual int  BioWrite(const char * buf, int len);
    virtual long BioControl(int cmd, long num, void * ptr);
    virtual int  BioClose();

    PSSLContext  * m_context;
    bool           m_autoDeleteContext;
    ssl_st       * m_ssl;
    bio_method_st* m_bioMethod;
    bio_st       * m_bio;
    VerifyNotifier m_verifyNotifier;
    PDECLARE_MUTEX(m_writeMutex);

    P_REMOVE_VIRTUAL(PBoolean,RawSSLRead(void *, PINDEX &),false);
    P_REMOVE_VIRTUAL(bool,OnVerify(bool,const PSSLCertificate&),false);
};


/**This class will start a secure SSL based channel.
*/
class PSSLChannelDTLS : public PSSLChannel
{
    PCLASSINFO(PSSLChannelDTLS, PSSLChannel)
  public:
    /**Create a new channel given the context.
       If no context is given a default one is created.
    */
    PSSLChannelDTLS(
      PSSLContext * context = NULL,   ///< Context for SSL channel
      bool autoDeleteContext = false  ///< Flag for context to be automatically deleted.
    );
    PSSLChannelDTLS(
      PSSLContext & context           ///< Context for SSL channel
    );

    /**Close and clear the SSL channel.
    */
    ~PSSLChannelDTLS();

    // Overrides from PChannel
    virtual PBoolean Read(void * buf, PINDEX len);
    virtual PBoolean Write(const void * buf, PINDEX len);

    /** Set the MTU for DTLS handshake.
        Note, should be done before calling ExecuteHandshake().
      */
    bool SetMTU(
      unsigned bytes
    );

    /** Perform negotiation handshake.
      */
    bool ExecuteHandshake();

    /// Indicate we are operating as a server
    bool IsServer() const;

    /// Get the selected profile name.
    PCaselessString GetSelectedProfile() const;

    /// Get the key material after exchange completed.
    PBYTEArray GetKeyMaterial(
      PINDEX materialSize,
      const char * name
    ) const;

  protected:
    virtual bool InternalAccept();
    virtual bool InternalConnect();
};


#else // P_SSL

class PSSLCertificateInfo
{
public:
  PSSLCertificateInfo(bool withDefaults = false) { }
  void SetSSLCredentials(const PSSLCertificateInfo &) { }
};

#endif // P_SSL

#endif // PTLIB_PSSL_H


// End Of File ///////////////////////////////////////////////////////////////
