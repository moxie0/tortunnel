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

#include <stdio.h>
#include <stdlib.h>
#include "Directory.h"

#include <openssl/rsa.h>
#include <iostream>

using namespace boost::asio;

Directory::Directory(boost::asio::io_service &io_service) : io_service(io_service) {}

void Directory::getServerListingComplete(boost::shared_ptr<ServerListing> serverListing,
					 RetrieveServerListingHandler handler,
					 const boost::system::error_code &err)
{
  handler(serverListing, err);
}

void Directory::getRandomServerListing(RetrieveServerListingHandler handler)
{
  int count = 0;
  int index = Util::getRandom() % serverListings.size();

  std::cerr << "Choosing exit node at index: " << index << " out of " << serverListings.size() << " listings..." << std::endl;

  std::list<boost::shared_ptr<ServerListing> >::iterator serverListingIterator = serverListings.begin();

  while (serverListingIterator != serverListings.end()) {
    if (count++ == index) {
      (*serverListingIterator)->getDescriptorList(boost::bind(&Directory::getServerListingComplete,
							      this, *serverListingIterator, handler, 
							      placeholders::error)); 
      return;
    }

    serverListingIterator++;
  }

  throw ServerNotFoundException();
}

void Directory::getServerListingFor(std::string &server, RetrieveServerListingHandler handler) 
{
  int position = directoryList.find(server);

  if (position == std::string::npos)
    throw ServerNotFoundException();

  position     = directoryList.rfind("\n", position);

  if (position == std::string::npos)
    throw ServerNotFoundException();

  position++;

  int end = directoryList.find("\n", position);

  if (end == std::string::npos)
    throw ServerNotFoundException();

  std::string serverString = directoryList.substr(position, end-position);
  boost::shared_ptr<ServerListing> serverListing(new ServerListing(io_service, serverString));
  serverListing->getDescriptorList(boost::bind(&Directory::getServerListingComplete,
					       this, serverListing, handler, 
					       placeholders::error));
}

void Directory::retrieveDirectoryListingComplete(DirectoryHandler handler, 
						 const boost::system::error_code &err)
{
  if (err) {
    handler(err);
    return;
  }

  FastExitNodeIterator iterator(*this);
  boost::shared_ptr<ServerListing> exitNode = iterator.next();

  while (exitNode != NULL) {
    serverListings.push_back(exitNode);
    // std::cerr << "Added: " << exitNode->getAddress() << std::endl;
    exitNode = iterator.next();
  }

  handler(err);
}

void Directory::retrieveDirectoryListing(DirectoryHandler handler) {
  boost::shared_ptr<std::string> 
    request(new std::string("GET /tor/status/all HTTP/1.0\r\nConnection: close\r\n\r\n"));

  std::string ip("128.31.0.34");

  Network::suckUrlToString(io_service, ip, 9031, request, &directoryList,
			   boost::bind(&Directory::retrieveDirectoryListingComplete,
				       this, handler, placeholders::error));
}
 


