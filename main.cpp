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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ByteStream.h"
#include "JsonParse.h"

// -----------------------------------------------------------------------------------------------------

static const char* NewCString( const JsonFragment& fragment )
{
  size_t len = fragment.m_Length;
  char* s = NULL;
  if( len )
  {
    s = new char[ len + 1 ];
    memcpy( s, fragment.m_Text, len );
    s[ len ] = '\0';
  }
  return s;
}

// -----------------------------------------------------------------------------------------------------

class JsonetteExample final : public JsonHandler
{
  struct Value
  {
    Value( const JsonFragment& name, const JsonFragment& value )
    {
      m_Next = NULL;
      m_Child = NULL;
      m_Type = value.m_Type;
      m_Name = NewCString( name );
      switch( m_Type )
      {
        case kJson_Int:
          m_Value.i = value.AsInt();
          break;
        case kJson_Float:
          m_Value.f = value.AsFloat();
          break;
        default:
          m_Value.s = NewCString( value );  // Add unescaping here
          break;
      }
    }

    ~Value()
    {
      delete[] m_Name;
      if( m_Type != kJson_Int && m_Type != kJson_Float )
      {
        delete[] m_Value.s;
      }
    }

    const char* m_Name;
    union
    {
      const char* s;
      double      f;
      int64_t     i;
    }           m_Value;
    Value*      m_Next;
    Value*      m_Child;
    JsonType        m_Type;

    void Write( JsonHandler* writer ) const;
  };

  Value* m_Value;
  Value* m_LastChild;

public:

  JsonetteExample( Value* parent_value = NULL )
  {
    m_Value = parent_value;
    m_LastChild = NULL;
  }

  Value* GetValue()
  {
    return m_LastChild;
  }

  // Implementation of interface:
  virtual void AddProperty( const JsonFragment& name, const JsonFragment& value ) override
  {
    Add( name, value );
  }

  virtual JsonHandler* BeginObject( const JsonFragment& name ) override
  {
    Add( name, JsonFragment( kJson_Object ) );
    return new JsonetteExample( m_LastChild );
  }

  virtual void EndObject( JsonHandler* byoc ) override
  {
    delete byoc;
  }

  virtual JsonHandler* BeginArray( const JsonFragment& name ) override
  {
    Add( name, JsonFragment( kJson_Array ) );
    return new JsonetteExample( m_LastChild );
  }

  virtual void EndArray( JsonHandler* byoc ) override
  {
    delete byoc;
  }

private:

  void Add( const JsonFragment& name, const JsonFragment& value )
  {
    Add( new JsonetteExample::Value( name, value ) );
  }

  void Add( JsonetteExample::Value* value )
  {
    if( m_LastChild )
    {
      m_LastChild->m_Next = value;
    }
    else if( m_Value )
    {
      m_Value->m_Child = value;
    }
    m_LastChild = value;
  }
};

// -----------------------------------------------------------------------------------------------------

void JsonetteExample::Value::Write( JsonHandler* writer ) const
{
  switch( m_Type )
  {
    case kJson_Null:
    case kJson_True:
    case kJson_False:
      writer->AddProperty( m_Name, m_Type );
      break;

    case kJson_String:
      writer->AddProperty( m_Name, JsonFragment( kJson_String, m_Value.s ) );
      break;

    case kJson_Int:
    {
      char buf[ 25 ];
      writer->AddProperty( m_Name, JsonFragment().FromInt( buf, m_Value.i ) );
      break;
    }

    case kJson_Float:
    {
      char buf[ 25 ];
      writer->AddProperty( m_Name, JsonFragment().FromFloat( buf, m_Value.f ) );
      break;
    }

    case kJson_Object:
    {
      JsonHandler* child_writer = writer->BeginObject( m_Name );
      for( JsonetteExample::Value* child = m_Child; child; child = child->m_Next )
      {
        child->Write( child_writer );
      }
      writer->EndObject( child_writer );
      break;
    }

    case kJson_Array:
    {
      JsonHandler* child_writer = writer->BeginArray( m_Name );
      for( JsonetteExample::Value* child = m_Child; child; child = child->m_Next )
      {
        child->Write( child_writer );
      }
      writer->EndArray( child_writer );
      break;
    }

    default:
      break;
  }
}

// -----------------------------------------------------------------------------------------------------

char json_text[] =
  "{" \
  " \"ctrl_chars\": \" \\\" \\\\ \\/ \\b \\f \\n \\r \\t \"," \
  " \"escaped_ctrl_chars\": \" \\\\/ \\\\b \\\\f \\\\n \\\\r \\\\t \"," \
  " \"bogus_ctrl_chars\": \" \\x \\y \\z \"," \
  " \"escaped_bogus_ctrl_chars\": \" \\\\x \\\\y \\\\z \"," \
  " \"unicode_escaped\": \"Copyright:\\u00A9 Notes:\\u266B Clef:\\uD834\\uDD1E\"," \
  " \"unicode_unescaped\": \"Copyright:© Notes:♫ Clef:𝄞\"," \
  " \"string\": \"hello\"," \
  " \"int\": 100," \
  " \"float\": 3.141592," \
  " \"int_too_large\": 1000000000000000000000000000000," \
  " \"bool\": true," \
  " \"object\": { \"first\": \"Jane\", \"last\": \"Austen\" }," \
  " \"empty_object\": {}," \
  " \"array_of_string\": [ \"first\", \"second\" ]," \
  " \"array_of_number\": [ 100, 200, 300 ]," \
  " \"array_of_bool\": [ false, true ]," \
  " \"array_of_object\": [ { \"first\": \"Jane\", \"last\": \"Austen\" }, { \"first\": \"Geoffrey\", \"last\": \"Chaucer\" } ]," \
  " \"array_of_array\": [ [ 1, 2 ], [ 3, 4 ] ]," \
  " \"empty_array\": []" \
  "}";

// -----------------------------------------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  printf( "%s\n", json_text );

  JsonetteExample example_reader;

  printf( "\n\n--------- read into own format, then write\n\n" );
  ByteStreamIn read_stream( json_text );
  if( !JsonParse( &example_reader, &read_stream ) )
  {
    printf( "ERROR: %s\n", read_stream.error );
    char buf[ 50 ];
    size_t len = sizeof( buf );
    strncpy( buf, read_stream.data + read_stream.index, len );
    buf[ len - 1 ] = 0;
    printf( "%s\n", buf );
  }
  else
  {
    ByteStreamOut write_stream;
    JsonWriter::Style style;
    style.m_EscapeUTF8 = false;
    style.m_IndentLevelString = "";
    style.m_NewlineString = "";
    style.m_SpaceAfterColonString = "";

    JsonWriter writer( &write_stream, &style );
    example_reader.GetValue()->Write( &writer );

    size_t count = write_stream.index;
    printf( "COUNT %d\n", ( int )count );
    char* buffer = new char[ count + 1 ]; // +1 for zero termination
    buffer[ count ] = '\0';
    write_stream = ByteStreamOut( buffer, count );
    example_reader.GetValue()->Write( &writer );
    printf( "%s\n", buffer );
    delete[] buffer;
  }
/*
  printf( "\n\n--------- read directly into writer (for fun, because we can)\n\n" );
  writer.SetModePrint();
  read_stream.Reset();
  JsonParse( &writer, &read_stream );
 */

  return 0;
}

// -----------------------------------------------------------------------------------------------------
