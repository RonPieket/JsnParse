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

/*******************************************************************************************************
 \class JsnStreamIn
 Simple in-memory byte stream reader.
 */
class JsnStreamIn
{
public:

  /**
   Return error string.
   \return Error string, or NULL if no error.
   */
  const char* GetError() const { return error; }

  /**
   Get pointer to current data being read.
   \return Pointer to data currently being read.
   */
  const char* GetCurrent() const { return data + index; }

  /**
   Return number of characters read.
   \return Number of characters read.
   */
  int GetCount() const { return index; }

  /**
   Construct from zero terminated string.
   \param[ in ] text Zero terminated string.
   */
  JsnStreamIn( const char* text )
  : data      ( text )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( text ? ( int )strlen( text ) : 0 )
  {}

  /**
   Construct from text, not necessarily zero terminated.
   \param[ in ] text Start of text.
   \param[ in ] text_length Length of text.
   */
  JsnStreamIn( const char* text, int text_length )
  : data      ( text )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( ( int )text_length )
  {}

  /**
   Construct from text, not necessarily zero terminated.
   \param[ int ] text Start of text.
   \param[ int ] text_length End of text.
   */
  JsnStreamIn( const char* text, const char* text_end )
  : data      ( text )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( ( int )( text_end - text ) )
  {}

  /**
   Move read position to the beginning of the data.
   */
  void Reset()
  {
    index = 0;
    error = NULL;
  }

  /**
   Read next character.
   \return The read character, or -1 if an error occurred (either during this read operation or previously)
   */
  int Read()
  {
    if( !error )
    {
      if( index < index_end )
      {
        // Ensure unsigned extension
        return ( uint8_t )data[ index++ ];
      }
      else
      {
        SetError( "Unexpected end of input data" );
      }
    }
    return -1;
  }

  /**
   Move read position back by one position.
   */
  void Unread()
  {
    if( !error && index )
    {
      index -= 1;
    }
  }

  /**
   Read next character without moving read position.
   \return The read character, or -1 if an error occurred (either during this read operation or previously)
   */
  int Peek( int offset = 0 ) const
  {
    if( error || index + offset >= index_end )
    {
      return -1;
    }
    return ( uint8_t )data[ index + offset ];
  }

  /**
   Set error string.
   \return Always zero.
   */
  int SetError( const char* msg )
  {
    if( !error )
    {
      error = msg;
    }
    return 0;
  }

private:
  const char* data;
  const char* error;
  int         index;
  int         index_end;
};

/*******************************************************************************************************
 \class JsnStreamOut
 Simple in-memory byte stream writer.
 */
class JsnStreamOut
{
private:
  char*       data;
  const char* error;
  int         index;
  int         index_end;
public:

  /**
   Return error string.
   \return Error string, or NULL if no error.
   */
  const char* GetError() const { return error; }

  /**
   Return number of characters written.
   \return Number of characters written.
   */
  int GetCount() const { return index; }

  JsnStreamOut( char* buf, int buf_size )
  : data      ( buf )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( buf_size )
  {}

  JsnStreamOut()
  : data      ( NULL )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( 0 )
  {}

  JsnStreamOut( char* buf, char* buf_end )
  : data      ( buf )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( ( int )( buf_end - buf ) )
  {}

  /**
   Move read position to the beginning of the data.
   */
  void Reset()
  {
    index = 0;
    error = NULL;
  }

  /**
   Write character to output data.
   \param[ in ] c Character to write.
   */
  void Write( int c )
  {
    if( !error )
    {
      if( !data )
      {
        index += 1; // Just counting
      }
      else if( c == -1 )
      {
        SetError( "Writing end-of-data marker" );
      }
      else if( index >= index_end )
      {
        SetError( "Out of room in output buffer" );
      }
      else
      {
        data[ index++ ] = ( char )c;
      }
    }
  }

  /**
   Write multiple characters to output data.
   \param[ in ] text Zero terminated string to write. (Terminator will not be written)
   */
  void WriteStr( const char* text )
  {
    while( char c = *text++ )
    {
      Write( c );
    }
  }

  /**
   Set error string.
   \return Always zero.
   */
  int SetError( const char* msg )
  {
    if( !error )
    {
      error = msg;
    }
    return 0;
  }
};

// *****************************************************************************************************
