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
// Filename    : unicodebase.h
// Description : Table types, table lookup and UTF-16 helpers (C++11)
//
//************************************************************************************************

#ifndef _unicodebase_h
#define _unicodebase_h

namespace CrossPlatformUnicode {

//************************************************************************************************
// Table types
//************************************************************************************************

/** Range of code points, first and last are included. */
struct Range
{
	char32_t first;
	char32_t last;
};

/** Mapping of a code point to another code point. */
struct Mapping
{
	char32_t key;
	char32_t value;
};

/** Mapping of a UTF-16 code unit to another code unit or a codepage character. */
struct CodeUnitMapping
{
	char16_t key;
	char16_t value;
};

//************************************************************************************************
// Table lookup
/** Tables are sorted by first, key or value, respectively. */
//************************************************************************************************

template <typename T, int count>
inline const T* findRange (const T (&items)[count], char32_t c)
{
	int low = 0;
	int high = count - 1;
	while(low <= high)
	{
		int middle = (low + high) / 2;
		if(c < items[middle].first)
			high = middle - 1;
		else if(c > items[middle].last)
			low = middle + 1;
		else
			return &items[middle];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, int count>
inline const T* findKey (const T (&items)[count], char32_t key)
{
	int low = 0;
	int high = count - 1;
	while(low <= high)
	{
		int middle = (low + high) / 2;
		if(key < items[middle].key)
			high = middle - 1;
		else if(key > items[middle].key)
			low = middle + 1;
		else
			return &items[middle];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, int count>
inline const T* findValue (const T (&items)[count], char32_t value)
{
	int low = 0;
	int high = count - 1;
	while(low <= high)
	{
		int middle = (low + high) / 2;
		if(value < char32_t(items[middle]))
			high = middle - 1;
		else if(value > char32_t(items[middle]))
			low = middle + 1;
		else
			return &items[middle];
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <int count>
inline char32_t lookup (const Mapping (&items)[count], char32_t key, char32_t defaultValue)
{
	const Mapping* item = findKey (items, key);
	return item ? item->value : defaultValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <int count>
inline char16_t lookup (const CodeUnitMapping (&items)[count], char32_t key, char16_t defaultValue = 0)
{
	const CodeUnitMapping* item = findKey (items, key);
	return item ? item->value : defaultValue;
}

//************************************************************************************************
// UTF-16 functions
/** String functions are templates for any 16 bit code unit type (char16_t, wchar_t on Windows,
	unsigned short). Lengths are given in code units. Unpaired surrogates are treated as code points. */
//************************************************************************************************

constexpr bool isHighSurrogate (char32_t c)
{
	return c >= 0xD800 && c <= 0xDBFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr bool isLowSurrogate (char32_t c)
{
	return c >= 0xDC00 && c <= 0xDFFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr bool isSurrogate (char32_t c)
{
	return c >= 0xD800 && c <= 0xDFFF;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr char32_t makeCodePoint (char32_t highSurrogate, char32_t lowSurrogate)
{
	return 0x10000 + ((highSurrogate - 0xD800) << 10) + (lowSurrogate - 0xDC00);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr int getUTF16Length (char32_t c)
{
	return c >= 0x10000 ? 2 : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
constexpr char32_t getCodeUnit (CharType c)
{
	return char16_t(c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
inline int getLength (const CharType s[])
{
	int length = 0;
	if(s)
		while(s[length] != 0)
			length++;
	return length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get the code point at index, units receives the number of code units used. */
template <typename CharType>
inline char32_t getCodePoint (const CharType s[], int length, int index, int& units)
{
	char32_t c = getCodeUnit (s[index]);
	if(isHighSurrogate (c) && index + 1 < length && isLowSurrogate (getCodeUnit (s[index + 1])))
	{
		units = 2;
		return makeCodePoint (c, getCodeUnit (s[index + 1]));
	}
	units = 1;
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Get the code point before index, units receives the number of code units used. */
template <typename CharType>
inline char32_t getCodePointBefore (const CharType s[], int index, int& units)
{
	char32_t c = getCodeUnit (s[index - 1]);
	if(isLowSurrogate (c) && index >= 2 && isHighSurrogate (getCodeUnit (s[index - 2])))
	{
		units = 2;
		return makeCodePoint (getCodeUnit (s[index - 2]), c);
	}
	units = 1;
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Write a code point as one or two code units, returns the number of code units. */
template <typename CharType>
inline int writeCodePoint (CharType dest[], char32_t c)
{
	if(c >= 0x10000)
	{
		c -= 0x10000;
		dest[0] = CharType(0xD800 + (c >> 10));
		dest[1] = CharType(0xDC00 + (c & 0x3FF));
		return 2;
	}
	dest[0] = CharType(c);
	return 1;
}

} // namespace CrossPlatformUnicode

#endif // _unicodebase_h
