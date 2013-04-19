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

struct JsnStreamIn;
struct JsnStreamOut;

/************************************************************************************************************/ /**
 Read input stream, convert to multi-byte sequences where necessary, and write result to output stream.
 */
void JsnUnescapeUTF8( JsnStreamOut* write_stream, JsnStreamIn* read_stream );

/************************************************************************************************************/ /**
 Read input stream, apply "\uXXXX" escaping where necessary, and write result to output stream.
 */
void JsnEscapeUTF8( JsnStreamOut* write_stream, JsnStreamIn* read_stream );

/************************************************************************************************************/ /**
 Read a single (possibly multi-byte or escaped) codepoint from the input stream. Will recognize UTF-8 bit
 pattern, or "\uXXXX" escape code and convert accordingly.
 \param[ in ] stream Input stream
 \returns Codepoint
 */
int JsnReadUTF8Char( JsnStreamIn* stream );

/************************************************************************************************************/ /**
 Write single codepoint to output stream with "\uXXXX" escaping if necessary.
 \param[ in ] codepoint Codepoint to write
 \param[ in ] stream Output stream to write to
 */
void JsnWriteEscapedUTF8Char( JsnStreamOut* stream, int codepoint );

/************************************************************************************************************/ /**
 Write a single codepoint to output stream, making it a multi-byte sequence if necessary.
 \param[ in ] codepoint Codepoint to write
 \param[ in ] stream Output stream to write to
 */
void JsnWriteUnescapedUTF8Char( JsnStreamOut* stream, int codepoint );
