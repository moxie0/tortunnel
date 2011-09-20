/*-
 * Copyright (c) 2009, Moxie Marlinspike
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of this program nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "ServerListing.h"

#include <openssl/pem.h>
#include <openssl/rsa.h>

#define ONION_KEY_TAG "onion-key"
#define PGP_BEGIN_TAG "-----BEGIN RSA PUBLIC KEY-----"
#define PGP_END_TAG "-----END RSA PUBLIC KEY-----"

ServerListing::ServerListing(boost::asio::io_service &io_service, 
			     std::string &serverListingString) 
  : io_service(io_service)
{
  std::string delimiters(" ");
  Util::tokenizeString(serverListingString, delimiters, serverListingTokens);

  address = serverListingTokens[6];
  port    = serverListingTokens[7];
}

ServerListing::ServerListing(boost::asio::io_service &io_service,
			     std::string &descriptorList,
			     bool isFullDescriptor)
  : io_service(io_service), descriptorList(descriptorList)
{
  std::cerr << "Full descriptor: " << std::endl << descriptorList << std::endl;
  
  int endOfFirstLine    = descriptorList.find("\n");
  std::string firstLine = descriptorList.substr(0, endOfFirstLine);

  std::vector<std::string> firstLineTokens;
  std::string delimiters(" ");
  Util::tokenizeString(firstLine, delimiters, firstLineTokens);

  address = firstLineTokens[2];
  port    = firstLineTokens[3];
}


std::string& ServerListing::getAddress() {
  return address;
}

std::string& ServerListing::getPort() {
  return port;
}

RSA* ServerListing::getOnionKey() {
  int onionKeyStringIndex = descriptorList.find(ONION_KEY_TAG);

  if (onionKeyStringIndex == std::string::npos)
    throw OnionKeyException();

  int onionKeyStart = onionKeyStringIndex + strlen(ONION_KEY_TAG);
  int onionKeyEnd   = descriptorList.find(PGP_END_TAG, onionKeyStart);

  if (onionKeyEnd == std::string::npos)
    throw OnionKeyException();

  int onionKeyLength       = (onionKeyEnd - onionKeyStart) + strlen(PGP_END_TAG) + 1;
  char * descriptorListStr = (char*)descriptorList.c_str() + onionKeyStart;

  RSA *onionKey     = RSA_new();
  BIO *publicKeyBio = BIO_new_mem_buf(descriptorListStr, onionKeyLength);

  if (publicKeyBio == NULL) throw OnionKeyException();

  onionKey = PEM_read_bio_RSAPublicKey(publicKeyBio, &onionKey, NULL, NULL);

  if (onionKey == NULL) throw OnionKeyException();

  return onionKey;
}

char* ServerListing::getBase16EncodedIdentity() {
  std::string &identityKey = serverListingTokens[2];  

  int destLen              = identityKey.length() * 2 + 1;
  char *dest               = (char*)malloc(destLen);

  destLen = Util::base64_decode(dest, destLen, identityKey.c_str(), identityKey.length());
  
  int encodedLength        = destLen * 2 + 1;
  char *encoded            = (char*)malloc(encodedLength);

  Util::base16_encode(encoded, encodedLength, dest, destLen);

  free(dest);

  return encoded;
}

void ServerListing::getDescriptorList(ServerListingHandler handler) 
{
  char* identity = getBase16EncodedIdentity();
  std::string identityString(identity);

  boost::shared_ptr<std::string>
    request(new std::string("GET /tor/server/fp/"));
  request->append(identityString);
  request->append(" HTTP/1.0\r\nConnection: close\r\n\r\n");

  free(identity);

  std::string ip("128.31.0.34");
  Network::suckUrlToString(io_service, ip, 9031, request, &descriptorList, handler);
}
