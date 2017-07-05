//! @file
//! @brief Connection (http) module - header file.
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
#ifndef _CONNECTION_HTTP_HEADER
#define _CONNECTION_HTTP_HEADER
//============================================
#include <map>
#include <vector>
#include "connection.hpp"
//============================================
namespace ict { namespace boost { namespace connection { namespace http {
//===========================================
enum phase_t {
  phase_before,
  phase_headers,
  phase_between,
  phase_body,
  phase_after,
  phase_end
};
struct header_config_t {
  //Czy nagłówek może mieć wiele wartośći.
  bool multiple_values;
  //Czy, gdy nagłówek ma wiele wartości, to zapisywane są one w odzielnych liniach.
  bool multiple_lines;
};
typedef std::map<std::string,header_config_t> config_t;
typedef std::map<std::string,std::vector<std::string>> headers_t;
//===========================================
extern const std::string endl;
extern const std::string space;
extern const std::string colon;
extern const std::string comma;
extern const std::string _GET_;
extern const std::string _HEAD_;
extern const std::string _POST_;
extern const std::string _PUT_;
extern const std::string _DELETE_;
extern const std::string _CONNECT_;
extern const std::string _OPTIONS_;
extern const std::string _TRACE_;
extern const std::string _PATCH_;
extern const std::string _HTTP_1_0_;
extern const std::string _HTTP_1_1_;
extern const std::string _content_length_;
extern const std::string _content_type_;
extern header_config_t default_config;
extern config_t config;
//===========================================
class Headers : public ict::boost::connection::TopString {
private:
  bool server;
  //! Normalizuje nazwę nagłówka (małe litery ASCII).
  void transform_name(std::string & name);
  //! Normalizuje wartość nagłówka i elementu start line (litery ASCII).
  void transform_value(std::string & value);
  //! Odczytuje pierwszy lub drugi element start line.
  int read_start_element(std::string & output,std::size_t min,std::size_t max,const std::string & descr);
  //! Zapisuje pierwszy lub drugi element start line.
  int write_start_element(std::string & input,std::size_t min,std::size_t max,const std::string & descr);
  //! Odczytuje trzeci element start line (opcjonalny).
  int read_start_last(std::string & output,std::size_t max,const std::string & descr);
  //! Zapisuje trzeci element start line (opcjonalny).
  int write_start_last(std::string & input,std::size_t max,const std::string & descr);
  //! Odczytuje start line (dla request).
  int read_request_line();
  //! Zapisuje start line (dla request).
  int write_request_line();
  //! Odczytuje start line (dla response).
  int read_status_line();
  //! Zapisuje start line (dla response).
  int write_status_line();
  //! Odczytuje nagłówki.
  int read_headers(headers_t & headers);
  //! Zapisuje nagłówki.
  int write_headers(headers_t & headers);
  //! Odczytuje start line i nagłówki (dla request).
  int read_request();
  //! Zapisuje start line i nagłówki (dla request).
  int write_request();
  //! Odczytuje start line i nagłówki (dla response).
  int read_response();
  //! Zapisuje start line i nagłówki (dla response).
  int write_response();
  //! Odczytuje start line i nagłówki.
  int read_all_headers();
  //! Zapisuje start line i nagłówki.
  int write_all_headers();
  //! Informacja, czy nagłówki są w tej chwili odczytywane.
  phase_t reading_phase;
  void stringRead();
  //! Informacja, czy nagłówki są w tej chwili zapisywane.
  phase_t writing_phase;
  void stringWrite();
protected:
  bool getServer(){return(server);}
  void doStart(){
    if (server) {
      asyncRead();
    } else {
      asyncWrite();
    }
  }
  std::string request_method;
  std::string request_uri;
  std::string request_version;
  headers_t   request_headers;
  std::string response_version;
  std::string response_code;
  std::string response_msg;
  headers_t   response_headers;
  //! Rozpoczyna odczyt nagłówków.
  void startRead(){
    if (server){
      request_method.clear();
      request_uri.clear();
      request_version.clear();
      request_headers.clear();
    } else {
      response_version.clear();
      response_code.clear();
      response_msg.clear();
      response_headers.clear();
    }
    reading_phase=phase_before;
    asyncRead();
  }
  //! Rozpoczyna zapis nagłówków.
  void startWrite(){
    writing_phase=phase_before;
    asyncWrite();
  }
  //!
  //! Funkcja wykonywana przed odczytem nagłówków (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int beforeRead()=0;
  //!
  //! Funkcja wykonywana przed zapisem nagłówków (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int beforeWrite()=0;
  //!
  //! Funkcja wykonywana pomiędzy odczytem nagłówków i body (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int betweenRead()=0;
  //!
  //! Funkcja wykonywana pomiędzy zapisem nagłówków i body (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int betweenWrite()=0;
  //!
  //! Obsługuje odczyt body (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int bodyRead()=0;
  //!
  //! Obsługuje zapis body (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int bodyWrite()=0;
  //!
  //! Funkcja wykonywana po odczytaniu body (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int afterRead()=0;
  //!
  //! Funkcja wykonywana po zapisaniu body (funkcja do nadpisania).
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int afterWrite()=0;
public:
  Headers(bool serverIn=true):
    server(serverIn),
    reading_phase(serverIn?phase_before:phase_end),
    writing_phase(serverIn?phase_end:phase_before){}
};
//============================================
class Body : public Headers{
private:
  std::size_t request_content_length=0;
  std::size_t response_content_length=0;
  //! Pobiera z nagłówków content_length.
  void get_content_length(headers_t & headers,std::size_t & size);
  //! Ustawia w nagłówkach content_length.
  void set_content_length(headers_t & headers,std::size_t & size);
  //!
  //! Odczyt body.
  //!
  //! @param content_length Probrane z nagłówka content_length.
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  int read_body(std::string & body,std::size_t & content_length);
  //!
  //! Zapis body.
  //!
  //! @param content_length Ustawione w nagłówku content_length.
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  int write_body(std::string & body,std::size_t & content_length);
  int beforeRead();
  int beforeWrite();
  int betweenRead();
  int betweenWrite();
  int bodyRead();
  int bodyWrite();
  int afterRead();
  int afterWrite();
protected:
  std::string request_body;
  std::string response_body;
  //!
  //! Operacje przed request.
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int beforeRequest(){return(0);}
  //!
  //! Operacje po request.
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int afterRequest(){return(0);}
  //!
  //! Operacje przed response.
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int beforeResponse(){return(0);}
  //!
  //! Operacje po response.
  //!
  //! @return Wartosci:
  //!  @li 0 - zakończone;
  //!  @li 1 - jeszcze trwa;
  //!  @li -1 - wystąpił błąd.
  //!
  virtual int afterResponse(){return(0);}
public:
  Body(bool serverIn=true):Headers(serverIn){}
};
//============================================
class Server : public Body{
public:
  Server():Body(true){}
};
//============================================
class Client : public Body{
public:
  Client():Body(false){}
};
//============================================
}}}}
//===========================================
#endif
