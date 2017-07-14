//! @file
//! @brief Connection module - Source file.
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
#include "connection.hpp"
//============================================
#ifdef ENABLE_TESTING
#include "test.hpp"
#endif
//============================================
namespace ict { namespace boost { namespace connection {
//============================================
Top::Top(){
}
Top::~Top(){
}
std::string Top::socketDesc() const {
  return(sDesc);
}
std::string Top::socketLocal() const {
  return(sLocal);
}
std::string Top::socketRemote() const {
  return(sRemote);
}
//============================================
TopString::~TopString(){
}
void TopString::doRead(){
  if ((readSize+readString.size())<readString.max_size()){
    readString.append((char*)readData,readSize);
    readSize=0;
    asyncRead();
  }
  stringRead();
}
void TopString::doWrite(){
  if (0<writeString.size()){
    writeSize=writeString.copy((char*)writeData,bufferSize);
    writeString.erase(0,writeSize);
    asyncWrite();
  } else {
    stringWrite();
  }
  if (!writeString.size()) if (closeStringWrite) doClose();
}
//============================================
}}}
//============================================
#ifdef ENABLE_TESTING
//REGISTER_TEST(connection,tc1){
//  return(0);
//}
#endif
//===========================================
