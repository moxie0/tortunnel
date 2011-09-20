#ifndef __TOR_SCANNER_H__
#define __TOR_SCANNER_H__

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

#include <string>
#include <list>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "protocol/ServerListing.h"
#include "protocol/Directory.h"
#include "protocol/ServerListingGroup.h"
#include "TorTunnel.h"

using namespace boost::asio;

/****
 * This class implements simple sslstrip scanning.  You give it the URL
 * for a page with an https link on it, and it will connect directly to
 * every running exit node and request that page.  Results are sent to
 * stdout, which can be redirected to a file for processing.
 *
 */

class TorScanner {

 private:
  boost::asio::io_service &io_service;
  std::string &destinationHost;
  uint16_t destinationPort;
  std::string &request;

  void torTunnelError(const boost::system::error_code &err);

  void readComplete(TorTunnel *tunnel, boost::shared_ptr<TorTunnelStream> stream,
		    unsigned char *buf, std::size_t transferred);

  void requestSent(TorTunnel *tunnel, boost::shared_ptr<TorTunnelStream> stream,
		   unsigned char *buf, const boost::system::error_code &err);

  void tunnelStreamOpen(TorTunnel *tunnel, boost::shared_ptr<TorTunnelStream> stream,
			const boost::system::error_code &err);

  void tunnelConnectionComplete(TorTunnel *tunnel, const boost::system::error_code &err);

  void serverDescriptorsComplete(boost::shared_ptr<ServerListingGroup> group,
				 const boost::system::error_code &err);
			       
  void directoryListingComplete(Directory *directory, const boost::system::error_code &err);


 public:
  TorScanner(boost::asio::io_service &io_service, 
	     std::string &destinationHost, 
	     std::string &destinationPort,
	     std::string &request);

  void scan();

};



#endif
