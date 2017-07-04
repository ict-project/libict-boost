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
  phase_start=-1,
  phase_headers=0,
  phase_body=1
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
extern const std::string _content_length_;
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
  phase_t reading_phase=phase_start;
  void stringRead();
  //! Informacja, czy nagłówki są w tej chwili zapisywane.
  phase_t writing_phase=phase_start;
  void stringWrite();
  void doStart(){
    if (server) {
      asyncRead();
    } else {
      asyncWrite();
    }
  }
protected:
  bool getServer(){return(server);}
  std::string request_method;
  std::string request_uri;
  std::string request_version;
  headers_t   request_headers;
  std::string response_version;
  std::string response_code;
  std::string response_msg;
  headers_t   response_headers;
  //! Rozpoczyna odczyt nagłówków.
  void headersRead(){reading_phase=phase_start;asyncRead();}
  //! Rozpoczyna zapis nagłówków.
  void headersWrite(){writing_phase=phase_start;asyncWrite();}
  //!
  //! Obsługuje odczyt body (funkcja do nadpisania).
  //!
  //! @param phase Aktualna faza odczytu.
  //!
  virtual void bodyRead(phase_t phase)=0;
  //!
  //! Obsługuje zapis body (funkcja do nadpisania).
  //!
  //! @param phase Aktualna faza zapisu.
  //!
  virtual void bodyWrite(phase_t phase)=0;
public:
  Headers(bool serverIn=true):server(serverIn){}
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
  //! @return Jeśli skończył to 'true'.
  //!
  bool read_body(std::string & body,std::size_t & content_length);
  //!
  //! Zapis body.
  //!
  //! @param content_length Ustawione w nagłówku content_length.
  //! @return Jeśli skończył to 'true'.
  //!
  bool write_body(std::string & body,std::size_t & content_length);
  void bodyRead(phase_t phase);
  void bodyWrite(phase_t phase);
protected:
  std::string request_body;
  std::string response_body;
  virtual void beforeRequest(){}
  virtual void afterRequest(){}
  virtual void beforeResponse(){}
  virtual void afterResponse(){}
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
