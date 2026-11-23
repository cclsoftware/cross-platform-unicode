//************************************************************************************************
//
// Cross-platform Unicode Library
//
// Copyright (c) 2026 CCL Software Licensing GmbH. All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// - Neither the name of the cross-platform-unicode project nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS",
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Filename    : codepageisolatin1.h
// Description : ISO Latin 1 encoding and codepages based on it
//
//************************************************************************************************

#ifndef _codepageisolatin1_h
#define _codepageisolatin1_h

#include "codepagebase.h"

namespace CrossPlatformUnicode {

namespace Tables {

inline constexpr CodeUnitMapping kISOLatin1Encode[] =
{
	#include "tables/latin1-encode.inc"
};

} // namespace Tables

//************************************************************************************************
// ISO Latin 1 encoding
/** Characters outside of ISO Latin 1 are transliterated if possible. */
//************************************************************************************************

inline char16_t encodeISOLatin1Char (char16_t c)
{
	if(c < 0x100)
		return c;
	if(!isSurrogate (c))
		if(char16_t result = lookup (Tables::kISOLatin1Encode, c))
			return result;
	return '?';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char16_t decodeISOLatin1Char (char16_t c)
{
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int encodeISOLatin1 (char dest[], int destSize, const CharType source[], int sourceLength)
{
	return encodeCString<encodeISOLatin1Char> (dest, destSize, source, sourceLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int decodeISOLatin1 (CharType dest[], int destSize, const char source[], int sourceLength)
{
	return decodeCString<decodeISOLatin1Char> (dest, destSize, source, sourceLength);
}

//************************************************************************************************
// Codepages based on ISO Latin 1
/** The tables contain the characters differing from ISO Latin 1. */
//************************************************************************************************

template <int encodeCount, int decodeCount>
char16_t encodeLatin1BasedChar (char16_t c, const CodeUnitMapping (&encodingTable)[encodeCount], const CodeUnitMapping (&decodingTable)[decodeCount])
{
	if(c < 0x80)
		return c;
	if(c < 0x100 && lookup (decodingTable, c) == 0)
		return c;
	if(isSurrogate (c))
		return '?';

	if(char16_t result = lookup (encodingTable, c))
		return result;

	// best fit via ISO Latin 1
	char16_t latin1 = lookup (Tables::kISOLatin1Encode, c);
	if(latin1 && latin1 != c)
		return encodeLatin1BasedChar (latin1, encodingTable, decodingTable);
	return '?';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <int decodeCount>
char16_t decodeLatin1BasedChar (char16_t c, const CodeUnitMapping (&decodingTable)[decodeCount])
{
	if(c < 0x80)
		return c;
	char16_t result = lookup (decodingTable, c);
	return result ? result : c;
}

} // namespace CrossPlatformUnicode

#endif // _codepageisolatin1_h
