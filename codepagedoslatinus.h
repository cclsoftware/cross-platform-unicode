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
// Filename    : codepagedoslatinus.h
// Description : DOS Latin US (codepage 437) encoding
//
//************************************************************************************************

#ifndef _codepagedoslatinus_h
#define _codepagedoslatinus_h

#include "codepagebase.h"

namespace CrossPlatformUnicode {

namespace Tables {

inline constexpr CodeUnitMapping kDOSLatinUSEncode[] =
{
	#include "tables/cp437-encode.inc"
};

inline constexpr CodeUnitMapping kDOSLatinUSDecode[] =
{
	#include "tables/cp437-decode.inc"
};

} // namespace Tables

//************************************************************************************************
// DOS Latin US (codepage 437) encoding
//************************************************************************************************

inline char16_t encodeDOSLatinUSChar (char16_t c)
{
	if(c < 0x80)
		return c;
	if(!isSurrogate (c))
		if(char16_t result = lookup (Tables::kDOSLatinUSEncode, c))
			return result;
	return '?';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char16_t decodeDOSLatinUSChar (char16_t c)
{
	if(c < 0x80)
		return c;
	char16_t result = lookup (Tables::kDOSLatinUSDecode, c);
	return result ? result : '?'; // all 0x80..0xFF are defined
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int encodeDOSLatinUS (char dest[], int destSize, const CharType source[], int sourceLength)
{
	return encodeCString<encodeDOSLatinUSChar> (dest, destSize, source, sourceLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int decodeDOSLatinUS (CharType dest[], int destSize, const char source[], int sourceLength)
{
	return decodeCString<decodeDOSLatinUSChar> (dest, destSize, source, sourceLength);
}

} // namespace CrossPlatformUnicode

#endif // _codepagedoslatinus_h
