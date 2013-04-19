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

/************************************************************************************************************/ /**
 \enum JsnType
 Identify type of JsnFragment.
 */
enum JsnType
{
  kJsn_Undefined, /**< Used if a JsnFragment represents something other than JSON text. For example,
                   when AddProperty is called with an array element, the name fragment will be of type
                   kJsn_Undefined, because array elements are not named. Also (ab)used for brackets,
                   commas, spaces etc. */
  kJsn_Null,      /**< JSON type null */
  kJsn_False,     /**< JSON type boolean value false */
  kJsn_True,      /**< JSON type boolean value true */
  kJsn_Int,       /**< JSON type number subtype integer */
  kJsn_Float,     /**< JSON type number subtype float */
  kJsn_String,    /**< JSON type string */
  kJsn_Object,    /**< JSON type object */
  kJsn_Array      /**< JSON type array */
};

/**
 \struct JsnFragment
 Represents a text fragment of the input JSON text. Note that the fragment text is NOT a zero-terminated
 string.
 */
struct JsnFragment
{
  const char* m_Text;   /**< Start of the fragment text (only for types kJsn_String, kJsn_Float, kJsn_Int) */
  int         m_Length; /**< Length of the fragment text */
  JsnType     m_Type;   /**< Type of the fragment */

  /**
   Construct from text and length.
   \param[ in ] t Type
   \param[ in ] text Start of text, not necessarily zero terminated
   \param[ in ] length Length of text string
   */
  JsnFragment( JsnType t, const char* text, int length )
  : m_Text( text )
  , m_Length( length )
  , m_Type( t )
  {}

  /**
   Construct from text start and end pointer.
   \param[ in ] t Type
   \param[ in ] text Start of text, not necessarily zero terminated
   \param[ in ] end End of text string
   */
  JsnFragment( JsnType t, const char* text, const char* end )
  : m_Text( text )
  , m_Length( ( int )( end - text ) )
  , m_Type( t )
  {}

  /**
   Construct from zero terminated text string.
   \param[ in ] t Type
   \param[ in ] text Start of zero terminated text string
   */
  JsnFragment( JsnType t, const char* text )
  : m_Text( text )
  , m_Length( text ? ( int )strlen( text ) : 0 )
  , m_Type( t )
  {}

  /**
   Construct from zero terminated text string. Type will be undefined. This is used for helper strings,
   such as spaces and commas.
   \param[ in ] text Start of zero terminated text string
   */
  JsnFragment( const char* text )
  : m_Text( text )
  , m_Length( text ? ( int )strlen( text ) : 0 )
  , m_Type( kJsn_Undefined )
  {}

  /**
   Copy constructor.
   \param[ in ] other JsnFragment to copy
   */
  JsnFragment( const JsnFragment& other )
  : m_Text( other.m_Text )
  , m_Length( other.m_Length )
  , m_Type( other.m_Type )
  {}

  /**
   Default constructor.
   \param[ in ] t Type
   */
  JsnFragment( JsnType t = kJsn_Undefined )
  : m_Text( NULL )
  , m_Length( 0 )
  , m_Type( t )
  {}

  /**
   Interpret fragment as double precision float using `atof()`
   */
  double AsFloat() const;
  /**
   Interpret fragment as integer using `atod()`
   */
  int64_t AsInt() const;

  /**
   Assignment from string.
   */
  JsnFragment& operator=( const char* text )
  {
    m_Text    = text;
    m_Length  = text ? ( int )strlen( text ) : 0;
    return *this;
  }

  /**
   Assignment from another JsnFragment.
   */
  JsnFragment& operator= ( const JsnFragment& rhs )
  {
    m_Text    = rhs.m_Text;
    m_Length  = rhs.m_Length;
    m_Type    = rhs.m_Type;
    return *this;
  }

  /**
   Build from a double precision float.
   \param[ in ] buf25 Buffer to write the text to. Note that this buffer must be managed by the caller,
   and remain valid for the life span of the object.
   \param[ in ] buf_size Size of buffer. Must be at least 25 bytes.
   \param[ in ] value The value to be represented.
   \return Fragment
   */
  static JsnFragment FromFloat( char* buf25, int buf_size, double value );

  /**
   Build from an integer.
   \param[ in ] buf25 Buffer to write the text to. Note that this buffer must be managed by the caller,
   and remain valid for the life span of the object.
   \param[ in ] buf_size Size of buffer. Must be at least 25 bytes.
   \param[ in ] value The value to be represented.
   \return Fragment
   */
  static JsnFragment FromInt( char* buf25, int buf_size, int64_t value );
};

/************************************************************************************************************/ /**
 \interface JsnHandler
 Abstract interface for receiving the JSON text in a piecemeal fashion. A JsnHandler instance is
 responsible for building one object or array. It will be passed properties through AddProperty(),
 nested objects through BeginObject()/EndObject(), and/or nested arrays through
 BeginArray()/EndArray().
 */
class JsnHandler
{
public:

  /**
   Add a new property to the object or array that this handler represents.
   \param[ in ] name Name of the property. This will be empty if this is an array element.
   \param[ in ] value Value ot he property.
   */
  virtual void        AddProperty( const JsnFragment& name, const JsnFragment& value ) = 0;
  /**
   Add a new nested object to the object or array that this handler represents.
   \param[ in ] name Name of the object. This will be empty if this is an array element.
   \return Handler that will receive properties of the nested object.
   */
  virtual JsnHandler* BeginObject( const JsnFragment& name ) = 0;
  /**
   Finalize the nested object.
   \param[ in ] handler Handler from BeginObject(), that was used to receive properties of the nested
   object.
   */
  virtual void        EndObject( JsnHandler* handler ) = 0;
  /**
   Add a new nested array to the object or array that this handler represents.
   \param[ in ] name Name of the array. This will be empty if this is an array element.
   \return Handler that will receive elements of the nested array.
   */
  virtual JsnHandler* BeginArray( const JsnFragment& name ) = 0;
  /**
   Finalize the nested array.
   \param[ in ] handler Handler from BeginArray(), that was used to receive elements of the nested
   array.
   */
  virtual void        EndArray( JsnHandler* handler ) = 0;

  virtual ~JsnHandler() {}
};

/************************************************************************************************************/ /**
 \class JsnWriter
 Used to format your data in valid JSON. Implements JsnHandler. You are expected to iterate through your
 data and call the JsnWriter members in more or less the same order as it was built (or would build it)
 with JsnHandler. See proveded example code JsnExample::Value::Write().
 */
class JsnWriter final : public JsnHandler
{
public:

  /**
   \struct Style
   Control certain formatting aspects of the generated JSON text
   */
  struct Style
  {
    JsnFragment  m_IndentString;  /**< Text to output for every level of indentation. Default is two
                                   spaces. Set to empty string for maximum compactness. */
    JsnFragment  m_NewlineString; /**< Text to output at the end of a line. Defaults to '\n'. Set to
                                   empty string for maximum compactness. */
    JsnFragment  m_SpaceAfterColonString; /**< Text to output after a colon between name and value.
                                           Defaults to single space.  Set to empty string for maximum
                                           compactness.*/
    bool         m_EscapeUTF8;    /**< Flag to force UTF-8 characters to be escaped. If false, UTF-8
                                   will be written to the output. If true, all UTF-8 characters will be
                                   replaced with their \\uXXXX equivalent */

    Style();
  };

  /**
   Construct JsnWriter with an input stream, and optional style.
   \param[ in ] stream Output stream.
   \param[ in ] style Settings that control details of the the output format.
   */
  JsnWriter( JsnStreamOut* stream, const Style* style = NULL );

  virtual void        AddProperty( const JsnFragment& name, const JsnFragment& value ) override;
  virtual JsnHandler* BeginObject( const JsnFragment& name ) override;
  virtual void        EndObject( JsnHandler* byoc ) override;
  virtual JsnHandler* BeginArray( const JsnFragment& name ) override;
  virtual void        EndArray( JsnHandler* byoc ) override;

private:

  JsnStreamOut*   m_Stream;
  const Style*    m_Style;
  const int       m_IndentLevel;
  int             m_ValueCount;

  JsnWriter( const JsnWriter& other );
  void WriteFragment( const JsnFragment& fragment );
  void WriteFragmentString( const JsnFragment& fragment );
  void WriteIndent();
  void WriteProperty( const JsnFragment& name, const JsnFragment& value );
};

/************************************************************************************************************/ /**
 Parse the input stream, call members of the handler implementation as elements in teh text are
 detected.
 */
bool JsnParse( JsnHandler* reader, JsnStreamIn* stream );

// -----------------------------------------------------------------------------------------------------
