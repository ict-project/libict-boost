//! @file
//! @brief Server module - header file.
//! @author Mariusz Ornowski (mariusz.ornowski@ict-project.pl)
//! @version 1.0
//! @date 2016-2017
//! @copyright ICT-Project Mariusz Ornowski (ict-project.pl)
/* **************************************************************
Copyright (c) 2016-2017, ICT-Project Mariusz Ornowski (ict-project.pl)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. Neither the name of the ICT-Project Mariusz Ornowski nor the names
of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**************************************************************/
#ifndef _SERVER_HEADER
#define _SERVER_HEADER
//============================================
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "resolver.hpp"
#include "connection.hpp"
//============================================
namespace ict { namespace boost { namespace server {
//===========================================
//! Klasa tworząca serwer do obsługi połączeń TCP.
class Tcp : public resolver::Tcp {
private:
  //! Czy serwer jest zatrzymany.
  bool stopped=false;
  //! Liczba błędnych połązeń przychodzących.
  uint8_t errors=0;
  //! Gniazdo do obsługi połączenia.
  ::boost::asio::ip::tcp::socket s;
  //! Akceptor do obsługi połączeń przychodzących.
  ::boost::asio::ip::tcp::acceptor a;
  //! Fabryka połączeń.
  ict::boost::connection::factory_tcp_t f;
public:
  Tcp(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory);
  Tcp(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory,resolver::error_handler_t onError);
  virtual ~Tcp();
  void doStop();
  void destroyThis(){doStop();}
private:
  //! Funkcja wykonywana, gdy zapytanie DNS zakończy się sukcesem.
  void afterResolve();
  //! Przypisuje gniazdo.
  bool doBind(const ::boost::asio::ip::tcp::endpoint & ep);
  bool doBind();
  //! Rozpoczyna akceptację połączeń.
  void doAccept();
};
//! Fabryka tworząca serwery do obsługi połączeń TCP.
void factory(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory);
//! Fabryka tworząca serwery do obsługi połączeń TCP.
void factory(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory,resolver::error_handler_t onError);
//============================================
//! Klasa tworząca serwer do obsługi połączeń lokalnych gniazd systemowych (Unix).
class Stream : public resolver::Stream {
private:
  //! Czy klient jest zatrzymany.
  bool stopped=false;
  //! Liczba błędnych połązeń przychodzących.
  uint8_t errors=0;
  //! Gniazdo do obsługi połączenia.
  ::boost::asio::local::stream_protocol::socket s;
  //! Akceptor do obsługi połączeń przychodzących.
  ::boost::asio::local::stream_protocol::acceptor a;
  //! Fabryka połączeń.
  ict::boost::connection::factory_stream_t f;
public:
  Stream(const std::string & path,ict::boost::connection::factory_stream_t factory);
  Stream(const std::string & path,ict::boost::connection::factory_stream_t factory,resolver::error_handler_t onError);
  virtual ~Stream();
  //! Zamyka połączenie.
  void doStop();
  void destroyThis(){doStop();}
private:
  //! Funkcja wykonywana, gdy zapytanie zakończy się sukcesem
  void afterResolve();
  //! Przypisuje gniazdo.
  void doBind();
  //! Rozpoczyna akceptację połączeń.
  void doAccept();
};
//! Fabryka tworząca serwery do obsługi połączeń lokalnych gniazd systemowych (Unix).
void factory(const std::string & path,ict::boost::connection::factory_stream_t factory);
//! Fabryka tworząca serwery do obsługi połączeń lokalnych gniazd systemowych (Unix).
void factory(const std::string & path,ict::boost::connection::factory_stream_t factory,resolver::error_handler_t onError);
//============================================
}}}
//===========================================
#endif