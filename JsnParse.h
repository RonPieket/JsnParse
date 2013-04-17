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

#include "JsnStream.h"

#include <string.h>
#include <stdint.h>

// *****************************************************************************************************
// Section: JsnHandler

enum JsnType { kJsn_Undefined, kJsn_Null, kJsn_False, kJsn_True, kJsn_Int, kJsn_Float, kJsn_String, kJsn_Object, kJsn_Array };

struct JsnFragment
{
  const char* m_Text;
  int         m_Length;
  JsnType     m_Type;

  JsnFragment( JsnType t, const char* text, int length )
  : m_Text( text )
  , m_Length( length )
  , m_Type( t )
  {}
  JsnFragment( JsnType t, const char* text, const char* end )
  : m_Text( text )
  , m_Length( ( int )( end - text ) )
  , m_Type( t )
  {}
  JsnFragment( JsnType t, const char* text )
  : m_Text( text )
  , m_Length( text ? ( int )strlen( text ) : 0 )
  , m_Type( t )
  {}
  JsnFragment( const char* text )
  : m_Text( text )
  , m_Length( text ? ( int )strlen( text ) : 0 )
  , m_Type( kJsn_Undefined )
  {}
  JsnFragment( const JsnFragment& other )
  : m_Text( other.m_Text )
  , m_Length( other.m_Length )
  , m_Type( other.m_Type )
  {}
  JsnFragment( JsnType t = kJsn_Undefined )
  : m_Text( NULL )
  , m_Length( 0 )
  , m_Type( t )
  {}
  double AsFloat() const;
  int64_t AsInt() const;
  JsnFragment FromFloat( char* buf25, int buf_size, double value );
  JsnFragment FromInt( char* buf25, int buf_size, int64_t value );
  JsnFragment& operator=( const char* text )
  {
    m_Text    = text;
    m_Length  = text ? ( int )strlen( text ) : 0;
    return *this;
  }
  JsnFragment& operator= ( const JsnFragment& rhs )
  {
    m_Text    = rhs.m_Text;
    m_Length  = rhs.m_Length;
    m_Type    = rhs.m_Type;
    return *this;
  }
};

// -----------------------------------------------------------------------------------------------------

class JsnHandler
{
public:

  virtual void        AddProperty( const JsnFragment& name, const JsnFragment& value ) = 0;
  virtual JsnHandler* BeginObject( const JsnFragment& name ) = 0;
  virtual void        EndObject( JsnHandler* byoc ) = 0;
  virtual JsnHandler* BeginArray( const JsnFragment& name ) = 0;
  virtual void        EndArray( JsnHandler* byoc ) = 0;

  virtual ~JsnHandler() {}
};

// -----------------------------------------------------------------------------------------------------

class JsnWriter final : public JsnHandler
{
public:

  struct Style
  {
    JsnFragment  m_IndentLevel;
    JsnFragment  m_NewlineString;
    JsnFragment  m_SpaceAfterColonString;
    bool         m_EscapeUTF8;

    Style();
  };

  JsnWriter( JsnStreamOut* stream, const Style* style );

  virtual void        AddProperty( const JsnFragment& name, const JsnFragment& value ) override;
  virtual JsnHandler* BeginObject( const JsnFragment& name ) override;
  virtual void        EndObject( JsnHandler* byoc ) override;
  virtual JsnHandler* BeginArray( const JsnFragment& name ) override;
  virtual void        EndArray( JsnHandler* byoc ) override;

private:

  JsnStreamOut*  m_Stream;
  const Style*    m_Style;
  const int       m_IndentLevel;
  int             m_ValueCount;

  JsnWriter( const JsnWriter& other );
  void WriteFragment( const JsnFragment& fragment );
  void WriteFragmentString( const JsnFragment& fragment );
  void WriteIndent();
  void WriteProperty( const JsnFragment& name, const JsnFragment& value );
};

// -----------------------------------------------------------------------------------------------------

bool JsnParse( JsnHandler* reader, JsnStreamIn* stream );

// -----------------------------------------------------------------------------------------------------
