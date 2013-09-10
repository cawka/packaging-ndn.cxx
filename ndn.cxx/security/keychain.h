/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef NDN_KEYCHAIN_H
#define NDN_KEYCHAIN_H

#include <string>
#include <map>

#include "ndn.cxx/common.h"
#include "ndn.cxx/data.h"
#include "ndn.cxx/interest.h"
#include "ndn.cxx/fields/name.h"
#include "ndn.cxx/fields/blob.h"
#include "ndn.cxx/fields/signature.h"
#include "ndn.cxx/regex/regex.h"

#include "identity/identity-manager.h"
#include "policy/policy-manager.h"
#include "encryption/encryption-manager.h"
#include "policy/policy-rule.h"
#include "cache/certificate-cache.h"
// #include "certificate/certificate.h"

#include "ndn.cxx/wrapper/closure.h"



using namespace std;

namespace ndn
{

  class Wrapper;

namespace security
{

  typedef boost::function<void (Ptr<Data>)> VerifiedCallback;
  typedef boost::function<void ()> VerifyFailCallback;
  typedef boost::function<void (Ptr<Data>)> RecursiveVerifiedCallback;

  /**
   * @brief Keychain class, the main class of security library
   *
   * Keychain provide a set of interfaces to the security libray,
   * such as identity management, policy configuration, security
   * transform (packet signing and verification, encryption and 
   * decryption), and etc.
   */
  class Keychain{
  public:
    /**
     * @brief Constructor
     * @param privateStorage the storage for private keys and some secret keys
     * @param policyPath the path to the policy file
     * @param encryptionPath the path to the encryption database
     */
    Keychain(Ptr<PrivatekeyStorage> privateStorage, const string & policyPath, const string & encryptionPath);

    /**
     * @brief Destructor
     */
    virtual
    ~Keychain(){};

    /*****************************************
     *          Identity Management          *
     *****************************************/

    /**
     * @brief Create identity, by default it will create a pair of Key-Signing-Key (KSK) for this identity and a self-signed certificate of the KSK
     * @param identity the name of the identity
     * @return the key name of the auto-generated KSK of the identity 
     */
    virtual Name 
    createIdentity(const Name & identity);

    /**
     * @brief get the default identity name
     * @return the name of the default identity
     */
    virtual Name
    getDefaultIdentity ();

    /**
     * @brief Generate a pair of RSA keys for the specified identity
     * @param identity the name of the identity
     * @param ksk create a KSK or not, true for KSK, false for DSK 
     * @param keySize the size of the key
     * @return the key name 
     */
    virtual Name
    generateRSAKeyPair (const Name & identity, bool ksk = false, int keySize = 2048);

    /**
     * @brief Set a key as the default key of an identity
     * @param keyName the name of the key
!!!  * @param identity the name of the identity, if not specified the identity name can be inferred from the keyName
     */
    virtual void
    setDefaultKeyForIdentity (const Name & keyName, const Name & identity = Name());

    virtual Name
    generateRSAKeyPairAsDefault (const Name & identity, bool ksk = false, int keySize = 2048);

    /**
     * @brief Create a public key signing request
     * @param identity the name of the identity
     * @param keyID the identifier of the public key
     * @param keyFormat the format of the request
     * @param pem True if output is encoded as PEM, False if output is encoded as DER
     * @returns signing request blob
     */
    virtual Ptr<Blob> 
    createSigningRequest(const Name & keyName);

    /**
     * @brief Install a certificate into identity
     * @param certificate the certificate in terms of Data packet
     */
    virtual void 
    installCertificate(Ptr<Certificate> certificatePtr);

    virtual void
    setDefaultCertificateForKey(const Name & certName);


    /**
     * @brief Get certificate for publishing
     * @param certName name of the cert
     * @param certSigner signer of the cert
     * @param certType type of the cert
     * @returns certificate Data 
     */
    virtual Ptr<Certificate> 
    getCertificate(const Name & certName);

    virtual Ptr<Certificate>
    getAnyCertificate(const Name & certName);

    virtual Ptr<Blob> 
    revokeKey(const Name & keyName);

    virtual Ptr<Blob> 
    revokeCertificate(const Name & certName);

    /*****************************************
     *           Policy Management           *
     *****************************************/

    virtual void 
    setSigningPolicyRule(Ptr<PolicyRule> policy);

    virtual void
    setVerificationExemption(Ptr<Regex> exempt);

    virtual void 
    setVerificationPolicyRule(Ptr<PolicyRule> policy);

    virtual void 
    setSigningInference(Ptr<Regex> inference);

    virtual void 
    setTrustAnchor(const Certificate & certificate);

    /*****************************************
     *              Sign/Verify              *
     *****************************************/

    virtual void 
    sign(Data & data, const Name & signerName = Name(), bool byID = true);
    
    virtual Ptr<Signature> 
    sign(const Blob & buf, const Name & signerName, bool byID = true);

    virtual void 
    verifyData(Ptr<Data> dataPtr, const VerifiedCallback & verifiedCallback, const VerifyFailCallback & failureCallback);

    /*****************************************
     *           Encrypt/Decrypt             *
     *****************************************/

    virtual void 
    generateSymmetricKey(const Name & keyName, KeyType keyType);

    virtual Ptr<Blob> 
    encrypt(const Name & keyName, const Blob & blob, bool sym = true, EncryptMode em = EM_DEFAULT);

    virtual Ptr<Blob> 
    decrypt(const Name & keyName, const Blob & blob, bool sym = true, EncryptMode em = EM_DEFAULT);
    

    //TMP:
    Ptr<Data>
    fakeFecthData(const Name & name);

    // void
    // setFakeWrapper(FakeWrapper * wrapper)
    // {
    //   m_handler = wrapper;
    // }

    void
    setWrapper(Wrapper * wrapper)
    {
      m_handler = wrapper;
    }

  private:    
    Ptr<Data> 
    fetchData(const Name & name);

    virtual void 
    stepVerify(Ptr<Data> dataPtr, 
               const bool isFirst,
               const int stepCount, 
               const RecursiveVerifiedCallback & recursiveVerifiedCallback, 
               const VerifyFailCallback & failureCallback);

    virtual void
    onCertInterestTimeout(Ptr<Closure> closurePtr, Ptr<Interest> interestPtr, int retry, const VerifyFailCallback & failureCallback);

    virtual void
    onCertVerified(Ptr<Data>cert, 
                   Ptr<Data>data, 
                   const RecursiveVerifiedCallback &preRecurVerifyCallback, 
                   const VerifyFailCallback &failureCallback);

    // virtual void
    // onOriginalCertVerified(Ptr<Certificate> cert, 
    //                        Ptr<Data>data, 
    //                        const VerifiedCallback &verifiedCallback, 
    //                        const VerifyFailCallback &failureCallback);

  private:
    Ptr<IdentityManager> m_identityManager;
    Ptr<PolicyManager> m_policyManager;
    Ptr<EncryptionManager> m_encryptionManager;
    Ptr<CertificateCache> m_certificateCache;
    // map<Name, Certificate> m_certCache;
    const int m_maxStep;
    Wrapper* m_handler;
  };
  

}//security

}//ndn

#endif