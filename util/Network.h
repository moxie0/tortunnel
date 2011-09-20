#ifndef __NETWORK_H__
#define __NETWORK_H__

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
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <string>

using namespace boost::asio;

typedef boost::function<void (const boost::system::error_code &error)> SocketSuckHandler;

class Network {

 private:
  static void socketReadComplete(boost::shared_ptr<ip::tcp::socket> socket,
				 boost::shared_ptr<char> buf,
				 std::string *result,
				 SocketSuckHandler handler,
				 const boost::system::error_code &err,
				 size_t transferred);

  static void socketReadResponse(boost::shared_ptr<ip::tcp::socket> socket,
				 boost::shared_ptr<char> buf,
				 std::string *result,
				 SocketSuckHandler handler);

  static void requestSent(boost::shared_ptr<ip::tcp::socket> socket,
			  boost::shared_ptr<std::string> request,
			  std::string *result,
			  SocketSuckHandler handler,
			  const boost::system::error_code &err);


  static void sockConnected(boost::shared_ptr<ip::tcp::socket> socket,
			    boost::shared_ptr<std::string> request,
			    std::string *result,
			    SocketSuckHandler handler, 
			    const boost::system::error_code &err);

 public:
  static void suckUrlToString(boost::asio::io_service &io_service,
			      std::string &ip,
			      int port,
			      boost::shared_ptr<std::string> request, 
			      std::string *result, 
			      SocketSuckHandler handler);


};


#endif
