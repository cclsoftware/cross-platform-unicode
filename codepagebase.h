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
// Filename    : codepagebase.h
// Description : Conversion between UTF-16 and 8 bit codepages
//
//************************************************************************************************

#ifndef _codepagebase_h
#define _codepagebase_h

#include "unicodecodec.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// Codepage conversion
/** Conversion functions take the length of the source string and the size of the destination buffer,
	including a null terminator. They return the number of characters written, including the null terminator.
	Without destination buffer, only the required size is calculated. Characters that can't be converted
	are replaced by '?', one per surrogate pair. */
//************************************************************************************************

typedef char16_t (*EncodeCharFunction) (char16_t c);	///< UTF-16 code unit to codepage character
typedef char16_t (*DecodeCharFunction) (char16_t c);	///< codepage character to UTF-16 code unit

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Encode UTF-16 to a single byte codepage. */
template <EncodeCharFunction encode, typename CharType>
int encodeCString (char dest[], int destSize, const CharType source[], int sourceLength)
{
	unsigned char* bytes = reinterpret_cast<unsigned char*> (dest);
	int used = 0;
	for(int i = 0; i < sourceLength; i++)
	{
		char32_t c = getCodeUnit (source[i]);
		unsigned char encoded = static_cast<unsigned char> (encode (char16_t(c)));
		if(isHighSurrogate (c) && i + 1 < sourceLength && isLowSurrogate (getCodeUnit (source[i + 1])))
			i++; // one replacement character per surrogate pair
		if(bytes)
		{
			if(used + 1 >= destSize)
				break;
			bytes[used] = encoded;
		}
		used++;
	}

	if(bytes == nullptr)
		used++; // null terminator
	else if(used < destSize)
		bytes[used++] = 0;

	return used;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Decode a single byte codepage to UTF-16. */
template <DecodeCharFunction decode, typename CharType>
int decodeCString (CharType dest[], int destSize, const char source[], int sourceLength)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*> (source);
	UTF16Writer<CharType> writer (dest, destSize);
	for(int i = 0; i < sourceLength; i++)
		if(!writer.writeNext (decode (bytes[i])))
			break;

	writer.finish ();
	return writer.getLength ();
}

} // namespace CrossPlatformUnicode

#endif // _codepagebase_h
