//
//  JamParser.cpp
//  JamJson
//
//  Created by Ronald Pieket on 4/11/13.
//  Copyright (c) 2013 It Should Just Work!™. All rights reserved.
//

#include "JsonByoc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char* JsonEatSpace( const char* p )
{
  if( p )
  {
    while( *p && *p <= ' ' )
    {
      p += 1;
    }
  }
  return p;
}

static const char* ParseString( JsonByoc::Fragment* fragment, const char* p )
{
  p += 1;
  const char* start = p;
  while( *p != '"' )
  {
    p += 1;
    if( *p == '\\' )
    {
      if( *p )
      {
        p += 1;
      }
    }
  }
  *fragment = JsonByoc::Fragment( JsonByoc::kString, start, p - start );
  return *p == '"' ? p + 1 : p;
}

static double TextAsFloat( const char* text, size_t length )
{
  char buf[ 100 ];  // Could be 1000000000000000000000000000000
  if( length < sizeof( buf ) - 1 )
  {
    memcpy( buf, text, length );
    buf[ length ] = '\0';
    return atof( buf );
  }
  return 0;
}

static const char* ParseNumber( JsonByoc::Fragment* fragment, const char* p )
{
  JsonByoc::Type t = JsonByoc::kInt;

  const char* start = p;
  if( *p == '-' )
  {
    p += 1;
  }

  if( *p >= '0' && *p <= '9' )
  {
    p += 1;
    while( *p >= '0' && *p <= '9' )
    {
      p += 1;
    }
  }

  if( p && *p == '.' )
  {
    t = JsonByoc::kFloat;
    p += 1;
    while( *p >= '0' && *p <= '9' )
    {
      p += 1;
    }
  }

  if( p && ( *p == 'e' || *p == 'E' ) )
  {
    t = JsonByoc::kFloat;
    p += 1;
    if( *p == '-' || *p == '+' )
    {
      p += 1;
    }
    while( *p >= '0' && *p <= '9' )
    {
      p += 1;
    }
  }

  if( t == JsonByoc::kInt )
  {
    double f = TextAsFloat( start, p - start );
    if( f > UINT64_MAX || f < INT64_MIN )
    {
      t = JsonByoc::kFloat;
    }
  }

  *fragment = JsonByoc::Fragment( t, start, p - start );

  return p;
}

double JsonByoc::Fragment::AsFloat() const
{
  return TextAsFloat( m_Text, m_Length );
}

int64_t JsonByoc::Fragment::AsInt() const
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

JsonByoc::Fragment JsonByoc::Fragment::FromFloat( char* buf25, double value )
{
  snprintf( buf25, 25, "%.16g", value );
  return Fragment( kFloat, buf25 );
}

JsonByoc::Fragment JsonByoc::Fragment::FromInt( char* buf25, int64_t value )
{
  snprintf( buf25, 25, "%lld", value );
  return Fragment( kInt, buf25 );
}

static const char* ParseTrue( const char* p )
{
  if( p[ 0 ] == 't' &&
      p[ 1 ] == 'r' &&
      p[ 2 ] == 'u' &&
      p[ 3 ] == 'e'
    )
  {
    p += 4;
  }
  return p;
}

static const char* ParseFalse( const char* p )
{
  if( p[ 0 ] == 'f' &&
      p[ 1 ] == 'a' &&
      p[ 2 ] == 'l' &&
      p[ 3 ] == 's' &&
      p[ 4 ] == 'e'
    )
  {
    p += 5;
  }
  return p;
}

static const char* ParseNull( const char* p )
{
  if( p[ 0 ] == 'n' &&
      p[ 1 ] == 'u' &&
      p[ 2 ] == 'l' &&
      p[ 3 ] == 'l'
    )
  {
    p += 4;
  }
  return p;
}

static const char* ParseValue( JsonByoc* reader, const char* p, const JsonByoc::Fragment& name );

static const char* ParseObject( JsonByoc* reader, const char* p )
{
  do
  {
    p = JsonEatSpace( p + 1 );
    if( *p != '}' )
    {
      JsonByoc::Fragment name;
      if( *p == '"' )
      {
        p = ParseString( &name, p );
      }
      else
      {
        reader->Error( "string expected", p );
        p = NULL;
        break;
      }

      p = JsonEatSpace( p );
      if( *p == ':' )
      {
        p = JsonEatSpace( p + 1 );
        p = ParseValue( reader, p, name );
      }
      else
      {
        reader->Error( "\":\" expected", p );
        break;
      }
      p = JsonEatSpace( p );
    }
  }
  while( p && *p == ',' );

  if( p && *p != '}' )
  {
    reader->Error( "\"}\" expected", p );
    p = NULL;
  }

  return p ? p + 1 : p;
}

static const char* ParseArray( JsonByoc* reader, const char* p )
{
  do
  {
    p = JsonEatSpace( p + 1 );
    if( *p != ']' )
    {
      p = ParseValue( reader, p, JsonByoc::Fragment() );
      p = JsonEatSpace( p );
    }
  }
  while( p && *p == ',' );

  if( p && *p != ']' )
  {
    reader->Error( "\"]\" expected", p );
    p = NULL;
  }

  return p ? p + 1 : p;
}

static const char* ParseValue( JsonByoc* reader, const char* p, const JsonByoc::Fragment& name )
{
  switch( *p )
  {
    case 't':
      p = ParseTrue( p );
      reader->AddProperty( name, JsonByoc::Fragment( JsonByoc::kTrue ) );
      break;

    case 'f':
      p = ParseFalse( p );
      reader->AddProperty( name, JsonByoc::Fragment( JsonByoc::kFalse ) );
      break;

    case 'n':
      p = ParseNull( p );
      reader->AddProperty( name, JsonByoc::Fragment( JsonByoc::kNull ) );
      break;

    case '"':
    {
      JsonByoc::Fragment value;
      p = ParseString( &value, p );
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
      JsonByoc::Fragment value;
      p = ParseNumber( &value, p );
      reader->AddProperty( name, value );
      break;
    }

    case '[':
    {
      JsonByoc* child_reader = reader->BeginArray( name );
      p = ParseArray( child_reader, p );
      reader->EndArray( child_reader );
      break;
    }

    case '{':
    {
      JsonByoc* child_reader = reader->BeginObject( name );
      p = ParseObject( child_reader, p );
      reader->EndObject( child_reader );
      break;
    }

    default:
      reader->Error( "unexpected character", p );
      p = NULL;
      break;
  }
  
  return p;
}

bool JsonParse( JsonByoc* reader, const char* json_text )
{
  return NULL != ParseValue( reader, json_text, JsonByoc::Fragment() );
}

void JsonWriter::WriteFragment( const Fragment& fragment )
{
  size_t len = fragment.m_Length;
  if( len )
  {
    switch( m_Shared->m_Mode )
    {
      case kPrint:
      {
        char buf[ 1000 ];
        if( len > sizeof( buf ) - 1 )
        {
          len = sizeof( buf ) - 1;
        }
        memcpy( buf, fragment.m_Text, len );
        buf[ len ] = '\0';
        printf( "%s", buf );
        break;
      }
      case kCount:
        m_Shared->m_Index += len;
        break;
      case kWrite:
        memcpy( m_Shared->m_Buffer + m_Shared->m_Index, fragment.m_Text, len );
        m_Shared->m_Index += len;
        break;
    }
  }
}

void JsonWriter::WriteIndent()
{
  for( int i = 0; i < m_Indent; ++i )
  {
    WriteFragment( m_Shared->m_IndentFragment );
  }
}

void JsonWriter::WriteValue( const Fragment& name, const Fragment& value )
{
  if( m_Indent )
  {
    if( m_ValueCount )
    {
      WriteFragment( "," );
    }
    WriteFragment( m_Shared->m_NewLineFragment );
    WriteIndent();
  }
  if( name.m_Length )
  {
    WriteFragment( "\"" );
    WriteFragment( name );
    WriteFragment( "\":" );
    WriteFragment( m_Shared->m_SpaceFragment );
  }
  if( value.m_Type == kString )
  {
    WriteFragment( "\"" );
  }
  WriteFragment( value );
  if( value.m_Type == kString )
  {
    WriteFragment( "\"" );
  }
  m_ValueCount += 1;
}

void JsonWriter::AddProperty( const Fragment& name, const Fragment& value )
{
  switch( value.m_Type )
  {
    case kInt:
    case kFloat:
    case kString:
      WriteValue( name, value );
      break;
    case kTrue:
      WriteValue( name, "true" );
      break;
    case kFalse:
      WriteValue( name, "false" );
      break;
    case kNull:
      WriteValue( name, "null" );
      break;

    default:
      break;
  }
}

JsonByoc* JsonWriter::BeginObject( const Fragment& name )
{
  WriteValue( name, "{" );
  return new JsonWriter( *this );
}

void JsonWriter::EndObject( JsonByoc* byoc )
{
  WriteFragment( m_Shared->m_NewLineFragment );
  WriteIndent();
  WriteFragment( "}" );
  if( !m_Indent )
  {
    WriteFragment( m_Shared->m_NewLineFragment );
  }
  delete byoc;
}

JsonByoc* JsonWriter::BeginArray( const Fragment& name )
{
  WriteValue( name, "[" );
  return new JsonWriter( *this );
}

void JsonWriter::EndArray( JsonByoc* byoc )
{
  WriteFragment( m_Shared->m_NewLineFragment );
  WriteIndent();
  WriteFragment( "]" );
  delete byoc;
}

JsonWriter::JsonWriter( const char* indent_str, const char* newline_str )
: m_SharedStorage
  (
    {
      NULL,         // m_Buffer
      0,            // m_Index
      kPrint,       // m_Mode
      indent_str,
      newline_str,
      indent_str && indent_str[ 0 ] ? " " : ""
    }
  )
, m_Shared( &m_SharedStorage )
, m_Indent( 0 )
, m_ValueCount( 0 )
{}

JsonWriter::JsonWriter( const JsonWriter& other )
: m_Shared( other.m_Shared )
, m_Indent( other.m_Indent + 1 )
, m_ValueCount( 0 )
{}

void JsonWriter::SetMode( Mode mode, char* buffer )
{
  if( !buffer && mode == kWrite )
  {
    mode = kCount;
  }
  m_SharedStorage.m_Buffer = buffer;
  m_SharedStorage.m_Mode = mode;
  m_SharedStorage.m_Index = 0;
}
