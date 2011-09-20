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

#include "TorProxy.h"

using namespace boost::asio;

TorProxy::TorProxy(TorTunnel *tunnel, io_service &io_service, int listenPort)
  : acceptor(io_service, ip::tcp::endpoint(ip::tcp::v4(), listenPort)),
    tunnel(tunnel)
{
  acceptIncomingConnection();
}

void TorProxy::acceptIncomingConnection() {
  boost::shared_ptr<ip::tcp::socket> socket(new ip::tcp::socket(acceptor.get_io_service()));
  acceptor.async_accept(*socket, boost::bind(&TorProxy::handleIncomingConnection,
					     this, socket, placeholders::error));
}

void TorProxy::handleIncomingConnection(boost::shared_ptr<ip::tcp::socket> socket,
					const boost::system::error_code &err) 
{
  if (err) {
    std::cerr << "Error accepting incoming connection: " << err << std::endl;
    acceptIncomingConnection();
    return;
  }

  std::cerr << "Got SOCKS Connection..." << std::endl;

  boost::shared_ptr<SocksConnection> connection(new SocksConnection(socket));
  connection->getRequest(boost::bind(&TorProxy::handleSocksRequest, this,
				     connection, _1, _2, _3));

  acceptIncomingConnection();
}

void TorProxy::handleSocksRequest(boost::shared_ptr<SocksConnection> connection,
				  std::string &host,
				  uint16_t port,
				  const boost::system::error_code &err)
{
  std::cerr << "Got SOCKS Request: " << host << ":" << port << std::endl;

  if (err) connection->close();
  else     tunnel->openStream(host, port, boost::bind(&TorProxy::handleStreamOpen,
						      this, connection, _1, _2));
}

void TorProxy::handleStreamOpen(boost::shared_ptr<SocksConnection> socks,
				boost::shared_ptr<TorTunnelStream> stream,
				const boost::system::error_code &err)
{
  if (err) {
    std::cerr << "Error opening stream: " << err << std::endl;
    socks->respondConnectError();
    socks->close();
    return;
  }

  std::cerr << "Successfully opened Tor exit Node stream..." << std::endl;

  socks->respondConnected(stream->getLocalEndpoint());

  boost::shared_ptr<ProxyShuffler> proxyShuffler(new ProxyShuffler(socks, stream));
  proxyShuffler->shuffle();
}

///////////////////////////////
// Setup

void torTunnelError(const boost::system::error_code &err) {
  std::cerr << "Error with TorTunnel, exiting..." << std::endl;
  exit(0);
}

void nodeConnectionComplete(TorTunnel *nodeTunnel, 
			    boost::asio::io_service &io_service, 
			    Arguments &arguments,
			    const boost::system::error_code &err) 
{
  if (err) {
    std::cerr << "Error Connecting to Exit Node: " << err << std::endl;
    exit(0);
  }

  TorProxy *proxy = new TorProxy(nodeTunnel, io_service, arguments.port);

  std::cerr << "Connected to Exit Node.  SOCKS proxy ready on " << arguments.port << "." << std::endl;
}

void getServerListingComplete(boost::asio::io_service &io_service,
			      Arguments &arguments,
			      boost::shared_ptr<ServerListing> serverListing,
			      const boost::system::error_code &err)
{
  if (err) {
    std::cerr << "Error Retrieving Directory: " << err << std::endl;
    exit(0);
  }

  std::cerr << "Connecting to exit node: " << serverListing->getAddress() 
	    << ":" << serverListing->getPort() << std::endl;
  
  TorTunnel *nodeTunnel = new TorTunnel(io_service, serverListing, 
					boost::bind(torTunnelError, placeholders::error));
  nodeTunnel->connect(boost::bind(nodeConnectionComplete, nodeTunnel,
				  boost::ref(io_service), arguments,
				  placeholders::error));
}

void getDirectoryListingComplete(boost::asio::io_service &io_service,
				 Directory &directory,
				 Arguments &arguments,
				 const boost::system::error_code &er)
{

  if (arguments.random) {
    directory.getRandomServerListing(boost::bind(getServerListingComplete,
						 boost::ref(io_service),
						 arguments,
						 _1, _2));
  } else {
    directory.getServerListingFor(arguments.host, boost::bind(getServerListingComplete,
							      boost::ref(io_service),
							      arguments,
							      _1, _2));
  }
}

void printUsage(char *name) {
  std::cerr << "Usage: " << name << " <options> " << std::endl << std::endl
	    << "Options:" << std::endl
	    << "-n <Exit node IP> -- Specify an exit node to use." << std::endl
	    << "-r                -- Use a randomly selected exit node." << std::endl
	    << "-p <local port>   -- Local port for SOCKS proxy interface." << std::endl
	    << "-h                -- Print this help message." << std::endl << std::endl;
  

  exit(0);
}

int parseOptions(int argc, char **argv, Arguments *arguments) {
  int c;
  arguments->port   = 5060;
  arguments->random = 0;

  opterr = 0;
     
  while ((c = getopt (argc, argv, "n:p:rh")) != -1) {
    switch (c) {
    case 'n':
      arguments->host = optarg;
      break;
    case 'p':
      arguments->port = atoi(optarg);
      break;
    case 'r':
      arguments->random = 1;
      break;
    case 'h':
      printUsage(argv[0]);
    default:
      std::cerr << "Unknown option: " << c << std::endl;
      printUsage(argv[0]);
    }     
  }

  if (arguments->host.empty() && arguments->random == 0) {
    return 0;
  }

  return 1;
}

int main(int argc, char** argv) {
  Arguments arguments;

  if (!parseOptions(argc, argv, &arguments)) {
    printUsage(argv[0]);
    return 2;
  }

  std::cerr << "torproxy " << VERSION << " by Moxie Marlinspike." << std::endl;
  std::cerr << "Retrieving directory listing..." << std::endl;

  boost::asio::io_service io_service;

  Directory directory(io_service);
  directory.retrieveDirectoryListing(boost::bind(getDirectoryListingComplete,
						 boost::ref(io_service),
						 boost::ref(directory),
						 boost::ref(arguments),
						 placeholders::error));  

  io_service::work work(io_service);
  io_service.run();
}
