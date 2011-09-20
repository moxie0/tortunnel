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

#include "TorScanner.h"

#include <cstdlib>

using namespace boost::asio;

TorScanner::TorScanner(boost::asio::io_service &io_service, 
		       std::string &destinationHost, 
		       std::string &destinationPort,
		       std::string &request) 
  : io_service(io_service), destinationHost(destinationHost), 
    destinationPort(atoi(destinationPort.c_str())), request(request)
{}

void TorScanner::readComplete(TorTunnel *tunnel,
			      boost::shared_ptr<TorTunnelStream> stream,
			      unsigned char *buf, std::size_t transferred)
{
  if (transferred == -1) {
    tunnel->close();
    delete tunnel;
    return;
  }

  std::string response((const char*)buf, transferred);
  std::cout << "Read from (" << stream->getRemoteNodeAddress() << "):" 
	    << std::endl << response << std::endl;

  stream->read(boost::bind(&TorScanner::readComplete, this, tunnel, stream, _1, _2));
}

void TorScanner::requestSent(TorTunnel *tunnel,
			     boost::shared_ptr<TorTunnelStream> stream,
			     unsigned char *buf,
			     const boost::system::error_code &err)
{
  free(buf);

  if (err) {
    delete tunnel;
    std::cout << "Error sending request: " << err << std::endl;
    return;
  }

  stream->read(boost::bind(&TorScanner::readComplete, this, tunnel, stream, _1, _2));
}

void TorScanner::tunnelStreamOpen(TorTunnel *tunnel, 
				  boost::shared_ptr<TorTunnelStream> stream,
				  const boost::system::error_code &err)
{
  if (err) {
    std::cout << "Error opening stream: " << err << " " << &(tunnel->nodeConnection) << std::endl;
    tunnel->close();
    delete tunnel;
    return;
  }

  std::cerr << "Connected to: " << stream->getRemoteNodeAddress() << std::endl;
  
  std::string fullRequest("GET ");
  fullRequest.append(request);
  fullRequest.append(" HTTP/1.0\r\nConnection:close\r\n\r\n");

  unsigned char *buf = (unsigned char*)malloc(fullRequest.size());
  memcpy(buf, fullRequest.c_str(), fullRequest.size());

  stream->write(buf, fullRequest.size(), boost::bind(&TorScanner::requestSent, this,
						     tunnel, stream, buf, placeholders::error));
}

void TorScanner::tunnelConnectionComplete(TorTunnel *tunnel, 
					  const boost::system::error_code &err)
{
  if (err) {
    delete tunnel;
    std::cout << "Error connecting: " << err << " " << &(tunnel->nodeConnection) << std::endl;
    return;
  }

  tunnel->openStream(destinationHost, destinationPort, boost::bind(&TorScanner::tunnelStreamOpen,
								   this, tunnel, _1, _2));
}

void TorScanner::torTunnelError(const boost::system::error_code &err) {
  std::cerr << "Error with tor tunnel: " << err << std::endl;
}

void TorScanner::serverDescriptorsComplete(boost::shared_ptr<ServerListingGroup> group,
					   const boost::system::error_code &err)
{
  if (err) {
    std::cerr << "Error retrieving Exit Node descriptors: " << err << std::endl;
    return;
  }

  ServerListingIterator iterator(*group);
  boost::shared_ptr<ServerListing> exitNode;

  while ((exitNode = iterator.next()) != NULL) {
    TorTunnel *tunnel = new TorTunnel(io_service, exitNode, 
				      boost::bind(&TorScanner::torTunnelError, this, 
						  placeholders::error));
    tunnel->connect(boost::bind(&TorScanner::tunnelConnectionComplete, this,
				tunnel, placeholders::error));
  }
}
			       

void TorScanner::directoryListingComplete(Directory *directory, 
					  const boost::system::error_code &err) 
{  
  if (err) {
    std::cerr << "Error retrieving directory listing: " << err << std::endl;
    return;
  }

  ExitNodeIterator iterator(*directory);
  boost::shared_ptr<ServerListing> exitNode = iterator.next();

  while (exitNode != NULL) {
    std::list<std::string> identityList;
    int count = 0;

    while (exitNode != NULL) {
      char *identity = exitNode->getBase16EncodedIdentity();
      std::string identityStr(identity);

      identityList.push_back(identityStr);
      free(identity);
      
      std::cerr << "Added: " << identityStr << std::endl;
      exitNode = iterator.next();

      if (count++ == 50)
	break;
    }

    boost::shared_ptr<ServerListingGroup> group(new ServerListingGroup(io_service, 
								       identityList));
    group->retrieveGroupList(boost::bind(&TorScanner::serverDescriptorsComplete, this,
					 group, placeholders::error));    
  }

  delete directory;
}

void TorScanner::scan() {
  Directory *directory = new Directory(io_service);
  directory->retrieveDirectoryListing(boost::bind(&TorScanner::directoryListingComplete,
						  this, directory, placeholders::error));
}

int main(int argc, char** argv) {
  std::string destinationHost(argv[1]);
  std::string destinationPort(argv[2]);
  std::string request(argv[3]);

  boost::asio::io_service io_service;

  TorScanner scanner(io_service, destinationHost, destinationPort, request);
  scanner.scan();


  io_service::work work(io_service);
  io_service.run();
}
