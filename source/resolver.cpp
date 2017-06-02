//! @file
//! @brief DNS resolver module - Source file.
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
//============================================
#include "resolver.hpp"
#include "asio.hpp"
#include "../libict/source/logger.hpp"
//============================================
#ifdef ENABLE_TESTING
#include "test.hpp"
#endif
//============================================
namespace ict { namespace boost { namespace resolver {
//============================================
Tcp::Tcp(const std::string & host,const std::string & port)
  :r(ict::boost::asio::ioService()),q(host,port),d(ict::boost::asio::ioService()){
  LOGGER_INFO<<__LOGGER__<<"ict::boost::resolver::Tcp has been created ..."<<std::endl;
}
Tcp::Tcp(const std::string & host,const std::string & port,error_handler_t onError)
  :r(ict::boost::asio::ioService()),q(host,port),d(ict::boost::asio::ioService()),Base(onError){
  LOGGER_INFO<<__LOGGER__<<"ict::boost::resolver::Tcp has been created ..."<<std::endl;
}
Tcp::~Tcp(){
  LOGGER_INFO<<__LOGGER__<<"ict::boost::resolver::Tcp has been destroyed ..."<<std::endl;
}
void Tcp::doResolve(){
  auto self(enable_shared_t::shared_from_this());
  LOGGER_DEBUG<<__LOGGER__<<"Trying to resolve "<<q.host_name()<<":"<<q.service_name()<<" ..."<<std::endl;
  if ((q.host_name()=="")||(q.host_name()=="0.0.0.0")||(q.host_name()=="[::]")){
    any=true;
    try{
      ::boost::asio::ip::tcp::endpoint endpoint(::boost::asio::ip::tcp::v6(),std::stol(q.service_name()));
      ep=endpoint;
      LOGGER_DEBUG<<__LOGGER__<<"Resolving "<<"<any>"<<":"<<q.service_name()<<" has succeeded ..."<<std::endl;
      afterResolve();
    }catch(...){
      LOGGER_INFO<<__LOGGER__<<"Resolving "<<"<any>"<<":"<<q.service_name()<<" has failed ..."<<std::endl;
    }
    return;
  }
  d.expires_from_now(::boost::posix_time::seconds(60));
  d.async_wait(
    [this,self](const ::boost::system::error_code& ec){
      LOGGER_LAYER;
      if (ec){
        if (e) e(ec);
        onError();
      } else {
        LOGGER_INFO<<__LOGGER__<<"Resolving timer "<<q.host_name()<<":"<<q.service_name()<<" has expired ..."<<std::endl;
        r.cancel();
        if (e) e(ec);
      }
    }
  );
  r.async_resolve(
    q,
    [this,self](const ::boost::system::error_code& ec,::boost::asio::ip::tcp::resolver::iterator endpoint_iterator){
      LOGGER_LAYER;
      if (ec){
        if (e) e(ec);
        onError();
      } else {
        d.cancel();
        ei=endpoint_iterator;
        LOGGER_DEBUG<<__LOGGER__<<"Resolving "<<q.host_name()<<":"<<q.service_name()<<" has succeeded ..."<<std::endl;
        afterResolve();
      }
    }
  );
}
void Tcp::cancelResolve(){
  r.cancel();
}
//============================================
Stream::Stream(const std::string & path):ep(path){
  LOGGER_INFO<<__LOGGER__<<"ict::boost::resolver::Stream has been created ..."<<std::endl;
}
Stream::Stream(const std::string & path,error_handler_t onError):ep(path),Base(onError){
  LOGGER_INFO<<__LOGGER__<<"ict::boost::resolver::Stream has been created ..."<<std::endl;
}
Stream::~Stream(){
  LOGGER_INFO<<__LOGGER__<<"ict::boost::resolver::Stream has been destroyed ..."<<std::endl;
}
void Stream::doResolve(){
  auto self(enable_shared_t::shared_from_this());
  afterResolve();
}
void Stream::cancelResolve(){
}
//============================================
}}}
//============================================
#ifdef ENABLE_TESTING
class TestTcp: public ict::boost::resolver::Tcp {
public:
  TestTcp(const std::string & host,const std::string & port,ict::boost::resolver::error_handler_t onError):
    ict::boost::resolver::Tcp(host,port,onError){}
private:
  void afterResolve(){
    std::cout<<"ict::boost::resolver::Tcp - OK: ";
    if (any){
      std::cout<<ep<<std::endl;
    } else {
      for (;ei!=boost::asio::ip::tcp::resolver::iterator();++ei) std::cout<<ei->endpoint()<<",";
      std::cout<<std::endl;
    }
    ict::boost::asio::ioService().stop();
  };
};
class TestStream: public ict::boost::resolver::Stream {
public:
  TestStream(const std::string & path,ict::boost::resolver::error_handler_t onError):
    ict::boost::resolver::Stream(path,onError){}
private:
  void afterResolve(){
    std::cout<<"ict::boost::resolver::Stream - OK: ";
    std::cout<<ep<<std::endl;
    ict::boost::asio::ioService().stop();
  };
};
REGISTER_TEST(resolver,tc1){
  bool err=false;
  auto ptr=std::make_shared<TestTcp>("wp.pl","80",[&](const ::boost::system::error_code & e){
    err=true;
    std::cout<<"ict::boost::resolver::Tcp - ERR: ";
    std::cout<<e<<std::endl;
    ict::boost::asio::ioService().stop();
  });
  if (ptr) ptr->initThis();
  ict::boost::asio::ioService().run();
  if (err) return(-1);
  return(0);
}
REGISTER_TEST(resolver,tc2){
  bool err=false;
  auto ptr=std::make_shared<TestStream>("/tmp/test.stream",[&](const ::boost::system::error_code & e){
    err=true;
    std::cout<<"ict::boost::resolver::Tcp - ERR: ";
    std::cout<<e<<std::endl;
    ict::boost::asio::ioService().stop();
  });
  if (ptr) ptr->initThis();
  ict::boost::asio::ioService().run();
  if (err) return(-1);
  return(0);
}
#endif
//===========================================
