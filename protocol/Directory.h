#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

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

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <list>
#include <string>
#include <stdio.h>

#include <openssl/rsa.h>

#include "ServerListing.h"
#include "../util/Network.h"

/*
 * This class implements the Tor Directory functionality.
 *
 */


using namespace boost::asio;

typedef boost::function<void (const boost::system::error_code &error)> DirectoryHandler;
typedef boost::function<void (boost::shared_ptr<ServerListing> serverListing,
			      const boost::system::error_code &error)> RetrieveServerListingHandler;

class ServerNotFoundException : public std::exception {
public:
  virtual const char* what() const throw() {
    return "Specified server not found in directory...";
  }
};

class ExitNodeIterator;
class FastExitNodeIterator;

class Directory {

  friend class ExitNodeIterator;

 private:
  boost::asio::io_service &io_service;
  std::string directoryList;
  std::list<boost::shared_ptr<ServerListing> > serverListings;  

 public:
  Directory(boost::asio::io_service &io_service);
  void retrieveDirectoryListing(DirectoryHandler handler);
  void retrieveDirectoryListingComplete(DirectoryHandler handler, 
					const boost::system::error_code &err);

  void getRandomServerListing(RetrieveServerListingHandler handler); 
  void getServerListingFor(std::string &server, RetrieveServerListingHandler handler);
  void getServerListingComplete(boost::shared_ptr<ServerListing> serverListing,
  				RetrieveServerListingHandler handler,
  				const boost::system::error_code &err);


};

class ExitNodeIterator {
 private:
  int index;
  Directory &directory;

 public:
  
 ExitNodeIterator(Directory &directory) : directory(directory), index(0) {}

  virtual const char* getExitNodeToken() {
    return "s Exit ";
  }
  
  boost::shared_ptr<ServerListing> next() {
    int listingStart, listingEnd, flagsEnd, runningFlag;
    
    for (;;) {
      index = directory.directoryList.find(getExitNodeToken(), index);
      
      if (index == std::string::npos) {
	boost::shared_ptr<ServerListing> null;
	return null;
      }
      
      flagsEnd    = directory.directoryList.find("\n", index);
      runningFlag = directory.directoryList.find("Running", index);
      
      if (runningFlag < flagsEnd) break;
      else                        index++;      
    }

    listingEnd   = directory.directoryList.rfind("\n", index);
    listingStart = directory.directoryList.rfind("\n", listingEnd-1);
    listingStart++;

    std::string listing = directory.directoryList.substr(listingStart, listingEnd-listingStart);
    boost::shared_ptr<ServerListing> serverListing(new ServerListing(directory.io_service, listing));        

    index++;

    return serverListing;
  }

};

class FastExitNodeIterator : public ExitNodeIterator {
 public:
 FastExitNodeIterator(Directory &directory) : ExitNodeIterator(directory) {}

  virtual const char* getExitNodeToken() {
    return "s Exit Fast ";
  }
};


#endif
