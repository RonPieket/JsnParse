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

#include "JsonParse.h"
#include "EscapeUTF8.h"
#include "ByteStream.h"

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

static void JsonEatSpace( ByteStreamIn* stream )
{
  while( stream->Peek() != -1 && stream->Peek() <= ' ' )
  {
    stream->Read();
  }
}

static JsonFragment ParseString( ByteStreamIn* stream )
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
  return JsonFragment( kJson_String, begin, end - begin );
}

static double TextAsFloat( const char* text, size_t length )
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

static JsonFragment ParseNumber( ByteStreamIn* stream )
{
  JsonType t = kJson_Int;

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
    t = kJson_Float;
    c = stream->Read();
    while( c >= '0' && c <= '9' )
    {
      c = stream->Read();
    }
  }

  if( c == 'e' || c == 'E' )
  {
    t = kJson_Float;
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

  if( t == kJson_Int )
  {
    double f = TextAsFloat( begin, end - begin );
    if( f > UINT64_MAX || f < INT64_MIN )
    {
      t = kJson_Float;
    }
  }

  return JsonFragment( t, begin, end - begin );
}

double JsonFragment::AsFloat() const
{
  return TextAsFloat( m_Text, m_Length );
}

int64_t JsonFragment::AsInt() const
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

JsonFragment JsonFragment::FromFloat( char* buf25, double value )
{
  snprintf( buf25, 25, "%.16g", value );
  return JsonFragment( kJson_Float, buf25 );
}

JsonFragment JsonFragment::FromInt( char* buf25, int64_t value )
{
  snprintf( buf25, 25, "%lld", value );
  return JsonFragment( kJson_Int, buf25 );
}

static void ParseTrue( ByteStreamIn* stream )
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

static void ParseFalse( ByteStreamIn* stream )
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

static void ParseNull( ByteStreamIn* stream )
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

static void ParseValue( JsonHandler* reader, ByteStreamIn* stream, const JsonFragment& name );

static void ParseObject( JsonHandler* reader, ByteStreamIn* stream )
{
  do
  {
    stream->Read(); // Skip open brace or comma
    JsonEatSpace( stream );
    int c = stream->Peek();
    if( c != '}' )
    {
      JsonFragment name;
      if( c == '"' )
      {
        name = ParseString( stream );
      }
      else
      {
        stream->Error( "String expected" );
      }

      JsonEatSpace( stream );
      int c = stream->Peek();
      if( c == ':' )
      {
        stream->Read(); // Skip colon
        JsonEatSpace( stream );
        ParseValue( reader, stream, name );
      }
      else
      {
        stream->Error( "\":\" expected" );
      }
      JsonEatSpace( stream );
    }
  }
  while( !stream->error && stream->Peek() == ',' );

  if( stream->Read() != '}' )
  {
    stream->Unread();
    stream->Error( "\"}\" expected" );
  }
}

static void ParseArray( JsonHandler* reader, ByteStreamIn* stream )
{
  do
  {
    stream->Read(); // Skip open bracket or comma
    JsonEatSpace( stream );
    if( stream->Peek() != ']' )
    {
      ParseValue( reader, stream, JsonFragment() );
      JsonEatSpace( stream );
    }
  }
  while( !stream->error && stream->Peek() == ',' );

  if( stream->Read() != ']' )
  {
    stream->Unread();
    stream->Error( "\"]\" expected" );
  }
}

static void ParseValue( JsonHandler* reader, ByteStreamIn* stream, const JsonFragment& name )
{
  switch( stream->Peek() )
  {
    case 't':
      ParseTrue( stream );
      reader->AddProperty( name, JsonFragment( kJson_True ) );
      break;

    case 'f':
      ParseFalse( stream );
      reader->AddProperty( name, JsonFragment( kJson_False ) );
      break;

    case 'n':
      ParseNull( stream );
      reader->AddProperty( name, JsonFragment( kJson_Null ) );
      break;

    case '"':
    {
      JsonFragment value = ParseString( stream );
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
      JsonFragment value = ParseNumber( stream );
      reader->AddProperty( name, value );
      break;
    }

    case '[':
    {
      JsonHandler* child_reader = reader->BeginArray( name );
      ParseArray( child_reader, stream );
      reader->EndArray( child_reader );
      break;
    }

    case '{':
    {
      JsonHandler* child_reader = reader->BeginObject( name );
      ParseObject( child_reader, stream );
      reader->EndObject( child_reader );
      break;
    }

    default:
      stream->Error( "Unexpected character" );
      break;
  }
}

bool JsonParse( JsonHandler* reader, ByteStreamIn* stream )
{
  ParseValue( reader, stream, JsonFragment() );
  return !stream->error;
}

static void WriteStringChar( ByteStreamOut* write_stream, ByteStreamIn* read_stream, bool escape )
{
  int codepoint1 = ReadUnescapedUTF8Char( read_stream );

  if( read_stream->error )
  {
    return;
  }

  if( codepoint1 == '\\' )
  {
    int codepoint2 = ReadUnescapedUTF8Char( read_stream );
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
        WriteEscapedUTF8Char( write_stream, codepoint1 );
        break;
    }
  }
  else if( codepoint1 >= 0x80 && escape )
  {
    WriteEscapedUTF8Char( write_stream, codepoint1 );
  }
  else
  {
    WriteUnescapedUTF8Char( write_stream, codepoint1 );
  }
}

void JsonWriter::WriteFragment( const JsonFragment& fragment )
{
  ByteStreamIn read_stream( fragment.m_Text, fragment.m_Length );
  while( !m_Stream->error && read_stream.Peek() != -1 )
  {
    m_Stream->Write( read_stream.Read() );
  }
}

void JsonWriter::WriteFragmentString( const JsonFragment& fragment )
{
  ByteStreamIn read_stream( fragment.m_Text, fragment.m_Length );
  m_Stream->Write( '"' );
  while( !read_stream.error && !m_Stream->error && read_stream.Peek() )
  {
    WriteStringChar( m_Stream, &read_stream, m_Style->m_EscapeUTF8 );
  }
  m_Stream->Write( '"' );
}

void JsonWriter::WriteIndent()
{
  for( int i = 0; i < m_IndentLevel; ++i )
  {
    WriteFragment( m_Style->m_IndentLevelString );
  }
}

void JsonWriter::WriteProperty( const JsonFragment& name, const JsonFragment& value )
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
  if( value.m_Type == kJson_String )
  {
    WriteFragmentString( value );
  }
  else
  {
    WriteFragment( value );
  }
  m_ValueCount += 1;
}

void JsonWriter::AddProperty( const JsonFragment& name, const JsonFragment& value )
{
  switch( value.m_Type )
  {
    case kJson_Int:
    case kJson_Float:
    case kJson_String:
      WriteProperty( name, value );
      break;
    case kJson_True:
      WriteProperty( name, "true" );
      break;
    case kJson_False:
      WriteProperty( name, "false" );
      break;
    case kJson_Null:
      WriteProperty( name, "null" );
      break;

    default:
      break;
  }
}

JsonHandler* JsonWriter::BeginObject( const JsonFragment& name )
{
  WriteProperty( name, "{" );
  return new JsonWriter( *this );
}

void JsonWriter::EndObject( JsonHandler* byoc )
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

JsonHandler* JsonWriter::BeginArray( const JsonFragment& name )
{
  WriteProperty( name, "[" );
  return new JsonWriter( *this );
}

void JsonWriter::EndArray( JsonHandler* byoc )
{
  WriteFragment( m_Style->m_NewlineString );
  WriteIndent();
  WriteFragment( "]" );
  delete byoc;
}

JsonWriter::Style::Style()
: m_IndentLevelString( "  " )
, m_NewlineString( "\n" )
, m_SpaceAfterColonString( " " )
, m_EscapeUTF8( true )
{}

static JsonWriter::Style g_DefaultStyle;


JsonWriter::JsonWriter( ByteStreamOut* stream, const Style* style )
: m_Stream( stream )
, m_Style( style ? style : &g_DefaultStyle )
, m_IndentLevel( 0 )
, m_ValueCount( 0 )
{}

JsonWriter::JsonWriter( const JsonWriter& other )
: m_Stream( other.m_Stream )
, m_Style( other.m_Style )
, m_IndentLevel( other.m_IndentLevel + 1 )
, m_ValueCount( 0 )
{}
