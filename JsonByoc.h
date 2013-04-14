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

#include <string.h>
#include <stdint.h>

/*
 \class JsonByoc
 Abstract interface class for reading JSON text.

 */
class JsonByoc
{
public:

  enum Type { kUndefined, kNull, kFalse, kTrue, kInt, kFloat, kString, kObject, kArray };

  struct Fragment
  {
    Fragment( Type t, const char* text, size_t length )
    : m_Text( text )
    , m_Length( length )
    , m_Type( t )
    {}
    Fragment( Type t, const char* text )
    : m_Text( text )
    , m_Length( text ? strlen( text ) : 0 )
    , m_Type( t )
    {}
    Fragment( const char* text )
    : m_Text( text )
    , m_Length( text ? strlen( text ) : 0 )
    , m_Type( kUndefined )
    {}
    Fragment( const Fragment& other )
    : m_Text( other.m_Text )
    , m_Length( other.m_Length )
    , m_Type( other.m_Type )
    {}
    Fragment( Type t = kUndefined )
    : m_Text( NULL )
    , m_Length( 0 )
    , m_Type( t )
    {}
    double AsFloat() const;
    int64_t AsInt() const;
    Fragment FromFloat( char* buf25, double value );
    Fragment FromInt( char* buf25, int64_t value );

    const char*   m_Text;
    size_t  m_Length;
    Type    m_Type;
  };

  virtual void AddProperty( const Fragment& name, const Fragment& value ) = 0;

  virtual JsonByoc* BeginObject( const Fragment& name ) = 0;
  virtual void EndObject( JsonByoc* byoc ) = 0;
  virtual JsonByoc* BeginArray( const Fragment& name ) = 0;
  virtual void EndArray( JsonByoc* byoc ) = 0;
  virtual void Error( const char* message, const char* p ) = 0;

  virtual ~JsonByoc() {}
};

class JsonWriter final : public JsonByoc
{
public:

  enum Mode { kCount, kWrite, kPrint };

  JsonWriter( const char* indent_str = "  ", const char* newline_str = "\n" );
  void SetMode( Mode mode, char* buffer = NULL );
  size_t GetCount() const { return m_SharedStorage.m_Index; }

  virtual void AddProperty( const Fragment& name, const Fragment& value ) override;
  virtual JsonByoc* BeginObject( const Fragment& name ) override;
  virtual void EndObject( JsonByoc* byoc ) override;
  virtual JsonByoc* BeginArray( const Fragment& name ) override;
  virtual void EndArray( JsonByoc* byoc ) override;
  virtual void Error( const char* message, const char* p ) override {}

private:

  struct Shared
  {
    char*           m_Buffer;
    size_t          m_Index;
    Mode            m_Mode;
    const Fragment  m_IndentFragment;
    const Fragment  m_NewLineFragment;
    const Fragment  m_SpaceFragment;
  };

  Shared          m_SharedStorage;
  Shared*         m_Shared;
  const int       m_Indent;
  int             m_ValueCount;

  JsonWriter( const JsonWriter& other );
  void WriteFragment( const Fragment& fragment );
  void WriteIndent();
  void WriteValue( const Fragment& name, const Fragment& value );
};

bool JsonParse( JsonByoc* reader, const char* json_text );
