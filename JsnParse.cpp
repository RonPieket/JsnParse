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

#include "JsnParse.h"
#include "JsnUTF8.h"
#include "JsnStream.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// *****************************************************************************************************

static bool IsUTF8Whitespace( int codepoint )
{
  return
  ( codepoint >= 0x2000 && codepoint <= 0x200B ) ||
  ( codepoint >= 0x0009 && codepoint <= 0x000D ) ||
  codepoint == 0x0020 ||
  codepoint == 0x00A0 ||
  codepoint == 0x1680 ||
  codepoint == 0x180E ||
  codepoint == 0x2028 ||
  codepoint == 0x2029 ||
  codepoint == 0x202F ||
  codepoint == 0x205F ||
  codepoint == 0x3000 ||
  codepoint == 0xFEFF;
}

static void JsnEatSpace( JsnStreamIn* stream )
{
  while( stream->Peek() != -1 && stream->Peek() <= ' ' )
  {
    stream->Read();
  }
}

static JsnFragment ParseString( JsnStreamIn* stream )
{
  stream->Read(); // Skip leading quote
  const char* begin = stream->data + stream->index;
  int c = stream->Read();
  while( c != '"' )
  {
    if( c == '\\' )
    {
      c = stream->Read(); // Skip escaped character
    }
    c = stream->Read();
  }
  const char* end = stream->data + stream->index - 1;
  return JsnFragment( kJsn_String, begin, end );
}

static double TextAsFloat( const char* text, int length )
{
  char buf[ 100 ];  // Could be 1000000000000000000000000000000000000000000000000000000000000
  if( length < sizeof( buf ) - 1 )
  {
    memcpy( buf, text, length );
    buf[ length ] = '\0';
    return atof( buf );
  }
  return 0;
}

static JsnFragment ParseNumber( JsnStreamIn* stream )
{
  JsnType t = kJsn_Int;

  const char* begin = stream->data + stream->index;
  int c = stream->Read();
  if( c == '-' )
  {
    c = stream->Read();
  }

  while( c >= '0' && c <= '9' )
  {
    c = stream->Read();
  }

  if( c == '.' )
  {
    t = kJsn_Float;
    c = stream->Read();
    while( c >= '0' && c <= '9' )
    {
      c = stream->Read();
    }
  }

  if( c == 'e' || c == 'E' )
  {
    t = kJsn_Float;
    c = stream->Read();
    if( c == '-' || c == '+' )
    {
      c = stream->Read();
    }
    while( c >= '0' && c <= '9' )
    {
      c = stream->Read();
    }
  }
  stream->Unread();
  const char* end = stream->data + stream->index;

  if( t == kJsn_Int )
  {
    double f = TextAsFloat( begin, ( int )( end - begin ) );
    if( f > UINT64_MAX || f < INT64_MIN )
    {
      t = kJsn_Float;
    }
  }

  return JsnFragment( t, begin, end );
}

double JsnFragment::AsFloat() const
{
  return TextAsFloat( m_Text, m_Length );
}

int64_t JsnFragment::AsInt() const
{
  char buf[ 25 ];
  if( m_Length < sizeof( buf ) - 1 )
  {
    memcpy( buf, m_Text, m_Length );
    buf[ m_Length ] = '\0';
    return atoi( buf );
  }
  return 0;
}

JsnFragment JsnFragment::FromFloat( char* buf25, int buf_size, double value )
{
  snprintf( buf25, buf_size, "%.16g", value );
  return JsnFragment( kJsn_Float, buf25 );
}

JsnFragment JsnFragment::FromInt( char* buf25, int buf_size, int64_t value )
{
  snprintf( buf25, buf_size, "%lld", value );
  return JsnFragment( kJsn_Int, buf25 );
}

static void ParseTrue( JsnStreamIn* stream )
{
  if( stream->Read() != 't' ||
      stream->Read() != 'r' ||
      stream->Read() != 'u' ||
      stream->Read() != 'e'
    )
  {
    stream->Error( "Syntax error" );
  }
}

static void ParseFalse( JsnStreamIn* stream )
{
  if( stream->Read() != 'f' ||
      stream->Read() != 'a' ||
      stream->Read() != 'l' ||
      stream->Read() != 's' ||
      stream->Read() != 'e'
    )
  {
    stream->Error( "Syntax error" );
  }
}

static void ParseNull( JsnStreamIn* stream )
{
  if( stream->Read() != 'n' ||
      stream->Read() != 'u' ||
      stream->Read() != 'l' ||
      stream->Read() != 'l'
    )
  {
    stream->Error( "Syntax error" );
  }
}

static void ParseValue( JsnHandler* reader, JsnStreamIn* stream, const JsnFragment& name );

static void ParseObject( JsnHandler* reader, JsnStreamIn* stream )
{
  do
  {
    stream->Read(); // Skip open brace or comma
    JsnEatSpace( stream );
    int c = stream->Peek();
    if( c != '}' )
    {
      JsnFragment name;
      if( c == '"' )
      {
        name = ParseString( stream );
      }
      else
      {
        stream->Error( "String expected" );
      }

      JsnEatSpace( stream );
      int c = stream->Peek();
      if( c == ':' )
      {
        stream->Read(); // Skip colon
        JsnEatSpace( stream );
        ParseValue( reader, stream, name );
      }
      else
      {
        stream->Error( "\":\" expected" );
      }
      JsnEatSpace( stream );
    }
  }
  while( !stream->error && stream->Peek() == ',' );

  if( stream->Read() != '}' )
  {
    stream->Unread();
    stream->Error( "\"}\" expected" );
  }
}

static void ParseArray( JsnHandler* reader, JsnStreamIn* stream )
{
  do
  {
    stream->Read(); // Skip open bracket or comma
    JsnEatSpace( stream );
    if( stream->Peek() != ']' )
    {
      ParseValue( reader, stream, JsnFragment() );
      JsnEatSpace( stream );
    }
  }
  while( !stream->error && stream->Peek() == ',' );

  if( stream->Read() != ']' )
  {
    stream->Unread();
    stream->Error( "\"]\" expected" );
  }
}

static void ParseValue( JsnHandler* reader, JsnStreamIn* stream, const JsnFragment& name )
{
  switch( stream->Peek() )
  {
    case 't':
      ParseTrue( stream );
      reader->AddProperty( name, JsnFragment( kJsn_True ) );
      break;

    case 'f':
      ParseFalse( stream );
      reader->AddProperty( name, JsnFragment( kJsn_False ) );
      break;

    case 'n':
      ParseNull( stream );
      reader->AddProperty( name, JsnFragment( kJsn_Null ) );
      break;

    case '"':
    {
      JsnFragment value = ParseString( stream );
      reader->AddProperty( name, value );
      break;
    }

    case '-':
    case '.':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
      JsnFragment value = ParseNumber( stream );
      reader->AddProperty( name, value );
      break;
    }

    case '[':
    {
      JsnHandler* child_reader = reader->BeginArray( name );
      ParseArray( child_reader, stream );
      reader->EndArray( child_reader );
      break;
    }

    case '{':
    {
      JsnHandler* child_reader = reader->BeginObject( name );
      ParseObject( child_reader, stream );
      reader->EndObject( child_reader );
      break;
    }

    default:
      stream->Error( "Unexpected character" );
      break;
  }
}

bool JsnParse( JsnHandler* reader, JsnStreamIn* stream )
{
  ParseValue( reader, stream, JsnFragment() );
  return !stream->error;
}

static void WriteStringChar( JsnStreamOut* write_stream, JsnStreamIn* read_stream, bool escape )
{
  int codepoint1 = JsnReadUnescapedUTF8Char( read_stream );

  if( read_stream->error )
  {
    return;
  }

  if( codepoint1 == '\\' )
  {
    int codepoint2 = JsnReadUnescapedUTF8Char( read_stream );
    switch( codepoint2 )
    {
      case '"':
      case '\\':
      case '/':
      case 'b':
      case 'f':
      case 'n':
      case 'r':
      case 't':
      case 'u':
        // Recognized backslash combo. Write complete combo not just backslash
        write_stream->Write( codepoint1 );
        write_stream->Write( codepoint2 );
        break;

      default:
        // Must escape backslash followed by anything else. It's not a combo
        write_stream->Write( '\\' );
        write_stream->Write( '\\' );
        read_stream->Unread();  // Didn't consume second code
        break;
    }
  }
  else if( codepoint1 == '"' )
  {
    write_stream->Write( '\\' );
    write_stream->Write( '"' );
  }
  else if( codepoint1 < 0x20 )
  {
    switch( codepoint1 )
    {
      case '\b':
        write_stream->WriteStr( "\\b" );
        break;
      case '\f':
        write_stream->WriteStr( "\\f" );
        break;
      case '\n':
        write_stream->WriteStr( "\\n" );
        break;
      case '\r':
        write_stream->WriteStr( "\\r" );
        break;
      case '\t':
        write_stream->WriteStr( "\\t" );
        break;
      default:
        JsnWriteEscapedUTF8Char( write_stream, codepoint1 );
        break;
    }
  }
  else if( codepoint1 >= 0x80 && escape )
  {
    JsnWriteEscapedUTF8Char( write_stream, codepoint1 );
  }
  else
  {
    JsnWriteUnescapedUTF8Char( write_stream, codepoint1 );
  }
}

void JsnWriter::WriteFragment( const JsnFragment& fragment )
{
  JsnStreamIn read_stream( fragment.m_Text, fragment.m_Length );
  while( !m_Stream->error && read_stream.Peek() != -1 )
  {
    m_Stream->Write( read_stream.Read() );
  }
}

void JsnWriter::WriteFragmentString( const JsnFragment& fragment )
{
  JsnStreamIn read_stream( fragment.m_Text, fragment.m_Length );
  m_Stream->Write( '"' );
  while( !read_stream.error && !m_Stream->error && read_stream.Peek() )
  {
    WriteStringChar( m_Stream, &read_stream, m_Style->m_EscapeUTF8 );
  }
  m_Stream->Write( '"' );
}

void JsnWriter::WriteIndent()
{
  for( int i = 0; i < m_IndentLevel; ++i )
  {
    WriteFragment( m_Style->m_IndentLevel );
  }
}

void JsnWriter::WriteProperty( const JsnFragment& name, const JsnFragment& value )
{
  if( m_IndentLevel )
  {
    if( m_ValueCount )
    {
      WriteFragment( "," );
    }
    WriteFragment( m_Style->m_NewlineString );
    WriteIndent();
  }
  if( name.m_Length )
  {
    WriteFragmentString( name );
    WriteFragment( ":" );
    WriteFragment( m_Style->m_SpaceAfterColonString );
  }
  if( value.m_Type == kJsn_String )
  {
    WriteFragmentString( value );
  }
  else
  {
    WriteFragment( value );
  }
  m_ValueCount += 1;
}

void JsnWriter::AddProperty( const JsnFragment& name, const JsnFragment& value )
{
  switch( value.m_Type )
  {
    case kJsn_Int:
    case kJsn_Float:
    case kJsn_String:
      WriteProperty( name, value );
      break;
    case kJsn_True:
      WriteProperty( name, "true" );
      break;
    case kJsn_False:
      WriteProperty( name, "false" );
      break;
    case kJsn_Null:
      WriteProperty( name, "null" );
      break;

    default:
      break;
  }
}

JsnHandler* JsnWriter::BeginObject( const JsnFragment& name )
{
  WriteProperty( name, "{" );
  return new JsnWriter( *this );
}

void JsnWriter::EndObject( JsnHandler* byoc )
{
  WriteFragment( m_Style->m_NewlineString );
  WriteIndent();
  WriteFragment( "}" );
  if( !m_IndentLevel )
  {
    WriteFragment( m_Style->m_NewlineString );
  }
  delete byoc;
}

JsnHandler* JsnWriter::BeginArray( const JsnFragment& name )
{
  WriteProperty( name, "[" );
  return new JsnWriter( *this );
}

void JsnWriter::EndArray( JsnHandler* byoc )
{
  WriteFragment( m_Style->m_NewlineString );
  WriteIndent();
  WriteFragment( "]" );
  delete byoc;
}

JsnWriter::Style::Style()
: m_IndentLevel( "  " )
, m_NewlineString( "\n" )
, m_SpaceAfterColonString( " " )
, m_EscapeUTF8( true )
{}

static JsnWriter::Style g_DefaultStyle;


JsnWriter::JsnWriter( JsnStreamOut* stream, const Style* style )
: m_Stream( stream )
, m_Style( style ? style : &g_DefaultStyle )
, m_IndentLevel( 0 )
, m_ValueCount( 0 )
{}

JsnWriter::JsnWriter( const JsnWriter& other )
: m_Stream( other.m_Stream )
, m_Style( other.m_Style )
, m_IndentLevel( other.m_IndentLevel + 1 )
, m_ValueCount( 0 )
{}
