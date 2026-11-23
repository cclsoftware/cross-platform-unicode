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
// Filename    : codepagewindowslatin1.h
// Description : Windows Latin 1 (codepage 1252) encoding
//
//************************************************************************************************

#ifndef _codepagewindowslatin1_h
#define _codepagewindowslatin1_h

#include "codepageisolatin1.h"

namespace CrossPlatformUnicode {

namespace Tables {

inline constexpr CodeUnitMapping kWindowsLatin1Encode[] =
{
	#include "tables/cp1252-encode.inc"
};

inline constexpr CodeUnitMapping kWindowsLatin1Decode[] =
{
	#include "tables/cp1252-decode.inc"
};

} // namespace Tables

//************************************************************************************************
// Windows Latin 1 (codepage 1252) encoding
//************************************************************************************************

inline char16_t encodeWindowsLatin1Char (char16_t c)
{
	return encodeLatin1BasedChar (c, Tables::kWindowsLatin1Encode, Tables::kWindowsLatin1Decode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char16_t decodeWindowsLatin1Char (char16_t c)
{
	return decodeLatin1BasedChar (c, Tables::kWindowsLatin1Decode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int encodeWindowsLatin1 (char dest[], int destSize, const CharType source[], int sourceLength)
{
	return encodeCString<encodeWindowsLatin1Char> (dest, destSize, source, sourceLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int decodeWindowsLatin1 (CharType dest[], int destSize, const char source[], int sourceLength)
{
	return decodeCString<decodeWindowsLatin1Char> (dest, destSize, source, sourceLength);
}

} // namespace CrossPlatformUnicode

#endif // _codepagewindowslatin1_h
