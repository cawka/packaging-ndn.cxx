/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef NDN_CERTIFICATE_EXTENSION_H
#define NDN_CERTIFICATE_EXTENSION_H

#include <vector>
#include "ndn.cxx/common.h"
#include "ndn.cxx/fields/blob.h"

#include "ndn.cxx/helpers/oid.h"
#include "ndn.cxx/helpers/der/der.h"

using namespace std;

namespace ndn
{

namespace security
{
  /**
   * @brief CertificateExtension class, Extension entry in certificate 
   */
  class CertificateExtension
  {
  public:
    /**
     * @brief constructor
     * @param oid the oid of subject description entry
     * @param critical if true, the entension must be handled
     * @param value the extension value
     */
    CertificateExtension (const string & oid, const bool & critical, const Blob & value);

    /**
     * @brief constructor
     * @param oid the oid of subject description entry
     * @param critical if true, the entension must be handled
     * @param value the extension value
     */
    CertificateExtension (const OID & oid, const bool & critical, const Blob & value);

    virtual
    ~CertificateExtension () {}

    /**
     * @brief encode the object into DER syntax tree
     * @return the encoded DER syntax tree
     */
    Ptr<der::DerNode> 
    toDER ();

    Ptr<Blob>
    toDERBlob ();

    inline const OID &
    getOID () const;

    inline const bool &
    getCritical () const;

    inline const Blob &
    getValue () const;
      
  protected:
    OID m_extnID;
    bool m_critical;
    Blob m_extnValue;
  };

  inline const OID &
  CertificateExtension::getOID () const
  { return m_extnID; }
  
  inline const bool &
  CertificateExtension::getCritical () const
  { return m_critical; }

  inline const Blob &
  CertificateExtension::getValue () const
  { return m_extnValue; }
  
}//security

}//ndn

#endif
