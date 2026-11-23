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
// Filename    : codepageshiftjis.h
// Description : Shift-JIS (codepage 932) encoding
//
//************************************************************************************************

#ifndef _codepageshiftjis_h
#define _codepageshiftjis_h

#include "codepageascii.h"

namespace CrossPlatformUnicode {

namespace Tables {

constexpr int kShiftJISTrailFirst = 0x40; // must match tools/generate-tables.py
constexpr int kShiftJISTrailCount = 0xFD - kShiftJISTrailFirst;

inline constexpr CodeUnitMapping kShiftJISEncode[] =
{
	#include "tables/cp932-encode.inc"
};

inline constexpr char16_t kShiftJISDecodeSingle[] =
{
	#include "tables/cp932-decode-single.inc"
};

inline constexpr unsigned char kShiftJISLeadBytes[] =
{
	#include "tables/cp932-lead-bytes.inc"
};

inline constexpr char16_t kShiftJISDecodeDouble[] =
{
	#include "tables/cp932-decode-double.inc"
};

} // namespace Tables

//************************************************************************************************
// Shift-JIS (codepage 932) encoding
/** Same conventions as the single byte codepages, see codepagebase.h. Characters take one or two bytes. */
//************************************************************************************************

template <typename CharType>
int encodeShiftJIS (char dest[], int destSize, const CharType source[], int sourceLength)
{
	unsigned char* bytes = reinterpret_cast<unsigned char*> (dest);
	int used = 0;
	for(int i = 0; i < sourceLength; i++)
	{
		char16_t c = char16_t(getCodeUnit (source[i]));
		char16_t code = c;
		if(isSurrogate (c))
		{
			code = '?';
			if(isHighSurrogate (c) && i + 1 < sourceLength && isLowSurrogate (getCodeUnit (source[i + 1])))
				i++; // one replacement character per surrogate pair
		}
		else if(c >= 0x80)
		{
			code = lookup (Tables::kShiftJISEncode, c);
			if(code == 0)
				code = lookup (Tables::kASCIIEncode, c);
			if(code == 0)
				code = '?';
		}

		int size = code > 0xFF ? 2 : 1;
		if(bytes)
		{
			if(used + size >= destSize)
				break;
			if(size == 2)
			{
				bytes[used] = static_cast<unsigned char> (code >> 8);
				bytes[used + 1] = static_cast<unsigned char> (code & 0xFF);
			}
			else
				bytes[used] = static_cast<unsigned char> (code);
		}
		used += size;
	}

	if(bytes == nullptr)
		used++; // null terminator
	else if(used < destSize)
		bytes[used++] = 0;

	return used;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int decodeShiftJIS (CharType dest[], int destSize, const char source[], int sourceLength)
{
	using namespace Tables;

	const unsigned char* bytes = reinterpret_cast<const unsigned char*> (source);
	UTF16Writer<CharType> writer (dest, destSize);
	for(int i = 0; i < sourceLength; i++)
	{
		unsigned char b = bytes[i];
		char16_t c = b;
		if(b >= 0x80)
		{
			c = kShiftJISDecodeSingle[b - 0x80];
			if(c == 0)
			{
				c = '?';
				const unsigned char* leadByte = findValue (kShiftJISLeadBytes, b);
				if(leadByte && i + 1 < sourceLength)
				{
					int trail = bytes[i + 1] - kShiftJISTrailFirst;
					if(trail >= 0 && trail < kShiftJISTrailCount)
					{
						int row = int(leadByte - kShiftJISLeadBytes);
						if(char16_t mapped = kShiftJISDecodeDouble[row * kShiftJISTrailCount + trail])
							c = mapped;
						i++;
					}
				}
			}
		}
		if(!writer.writeNext (c))
			break;
	}

	writer.finish ();
	return writer.getLength ();
}

} // namespace CrossPlatformUnicode

#endif // _codepageshiftjis_h
