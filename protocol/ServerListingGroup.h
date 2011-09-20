#ifndef __SERVER_LISTING_GROUP_H__
#define __SERVER_LISTING_GROUP_H__

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


#include <list>
#include <string>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include "ServerListing.h"

#include "../util/Network.h"

using namespace boost::asio;

typedef boost::function<void (const boost::system::error_code &error)> ServerListingGroupHandler;

class ServerListingIterator;

class ServerListingGroup {

  friend class ServerListingIterator;

 private:
  std::list<std::string> &groupList;
  boost::asio::io_service &io_service;

 public:
  std::string descriptorList;
  ServerListingGroup(boost::asio::io_service &io_service, std::list<std::string> &groupList);
  void retrieveGroupList(ServerListingGroupHandler handler);
};

class ServerListingIterator {
 private:
  int index;
  ServerListingGroup &group;

 public:
  
 ServerListingIterator(ServerListingGroup &group) : group(group), index(0) {}
  
  boost::shared_ptr<ServerListing> next() {
    int listingStart, listingEnd;

    listingStart = group.descriptorList.find("router ", index);

    if (listingStart == std::string::npos) {
      boost::shared_ptr<ServerListing> null;
      return null;
    }

    listingEnd  = group.descriptorList.find("-----END SIGNATURE-----", index);
    listingEnd += 23;

    std::string listing = group.descriptorList.substr(listingStart, listingEnd-listingStart);
    boost::shared_ptr<ServerListing> serverListing(new ServerListing(group.io_service, 
								     listing, true));

    index = listingEnd;

    return serverListing;
  }
};

#endif
