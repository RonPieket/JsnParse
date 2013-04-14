//
//  main.cpp
//  JsonParse
//
//  Created by Ronald Pieket on 4/10/13.
//  Copyright (c) 2013 It Should Just Work!™. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>

#include "JsonByoc.h"

static const char* NewCString( const JsonByoc::Fragment& fragment )
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

class JsonByocExample final : public JsonByoc
{
  struct Value
  {
    Value( const JsonByoc::Fragment& name, const JsonByoc::Fragment& value )
    {
      m_Next = NULL;
      m_Child = NULL;
      m_Type = value.m_Type;
      m_Name = NewCString( name );
      switch( m_Type )
      {
        case JsonByoc::kInt:
          m_Value.i = value.AsInt();
          break;
        case JsonByoc::kFloat:
          m_Value.f = value.AsFloat();
          break;
        default:
          m_Value.s = NewCString( value );
          break;
      }
    }

    ~Value()
    {
      delete[] m_Name;
      switch( m_Type )
      {
        case JsonByoc::kInt:
        case JsonByoc::kFloat:
          break;
        default:
          delete[] m_Value.s;
          break;
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
    Type        m_Type;

    void Write( JsonByoc* writer ) const;
  };

  Value* m_Value;
  Value* m_LastChild;

public:

  JsonByocExample( Value* parent_value = NULL )
  {
    m_Value = parent_value;
    m_LastChild = NULL;
  }

  Value* GetValue()
  {
    return m_LastChild;
  }

  // Implementation of interface:
  virtual void AddProperty( const Fragment& name, const Fragment& value ) override
  {
    Add( name, value );
  }

  virtual JsonByoc* BeginObject( const Fragment& name ) override
  {
    Add( name, Fragment( JsonByoc::kObject ) );
    return new JsonByocExample( m_LastChild );
  }

  virtual void EndObject( JsonByoc* byoc ) override
  {
    delete byoc;
  }

  virtual JsonByoc* BeginArray( const Fragment& name ) override
  {
    Add( name, Fragment( JsonByoc::kArray ) );
    return new JsonByocExample( m_LastChild );
  }

  virtual void EndArray( JsonByoc* byoc ) override
  {
    delete byoc;
  }

  virtual void Error( const char* message, const char* p ) override
  {
    char buf[ 50 ];
    strncpy( buf, p, sizeof( buf ) );
    buf[ sizeof( buf ) - 1 ] = '\0';
    printf( "%s here:\n%s\n", message, buf );
  }

private:

  void Add( const Fragment& name, const Fragment& value )
  {
    Add( new JsonByocExample::Value( name, value ) );
  }

  void Add( JsonByocExample::Value* value )
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


void JsonByocExample::Value::Write( JsonByoc* writer ) const
{
  switch( m_Type )
  {
    case JsonByoc::kNull:
    case JsonByoc::kTrue:
    case JsonByoc::kFalse:
      writer->AddProperty( m_Name, m_Type );
      break;

    case JsonByoc::kString:
      writer->AddProperty( m_Name, JsonByoc::Fragment( JsonByoc::kString, m_Value.s ) );
      break;

    case JsonByoc::kInt:
    {
      char buf[ 25 ];
      writer->AddProperty( m_Name, JsonByoc::Fragment().FromInt( buf, m_Value.i ) );
      break;
    }

    case JsonByoc::kFloat:
    {
      char buf[ 25 ];
      writer->AddProperty( m_Name, JsonByoc::Fragment().FromFloat( buf, m_Value.f ) );
      break;
    }

    case kObject:
    {
      JsonByoc* child_writer = writer->BeginObject( m_Name );
      for( JsonByocExample::Value* child = m_Child; child; child = child->m_Next )
      {
        child->Write( child_writer );
      }
      writer->EndObject( child_writer );
      break;
    }

    case kArray:
    {
      JsonByoc* child_writer = writer->BeginArray( m_Name );
      for( JsonByocExample::Value* child = m_Child; child; child = child->m_Next )
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

char json_text[] =
  "{" \
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

int main(int argc, const char * argv[])
{
  JsonByocExample example_reader;
  JsonWriter writer( "", "" );

  printf( "\n\n--------- read into own format, then write\n\n" );
  if( JsonParse( &example_reader, json_text ) )
  {
    writer.SetMode( JsonWriter::kCount );
    example_reader.GetValue()->Write( &writer );
    size_t count = writer.GetCount();
    printf( "COUNT %d\n", ( int )count );
    char* buffer = new char[ count + 1 ]; // +1 for zero termination
    buffer[ count ] = '\0';
    writer.SetMode( JsonWriter::kWrite, buffer );
    example_reader.GetValue()->Write( &writer );
    printf( "%s\n", buffer );
    delete[] buffer;
  }

  printf( "\n\n--------- read directly into writer (for fun, because we can)\n\n" );
  writer.SetMode( JsonWriter::kPrint );
  JsonParse( &writer, json_text );

  return 0;
}

