//! @file
//! @brief Client module - Source file.
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
#include "client.hpp"
#include "asio.hpp"
#include "../libict/source/logger.hpp"
#include "../libict/source/options.hpp"
//============================================
#ifdef ENABLE_TESTING
#include "test.hpp"
#endif
//============================================
#define REGISTER_CLIENT_TCP ict::reg::get<Tcp>()
#define REGISTER_CLIENT_STREAM ict::reg::get<Stream>()
//============================================
namespace ict { namespace boost { namespace client {
//============================================
Tcp::Tcp(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory)
  :resolver::Tcp(host,port),s(ict::boost::asio::ioService()),f(factory),d(ict::boost::asio::ioService()){
  LOGGER_INFO<<__LOGGER__<<"smpp::client::Tcp has been created ..."<<std::endl;
  REGISTER_CLIENT_TCP.add(this,"smpp::client::Tcp "+host+":"+port);
}
Tcp::Tcp(const std::string & host,const std::string & port,ict::boost::connection::factory_tcp_t factory,resolver::error_handler_t onError)
  :resolver::Tcp(host,port,onError),s(ict::boost::asio::ioService()),f(factory),d(ict::boost::asio::ioService()){
  LOGGER_INFO<<__LOGGER__<<"smpp::client::Tcp has been created ..."<<std::endl;
  REGISTER_CLIENT_TCP.add(this,"smpp::client::Tcp "+host+":"+port);
}
Tcp::~Tcp(){
  LOGGER_INFO<<__LOGGER__<<"smpp::client::Tcp has been destroyed ..."<<std::endl;
  REGISTER_CLIENT_TCP.del(this);
}
void Tcp::doStop(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  stopped=true;
  s.close();
  d.cancel();
}
void Tcp::afterResolve(){
  auto self(enable_shared_t::shared_from_this());
  doConnect();
}
void Tcp::doConnect(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  d.expires_from_now(::boost::posix_time::seconds(60));
  d.async_wait(
    [this,self](const ::boost::system::error_code& ec){
      LOGGER_LAYER;
      if (ec){
        if (e) e(ec);
      } else {
        LOGGER_INFO<<__LOGGER__<<"Connection timer "<<ei->endpoint()<<" has expired ..."<<std::endl;
        doStop();
        if (e) e(ec);
      }
    }
  );
  if (ei!=::boost::asio::ip::tcp::resolver::iterator()){
    LOGGER_DEBUG<<__LOGGER__<<"Trying to connect "<<ei->endpoint()<<" ..."<<std::endl;
    s.async_connect(
      ei->endpoint(),
      [this,self](::boost::system::error_code ec){
        LOGGER_LAYER;
        d.cancel();
        if (ec){
          LOGGER_INFO<<__LOGGER__<<"Connection to "<<ei->endpoint()<<" has failed ..."<<std::endl;
          ++ei;
          doConnect();
        } else {
          LOGGER_DEBUG<<__LOGGER__<<"Connection to "<<ei->endpoint()<<" has succeeded ..."<<std::endl;
          if (f) {
            f(s);
          } else {
            std::move(s).close();
            LOGGER_ERR<<__LOGGER__<<"Factory for "<<ei->endpoint()<<" is empty ..."<<std::endl;
            if (e) e(ec);
          }
        }
      }
    );
  } else {
    ::boost::system::error_code ec;
    LOGGER_NOTICE<<__LOGGER__<<"Connection has finally failed ..."<<std::endl;
    doStop();
    if (e) e(ec);
  }
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
  :resolver::Stream(path),s(ict::boost::asio::ioService()),f(factory){
  LOGGER_INFO<<__LOGGER__<<"smpp::client::Stream has been created ..."<<std::endl;
  REGISTER_CLIENT_STREAM.add(this,"smpp::client::Stream "+path);
}
Stream::Stream(const std::string & path,ict::boost::connection::factory_stream_t factory,resolver::error_handler_t onError)
  :resolver::Stream(path,onError),s(ict::boost::asio::ioService()),f(factory){
  LOGGER_INFO<<__LOGGER__<<"smpp::client::Stream has been created ..."<<std::endl;
  REGISTER_CLIENT_STREAM.add(this,"smpp::client::Stream "+path);
}
Stream::~Stream(){
  LOGGER_INFO<<__LOGGER__<<"smpp::client::Stream has been destroyed ..."<<std::endl;
  REGISTER_CLIENT_STREAM.del(this);
}
void Stream::doStop(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  stopped=true;
  s.close();
}
void Stream::afterResolve(){
  auto self(enable_shared_t::shared_from_this());
  doConnect();
}
void Stream::doConnect(){
  auto self(enable_shared_t::shared_from_this());
  if (stopped) return;
  LOGGER_DEBUG<<__LOGGER__<<"Trying to connect "<<ep<<" ..."<<std::endl;
  try {
    s.connect(ep);
    LOGGER_DEBUG<<__LOGGER__<<"Connection to "<<ep<<" has succeeded ..."<<std::endl;
    if (f) {
      f(s);
    } else {
      ::boost::system::error_code ec;
      std::move(s).close();
      LOGGER_ERR<<__LOGGER__<<"Factory for "<<ep<<" is empty ..."<<std::endl;
      if (e) e(ec);
    }
  } catch (std::exception& ex) {
    ::boost::system::error_code ec;
    LOGGER_WARN<<__LOGGER__<<"Unable connect to "<<ep<<": "<<ex.what()<<std::endl;
    if (e) e(ec);
  }
}
void factory(const std::string & path,ict::boost::connection::factory_stream_t factory){
  auto ptr=std::make_shared<Stream>(path,factory);
  if (ptr) ptr->init();
}
void factory(const std::string & path,ict::boost::connection::factory_stream_t factory,resolver::error_handler_t onError){
  auto ptr=std::make_shared<Stream>(path,factory,onError);
  if (ptr) ptr->init();
}
//============================================
}}}
//============================================
#ifdef ENABLE_TESTING
//REGISTER_TEST(client,tc1){
//  return(0);
//}
#endif
//===========================================

