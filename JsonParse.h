/*
 Copyright (c) 2013, Insomniac Games

 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 following conditions are met:
 - Redistributions of source code must retain the above copyright notice, this list of conditions and the
 following disclaimer.
 - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 \file
 \author Ron Pieket \n<http://www.ItShouldJustWorkTM.com> \n<http://twitter.com/RonPieket>
 */
#pragma once

#include "ByteStream.h"

#include <string.h>
#include <stdint.h>

// *****************************************************************************************************
// Section: JsonHandler

enum JsonType { kJson_Undefined, kJson_Null, kJson_False, kJson_True, kJson_Int, kJson_Float, kJson_String, kJson_Object, kJson_Array };

struct JsonFragment
{
  const char* m_Text;
  size_t      m_Length;
  JsonType    m_Type;

  JsonFragment( JsonType t, const char* text, size_t length )
  : m_Text( text )
  , m_Length( length )
  , m_Type( t )
  {}
  JsonFragment( JsonType t, const char* text )
  : m_Text( text )
  , m_Length( text ? strlen( text ) : 0 )
  , m_Type( t )
  {}
  JsonFragment( const char* text )
  : m_Text( text )
  , m_Length( text ? strlen( text ) : 0 )
  , m_Type( kJson_Undefined )
  {}
  JsonFragment( const JsonFragment& other )
  : m_Text( other.m_Text )
  , m_Length( other.m_Length )
  , m_Type( other.m_Type )
  {}
  JsonFragment( JsonType t = kJson_Undefined )
  : m_Text( NULL )
  , m_Length( 0 )
  , m_Type( t )
  {}
  double AsFloat() const;
  int64_t AsInt() const;
  JsonFragment FromFloat( char* buf25, double value );
  JsonFragment FromInt( char* buf25, int64_t value );
  JsonFragment& operator=( const char* text )
  {
    m_Text    = text;
    m_Length  = text ? strlen( text ) : 0;
    return *this;
  }
  JsonFragment& operator= ( const JsonFragment& rhs )
  {
    m_Text    = rhs.m_Text;
    m_Length  = rhs.m_Length;
    m_Type    = rhs.m_Type;
    return *this;
  }
};

// -----------------------------------------------------------------------------------------------------

class JsonHandler
{
public:

  virtual void AddProperty( const JsonFragment& name, const JsonFragment& value ) = 0;
  virtual JsonHandler* BeginObject( const JsonFragment& name ) = 0;
  virtual void EndObject( JsonHandler* byoc ) = 0;
  virtual JsonHandler* BeginArray( const JsonFragment& name ) = 0;
  virtual void EndArray( JsonHandler* byoc ) = 0;

  virtual ~JsonHandler() {}
};

bool JsonParse( JsonHandler* reader, ByteStreamIn* stream );

// -----------------------------------------------------------------------------------------------------

class JsonWriter final : public JsonHandler
{
public:

  struct Style
  {
    JsonFragment  m_IndentLevelString;
    JsonFragment  m_NewlineString;
    JsonFragment  m_SpaceAfterColonString;
    bool          m_EscapeUTF8;

    Style();
  };

  JsonWriter( ByteStreamOut* stream, const Style* style );

  virtual void AddProperty( const JsonFragment& name, const JsonFragment& value ) override;
  virtual JsonHandler* BeginObject( const JsonFragment& name ) override;
  virtual void EndObject( JsonHandler* byoc ) override;
  virtual JsonHandler* BeginArray( const JsonFragment& name ) override;
  virtual void EndArray( JsonHandler* byoc ) override;

private:

  ByteStreamOut*    m_Stream;
  const Style*    m_Style;
  const int       m_IndentLevel;
  int             m_ValueCount;

  JsonWriter( const JsonWriter& other );
  void WriteFragment( const JsonFragment& fragment );
  void WriteFragmentString( const JsonFragment& fragment );
  void WriteIndent();
  void WriteProperty( const JsonFragment& name, const JsonFragment& value );
};

