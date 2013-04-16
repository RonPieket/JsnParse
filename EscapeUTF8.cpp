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

#include "ByteStream.h"
#include "EscapeUTF8.h"

#include <stdint.h>

// *****************************************************************************************************
// Section: read individual unicode characters

static int ReadEscapedUTF8Hex4( ByteStreamIn* stream )
{
  int result = 0;
  int c = stream->Read();
  if( c != '\\' )
  {
    return stream->Error( "Expected '\\' character" );
  }
  c = stream->Read();
  if( c != 'u' && c != 'U' )
  {
    return stream->Error( "Expected 'u' character" );
  }
  for( int i = 0; i < 4; ++i )
  {
    c = stream->Read();
    if( c >= 'a' && c <= 'f' )
    {
      c += 10 - 'a';
    }
    else if( c >= 'A' && c <= 'F' )
    {
      c += 10 - 'A';
    }
    else if( c >= '0' && c <= '9' )
    {
      c += 0 - '0';
    }
    else
    {
      return stream->Error( "Expected hex digit" );
    }
    result = ( result << 4 ) + c;
  }
  return result;
}

// -----------------------------------------------------------------------------------------------------

int ReadEscapedUTF8Char( ByteStreamIn* stream )
{
  int result = ReadEscapedUTF8Hex4( stream );
  if( result >= 0xD800 && result <= 0xDBFF )
  {
    // UTF-16 surrogate pair. Go fetch other half
    int temp = ReadEscapedUTF8Hex4( stream );
    if( temp >= 0xDC00 && temp <= 0xDFFF )
    {
      result = ( ( ( result - 0xD800 ) << 10 ) | ( temp - 0xDC00 ) ) + 0x10000;
    }
  }
  return result;
}

// -----------------------------------------------------------------------------------------------------

int ReadUnescapedUTF8Char( ByteStreamIn* stream )
{
  int result = 0;
  int c = stream->Read();
  if( !stream->error )
  {
    if( c <= 0x7f )
    {
      // 7 bits
      result = c;
    }
    else if( ( c & 0xe0 ) == 0xc0 )
    {
      // 11 bits
      result = c & 0x1f;
      c = stream->Read();
      if( ( c & 0xc0 ) != 0x80 )
      {
        return stream->Error( "Multi-byte sequence error" );
      }
      result = ( result << 6 ) | ( c & 0x3f );
    }
    else if( ( c & 0xf0 ) == 0xe0 )
    {
      // 16 bits
      result = c & 0x0f;
      c = stream->Read();
      if( ( c & 0xc0 ) != 0x80 )
      {
        return stream->Error( "Multi-byte sequence error" );
      }
      result = ( result << 6 ) | ( c & 0x3f );
      c = stream->Read();
      if( ( c & 0xc0 ) != 0x80 )
      {
        return stream->Error( "Multi-byte sequence error" );
      }
      result = ( result << 6 ) | ( c & 0x3f );
    }
    else if( ( c & 0xf7 ) == 0xf0 )
    {
      // 21 bits
      result = c & 0x07;
      c = stream->Read();
      if( ( c & 0xc0 ) != 0x80 )
      {
        return stream->Error( "Multi-byte sequence error" );
      }
      result = ( result << 6 ) | ( c & 0x3f );
      c = stream->Read();
      if( ( c & 0xc0 ) != 0x80 )
      {
        return stream->Error( "Multi-byte sequence error" );
      }
      result = ( result << 6 ) | ( c & 0x3f );
      c = stream->Read();
      if( ( c & 0xc0 ) != 0x80 )
      {
        return stream->Error( "Multi-byte sequence error" );
      }
      result = ( result << 6 ) | ( c & 0x3f );
    }
    else
    {
      return stream->Error( "Multi-byte sequence error" );
    }
  }
  return result;
}

// *****************************************************************************************************
// Section: write individual unicode characters

static void WriteEscapedUTF8Hex4( ByteStreamOut* stream, int code16 )
{
  stream->Write( '\\' );
  stream->Write( 'u' );
  for( int i = 0; i < 4; ++i )
  {
    int c = ( code16 >> ( 12 - i * 4 ) ) & 0x0f;
    stream->Write( "0123456789ABCDEF"[ c ] );
  }
}

// -----------------------------------------------------------------------------------------------------

void WriteEscapedUTF8Char( ByteStreamOut* stream, int codepoint )
{
  if( codepoint >= 0x10000 )
  {
    codepoint -= 0x10000;
    WriteEscapedUTF8Hex4( stream, ( ( codepoint >> 10 ) & 0x03ff ) + 0xd800 );
    WriteEscapedUTF8Hex4( stream, (   codepoint         & 0x03ff ) + 0xdc00 );
  }
  else
  {
    WriteEscapedUTF8Hex4( stream, codepoint );
  }
}

// -----------------------------------------------------------------------------------------------------

void WriteUnescapedUTF8Char( ByteStreamOut* stream, int codepoint )
{
  if( codepoint < 0x80 )
  {
    // 7 bits
    stream->Write( codepoint );
  }
  else if( codepoint < 0x800 )
  {
    // 11 bits
    stream->Write( ( ( codepoint >> 6 ) & 0x1f ) | 0xc0 );
    stream->Write( ( ( codepoint      ) & 0x3f ) | 0x80 );
  }
  else if( codepoint < 0x10000 )
  {
    // 16 bits
    stream->Write( ( ( codepoint >> 12 ) & 0x0f ) | 0xe0 );
    stream->Write( ( ( codepoint >>  6 ) & 0x3f ) | 0x80 );
    stream->Write( ( ( codepoint       ) & 0x3f ) | 0x80 );
  }
  else
  {
    // 21 bits
    stream->Write( ( ( codepoint >> 18 ) & 0x07 ) | 0xf0 );
    stream->Write( ( ( codepoint >> 12 ) & 0x3f ) | 0x80 );
    stream->Write( ( ( codepoint >>  6 ) & 0x3f ) | 0x80 );
    stream->Write( ( ( codepoint       ) & 0x3f ) | 0x80 );
  }
}

// *****************************************************************************************************
// Section: converting strings

void UnescapeUTF8( ByteStreamOut* write_stream, ByteStreamIn* read_stream )
{
  while( !read_stream->error && !write_stream->error && read_stream->Peek() )
  {
    int codepoint = 0;
    int c0 = read_stream->Peek( 0 );
    int c1 = read_stream->Peek( 1 );
    if( c0 == '\\' && ( c1 == 'u' || c1 == 'U' ) )
    {
      codepoint = ReadEscapedUTF8Char( read_stream );
    }
    else
    {
      codepoint = read_stream->Read();
    }
    if( !read_stream->error )
    {
      WriteUnescapedUTF8Char( write_stream, codepoint );
    }
  }
  write_stream->Write( 0 );
}

// -----------------------------------------------------------------------------------------------------

void EscapeUTF8( ByteStreamOut* write_stream, ByteStreamIn* read_stream )
{
  while( !read_stream->error && !write_stream->error && read_stream->Peek() )
  {
    int codepoint = ReadUnescapedUTF8Char( read_stream );
    if( !read_stream->error )
    {
      if( codepoint < 0x80 )
      {
        write_stream->Write( codepoint );
      }
      else
      {
        WriteEscapedUTF8Char( write_stream, codepoint );
      }
    }
  }
  write_stream->Write( 0 );
}

// *****************************************************************************************************
