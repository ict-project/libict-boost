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
#include <regex>
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
const std::string _cookie_("cookie");
const std::string _set_cookie_("set-cookie");
const std::string _connection_("connection");
const std::string _forwarded_("forwarded");
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
void Headers::headerKeyValueParser(const std::string & input,std::map<std::string,std::string> & output){
  std::string s(input);
  std::smatch m;
  std::regex e("\\s*([^;=]+)\\s*(=([^;]*))?;?");
  output.clear();
  while (std::regex_search(s,m,e)) {
    if (m.size()==2){
      output[m[1]]="";
    } else if (m.size()>3){
      std::string tmp(m[3]);
      if ((tmp.front()=='"')&&(tmp.back()=='"')){
        output[m[1]]=tmp.substr(1,tmp.size()-2);
      } else {
        output[m[1]]=tmp;
      }
    }
    s=m.suffix().str();
  }
}
std::string Headers::headerSetCookieHeader(const std::string & name,const std::string & value,ict::time::unix_t maxAge,const std::string & path,const std::string & domain,bool secure,bool httpOnly){
  std::string out;
  out+=name;
  out+="=";
  out+=value;
  out+="; ";
  if (maxAge!=-1){
    out+="Max-Age=";
    out+=std::to_string(maxAge);
    out+="; ";
  }
  if (path.size()){
    out+="path=";
    out+=path;
    out+="; ";
  }
  if (domain.size()){
    out+="path=";
    out+=domain;
    out+="; ";
  }
  if (secure){
    out+="Secure; ";
  }
  if (httpOnly){
    out+="HttpOnly; ";
  }
  return(out);
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
#define READ_WRITE_1(operation)\
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
  READ_WRITE_1(read_start_element(request_method,min_method_size,max_method_size,"HTTP request method"))
  READ_WRITE_1(read_start_element(request_uri,min_uri_size,max_uri_size,"HTTP request URI"))
  READ_WRITE_1(read_start_last(request_version,max_version_size,"HTTP request version"))
  return(0);
}
//! Zapisuje start line (dla request).
int Headers::write_request_line(){
  static const std::size_t min_method_size(3);
  static const std::size_t max_method_size(10);
  static const std::size_t min_uri_size(1);
  static const std::size_t max_uri_size(10000);
  static const std::size_t max_version_size(10);
  READ_WRITE_1(write_start_element(request_method,min_method_size,max_method_size,"HTTP request method"))
  READ_WRITE_1(write_start_element(request_uri,min_uri_size,max_uri_size,"HTTP request URI"))
  READ_WRITE_1(write_start_last(request_version,max_version_size,"HTTP request version"))
  return(0);
}
//! Odczytuje start line (dla response).
int Headers::read_status_line(){
  static const std::size_t min_version_size(1);
  static const std::size_t max_version_size(10);
  static const std::size_t min_code_size(3);
  static const std::size_t max_code_size(4);
  static const std::size_t max_msg_size(1000);
  READ_WRITE_1(read_start_element(response_version,min_version_size,max_version_size,"HTTP response version"))
  READ_WRITE_1(read_start_element(response_code,min_code_size,max_code_size,"HTTP response code"))
  READ_WRITE_1(read_start_last(response_msg,max_msg_size,"HTTP response message"))
  return(0);
}
//! Zapisuje start line (dla response).
int Headers::write_status_line(){
  static const std::size_t min_version_size(1);
  static const std::size_t max_version_size(10);
  static const std::size_t min_code_size(3);
  static const std::size_t max_code_size(4);
  static const std::size_t max_msg_size(1000);
  READ_WRITE_1(write_start_element(response_version,min_version_size,max_version_size,"HTTP response version"))
  READ_WRITE_1(write_start_element(response_code,min_code_size,max_code_size,"HTTP response code"))
  READ_WRITE_1(write_start_last(response_msg,max_msg_size,"HTTP response message"))
  return(0);
}
//! Odczytuje nagłówki.
int Headers::read_headers(headers_t & headers){
  const static std::string _too_big_(" - too big...");
  const static std::string _too_small_(" - too small...");
  const static std::string _missing_(" - missing...");
  const static std::size_t max_header_line_size(10000);
  const static std::size_t min_header_name_size(3);
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
  READ_WRITE_1(read_request_line())
  READ_WRITE_1(read_headers(request_headers))
  return(0);
}
//! Zapisuje start line i nagłówki (dla request).
int Headers::write_request(){
  READ_WRITE_1(write_request_line())
  READ_WRITE_1(write_headers(request_headers))
  return(0);
}
//! Odczytuje start line i nagłówki (dla response).
int Headers::read_response(){
  READ_WRITE_1(read_status_line())
  READ_WRITE_1(read_headers(response_headers))
  return(0);
}
//! Zapisuje start line i nagłówki (dla response).
int Headers::write_response(){
  READ_WRITE_1(write_status_line())
  READ_WRITE_1(write_headers(response_headers))
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
#define READ_WRITE_2(operation)\
  switch (operation){ \
    case 0:break; \
    case 1:return; \
    default:doClose();return; \
  }
void Headers::stringRead(){
  switch(reading_phase){
    case phase_before:{
      LOGGER_DEBUG<<__LOGGER__<<"read - phase_start"<<std::endl;
      READ_WRITE_2(beforeRead());
      reading_phase=phase_headers;
    }
    case phase_headers:{
      LOGGER_DEBUG<<__LOGGER__<<"read - phase_headers"<<std::endl;
      READ_WRITE_2(read_all_headers());
      reading_phase=phase_between;
    }
    case phase_between:{
      LOGGER_DEBUG<<__LOGGER__<<"read - phase_between"<<std::endl;
      READ_WRITE_2(betweenRead());
      reading_phase=phase_body;
    }
    case phase_body:{
      LOGGER_DEBUG<<__LOGGER__<<"read - phase_body"<<std::endl;
      READ_WRITE_2(bodyRead());
      reading_phase=phase_after;
    }
    case phase_after:{
      LOGGER_DEBUG<<__LOGGER__<<"read - phase_after"<<std::endl;
      READ_WRITE_2(afterRead());
      reading_phase=phase_end;
    }
    default:break;
  }
  asyncRead();
}
void Headers::stringWrite(){
  switch(writing_phase){
    case phase_before:{
      LOGGER_DEBUG<<__LOGGER__<<"write - phase_start"<<std::endl;
      READ_WRITE_2(beforeWrite());
      writing_phase=phase_headers;
    }
    case phase_headers:{
      LOGGER_DEBUG<<__LOGGER__<<"write - phase_headers"<<std::endl;
      READ_WRITE_2(write_all_headers());
      writing_phase=phase_between;
    }
    case phase_between:{
      LOGGER_DEBUG<<__LOGGER__<<"write - phase_between"<<std::endl;
      READ_WRITE_2(betweenWrite());
      writing_phase=phase_body;
    }
    case phase_body:{
      LOGGER_DEBUG<<__LOGGER__<<"write - phase_body"<<std::endl;
      READ_WRITE_2(bodyWrite());
      writing_phase=phase_after;
    }
    case phase_after:{
      LOGGER_DEBUG<<__LOGGER__<<"write - phase_after"<<std::endl;
      READ_WRITE_2(afterWrite());
      writing_phase=phase_end;
    }
    default:break;
  }
  if (0<writeString.size()) asyncWrite();
}
//============================================
void Body::get_single_header(headers_t & headers,const std::string & name,std::string & value){
  value.clear();
  if (headers.count(name))
    if (headers.at(name).size())
      value=headers.at(name).at(0);
}
void Body::set_single_header(headers_t & headers,const std::string & name,const std::string & value){
  if (headers.count(name)) headers.erase(name);
  if (value.size()) headers[name].push_back(value);
}
void Body::get_content_length(headers_t & headers,std::size_t & size){
  std::string value;
  size=0;
  get_single_header(headers,_content_length_,value);
  if (value.size()) try {
    size=std::stoull(value);
  } catch(...) {}
}
void Body::set_content_length(headers_t & headers,const std::size_t & size){
  std::string value;
  if (size) {
    value=std::to_string(size);
  } else if (getServer()) if (request_method=="OPTIONS") value="0";
  set_single_header(headers,_content_length_,value);
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
void Body::setResponseCode(unsigned int code){
  static const std::map<unsigned int,std::string> code_map={
    //Source: https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
    {100,"Continue"},//The server has received the request headers and the client should proceed to send the request body (in the case of a request for which a body needs to be sent; for example, a POST request). Sending a large request body to a server after a request has been rejected for inappropriate headers would be inefficient. To have a server check the request's headers, a client must send Expect: 100-continue as a header in its initial request and receive a 100 Continue status code in response before sending the body. The response 417 Expectation Failed indicates the request should not be continued.[2]
    {101,"Switching Protocols"},//The requester has asked the server to switch protocols and the server has agreed to do so.[5]
    {102,"Processing (WebDAV; RFC 2518)"},//A WebDAV request may contain many sub-requests involving file operations, requiring a long time to complete the request. This code indicates that the server has received and is processing the request, but no response is available yet.[6] This prevents the client from timing out and assuming the request was lost.
    {200,"OK"},//Standard response for successful HTTP requests. The actual response will depend on the request method used. In a GET request, the response will contain an entity corresponding to the requested resource. In a POST request, the response will contain an entity describing or containing the result of the action.[7]
    {201,"Created"},//The request has been fulfilled, resulting in the creation of a new resource.[8]
    {202,"Accepted"},//The request has been accepted for processing, but the processing has not been completed. The request might or might not be eventually acted upon, and may be disallowed when processing occurs.[9]
    {203,"Non-Authoritative Information"},//The server is a transforming proxy (e.g. a Web accelerator) that received a 200 OK from its origin, but is returning a modified version of the origin's response.[10][11]
    {204,"No Content"},//The server successfully processed the request and is not returning any content.[12]
    {205,"Reset Content"},//The server successfully processed the request, but is not returning any content. Unlike a 204 response, this response requires that the requester reset the document view.[13]
    {206,"Partial Content (RFC 7233)"},//The server is delivering only part of the resource (byte serving) due to a range header sent by the client. The range header is used by HTTP clients to enable resuming of interrupted downloads, or split a download into multiple simultaneous streams.[14]
    {207,"Multi-Status (WebDAV; RFC 4918)"},//The message body that follows is an XML message and can contain a number of separate response codes, depending on how many sub-requests were made.[15]
    {208,"Already Reported (WebDAV; RFC 5842)"},//The members of a DAV binding have already been enumerated in a preceding part of the (multistatus) response, and are not being included again.
    {226,"IM Used (RFC 3229)"},//The server has fulfilled a request for the resource, and the response is a representation of the result of one or more instance-manipulations applied to the current instance.[16]
    {300,"Multiple Choices"},//Indicates multiple options for the resource from which the client may choose (via agent-driven content negotiation). For example, this code could be used to present multiple video format options, to list files with different filename extensions, or to suggest word-sense disambiguation.[18]
    {301,"Moved Permanently"},//This and all future requests should be directed to the given URI.[19]
    {302,"Found"},//This is an example of industry practice contradicting the standard. The HTTP/1.0 specification (RFC 1945) required the client to perform a temporary redirect (the original describing phrase was "Moved Temporarily"),[20] but popular browsers implemented 302 with the functionality of a 303 See Other. Therefore, HTTP/1.1 added status codes 303 and 307 to distinguish between the two behaviours.[21] However, some Web applications and frameworks use the 302 status code as if it were the 303.[22]
    {303,"See Other"},//The response to the request can be found under another URI using a GET method. When received in response to a POST (or PUT/DELETE), the client should presume that the server has received the data and should issue a redirect with a separate GET message.[23]
    {304,"Not Modified (RFC 7232)"},//Indicates that the resource has not been modified since the version specified by the request headers If-Modified-Since or If-None-Match. In such case, there is no need to retransmit the resource since the client still has a previously-downloaded copy.[24]
    {305,"Use Proxy (since HTTP/1.1)"},//The requested resource is available only through a proxy, the address for which is provided in the response. Many HTTP clients (such as Mozilla[25] and Internet Explorer) do not correctly handle responses with this status code, primarily for security reasons.[26]
    {306,"Switch Proxy"},//No longer used. Originally meant "Subsequent requests should use the specified proxy."[27]
    {307,"Temporary Redirect (since HTTP/1.1)"},//In this case, the request should be repeated with another URI; however, future requests should still use the original URI. In contrast to how 302 was historically implemented, the request method is not allowed to be changed when reissuing the original request. For example, a POST request should be repeated using another POST request.[28]
    {308,"Permanent Redirect (RFC 7538)"},//The request and all future requests should be repeated using another URI. 307 and 308 parallel the behaviors of 302 and 301, but do not allow the HTTP method to change. So, for example, submitting a form to a permanently redirected resource may continue smoothly.[29]
    {400,"Bad Request"},//The server cannot or will not process the request due to an apparent client error (e.g., malformed request syntax, size too large, invalid request message framing, or deceptive request routing).[31]
    {401,"Unauthorized (RFC 7235)"},//Similar to 403 Forbidden, but specifically for use when authentication is required and has failed or has not yet been provided. The response must include a WWW-Authenticate header field containing a challenge applicable to the requested resource. See Basic access authentication and Digest access authentication.[32] 401 semantically means "unauthenticated",[33] i.e. the user does not have the necessary credentials.
    {402,"Payment Required"},//Reserved for future use. The original intention was that this code might be used as part of some form of digital cash or micropayment scheme, as proposed for example by GNU Taler[34], but that has not yet happened, and this code is not usually used. Google Developers API uses this status if a particular developer has exceeded the daily limit on requests.[35]
    {403,"Forbidden"},//The request was valid, but the server is refusing action. The user might not have the necessary permissions for a resource, or may need an account of some sort.
    {404,"Not Found"},//The requested resource could not be found but may be available in the future. Subsequent requests by the client are permissible.[36]
    {405,"Method Not Allowed"},//A request method is not supported for the requested resource; for example, a GET request on a form that requires data to be presented via POST, or a PUT request on a read-only resource.
    {406,"Not Acceptable"},//The requested resource is capable of generating only content not acceptable according to the Accept headers sent in the request.[37] See Content negotiation.
    {407,"Proxy Authentication Required (RFC 7235)"},//The client must first authenticate itself with the proxy.[38]
    {408,"Request Timeout"},//The server timed out waiting for the request. According to HTTP specifications: "The client did not produce a request within the time that the server was prepared to wait. The client MAY repeat the request without modifications at any later time."[39]
    {409,"Conflict"},//Indicates that the request could not be processed because of conflict in the request, such as an edit conflict between multiple simultaneous updates.
    {410,"Gone"},//Indicates that the resource requested is no longer available and will not be available again. This should be used when a resource has been intentionally removed and the resource should be purged. Upon receiving a 410 status code, the client should not request the resource in the future. Clients such as search engines should remove the resource from their indices.[40] Most use cases do not require clients and search engines to purge the resource, and a "404 Not Found" may be used instead.
    {411,"Length Required"},//The request did not specify the length of its content, which is required by the requested resource.[41]
    {412,"Precondition Failed (RFC 7232)"},//The server does not meet one of the preconditions that the requester put on the request.[42]
    {413,"Payload Too Large (RFC 7231)"},//The request is larger than the server is willing or able to process. Previously called "Request Entity Too Large".[43]
    {414,"URI Too Long (RFC 7231)"},//The URI provided was too long for the server to process. Often the result of too much data being encoded as a query-string of a GET request, in which case it should be converted to a POST request.[44] Called "Request-URI Too Long" previously.[45]
    {415,"Unsupported Media Type"},//The request entity has a media type which the server or resource does not support. For example, the client uploads an image as image/svg+xml, but the server requires that images use a different format.
    {416,"Range Not Satisfiable (RFC 7233)"},//The client has asked for a portion of the file (byte serving), but the server cannot supply that portion. For example, if the client asked for a part of the file that lies beyond the end of the file.[46] Called "Requested Range Not Satisfiable" previously.[47]
    {417,"Expectation Failed"},//The server cannot meet the requirements of the Expect request-header field.[48]
    {418,"I'm a teapot (RFC 2324)"},//This code was defined in 1998 as one of the traditional IETF April Fools' jokes, in RFC 2324, Hyper Text Coffee Pot Control Protocol, and is not expected to be implemented by actual HTTP servers. The RFC specifies this code should be returned by teapots requested to brew coffee.[49] This HTTP status is used as an Easter egg in some websites, including Google.com.[50]
    {421,"Misdirected Request (RFC 7540)"},//The request was directed at a server that is not able to produce a response (for example because a connection reuse).[51]
    {422,"Unprocessable Entity (WebDAV; RFC 4918)"},//The request was well-formed but was unable to be followed due to semantic errors.[15]
    {423,"Locked (WebDAV; RFC 4918)"},//The resource that is being accessed is locked.[15]
    {424,"Failed Dependency (WebDAV; RFC 4918)"},//The request failed due to failure of a previous request (e.g., a PROPPATCH).[15]
    {426,"Upgrade Required"},//The client should switch to a different protocol such as TLS/1.0, given in the Upgrade header field.[52]
    {428,"Precondition Required (RFC 6585)"},//The origin server requires the request to be conditional. Intended to prevent the 'lost update' problem, where a client GETs a resource's state, modifies it, and PUTs it back to the server, when meanwhile a third party has modified the state on the server, leading to a conflict."[53]
    {429,"Too Many Requests (RFC 6585)"},//The user has sent too many requests in a given amount of time. Intended for use with rate-limiting schemes.[53]
    {431,"Request Header Fields Too Large (RFC 6585)"},//The server is unwilling to process the request because either an individual header field, or all the header fields collectively, are too large.[53]
    {451,"Unavailable For Legal Reasons (RFC 7725)"},//A server operator has received a legal demand to deny access to a resource or to a set of resources that includes the requested resource.[54] The code 451 was chosen as a reference to the novel Fahrenheit 451.
    {500,"Internal Server Error"},//A generic error message, given when an unexpected condition was encountered and no more specific message is suitable.[57]
    {501,"Not Implemented"},//The server either does not recognize the request method, or it lacks the ability to fulfil the request. Usually this implies future availability (e.g., a new feature of a web-service API).[58]
    {502,"Bad Gateway"},//The server was acting as a gateway or proxy and received an invalid response from the upstream server.[59]
    {503,"Service Unavailable"},//The server is currently unavailable (because it is overloaded or down for maintenance). Generally, this is a temporary state.[60]
    {504,"Gateway Timeout"},//The server was acting as a gateway or proxy and did not receive a timely response from the upstream server.[61]
    {505,"HTTP Version Not Supported"},//The server does not support the HTTP protocol version used in the request.[62]
    {506,"Variant Also Negotiates (RFC 2295)"},//Transparent content negotiation for the request results in a circular reference.[63]
    {507,"Insufficient Storage (WebDAV; RFC 4918)"},//The server is unable to store the representation needed to complete the request.[15]
    {508,"Loop Detected (WebDAV; RFC 5842)"},//The server detected an infinite loop while processing the request (sent in lieu of 208 Already Reported).
    {510,"Not Extended (RFC 2774)"},//Further extensions to the request are required for the server to fulfil it.[64]
    {511,"Network Authentication Required (RFC 6585)"},//The client needs to authenticate to gain network access. Intended for use by intercepting proxies used to control access to the network (e.g., "captive portals" used to require agreement to Terms of Service before granting full Internet access via a Wi-Fi hotspot).[53]
    {103,"Checkpoint"},//Used in the resumable requests proposal to resume aborted PUT or POST requests.[65]
    {103,"Early Hints"},//Used to return some response headers before entire HTTP response.[66][67]
    {420,"Method Failure (Spring Framework)"},//A deprecated response used by the Spring Framework when a method has failed.[68]
    {420,"Enhance Your Calm (Twitter)"},//Returned by version 1 of the Twitter Search and Trends API when the client is being rate limited; versions 1.1 and later use the 429 Too Many Requests response code instead.[69]
    {450,"Blocked by Windows Parental Controls (Microsoft)"},//The Microsoft extension code indicated when Windows Parental Controls are turned on and are blocking access to the requested webpage.[70]
    {498,"Invalid Token (Esri)"},//Returned by ArcGIS for Server. Code 498 indicates an expired or otherwise invalid token.[71]
    {499,"Token Required (Esri)"},//Returned by ArcGIS for Server. Code 499 indicates that a token is required but was not submitted.[71]
    {509,"Bandwidth Limit Exceeded (Apache Web Server/cPanel)"},//The server has exceeded the bandwidth specified by the server administrator; this is often used by shared hosting providers to limit the bandwidth of customers.[72]
    {530,"Site is frozen"},//Used by the Pantheon web platform to indicate a site that has been frozen due to inactivity.[73]
    {598,"(Informal convention) Network read timeout error"},//Used by some HTTP proxies to signal a network read timeout behind the proxy to a client in front of the proxy.[74][75]
    {599,"(Informal convention) Network connect timeout error"},//Used to indicate when the connection to the network times out.[76][citation needed]
    {440,"Login Time-out"},//The client's session has expired and must log in again.[77]
    {449,"Retry With"},//The server cannot honour the request because the user has not provided the required information.[78]
    {451,"Redirect"},//Used in Exchange ActiveSync when either a more efficient server is available or the server cannot access the users' mailbox.[79] The client is expected to re-run the HTTP AutoDiscover operation to find a more appropriate server.[80]
    {444,"No Response"},//Used to indicate that the server has returned no information to the client and closed the connection.
    {495,"SSL Certificate Error"},//An expansion of the 400 Bad Request response code, used when the client has provided an invalid client certificate.
    {496,"SSL Certificate Required"},//An expansion of the 400 Bad Request response code, used when a client certificate is required but not provided.
    {497,"HTTP Request Sent to HTTPS Port"},//An expansion of the 400 Bad Request response code, used when the client has made a HTTP request to a port listening for HTTPS requests.
    {499,"Client Closed Request"},//Used when the client has closed the request before the server could send a response.
    {520,"Unknown Error"},//The 520 error is used as a "catch-all response for when the origin server returns something unexpected", listing connection resets, large headers, and empty or invalid responses as common triggers.
    {521,"Web Server Is Down"},//The origin server has refused the connection from Cloudflare.
    {522,"Connection Timed Out"},//Cloudflare could not negotiate a TCP handshake with the origin server.
    {523,"Origin Is Unreachable"},//Cloudflare could not reach the origin server; for example, if the DNS records for the origin server are incorrect.
    {524,"A Timeout Occurred"},//Cloudflare was able to complete a TCP connection to the origin server, but did not receive a timely HTTP response.
    {525,"SSL Handshake Failed"},//Cloudflare could not negotiate a SSL/TLS handshake with the origin server.
    {526,"Invalid SSL Certificate"},//Cloudflare could not validate the SSL/TLS certificate that the origin server presented.
    {527,"Railgun Error"},//Error 527 indicates that the request timed out or failed after the WAN connection had been established.[84]
  };
  if ((code<100)||(999<code)) code=500;
  response_code=std::to_string(code);
  if (code_map.count(code)) response_msg=code_map.at(code);
}
int Body::beforeRead(){
  if (getServer()) {
    before_request();
    READ_WRITE_1(beforeRequest())
  } else {
    before_response();
    READ_WRITE_1(beforeResponse())
  }
  return(0);
}
int Body::beforeWrite(){
  if (getServer()) {
    before_response();
    READ_WRITE_1(beforeResponse())
  } else {
    request_version=_HTTP_1_1_;
    before_request();
    READ_WRITE_1(beforeRequest())
  }
  if (getServer()) {
    response_content_length=response_body.size();
    set_content_length(response_headers,response_content_length);
  } else {
    request_content_length=request_body.size();
    set_content_length(request_headers,request_content_length);
  }
  return(0);
}
int Body::betweenRead(){
  if (getServer()) {
    get_content_length(request_headers,request_content_length);
    request_body.clear();
  } else {
    get_content_length(response_headers,response_content_length);
    response_body.clear();
  }
  return(0);
}
int Body::betweenWrite(){
  return(0);
}
int Body::bodyRead(){
  return(getServer()?read_body(request_body,request_content_length):read_body(response_body,response_content_length));
}
int Body::bodyWrite(){
  return(getServer()?write_body(response_body,response_content_length):write_body(request_body,request_content_length));
}
int Body::afterRead(){
  static const std::string _keep_alive_("keep-alive");
  static const std::string _close_("close");
  std::string connection;
  if (getServer()) {
    get_single_header(request_headers,_connection_,connection);
    transform_name(connection);
    if (request_version==_HTTP_1_1_){
      response_version=_HTTP_1_1_;
      if (connection==_close_){
        keep_alive=false;
      } else {
        keep_alive=true;
      }
    } else {
      response_version=_HTTP_1_0_;
      if (connection==_keep_alive_){
        keep_alive=true;
      } else {
        keep_alive=false;
      }
    }
    READ_WRITE_1(afterRequest())
    after_request();
  } else {
    get_single_header(response_headers,_connection_,connection);
    transform_name(connection);
    if (response_version==_HTTP_1_1_){
      if (connection==_close_){
        keep_alive=false;
      } else {
        keep_alive=true;
      }
    } else {
      if (connection==_keep_alive_){
        keep_alive=true;
      } else {
        keep_alive=false;
      }
    }
    READ_WRITE_1(afterResponse())
    after_response();
  }
  return(0);
}
int Body::afterWrite(){
  if (getServer()) {
    READ_WRITE_1(afterResponse())
    after_response();
  } else {
    READ_WRITE_1(afterRequest())
    after_request();
  }
  return(0);
}
void Body::before_request(){
}
void Body::after_request(){
}
void Body::before_response(){
}
void Body::after_response(){
  if (keep_alive){
    startRead();
  } else {
    closeStringWrite=true;
  }
}
//============================================
}}}}
//============================================
#ifdef ENABLE_TESTING
#include "server.hpp"
class TestServer : public ict::boost::connection::http::Server{
private:
  int afterRequest(){
    LOGGER_DEBUG<<__LOGGER__<<request_method<<" "<<request_uri<<" "<<request_version<<std::endl;
    setResponseCode(200);
    if (request_method==ict::boost::connection::http::_GET_){
      response_body="Czesc!!!";
      setSingleResponseHeader(ict::boost::connection::http::_content_type_,"text/text");
    } else if (request_method==ict::boost::connection::http::_POST_) {
      response_body=request_body;
      setSingleResponseHeader(ict::boost::connection::http::_content_type_,"text/text");
    } else {
      response_code="405";
      response_msg="Method not allowed";
    }
    startWrite();
    return(0);
  }
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
