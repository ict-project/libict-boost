//! @file
//! @brief ASIO module - header file.
//! @author Mariusz Ornowski (mariusz.ornowski@ict-project.pl)
//! @version 1.0
//! @date 2017
//! @copyright ICT-Project Mariusz Ornowski (ict-project.pl)
/* **************************************************************
Copyright (c) 2017, ICT-Project Mariusz Ornowski (ict-project.pl)
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
#ifndef _ASIO_HEADER
#define _ASIO_HEADER
//============================================
#include <map>
#include <thread>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "../libict/source/logger.hpp"
#include "../libict/source/register.hpp"
//============================================
namespace ict { namespace boost { namespace asio {
//===========================================
::boost::asio::io_service & ioService();
::boost::asio::io_service & ioService(std::thread::id id);
//===========================================
//! Stos do obsługi połączenia - góra.
class Top: public ::boost::enable_shared_from_this<Top>, public ict::reg::Base{
protected:
  typedef std::enable_shared_from_this<Top> enable_shared_t;
  //!
  //! Rozpoczyna operacje asynchroniczne.
  //! Funkcja jest nadpisana przez klasę Bottom.
  //! Funkcja powinna być wykonywana przez stos, 
  //!  gdy rozpoczona jakieś operacje na gnieździe.
  //!
  virtual void startOperations()=0;
  //!
  //! Sprawdza, czy stos chce odczytywać z gniazda.
  //! Funkcja do nadpisania przez stos (opcjonalnie).
  //!
  //! @return Wartości:
  //!  @li true - stos chce odczytywać;
  //!  @li false - stos nie chce odczytywać.
  //!
  virtual bool wantRead() const {return(false);}
  //!
  //! Sprawdza, czy stos chce zapisywać do gniazda.
  //! Funkcja do nadpisania przez stos (opcjonalnie).
  //!
  //! @return Wartości:
  //!  @li true - stos chce zapisywać;
  //!  @li false - stos nie chce zapisywać.
  //!
  virtual bool wantWrite() const {return(false);}
  //!
  //! Zmusza stos do wykonania odczytu na gnieździe.
  //! Funkcja do nadpisania przez stos (opcjonalnie).
  //!
  //! @param ec Błąd zwrócony przez funkcje asynchronicznego odczytu.
  //!
  virtual void execRead(const ::boost::system::error_code & ec){};
  //!
  //! Zmusza stos do wykonania zapisu na gnieździe.
  //! Funkcja do nadpisania przez stos (opcjonalnie).
  //!
  //! @param ec Błąd zwrócony przez funkcje asynchronicznego zapisu.
  //!
  virtual void execWrite(const ::boost::system::error_code & ec){};
  //!
  //! Zwraca deskryptor gniazda używanogo przez stos.
  //! Funkcja do nadpisania przez stos (obowiązkowo).
  //!
  //! @return Systemoyw deskryptor gniazda.
  //!
  virtual int getSocket() const=0;
public:
  Top(){};
};
//===========================================
//! Stos do obsługi połączenia - dół.
template<class Socket,class Stack> class Bottom: public Stack {
private:
  ::boost::asio::io_service & io_service;
  ::boost::asio::strand n;
  bool error=false;
  //! Informuje, czy odczyt trwa.
  bool readInProgress=false;
  //! Informuje, czy zapis trwa.
  bool writeInProgress=false;
  std::unique_ptr<Socket> socketPtr;
  void startOperations(){
    auto self(Stack::shared_from_this());
    n.post([this,self](){
      LOGGER_LAYER;
      startOperationsLocal();
    });
  }
  void startOperationsLocal();
  void createSocket(std::unique_ptr<::boost::asio::ip::tcp::socket> & ptr);
  void createSocket(std::unique_ptr<::boost::asio::local::stream_protocol::socket> & ptr);
public:
  Bottom():io_service(ioService()),n(io_service){}
  Bottom(::boost::asio::io_service & io_service_in):io_service(io_service_in),n(io_service){}
};

template<class Socket,class Stack>void Bottom<Socket,Stack>::startOperationsLocal(){
  auto self(Stack::shared_from_this());
  if (error) return;
  if (!socketPtr) {
    createSocket(socketPtr);
    if (socketPtr) socketPtr->non_blocking(true);
  }
  //Read
  if (Stack::wantRead()&&(!readInProgress)&&socketPtr){
    readInProgress=true;
    socketPtr->async_read_some(
      ::boost::asio::null_buffers(),
      [this,self](const ::boost::system::error_code & ec,std::size_t length){
        LOGGER_LAYER;
        readInProgress=false;
        if (!ec) Stack::execRead(ec);
        if ((!ec)||ec==::boost::asio::error::would_block) {
          startOperationsLocal();
        } else if (socketPtr) {
          socketPtr->close();
          error=true;
          LOGGER_INFO<<__LOGGER__<<"Socket result: "<<ec<<std::endl;
        }
      }
    );
  }
  //Write
  if (Stack::wantWrite()&&(!writeInProgress)&&socketPtr){
    writeInProgress=true;
    socketPtr->async_write_some(
      ::boost::asio::null_buffers(),
      [this,self](const ::boost::system::error_code & ec,std::size_t length){
        LOGGER_LAYER;
        writeInProgress=false;
        if (!ec) Stack::execWrite(ec);
        if ((!ec)||ec==::boost::asio::error::would_block) {
          startOperationsLocal();
        } else if (socketPtr) {
          socketPtr->close();
          error=true;
          LOGGER_INFO<<__LOGGER__<<"Socket result: "<<ec<<std::endl;
        }
      }
    );
  }
}
template<class Socket,class Stack>void Bottom<Socket,Stack>::createSocket(std::unique_ptr<::boost::asio::ip::tcp::socket> & ptr){
  int s=Stack::getSocket();
  int s_type=0;
  int s_domain=0;
  ::socklen_t l;
  if (s){
    l=sizeof(s_type);
    ::getsockopt(s,SOL_SOCKET,SO_TYPE,&s_type,&l);
    l=sizeof(s_domain);
    ::getsockopt(s,SOL_SOCKET,SO_DOMAIN,&s_domain,&l);
    if (s_type==SOCK_STREAM) switch (s_domain){
      case AF_INET:
        ptr.reset(new ::boost::asio::ip::tcp::socket(io_service,::boost::asio::ip::tcp::v4(),s));
        break;
      case AF_INET6:
        ptr.reset(new ::boost::asio::ip::tcp::socket(io_service,::boost::asio::ip::tcp::v6(),s));
        break;
      default:break;
    }
  }
  if (!ptr) {
    error=true;
    LOGGER_ERR<<__LOGGER__<<"Socket error: s="<<s<<", s_type="<<s_type<<", s_domain="<<s_domain<<std::endl;
  }
}
template<class Socket,class Stack>void Bottom<Socket,Stack>::createSocket(std::unique_ptr<::boost::asio::local::stream_protocol::socket> & ptr){
  int s=Stack::getSocket();
  int s_type=0;
  ::socklen_t l;
  if (s){
    l=sizeof(s_type);
    ::getsockopt(s,SOL_SOCKET,SO_TYPE,&s_type,&l);
    if (s_type==SOCK_STREAM) ptr.reset(new ::boost::asio::local::stream_protocol::socket(io_service,::boost::asio::local::stream_protocol(),s));
  }
  if (!ptr) {
    error=true;
    LOGGER_ERR<<__LOGGER__<<"Socket error: s="<<s<<", s_type="<<s_type<<std::endl;
  }
}
//============================================
}}}
//===========================================
#endif
