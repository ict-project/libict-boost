//! @file
//! @brief Server module - Source file.
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
#include "server.hpp"
#include "asio.hpp"
#include "../libict/source/logger.hpp"
#include "../libict/source/options.hpp"
#include "../libict/source/register.hpp"
//============================================
#ifdef ENABLE_TESTING
#include "test.hpp"
#endif
//============================================
#define REGISTER_SERVER_TCP ict::reg::get<Tcp>()
#define REGISTER_SERVER_STREAM ict::reg::get<Stream>()
//============================================
namespace ict { namespace boost { namespace server {
//============================================
Tcp::Tcp(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory)
  :resolver::Tcp(host,port),s(ict::boost::asio::ioService()),a(ict::boost::asio::ioService()),f(factory){
  LOGGER_INFO<<__LOGGER__<<"smpp::server::Tcp has been created ..."<<std::endl;
  REGISTER_SERVER_TCP.add(this,"smpp::server::Tcp "+host+":"+port);
}
Tcp::Tcp(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory,resolver::error_handler_t onError)
  :resolver::Tcp(host,port,onError),s(ict::boost::asio::ioService()),a(ict::boost::asio::ioService()),f(factory){
  LOGGER_INFO<<__LOGGER__<<"smpp::server::Tcp has been created ..."<<std::endl;
  REGISTER_SERVER_TCP.add(this,"smpp::server::Tcp "+host+":"+port);
}
Tcp::~Tcp(){
  LOGGER_INFO<<__LOGGER__<<"smpp::server::Tcp has been destroyed ..."<<std::endl;
  REGISTER_SERVER_TCP.del(this);
}
void Tcp::doStop(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  stopped=true;
  s.close();
  a.close();
}
void Tcp::afterResolve(){
  auto self(enable_shared_t::shared_from_this());
  if (any){
    doBind(ep);
  } else {
    doBind();
  }
}
bool Tcp::doBind(const ::boost::asio::ip::tcp::endpoint & ep){
  if (stopped) return(false);
  {
    LOGGER_DEBUG<<__LOGGER__<<"Trying to bind "<<ep<<" ..."<<std::endl;
    {
      ::boost::system::error_code ec;
      a.open(ep.protocol(),ec);
      if (ec) {
        LOGGER_INFO<<__LOGGER__<<"Bind to "<<ep<<" has failed ..."<<std::endl;
        doBind();
        return(false);
      } else {
        a.bind(ep,ec);
        if (ec) {
          LOGGER_INFO<<__LOGGER__<<"Bind to "<<ep<<" has failed ..."<<std::endl;
          doBind();
          return(false);
        } else {
          a.listen(::boost::asio::socket_base::max_connections,ec);
          if (ec) {
            LOGGER_INFO<<__LOGGER__<<"Bind to "<<ep<<" has failed ..."<<std::endl;
            doBind();
            return(false);
          } else {
            LOGGER_DEBUG<<__LOGGER__<<"Bind to "<<ep<<" has succeeded ..."<<std::endl;
            doAccept();
          }
        }
      }
    }
  }
  return(true);
}
bool Tcp::doBind(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return(false);
  if (ei!=::boost::asio::ip::tcp::resolver::iterator()){
    if (doBind(ei->endpoint())){
      return(true);
    } else {
      ++ei;
    }
  } else {
    ::boost::system::error_code ec;
    LOGGER_NOTICE<<__LOGGER__<<"Bind has finally failed ..."<<std::endl;
    doStop();
    if (e) e(ec);
  }
  return(false);
}
void Tcp::doAccept(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  a.async_accept(
    s,
    [this,self](::boost::system::error_code ec){
      LOGGER_LAYER;
      if (ec){
        LOGGER_INFO<<__LOGGER__<<"Unable to accept a new connection ("<<ec<<") ..."<<std::endl;
        errors++;
      } else {
        LOGGER_INFO<<__LOGGER__<<"A new connection has been accepted ..."<<std::endl;
        if (f) {
          f(s);
          errors=0;
        } else {
          std::move(s).close();
          LOGGER_ERR<<__LOGGER__<<"Factory is empty ..."<<std::endl;
          errors++;
        }
      }
      if (errors<10) {doAccept();} else {doStop();}
    }
  );
}
void factory(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory){
  auto ptr=std::make_shared<Tcp>(host,port,factory);
  if (ptr) ptr->init();
}
void factory(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory,resolver::error_handler_t onError){
  auto ptr=std::make_shared<Tcp>(host,port,factory,onError);
  if (ptr) ptr->init();
}
//============================================
Stream::Stream(const std::string & path,ict::boost::connection::factory_stream_t factory)
  :resolver::Stream(path),s(ict::boost::asio::ioService()),a(ict::boost::asio::ioService()),f(factory){
  LOGGER_INFO<<__LOGGER__<<"smpp::server::Stream has been created ..."<<std::endl;
  REGISTER_SERVER_STREAM.add(this,"smpp::server::Stream "+path);
}
Stream::Stream(const std::string & path,ict::boost::connection::factory_stream_t factory,resolver::error_handler_t onError)
  :resolver::Stream(path,onError),s(ict::boost::asio::ioService()),a(ict::boost::asio::ioService()),f(factory){
  LOGGER_INFO<<__LOGGER__<<"smpp::server::Stream has been created ..."<<std::endl;
  REGISTER_SERVER_STREAM.add(this,"smpp::server::Stream "+path);
}
Stream::~Stream(){
  LOGGER_INFO<<__LOGGER__<<"smpp::server::Stream has been destroyed ..."<<std::endl;
  REGISTER_SERVER_STREAM.del(this);
}
void Stream::doStop(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  stopped=true;
  s.close();
  a.close();
  try {::unlink(ep.path().c_str());} catch (...){}
}
void Stream::afterResolve(){
  auto self(enable_shared_t::shared_from_this());
  doBind();
}
void Stream::doBind(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  LOGGER_DEBUG<<__LOGGER__<<"Trying to bind "<<ep<<" ..."<<std::endl;
  {
    ::boost::system::error_code ec;
    a.open(ep.protocol(),ec);
    if (ec) {
      LOGGER_INFO<<__LOGGER__<<"Bind to "<<ep<<" has finally failed ..."<<std::endl;
      doBind();
      if (e) e(ec);
    } else {
      a.bind(ep,ec);
      if (ec) {
        LOGGER_INFO<<__LOGGER__<<"Bind to "<<ep<<" has finally failed ..."<<std::endl;
        doBind();
        if (e) e(ec);
      } else {
        a.listen(::boost::asio::socket_base::max_connections,ec);
        if (ec) {
          LOGGER_INFO<<__LOGGER__<<"Bind to "<<ep<<" has finally failed ..."<<std::endl;
          doBind();
          if (e) e(ec);
        } else {
          LOGGER_DEBUG<<__LOGGER__<<"Bind to "<<ep<<" has succeeded ..."<<std::endl;
          doAccept();
        }
      }
    }
  }
}
void Stream::doAccept(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  a.async_accept(
    s,
    [this,self](::boost::system::error_code ec){
      LOGGER_LAYER;
      if (ec){
        LOGGER_INFO<<__LOGGER__<<"Unable to accept a new connection ("<<ec<<") ..."<<std::endl;
        errors++;
      } else {
        LOGGER_INFO<<__LOGGER__<<"A new connection has been accepted ..."<<std::endl;
        if (f) {
          f(s);
          errors=0;
        } else {
          std::move(s).close();
          LOGGER_ERR<<__LOGGER__<<"Factory is empty ..."<<std::endl;
          errors++;
        }
      }
      if (errors<10) {doAccept();} else {doStop();}
    }
  );
}
void factory(const std::string & path,ict::boost::connection::factory_stream_t factory){
  try {::unlink(path.c_str());} catch (...){}
  {
    auto ptr=std::make_shared<Stream>(path,factory);
    if (ptr) ptr->init();
  }
}
void factory(const std::string & path,ict::boost::connection::factory_stream_t factory,resolver::error_handler_t onError){
  try {::unlink(path.c_str());} catch (...){}
  {
    auto ptr=std::make_shared<Stream>(path,factory,onError);
    if (ptr) ptr->init();
  }
}
//============================================
}}}
//============================================
#ifdef ENABLE_TESTING
//REGISTER_TEST(server,tc1){
//  return(0);
//}
#endif
//===========================================
