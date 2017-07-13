//! @file
//! @brief Connection module - header file.
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
#ifndef _CONNECTION_HEADER
#define _CONNECTION_HEADER
//============================================
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>
#include "../libict/source/logger.hpp"
#include "../libict/source/register.hpp"
#include "asio.hpp"
#include "connection-string.hpp"
//============================================
namespace ict { namespace boost { namespace connection {
//===========================================
//! Stos do obsługi połączenia - góra.
class Top : public std::enable_shared_from_this<Top>, public ict::reg::Base {
protected:
  typedef std::enable_shared_from_this<Top> enable_shared_t;
  //! Opis połączenia.
  std::string sDesc;
  //! Rozmiar lokalnego bufora (zapisu i odczytu).
  enum {bufferSize=1024};
  //! Lokalny bufor odczytu.
  unsigned char readData[bufferSize];
  //! Rozmiar odczytanych danych.
  std::size_t readSize=0;
  //! Lokalny bufor zapisu.
  unsigned char writeData[bufferSize];
  //! Rozmiar danych do zapisu.
  std::size_t writeSize=0;
  //! Minimalna liczba bajtów na minutę przy odczycie (jeśli nie jest zachowana to połączene jest zamykane) - jeśli 0, to brak ograniczenia.
  std::size_t readMinFlow=0;
  //! Minimalna liczba bajtów na minutę przy zapisie (jeśli nie jest zachowana to połączene jest zamykane) - jeśli 0, to brak ograniczenia.
  std::size_t writeMinFlow=0;
  //! Ustawienie asychronicznego odczytu (funkcja nadpisania w Bottom).
  virtual void asyncRead()=0;
  //! Ustawienie asychronicznego zapisu (funkcja nadpisania w Bottom).
  virtual void asyncWrite()=0;
  //! Zamyka połącznie (funkcja nadpisania w Bottom).
  virtual void doClose()=0;
  //!
  //! @brief Wykonuje odczyt (funkcja obowiązkowo do nadpisania).
  //!  Powinna ustawić przy zakończeniu nowy asynchroniczny odczyt 
  //!  (lub ewentualnie zapis) - funkcje asyncRead() i/lub asyncWrite().
  //!  Jeśli tego nie zrobi połączenie może być automatycznie zamknięte.
  //!
  virtual void doRead()=0;
  //!
  //! @brief Wykonuje zapis (funkcja obowiązkowo do nadpisania).
  //!  Powinna ustawić przy zakończeniu nowy asynchroniczny zapis 
  //!  (lub ewentualnie odczyt) - funkcje asyncWrite() i/lub asyncRead().
  //!  Jeśli tego nie zrobi połączenie może być automatycznie zamknięte.
  //!
  virtual void doWrite()=0;
  //!
  //! @brief Obsługuje błąd odczytu (funkcja ewentualnie do nadpisania).
  //!  Może ustawić przy zakończeniu nowy asynchroniczny odczyt 
  //!  (lub ewentualnie zapis) - funkcje asyncRead() i/lub asyncWrite().
  //!  Jeśli tego nie zrobi połączenie może być automatycznie zamknięte.
  //! @param ec Kod błędu.
  //!
  virtual void readError(::boost::system::error_code ec){}
  //!
  //! @brief Obsługuje błąd zapisu (funkcja ewentualnie do nadpisania).
  //!  Może ustawić przy zakończeniu nowy asynchroniczny zapis 
  //!  (lub ewentualnie odczyt) - funkcje asyncWrite() i/lub asyncRead().
  //!  Jeśli tego nie zrobi połączenie może być automatycznie zamknięte.
  //! @param ec Kod błędu.
  //!
  virtual void writeError(::boost::system::error_code ec){}
  //! Standardowe funkcje (bez kodu) - do ewentualnego nadpisania.
  virtual void doStart(){};
  //! Standardowe funkcje (bez kodu) - do ewentualnego nadpisania.
  virtual void doStop(){};
public:
  Top();
  virtual ~Top();
  //! Zwraca opis połączenia.
  std::string socketDesc() const;
};
//============================================
//! Stos do obsługi połączenia za pomocą bufora std::string  - góra.
class TopString : public Top,public ict::boost::connection::string::Top {
protected:
  void doRead();
  void doWrite();
public:
  virtual ~TopString();
};
//============================================
//! Stos do obsługi połączenia - dół.
template<class Socket,class Stack>class Bottom : public Stack{
private:
  //! Informuje, czy połączenie jest zamknięte.
  bool stopped=false;
  //! Informuje, czy odczyt został ustawiony i czeka.
  bool readWaiting=false;
  //! Informuje, czy zapis został ustawiony i czeka.
  bool writeWaiting=false;
  //! Timer do obliczania liczby bajtów na minutę.
  ::boost::asio::deadline_timer d;
  //! Rozmiar odczytanych danych.
  std::size_t readSizeLast=0;
  //! Rozmiar zapisanych danych.
  std::size_t writeSizeLast=0;
  //! Okres czasu, w którym jest wykonywany cykl pomiarowy (w sekundach).
  static const uint8_t duration=3;
  //! Krok do oblizcania prędkości odczytu i zapisu.
  uint8_t step=0;
  //! Prędkość odczytu (bajty na minutę).
  float readFlow=0;
  //! Prędkość odczytu (bajty na minutę).
  float writeFlow=0;
  //! Funkcja ustawiająca timer do obliczania liczby bajtów na minutę.
  void scheduleMinFlow();
  //! Funkkcja sprawdzająca liczbę bajtów na minutę i zamukająca połączenie, gdy nie są spełnione określone minima.
  void checkMinFlow();
protected:
  //! Socket.
  Socket s;
  //! Ustawienie asychronicznego odczytu.
  void asyncRead();
  //! Ustawienie asychronicznego zapisu.
  void asyncWrite();
public:
  Bottom(Socket & socket);
  virtual ~Bottom();
  //! Zamyka połącznie.
  void doClose();
  void initThis();
  void destroyThis();
};
template<class Socket,class Stack>void Bottom<Socket,Stack>::scheduleMinFlow() {
  auto self(Stack::shared_from_this());
  if (stopped) return;
  d.expires_from_now(::boost::posix_time::seconds(duration));
  d.async_wait(
    [this,self](const ::boost::system::error_code & ec){
      LOGGER_LAYER;
      if(!ec){
        if (stopped) return;
        {
          float tmpFlow;
          tmpFlow=readSizeLast*60;
          tmpFlow/=duration;
          readFlow=(readFlow*step)+tmpFlow;
          readFlow/=(step+1);
          readSizeLast=0;
          tmpFlow=writeSizeLast*60;
          tmpFlow/=duration;
          writeFlow=(writeFlow*step)+tmpFlow;
          writeFlow/=(step+1);
          writeSizeLast=0;
          if (step<(60/duration)) step++;
        }
        if ((Stack::readMinFlow&&(readFlow<Stack::readMinFlow))||(Stack::writeMinFlow&&(writeFlow<Stack::writeMinFlow))){
          doClose();
          if (readFlow<Stack::readMinFlow) LOGGER_WARN<<__LOGGER__<<"Read flow to low ("<<readFlow<<"<"<<Stack::readMinFlow<<") on connection "<<Stack::socketDesc()<<std::endl;
          if (writeFlow<Stack::writeMinFlow) LOGGER_WARN<<__LOGGER__<<"Write flow to low ("<<writeFlow<<"<"<<Stack::writeMinFlow<<") on connection "<<Stack::socketDesc()<<std::endl;
        }
        scheduleMinFlow();
      }
    }
  );
  LOGGER_DEBUG<<__LOGGER__<<"Connection "<<Stack::socketDesc()<<" use count: "<<self.use_count()<<std::endl;
}
template<class Socket,class Stack>void Bottom<Socket,Stack>::asyncRead(){
  auto self(Stack::shared_from_this());
  if (stopped) return;
  if (readWaiting) return;
  //::boost::asio::async_read(
    //s,
  s.async_read_some(
    ::boost::asio::buffer(Stack::readData,Stack::bufferSize),
    [this,self](const ::boost::system::error_code & ec, std::size_t length){
      LOGGER_LAYER;
      readWaiting=false;
      try {
        if (ec) {
          Stack::readError(ec);
          doClose();
        } else {
          Stack::readSize=length;
          readSizeLast+=length;
            //LOGGER_DEBUG<<__LOGGER__;
            //smpp::main::memoryDump(LOGGER_DEBUG,Stack::readData,length);
            //LOGGER_DEBUG<<std::endl;
          Stack::doRead();
          LOGGER_DEBUG<<__LOGGER__<<"Connection "<<Stack::socketDesc()<<" read("<<ec<<") count: "<<length<<std::endl;
        }
      } catch (std::exception& e) {
        LOGGER_ERR<<__LOGGER__<<"Exception: "<<e.what()<<std::endl;
        doClose();
      }
    }
  );
  readWaiting=true;
  LOGGER_DEBUG<<__LOGGER__<<"Connection "<<Stack::socketDesc()<<" use count: "<<self.use_count()<<std::endl;
}
template<class Socket,class Stack>void Bottom<Socket,Stack>::asyncWrite(){
  auto self(Stack::shared_from_this());
  if (stopped) return;
  if (writeWaiting) return;
  ::boost::asio::async_write(
    s,
    ::boost::asio::buffer(Stack::writeData,Stack::bufferSize>Stack::writeSize?Stack::writeSize:Stack::bufferSize),
    [this,self](const ::boost::system::error_code & ec, std::size_t length){
      LOGGER_LAYER;
      writeWaiting=false;
      try {
        if (ec){
          Stack::readError(ec);
          doClose();
        } else {
            //LOGGER_DEBUG<<__LOGGER__;
            //smpp::main::memoryDump(LOGGER_DEBUG,Stack::writeData,length);
            //LOGGER_DEBUG<<std::endl;
          Stack::writeSize=0;
          writeSizeLast+=length;
          Stack::doWrite();
          LOGGER_DEBUG<<__LOGGER__<<"Connection "<<Stack::socketDesc()<<" write("<<ec<<") count: "<<length<<std::endl;
        }
      } catch (std::exception& e) {
        LOGGER_ERR<<__LOGGER__<<"Exception: "<<e.what()<<std::endl;
        doClose();
      }
    }
  );
  writeWaiting=true;
  LOGGER_DEBUG<<__LOGGER__<<"Connection "<<Stack::socketDesc()<<" use count: "<<self.use_count()<<std::endl;
}
template<class Socket,class Stack>Bottom<Socket,Stack>::Bottom(Socket & socket):d(ict::boost::asio::ioService()),s(std::move(socket)){
  std::ostringstream out;
  LOGGER_INFO<<__LOGGER__<<"smpp::connection::Connection has been created ..."<<std::endl;
  out<<"{local:"<<s.local_endpoint()<<", remote:"<<s.remote_endpoint()<<", ptr:"<<this<<"}";
  Stack::sDesc=out.str();
}
template<class Socket,class Stack> void Bottom<Socket,Stack>::initThis() {
  scheduleMinFlow();
  Stack::doStart();
}
template<class Socket,class Stack>Bottom<Socket,Stack>::~Bottom() {
  LOGGER_INFO<<__LOGGER__<<"smpp::connection::Connection has been destroyed ..."<<std::endl;
}
template<class Socket,class Stack>void Bottom<Socket,Stack>::doClose(){
  auto self(Stack::shared_from_this());
  ::boost::system::error_code ec;
  if (stopped) return;
  stopped=true;
  Stack::doStop();
  s.close();
  d.cancel();
  LOGGER_DEBUG<<__LOGGER__<<"Connection "<<Stack::socketDesc()<<" use count: "<<self.use_count()<<std::endl;
}
template<class Socket,class Stack>void Bottom<Socket,Stack>::destroyThis(){
  doClose();
}
//============================================
typedef std::function<void(::boost::asio::ip::tcp::socket &)> factory_tcp_t;
typedef std::function<void(::boost::asio::local::stream_protocol::socket &)> factory_stream_t;
//============================================
}}}
//===========================================
#endif
