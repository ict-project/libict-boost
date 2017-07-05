//! @file
//! @brief Connection (http) module - Source file.
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
#include "connection-http.hpp"
//============================================
#ifdef ENABLE_TESTING
#include "test.hpp"
#endif
//============================================
namespace ict { namespace boost { namespace connection { namespace http {
//============================================
const std::string endl("\r\n");
const std::string space(" ");
const std::string colon(":");
const std::string comma(",");
const std::string _GET_("GET");
const std::string _HEAD_("HEAD");
const std::string _POST_("POST");
const std::string _PUT_("PUT");
const std::string _DELETE_("DELETE");
const std::string _CONNECT_("CONNECT");
const std::string _OPTIONS_("OPTIONS");
const std::string _TRACE_("TRACE");
const std::string _PATCH_("PATCH");
const std::string _HTTP_1_0_("HTTP/1.0");
const std::string _HTTP_1_1_("HTTP/1.1");
const std::string _content_length_("content-length");
const std::string _content_type_("content-type");
header_config_t default_config={.multiple_values=true,.multiple_lines=true};
config_t config={
  {_content_length_,      {.multiple_values=false,.multiple_lines=false}},
  {_content_type_,        {.multiple_values=false,.multiple_lines=false}}
};
//============================================
void Headers::transform_name(std::string & name){
  std::transform(name.begin(),name.end(),name.begin(),[](char c)->char{
    if (::isascii(c)) return(::tolower(c));
    return('_');
  });
}
//! Normalizuje wartość nagłówka i elementu start line (litery ASCII).
void Headers::transform_value(std::string & value){
  std::transform(value.begin(),value.end(),value.begin(),[](char c)->char{
    if (::isascii(c)) return(c);
    return('_');
  });
  value.erase(value.begin(),std::find_if(value.begin(),value.end(),std::not1(std::ptr_fun<int,int>(std::isspace))));
  value.erase(std::find_if(value.rbegin(),value.rend(),std::not1(std::ptr_fun<int,int>(std::isspace))).base(),value.end());
}
//! Odczytuje pierwszy lub drugi element start line.
int Headers::read_start_element(std::string & output,std::size_t min,std::size_t max,const std::string & descr){
  const static std::string _too_big_(" - too big...");
  const static std::string _too_small_(" - too small...");
  const static std::string _missing_(" - missing...");
  if (!output.size()){
    std::size_t e=readString.find(endl);
    std::size_t s=readString.find(space);
    if (e==std::string::npos){//Końca linii nie znaleziono.
      if (s==std::string::npos){//Spacji nie znaleziono.
          if (readString.size()>max){
            LOGGER_WARN<<__LOGGER__<<descr<<_too_big_<<std::endl;
            return(-1);
          } else {
            return(1);//Nie wszystkie dane.
          }
        } else {//Spację znaleziono.
          if (s>max){
            LOGGER_WARN<<__LOGGER__<<descr<<_too_big_<<std::endl;
            return(-1);
          } else if (s<min) {
            LOGGER_WARN<<__LOGGER__<<descr<<_too_small_<<std::endl;
            return(-1);
          } else {
            output=readString.substr(0,s);
            readString.erase(0,s+space.size());
            transform_value(output);
          }
        }
      } else {//Koniec linii znaleziono.
        if (s==std::string::npos){//Spacji nie znaleziono.
          LOGGER_WARN<<__LOGGER__<<descr<<_missing_<<std::endl;
          return(-1);
        } else {//Spację znaleziono.
          if (e>s){//Spacja przed końcem linii.
            if (s>max){
              LOGGER_WARN<<__LOGGER__<<descr<<_too_big_<<std::endl;
              return(-1);
            } else if (s<min) {
              LOGGER_WARN<<__LOGGER__<<descr<<_too_small_<<std::endl;
              return(-1);
            } else {
              output=readString.substr(0,s);
              readString.erase(0,s+space.size());
              transform_value(output);
            }
          } else {//Spacja po końcu linii.
            LOGGER_WARN<<__LOGGER__<<descr<<_missing_<<std::endl;
            return(-1);
          }
        }
      }
  }
  return(0);//Odczytano element.
}
//! Zapisuje pierwszy lub drugi element start line.
int Headers::write_start_element(std::string & input,std::size_t min,std::size_t max,const std::string & descr){
  const static std::string _too_big_(" - too big...");
  const static std::string _too_small_(" - too small...");
  if (input.size()){
    if (input.size()>max) {
      LOGGER_ERR<<__LOGGER__<<descr<<_too_big_<<std::endl;
      return(-1);
    } else if (input.size()<min) {
      LOGGER_ERR<<__LOGGER__<<descr<<_too_small_<<std::endl;
      return(-1);
    } else {
      if ((writeString.size()+input.size()+space.size())<writeString.max_size()){
        writeString+=input+space;
        input.clear();
      } else {
        return(1);
      }
    }
  }
  return(0);
}
//! Odczytuje trzeci element start line (opcjonalny).
int Headers::read_start_last(std::string & output,std::size_t max,const std::string & descr){
  const static std::string _too_big_(" - too big...");
  if (!output.size()){
    std::size_t e=readString.find(endl);
    if (e==std::string::npos){//Końca linii nie znaleziono.
      if (readString.size()>max){
        LOGGER_WARN<<__LOGGER__<<descr<<_too_big_<<std::endl;
        return(-1);
      } else {
        return(1);//Nie wszystkie dane.
      }
    } else {//Koniec linii znaleziono.
      if (e>max){
        LOGGER_WARN<<__LOGGER__<<descr<<_too_big_<<std::endl;
        return(-1);
      } else if (e>0) {
        output=readString.substr(0,e);
        readString.erase(0,e+endl.size());
        transform_value(output);
      } else {
        output="_";
        readString.erase(0,e+endl.size());
      }
    }
  }
  return(0);//Odczytano element.
}
//! Zapisuje trzeci element start line (opcjonalny).
int Headers::write_start_last(std::string & input,std::size_t max,const std::string & descr){
  const static std::string _too_big_(" - too big...");
  if (input.size()){
    if (input.size()>max) {
      LOGGER_ERR<<__LOGGER__<<descr<<_too_big_<<std::endl;
      return(-1);
    } else {
      if ((writeString.size()+input.size()+endl.size())<writeString.max_size()){
        writeString+=input+endl;
        input.clear();
      } else {
        return(1);
      }
    }
  }
  return(0);
}
#define READ_WRITE(operation)\
  switch(operation){\
    case 0:break;\
    case 1:return(1);\
    default:return(-1);\
  }
//! Odczytuje start line (dla request).
int Headers::read_request_line(){
  static const std::size_t min_method_size(3);
  static const std::size_t max_method_size(10);
  static const std::size_t min_uri_size(1);
  static const std::size_t max_uri_size(10000);
  static const std::size_t max_version_size(10);
  READ_WRITE(read_start_element(request_method,min_method_size,max_method_size,"HTTP request method"))
  READ_WRITE(read_start_element(request_uri,min_uri_size,max_uri_size,"HTTP request URI"))
  READ_WRITE(read_start_last(request_version,max_version_size,"HTTP request version"))
  return(0);
}
//! Zapisuje start line (dla request).
int Headers::write_request_line(){
  static const std::size_t min_method_size(3);
  static const std::size_t max_method_size(10);
  static const std::size_t min_uri_size(1);
  static const std::size_t max_uri_size(10000);
  static const std::size_t max_version_size(10);
  READ_WRITE(write_start_element(request_method,min_method_size,max_method_size,"HTTP request method"))
  READ_WRITE(write_start_element(request_uri,min_uri_size,max_uri_size,"HTTP request URI"))
  READ_WRITE(write_start_last(request_version,max_version_size,"HTTP request version"))
  return(0);
}
//! Odczytuje start line (dla response).
int Headers::read_status_line(){
  static const std::size_t min_version_size(1);
  static const std::size_t max_version_size(10);
  static const std::size_t min_code_size(3);
  static const std::size_t max_code_size(4);
  static const std::size_t max_msg_size(1000);
  READ_WRITE(read_start_element(response_version,min_version_size,max_version_size,"HTTP response version"))
  READ_WRITE(read_start_element(response_code,min_code_size,max_code_size,"HTTP response code"))
  READ_WRITE(read_start_last(response_msg,max_msg_size,"HTTP response message"))
  return(0);
}
//! Zapisuje start line (dla response).
int Headers::write_status_line(){
  static const std::size_t min_version_size(1);
  static const std::size_t max_version_size(10);
  static const std::size_t min_code_size(3);
  static const std::size_t max_code_size(4);
  static const std::size_t max_msg_size(1000);
  READ_WRITE(write_start_element(response_version,min_version_size,max_version_size,"HTTP response version"))
  READ_WRITE(write_start_element(response_code,min_code_size,max_code_size,"HTTP response code"))
  READ_WRITE(write_start_last(response_msg,max_msg_size,"HTTP response message"))
  return(0);
}
//! Odczytuje nagłówki.
int Headers::read_headers(headers_t & headers){
  const static std::string _too_big_(" - too big...");
  const static std::string _too_small_(" - too small...");
  const static std::string _missing_(" - missing...");
  const static std::size_t max_header_line_size(10000);
  const static std::size_t min_header_name_size(5);
  const static std::size_t max_header_name_size(100);
  std::size_t e;
  while((e=readString.find(endl))!=std::string::npos){
    if (e==0){
      readString.erase(0,endl.size());
      return(0);//Koniec nagłówków.
    } else if (e>max_header_line_size){
      LOGGER_WARN<<__LOGGER__<<"HTTP header line"<<_too_big_<<std::endl;
      return(-1);
    } else {
      std::string h(readString.substr(0,e));
      std::size_t c=h.find(colon);
      readString.erase(0,e+endl.size());
      if (c==std::string::npos){
        LOGGER_WARN<<__LOGGER__<<"HTTP header name"<<_missing_<<std::endl;
        return(-1);
      } else {
        if (c>max_header_name_size){
          LOGGER_WARN<<__LOGGER__<<"HTTP header name"<<_too_big_<<std::endl;
          return(-1);
        } else if (c<min_header_name_size) {
          LOGGER_WARN<<__LOGGER__<<"HTTP header name"<<_too_small_<<std::endl;
          return(-1);
        } else {
          header_config_t header_config(default_config);
          std::string header_name(h.substr(0,c));
          transform_name(header_name);
          if  (config.count(header_name)) header_config=config.at(header_name);
          h.erase(0,c+colon.size());
          while((c=h.find(comma))!=std::string::npos){
            std::string header_value(h.substr(0,c));
            h.erase(0,c+comma.size());
            transform_value(header_value);
            headers[header_name].push_back(header_value);
          }
          transform_value(h);
          headers[header_name].push_back(h);
        }
      }
    }
  }
  return(1);//Nie wszystkie nagłówki - czekaj.
}
//! Zapisuje nagłówki.
int Headers::write_headers(headers_t & headers){
  const static std::string _too_big_(" - too big...");
  const static std::string _too_small_(" - too small...");
  const static std::string _missing_(" - missing...");
  const static std::size_t max_header_line_size(10000);
  const static std::size_t min_header_name_size(5);
  const static std::size_t max_header_name_size(100);
  while (headers.size()){
    headers_t::const_iterator it=headers.cbegin();
    header_config_t header_config(default_config);
    std::string header_name(it->first);
    if (header_name.size()){
      transform_name(header_name);
      if  (config.count(header_name)) header_config=config.at(header_name);
      if (header_name.size()>max_header_name_size){
        LOGGER_ERR<<__LOGGER__<<"HTTP header name"<<_too_big_<<std::endl;
        return(-1);
      } else if (header_name.size()<min_header_name_size){
        LOGGER_ERR<<__LOGGER__<<"HTTP header name"<<_too_small_<<std::endl;
        return(-1);
      } else {
        std::string output;
        if (header_config.multiple_lines){
          for (const std::string & v : it->second){
            std::string header_value(v);
            transform_value(header_value);
            if ((header_name.size()+colon.size()+space.size()+header_value.size()+endl.size())>max_header_line_size){
              LOGGER_ERR<<__LOGGER__<<"HTTP header line"<<_too_big_<<std::endl;
              return(-1);
            }
            output+=header_name+colon+space+header_value+endl;
            if (!header_config.multiple_values) break;
          }
        } else {
          bool first=true;
          output+=header_name+colon+space;
          for (const std::string & v : it->second){
            std::string header_value(v);
            transform_value(header_value);
            if (first){
              first=false;
            } else {
              output+=comma;
            }
            output+=header_value;
            if (!header_config.multiple_values) break;
          }
          output+=endl;
          if (output.size()>max_header_line_size){
            LOGGER_ERR<<__LOGGER__<<"HTTP header line"<<_too_big_<<std::endl;
            return(-1);
          }
        }
        if ((writeString.size()+output.size())<writeString.max_size()){
          writeString+=output;
        } else {
          return(1);//Jeszcze poczekaj.
        }
      }
    }
    headers.erase(it->first);
    if (!headers.size()) {//Koniec nagłówków.
      if ((writeString.size()+endl.size())<writeString.max_size()){
        writeString+=endl;
      } else {
        headers[""];//Jeszcze poczekaj.
        return(1);
      }
    }
  }
  return(0);//Koniec nagłówków.
}
//! Odczytuje start line i nagłówki (dla request).
int Headers::read_request(){
  READ_WRITE(read_request_line())
  READ_WRITE(read_headers(request_headers))
  return(0);
}
//! Zapisuje start line i nagłówki (dla request).
int Headers::write_request(){
  READ_WRITE(write_request_line())
  READ_WRITE(write_headers(request_headers))
  return(0);
}
//! Odczytuje start line i nagłówki (dla response).
int Headers::read_response(){
  READ_WRITE(read_status_line())
  READ_WRITE(read_headers(response_headers))
  return(0);
}
//! Zapisuje start line i nagłówki (dla response).
int Headers::write_response(){
  READ_WRITE(write_status_line())
  READ_WRITE(write_headers(response_headers))
  return(0);
}
//! Odczytuje start line i nagłówki.
int Headers::read_all_headers(){
  return(server?read_request():read_response());
}
//! Zapisuje start line i nagłówki.
int Headers::write_all_headers(){
  return(server?write_response():write_request());
}
void  Headers::stringRead(){
  if (reading_phase==phase_start){
    LOGGER_DEBUG<<__LOGGER__<<"read - phase_start"<<std::endl;
    bodyRead(phase_start);
    reading_phase=phase_headers;
  }
  if (!reading_phase){
    switch(read_all_headers()){
      case 0:{
        LOGGER_DEBUG<<__LOGGER__<<"read - phase_headers"<<std::endl;
        asyncRead();
        bodyRead(phase_headers);
        reading_phase=phase_body;
      }break;
      case 1:asyncRead();break;
      default:doClose();break;
    }
  } else {
    LOGGER_DEBUG<<__LOGGER__<<"read - phase_body"<<std::endl;
    bodyRead(phase_body);
  }
}
void Headers::stringWrite(){
  if (writing_phase==phase_start){
    LOGGER_DEBUG<<__LOGGER__<<"write - phase_start"<<std::endl;
    bodyWrite(phase_start);
    writing_phase=phase_headers;
  }
  if (!writing_phase){
    switch(write_all_headers()){
      case 0:{
        LOGGER_DEBUG<<__LOGGER__<<"write - phase_headers"<<std::endl;
        asyncWrite();
        bodyWrite(phase_headers);
        writing_phase=phase_body;
      }break;
      case 1:asyncWrite();break;
      default:doClose();break;
    }
  } else {
    LOGGER_DEBUG<<__LOGGER__<<"write - phase_body"<<std::endl;
    bodyWrite(phase_body);
  }
}
//============================================
void Body::get_content_length(headers_t & headers,std::size_t & size){
  size=0;
  if (headers.count(_content_length_))
    if (headers.at(_content_length_).size()){
      try {
        size=std::stoull(headers.at(_content_length_).at(0));
      } catch(...) {
      }
    }
}
void Body::set_content_length(headers_t & headers,std::size_t & size){
  if (headers.count(_content_length_)) headers.erase(_content_length_);
  if (size) headers[_content_length_].push_back(std::to_string(size));
}
int Body::read_body(std::string & body,std::size_t & content_length){
  std::size_t s=(content_length>body.size())?(content_length-body.size()):0;
  if (s){
    body.append(readString,0,s);
    readString.erase(0,s);
  }
  if (body.size()==content_length) return(0);
  return(1);
}
int Body::write_body(std::string & body,std::size_t & content_length){
  if ((body.size()+writeString.size())<writeString.max_size()){
    writeString+=body;
    body.clear();
  }
  if (body.size()==0) return(0);
  return(1);
}
void Body::bodyRead(phase_t phase){
  switch(phase){
    case phase_start:{
      if (getServer()) {
        beforeRequest();
      } else {
        beforeResponse();
      }
    }break;
    case phase_headers:{
      if (getServer()) {
        get_content_length(request_headers,request_content_length);
        request_body.clear();
      } else {
        get_content_length(response_headers,response_content_length);
        response_body.clear();
      }
    }
    default:{
      switch (getServer()?read_body(request_body,request_content_length):read_body(response_body,response_content_length)){
        case 0:{
          asyncRead();
          if (getServer()) {
            afterRequest();
          } else {
            afterResponse();
          }
        }break;
        case 1:asyncRead();break;
        default:doClose();break;
      }
    }break;
  }
}
void Body::bodyWrite(phase_t phase){
  switch(phase){
    case phase_start:{
      if (getServer()) {
        beforeResponse();
      } else {
        beforeRequest();
      }
    }break;
    case phase_headers:{
      if (getServer()) {
        response_content_length=response_body.size();
        set_content_length(response_headers,response_content_length);
      } else {
        request_content_length=request_body.size();
        set_content_length(request_headers,request_content_length);
      }
    }
    default:{
      switch (getServer()?write_body(response_body,response_content_length):write_body(request_body,request_content_length)){
        case 0:{
          asyncWrite();
          if (getServer()) {
            afterResponse();
          } else {
            afterRequest();
          }
        }break;
        case 1:asyncWrite();break;
        default:doClose();break;
      }
    }break;
  }
}
//============================================
}}}}
//============================================
#ifdef ENABLE_TESTING
#include "server.hpp"
class TestServer : public ict::boost::connection::http::Server{
private:
  void beforeRequest(){}
  void afterRequest(){
    response_version=ict::boost::connection::http::_HTTP_1_0_;
    response_code="200";
    response_msg="OK";
    if (request_method==ict::boost::connection::http::_GET_){
      response_body="Czesc!!!";
      response_headers[ict::boost::connection::http::_content_type_].push_back("text/text");
    } else if (request_method==ict::boost::connection::http::_POST_) {
      response_body=request_body;
      response_headers[ict::boost::connection::http::_content_type_].push_back("text/text");
    } else {
      response_code="405";
      response_msg="Method not allowed";
    }
  }
  void beforeResponse(){}
  void afterResponse(){}
public:
  TestServer(){
    ict::reg::get<TestServer>().add(this);
  }
  ~TestServer(){
    ict::reg::get<TestServer>().del(this);
  }
};
REGISTER_TEST(connection_http,tc1){
  ict::boost::server::factory("localhost","4567",[](::boost::asio::ip::tcp::socket & socket){
    auto ptr=std::make_shared<ict::boost::connection::Bottom<boost::asio::ip::tcp::socket,TestServer>>(socket);
    if (ptr) ptr->initThis();
  });
  ict::reg::get<TestServer>().destroy();
  return(0);
}
#endif
//===========================================
