/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Xingyu Ma <maxy12@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "simplekey-store.h"

#include "ndn.cxx/security/certificate/publickey.h"
#include "ndn.cxx/security/exception.h"

#include <string>

#include <boost/algorithm/string.hpp>

#include <cryptopp/rsa.h>
#include <cryptopp/files.h>
#include <cryptopp/base64.h>
#include <cryptopp/hex.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>
#include <cryptopp/pssr.h>
#include <cryptopp/modes.h>

#include <sys/types.h>
#include <sys/stat.h>

using namespace CryptoPP;
using namespace ndn::security;
using namespace std;

namespace ndn
{

namespace security
{
SimpleKeyStore::SimpleKeyStore(const string & dir)
{
  if(dir.empty())
    {
      m_keystorePath = boost::filesystem::path(getenv("HOME")) / ".ndnx" / "ndnsec-keys";
    }
  else
    {
      m_keystorePath = dir;
    }
  boost::filesystem::create_directories (m_keystorePath);
};

/**
 * @brief generate a pair of asymmetric keys
 * @param keyName the name of the key pair
 * @param keyType the type of the key pair, e.g. RSA
 * @param keySize the size of the key pair
 * @returns true if keys have been successfully generated
 */

void
SimpleKeyStore::generateKeyPair(const Name & keyName, KeyType keyType, int keySize)
{
  string keyURI = keyName.toUri();
  if (SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_PUBLIC))
    {
      throw security::SecException("public key exists");
    }
  if ( SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_PRIVATE))
    {
      throw security::SecException("private key exists");
    }

  if (keyType == KEY_TYPE_RSA) {
    AutoSeededRandomPool rng;
    InvertibleRSAFunction privkey;
    privkey.Initialize(rng, keySize);
    string privateKeyName = SimpleKeyStore::nameTransform(keyURI, ".pri");
    SimpleKeyStore::maintainMapping(keyURI,SimpleKeyStore::nameTransform(keyURI, ""));
    Base64Encoder privkeysink(new FileSink(privateKeyName.c_str()));
    privkey.DEREncode(privkeysink);
    privkeysink.MessageEnd();

    RSAFunction pubkey(privkey);
    string publicKeyName = SimpleKeyStore::nameTransform(keyURI, ".pub");
    Base64Encoder pubkeysink(new FileSink( publicKeyName.c_str()));
    pubkey.DEREncode(pubkeysink);
    pubkeysink.MessageEnd();

    /*set file permission*/
    chmod(privateKeyName.c_str(), 0000400);
    chmod(publicKeyName.c_str(), 0000444);
  }
  return;
}

Ptr<Publickey>
SimpleKeyStore::getPublickey(const Name & keyName)
{
  string keyURI = keyName.toUri();
  if  (!SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_PUBLIC))
    {
      throw security::SecException("public key doesn't exists");
      return 0;
    }
  string publicKeyName = SimpleKeyStore::nameTransform(keyURI, ".pub");
  ifstream file (publicKeyName.c_str(), ios::in|ios::binary|ios::ate);
  if (file.is_open())
    {
      ifstream::pos_type size = file.tellg();
      char * memblock = new char [size];
      file.seekg (0, ios::beg);
      file.read (memblock, size);
      file.close();
      string encoded = string(memblock, size);
      string decoded;
      CryptoPP::StringSource ss2(reinterpret_cast<const unsigned char *>(encoded.c_str()), encoded.size(), true,
                                 new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));
      Blob b(decoded.c_str(), decoded.size());
      return Publickey::fromDER(b);
    }
  return NULL;
}

/**
 * @brief sign data
 * @param keyName the name of the signing key
 * @param digestAlgo the digest algorithm
 * @param pData the pointer to data
 * @returns signature, NULL if signing fails
 */
Ptr<Blob>
SimpleKeyStore::sign(const Blob & pData, const Name & keyName, DigestAlgorithm digestAlgo)
{
  string keyURI = keyName.toUri();
  if  (!SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_PRIVATE))
    {
      throw SecException("private key doesn't exists");
      return NULL;
    }
  try
    {
      AutoSeededRandomPool rng;
      string strContents = string(pData.buf(),pData.size());
      //Read private key
      CryptoPP::ByteQueue bytes;
      string privateKeyName = SimpleKeyStore::nameTransform(keyURI, ".pri");
      FileSource file(privateKeyName.c_str(), true, new Base64Decoder);
      file.TransferTo(bytes);
      bytes.MessageEnd();
      RSA::PrivateKey privateKey;
      privateKey.Load(bytes);
      //Sign message
      if (digestAlgo == DIGEST_SHA256)
        {
          RSASS<PKCS1v15, SHA256>::Signer signer(privateKey);
          size_t length = signer.MaxSignatureLength();
          SecByteBlock signature(length);
          signer.SignMessage(rng, (const byte*) strContents.c_str(),
                             strContents.length(), signature);
          Ptr<Blob> ret = Ptr<Blob>(new Blob(signature, signature.size()));
          return ret;
        }
    }
  catch(const CryptoPP::Exception& e)
    {
      return NULL;
    }
  return NULL;
}

/**
 * @brief decrypt data
 * @param keyName the name of the decrypting key
 * @param pData the pointer to encrypted data
 * @returns decrypted data
 */
Ptr<Blob>
SimpleKeyStore::decrypt(const Name & keyName, const Blob & pData, bool sym)
{
  string keyURI = keyName.toUri();
  if (!sym)
    {
      if(!SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_PRIVATE))
        {
          throw SecException("private key doesn't exist");
          return NULL;
        }
      try
        {
          AutoSeededRandomPool rng;
          CryptoPP::ByteQueue bytes;
          string privateKeyName = SimpleKeyStore::nameTransform(keyURI, ".pri");
          FileSource file(privateKeyName.c_str(), true, new Base64Decoder);
          file.TransferTo(bytes);
          bytes.MessageEnd();
          RSA::PrivateKey privateKey;
          privateKey.Load(bytes);
          string recovered;
          RSAES_PKCS1v15_Decryptor d( privateKey );
          StringSource( string(pData.buf(), pData.size()), true,
                        new PK_DecryptorFilter( rng, d,
                                                new StringSink( recovered )
                                                )
                        );
          Ptr<Blob> ret = Ptr<Blob>(new Blob(recovered.c_str (), recovered.size()));
          return ret;
        }
      catch(const CryptoPP::Exception& e)
        {
          return NULL;
        }
    }
  else
    {
      if  (!SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_SYMMETRIC))
        {
          throw SecException("symmetric key doesn't exist");
          return NULL;
        }
      string symKeyName = SimpleKeyStore::nameTransform(keyURI, ".key");
      string cipher, decoded, recovered;
      Ptr<Blob> key_content = SimpleKeyStore::readSymetricKey(symKeyName);
      string key = string(key_content->buf(),key_content->size());
      CryptoPP::StringSource ss2(reinterpret_cast<const unsigned char *>(key.c_str()), key.size(), true,
                                 new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded)));

      using CryptoPP::AES;
      AutoSeededRandomPool rnd;

      byte iv[AES::BLOCKSIZE];
      rnd.GenerateBlock(iv, AES::BLOCKSIZE);
      try
        {
          CFB_Mode< AES >::Decryption d;
          d.SetKeyWithIV(reinterpret_cast<const unsigned char *>(decoded.c_str()),sizeof(decoded.c_str()), iv);
          StringSource s(cipher, true, new StreamTransformationFilter(d,new StringSink(recovered)));
          Ptr<Blob> ret = Ptr<Blob>(new Blob(recovered.c_str (), recovered.size()));
          return ret;
        }
      catch(const CryptoPP::Exception& e)
        {
          return NULL;
        }
    }
  return NULL;
}

Ptr<Blob>
SimpleKeyStore::encrypt(const Name & keyName, const Blob & pData, bool sym)
{
  string plain = string(pData.buf(),pData.size());
  string keyURI = keyName.toUri();
  if (!sym)
    {
      if(!SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_PUBLIC))
        {
          throw security::SecException("public key doesn't exist");
          return NULL;
        }
      try
        {
          AutoSeededRandomPool rng;
          CryptoPP::ByteQueue bytes;
          string publicKeyName = SimpleKeyStore::nameTransform(keyURI, ".pub");
          FileSource file(publicKeyName.c_str(), true, new Base64Decoder);
          file.TransferTo(bytes);
          bytes.MessageEnd();
          RSA::PublicKey publicKey;
          publicKey.Load(bytes);

          string cipher;
          RSAES_PKCS1v15_Encryptor e( publicKey );

          StringSource( plain, true, new PK_EncryptorFilter( rng, e,new StringSink( cipher )));
          Ptr<Blob> ret = Ptr<Blob>(new Blob(cipher.c_str (), cipher.size()));
          return ret;
        }
      catch(const CryptoPP::Exception& e)
        {
          return NULL;
        }
    }
  else
    {
      if  (!SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_SYMMETRIC))
        {
          throw SecException("symmetric key doesn't exist");
          return NULL;
        }
      string symKeyName = SimpleKeyStore::nameTransform(keyURI, ".key");
      string cipher, decoded;
      Ptr<Blob> key_content = SimpleKeyStore::readSymetricKey(symKeyName);
      string key = string(key_content->buf(),key_content->size());
      CryptoPP::StringSource ss2(reinterpret_cast<const unsigned char *>(key.c_str()), key.size(), true,
                                 new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded)));

      using CryptoPP::AES;
      AutoSeededRandomPool rnd;
      byte iv[AES::BLOCKSIZE];
      rnd.GenerateBlock(iv, AES::BLOCKSIZE);
      try
        {
          CFB_Mode< AES >::Encryption e;
          e.SetKeyWithIV(reinterpret_cast<const unsigned char *>(decoded.c_str()),sizeof(decoded.c_str()), iv);
          StringSource(plain, true,
                       new StreamTransformationFilter(e,
                                                      new StringSink(cipher)
                                                      ) // StreamTransformationFilter
                       ); // StringSource
          Ptr<Blob> ret = Ptr<Blob>(new Blob(cipher.c_str (), cipher.size()));
          return ret;
        }
      catch(const CryptoPP::Exception& e)
        {
          return NULL;
        }
    }

  return NULL;
}


/**
 * @brief generate a symmetric keys
 * @param keyName the name of the key
 * @param keyType the type of the key, e.g. AES
 * @param keySize the size of the key
 * @returns true if key have been successfully generated
 */
void
SimpleKeyStore::generateKey(const Name & keyName, KeyType keyType, int keySize)
{
  string keyURI = keyName.toUri();
  if ( SimpleKeyStore::doesKeyExist(keyName, KEY_CLASS_SYMMETRIC))
    {
      throw security::SecException("symmetric key exists");
      return;
    }

  if (keyType == KEY_TYPE_AES)
    {
      AutoSeededRandomPool rnd;
      SecByteBlock key(0x00, keySize);
      rnd.GenerateBlock( key, keySize );
      string encoded;
      encoded.clear();
      StringSource(key, key.size(), true,
                   new HexEncoder(
                                  new StringSink(encoded)
                                  ) // HexEncoder
                   ); // StringSource
      SimpleKeyStore::maintainMapping(keyURI,SimpleKeyStore::nameTransform(keyURI, ""));
      string symKeyName = SimpleKeyStore::nameTransform(keyURI, ".key");
      Blob blob(encoded.c_str(), encoded.size());
      SimpleKeyStore::writeSymetricKey(symKeyName, blob);

      chmod(symKeyName.c_str(), 0000400);
    }
  return;
}

bool
SimpleKeyStore::doesKeyExist(const Name & keyName, KeyClass keyClass)
{
  string keyURI = keyName.toUri();
  if (keyClass == KEY_CLASS_PUBLIC)
    {
      string publicKeyName = SimpleKeyStore::nameTransform(keyURI, ".pub");
      fstream fin(publicKeyName.c_str(),ios::in);
      if (fin)
        return true;
      else
        return false;
    }
  if (keyClass == KEY_CLASS_PRIVATE)
    {
      string privateKeyName = SimpleKeyStore::nameTransform(keyURI, ".pri");
      fstream fin(privateKeyName.c_str(),ios::in);
      if (fin)
        return true;
      else
        return false;
    }
  if (keyClass == KEY_CLASS_SYMMETRIC)
    {
      string symmetricKeyName = SimpleKeyStore::nameTransform(keyURI, ".key");
      fstream fin(symmetricKeyName.c_str(),ios::in);
      if (fin)
        return true;
      else
        return false;
    }
  return false;
}

std::string SimpleKeyStore::nameTransform(const string &keyName, const string &extension)
{
  std::string digest;
  CryptoPP::SHA256 hash;
  CryptoPP::StringSource foo(keyName, true,
                             new CryptoPP::HashFilter(hash,
                                                      new CryptoPP::Base64Encoder (new CryptoPP::StringSink(digest))
                                                      )
                             );
  boost::algorithm::trim(digest);
  for (std::string::iterator ch = digest.begin(); ch != digest.end(); ch++)
    {
      if (*ch == '/')
        {
          *ch = '%';
        }
    }

  return (m_keystorePath / (digest + extension)).string();
}

Ptr<Blob>
SimpleKeyStore::readSymetricKey(const string &filename)
{
  ifstream file (filename.c_str(), ios::in|ios::binary|ios::ate);
  if (file.is_open())
    {
      ifstream::pos_type size = file.tellg();
      char * memblock = new char [size];
      file.seekg (0, ios::beg);
      file.read (memblock, size);
      file.close();
      Ptr<Blob> ret = Ptr<Blob>(new Blob(memblock, size));
      delete []memblock;
      return ret;
    }
  else
    return NULL;
}

void SimpleKeyStore::maintainMapping(string str1, string str2)
{
  std::ofstream outfile;
  string dirFile = (m_keystorePath / "mapping.txt").string();

  outfile.open(dirFile.c_str(), std::ios_base::app);
  outfile << str1+" "+str2;
  outfile.close();
}

void
SimpleKeyStore::writeSymetricKey(const string &filename, const Blob & pData)
{
  ofstream file (filename.c_str());
  string key_content = string(pData.buf(),pData.size());
  if (file.is_open())
    {
      file<<(key_content.c_str());
      file.close();
    }
  return;
}
} //ndn
}
