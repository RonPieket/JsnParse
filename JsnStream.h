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

// *****************************************************************************************************
// Section: simple in-memory read and write streams

struct JsnStreamIn
{
  const char* data;
  const char* error;
  int         index;
  int         index_end;

  JsnStreamIn( const char* text )
  : data      ( text )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( text ? ( int )strlen( text ) : 0 )
  {}

  JsnStreamIn( const char* text, int text_length )
  : data      ( text )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( ( int )text_length )
  {}

  JsnStreamIn( const char* text, const char* text_end )
  : data      ( text )
  , error     ( NULL )
  , index     ( 0 )
  , index_end ( ( int )( text_end - text ) )
  {}

  void Reset()
  {
    index = 0;
    error = NULL;
  }

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
        error = "Unexpected end of input data";
      }
    }
    return -1;
  }

  void Unread()
  {
    if( !error && index )
    {
      index -= 1;
    }
  }

  int Peek( int offset = 0 ) const
  {
    if( error || index + offset >= index_end )
    {
      return -1;
    }
    return ( uint8_t )data[ index + offset ];
  }

  int Error( const char* msg )
  {
    if( !error )
    {
      error = msg;
    }
    return 0;
  }
};

// -----------------------------------------------------------------------------------------------------

struct JsnStreamOut
{
  char*       data;
  const char* error;
  int         index;
  int         index_end;

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

  void Reset()
  {
    index = 0;
    error = NULL;
  }

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
        error = "Writing end-of-data marker";
      }
      else if( index >= index_end )
      {
        error = "Out of room in output buffer";
      }
      else
      {
        data[ index++ ] = ( char )c;
      }
    }
  }

  void WriteStr( const char* text )
  {
    while( char c = *text++ )
    {
      Write( c );
    }
  }

  int Error( const char* msg )
  {
    if( !error )
    {
      error = msg;
    }
    return 0;
  }
};

// *****************************************************************************************************
