//! @file
//! @brief DNS resolver module - header file.
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
#ifndef _RESOLVER_HEADER
#define _RESOLVER_HEADER
//============================================
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>
#include "../libict/source/register.hpp"
//============================================
namespace ict { namespace boost { namespace resolver {
//===========================================
typedef std::function<void(const ::boost::system::error_code&)> error_handler_t;
//===========================================
//! Klasa podstawowa.
class Base : public ict::reg::Base {
public:
  //! Konstruktor.
  Base(){}
  //! Konstruktor z dodatkową (zewnętrzną) obsługą błędów.
  Base(error_handler_t onError):e(onError){}
  //! Destruktor.
  virtual ~Base(){}
  //! Inicjuje rozwiązywanie nazw DNS.
  void init(){doResolve();}
  //! Inicjuje rozwiązywanie nazw DNS.
  void initThis(){doResolve();}
  //! Przerywa rozwiązywanie nazw DNS.
  void destroyThis(){cancelResolve();}
protected:
  //! Rozpoczyna rozwiązywanie nazw (funkcja obowiązkowo do nadpisania).
  virtual void doResolve()=0;
  //! Anuluje rozwiązywanie nazw (funkcja obowiązkowo do nadpisania).
  virtual void cancelResolve()=0;
  //! Funkcja wykonywana, gdy zapytanie zakończy się sukcesem (funkcja obowiązkowo do nadpisania).
  virtual void afterResolve()=0;
  //! Wewnętrzna obsługa błedów (funkcja obowiązkowo do nadpisania).
  virtual void onError()=0;
  //! Zewnętrzna obsługa błędów.
  error_handler_t e;
};
//============================================
//! Klasa do obsługi TCP.
class Tcp : public std::enable_shared_from_this<Tcp>, public Base {
private:
  //! Asynchroniczne rozwiązywanie nazw DNS.
  ::boost::asio::ip::tcp::resolver r;
  //! Zapytanie  DNS.
  ::boost::asio::ip::tcp::resolver::query q;
  //! Timer dla zapytania DNS.
  ::boost::asio::deadline_timer d;
public:
  //! Konstruktor.
  Tcp(const std::string & host,const std::string & port);
  //! Konstruktor z dodatkową (zewnętrzną) obsługą błędów.
  Tcp(const std::string & host,const std::string & port,error_handler_t onError);
  //! Destruktor.
  virtual ~Tcp();
private:
  //! Rozpoczyna rozwiązywanie nazw.
  void doResolve();
  //! Anuluje rozwiązywanie nazw.
  void cancelResolve();
protected:
  typedef std::enable_shared_from_this<Tcp> enable_shared_t;
  //! Wynik zapytania do DNS.
  ::boost::asio::ip::tcp::resolver::iterator ei;
  //! Czy ma być użyty adres <<any>>
  bool any=false;
  //! Jeśli any to użyj tego endpointu.
  ::boost::asio::ip::tcp::endpoint ep;
  //! Wewnętrzna obsługa błedów.
  virtual void onError(){};
};
//============================================
//! Klasa do obsługi lokalnych strumieni (Unix).
class Stream : public std::enable_shared_from_this<Stream>, public Base {
public:
  //! Konstruktor.
  Stream(const std::string & path);
  //! Konstruktor z dodatkową (zewnętrzną) obsługą błędów.
  Stream(const std::string & path,error_handler_t onError);
  //! Destruktor.
  virtual ~Stream();
private:
  //! Rozpoczyna rozwiązywanie nazw.
  void doResolve();
  //! Anuluje rozwiązywanie nazw.
  void cancelResolve();
protected:
  typedef std::enable_shared_from_this<Stream> enable_shared_t;
  //! Lokalny endpoint.
  ::boost::asio::local::stream_protocol::endpoint ep;
  //! Wewnętrzna obsługa błedów.
  virtual void onError(){};
};
//============================================
}}}
//===========================================
#endif
