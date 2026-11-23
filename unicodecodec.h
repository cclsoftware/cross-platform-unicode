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
// Filename    : unicodecodec.h
// Description : UTF-8 and UTF-16 encoding (C++11)
//
//************************************************************************************************

#ifndef _unicodecodec_h
#define _unicodecodec_h

#include "unicodebase.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// UTF codec
/** Decode and encode functions return the number of bytes used, or one of the special return values. */
//************************************************************************************************

const int kBufferTooSmall = 0;
const int kIllegalInput = -1;

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Decode a UTF-8 sequence (RFC 2279). */
inline int decodeUTF8 (char32_t& c, const unsigned char* source, int sourceSize)
{
	if(sourceSize <= 0)
		return kBufferTooSmall;

	unsigned char s0 = source[0];
	if(s0 < 0x80)
	{
		c = s0;
		return 1;
	}

	// lead byte: number of bytes, payload mask, minimum second byte for the shortest form
	int length = 0;
	unsigned char mask = 0;
	unsigned char shortestLead = 0;
	unsigned char shortestSecond = 0;
	if(s0 < 0xC2)
		return kIllegalInput;
	else if(s0 < 0xE0)
	{
		length = 2;
		mask = 0x1F;
	}
	else if(s0 < 0xF0)
	{
		length = 3;
		mask = 0x0F;
		shortestLead = 0xE1;
		shortestSecond = 0xA0;
	}
	else if(s0 < 0xF8)
	{
		length = 4;
		mask = 0x07;
		shortestLead = 0xF1;
		shortestSecond = 0x90;
	}
	else if(s0 < 0xFC)
	{
		length = 5;
		mask = 0x03;
		shortestLead = 0xF9;
		shortestSecond = 0x88;
	}
	else if(s0 < 0xFE)
	{
		length = 6;
		mask = 0x01;
		shortestLead = 0xFD;
		shortestSecond = 0x84;
	}
	else
		return kIllegalInput;

	if(sourceSize < length)
		return kBufferTooSmall;

	for(int i = 1; i < length; i++)
		if((source[i] ^ 0x80) >= 0x40)
			return kIllegalInput;
	if(length > 2 && s0 < shortestLead && source[1] < shortestSecond)
		return kIllegalInput; // overlong form

	char32_t result = s0 & mask;
	for(int i = 1; i < length; i++)
		result = (result << 6) | char32_t(source[i] ^ 0x80);
	c = result;
	return length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Number of bytes needed to encode a code point in UTF-8 (RFC 2279). */
inline int getUTF8Length (char32_t c)
{
	if(c < 0x80)
		return 1;
	else if(c < 0x800)
		return 2;
	else if(c < 0x10000)
		return 3;
	else if(c < 0x200000)
		return 4;
	else if(c < 0x4000000)
		return 5;
	else if(c <= 0x7FFFFFFF)
		return 6;
	return kIllegalInput;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Encode a code point in UTF-8 (RFC 2279). */
inline int encodeUTF8 (char32_t c, unsigned char* dest, int destSize)
{
	int length = getUTF8Length (c);
	if(length == kIllegalInput)
		return kIllegalInput;
	if(destSize < length)
		return kBufferTooSmall;

	if(length == 1)
	{
		dest[0] = static_cast<unsigned char> (c);
		return 1;
	}

	static const unsigned char kLeadBytes[] = {0, 0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
	for(int i = length - 1; i > 0; i--)
	{
		dest[i] = static_cast<unsigned char> (0x80 | (c & 0x3F));
		c >>= 6;
	}
	dest[0] = static_cast<unsigned char> (kLeadBytes[length] | c);
	return length;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char32_t read16Bit (const unsigned char* source, bool bigEndian)
{
	return bigEndian ? char32_t((source[0] << 8) | source[1]) : char32_t((source[1] << 8) | source[0]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void write16Bit (unsigned char* dest, char32_t value, bool bigEndian)
{
	dest[bigEndian ? 0 : 1] = static_cast<unsigned char> (value >> 8);
	dest[bigEndian ? 1 : 0] = static_cast<unsigned char> (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Decode UTF-16 bytes (RFC 2781). */
inline int decodeUTF16 (char32_t& c, const unsigned char* source, int sourceSize, bool bigEndian)
{
	if(sourceSize < 2)
		return kBufferTooSmall;

	char32_t first = read16Bit (source, bigEndian);
	if(!isSurrogate (first))
	{
		c = first;
		return 2;
	}
	if(!isHighSurrogate (first))
		return kIllegalInput;
	if(sourceSize < 4)
		return kBufferTooSmall; // second unit may follow in next call

	char32_t second = read16Bit (source + 2, bigEndian);
	if(!isLowSurrogate (second))
		return kIllegalInput;

	c = makeCodePoint (first, second);
	return 4;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Encode a code point as UTF-16 bytes (RFC 2781). */
inline int encodeUTF16 (char32_t c, unsigned char* dest, int destSize, bool bigEndian)
{
	if(isSurrogate (c) || c >= 0x110000)
		return kIllegalInput;

	if(c < 0x10000)
	{
		if(destSize < 2)
			return kBufferTooSmall;
		write16Bit (dest, c, bigEndian);
		return 2;
	}

	if(destSize < 4)
		return kBufferTooSmall;
	write16Bit (dest, 0xD800 + ((c - 0x10000) >> 10), bigEndian);
	write16Bit (dest + 2, 0xDC00 + ((c - 0x10000) & 0x3FF), bigEndian);
	return 4;
}

//************************************************************************************************
// UTF8Reader
/** Reads code points, returns 0 at the end or at an illegal sequence. */
//************************************************************************************************

class UTF8Reader
{
public:
	UTF8Reader (const char source[], int length)
	: source (reinterpret_cast<const unsigned char*> (source)),
	  length (length)
	{}

	char32_t getNext (int* bytesUsed = nullptr)
	{
		char32_t c = 0;
		int used = decodeUTF8 (c, source, length);
		if(used <= 0)
			return 0;
		source += used;
		length -= used;
		if(bytesUsed)
			*bytesUsed = used;
		return c;
	}

protected:
	const unsigned char* source;
	int length;
};

//************************************************************************************************
// UTF8Writer
/** Writes code points to a buffer of size bytes including a null terminator.
	Without buffer, only the required size is calculated. */
//************************************************************************************************

class UTF8Writer
{
public:
	UTF8Writer (char dest[], int size)
	: dest (reinterpret_cast<unsigned char*> (dest)),
	  size (size),
	  length (0)
	{}

	bool writeNext (char32_t c)
	{
		unsigned char output[6] = {};
		int used = encodeUTF8 (c, output, 6);
		if(used <= 0)
			return false;
		if(dest)
		{
			if(length + used > size)
				return false;
			for(int i = 0; i < used; i++)
				dest[length + i] = output[i];
		}
		length += used;
		return true;
	}

	void finish ()
	{
		if(dest == nullptr)
			length++;
		else if(length < size)
			dest[length++] = 0;
	}

	/** Number of bytes written, including the null terminator after finish (). */
	int getLength () const
	{
		return length;
	}

protected:
	unsigned char* dest;
	int size;
	int length;
};

//************************************************************************************************
// UTF16Reader
/** Reads code points, returns 0 at the end or at an unpaired surrogate. */
//************************************************************************************************

template <typename CharType>
class UTF16Reader
{
public:
	UTF16Reader (const CharType source[], int length)
	: source (source),
	  length (length)
	{}

	char32_t getNext (int* unitsUsed = nullptr)
	{
		if(length <= 0)
			return 0;

		char32_t c = getCodeUnit (source[0]);
		int units = 1;
		if(isSurrogate (c))
		{
			if(!isHighSurrogate (c) || length < 2 || !isLowSurrogate (getCodeUnit (source[1])))
				return 0;
			c = makeCodePoint (c, getCodeUnit (source[1]));
			units = 2;
		}
		source += units;
		length -= units;
		if(unitsUsed)
			*unitsUsed = units;
		return c;
	}

protected:
	const CharType* source;
	int length;
};

//************************************************************************************************
// UTF16Writer
/** Writes code points to a buffer of size code units including a null terminator.
	Without buffer, only the required length is calculated. Surrogates and code points
	beyond U+10FFFF are rejected. */
//************************************************************************************************

template <typename CharType>
class UTF16Writer
{
public:
	UTF16Writer (CharType dest[], int size)
	: dest (dest),
	  size (size),
	  length (0)
	{}

	bool writeNext (char32_t c)
	{
		if(isSurrogate (c) || c >= 0x110000)
			return false;

		int units = getUTF16Length (c);
		if(dest)
		{
			if(length + units > size)
				return false;
			writeCodePoint (dest + length, c);
		}
		length += units;
		return true;
	}

	void finish ()
	{
		if(dest == nullptr)
			length++;
		else if(length < size)
			dest[length++] = 0;
	}

	/** Number of code units written, including the null terminator after finish (). */
	int getLength () const
	{
		return length;
	}

protected:
	CharType* dest;
	int size;
	int length;
};

//************************************************************************************************
// String conversion
/** Same conventions as the codepage conversion: the destination size includes a null terminator,
	the result is the number of characters written including the null terminator. Conversion stops
	at a null character or an illegal sequence. */
//************************************************************************************************

template <typename CharType>
int decodeUTF8 (CharType dest[], int destSize, const char source[], int sourceLength)
{
	UTF8Reader reader (source, sourceLength);
	UTF16Writer<CharType> writer (dest, destSize);

	char32_t c = 0;
	while((c = reader.getNext ()))
		if(!writer.writeNext (c))
			break;

	writer.finish ();
	return writer.getLength ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
int encodeUTF8 (char dest[], int destSize, const CharType source[], int sourceLength)
{
	UTF16Reader<CharType> reader (source, sourceLength);
	UTF8Writer writer (dest, destSize);

	char32_t c = 0;
	while((c = reader.getNext ()))
		if(!writer.writeNext (c))
			break;

	writer.finish ();
	return writer.getLength ();
}

//************************************************************************************************
// UTF16CodePointIterator
/** Bidirectional iterator over the code points of a UTF-16 string.
	The position is given in code units and lies between code points.
	Unpaired surrogates are returned as individual code points. */
//************************************************************************************************

template <typename CharType>
class UTF16CodePointIterator
{
public:
	UTF16CodePointIterator (const CharType text[], int length, int position = 0)
	: text (text),
	  length (length),
	  position (position)
	{}

	int getLength () const { return length; }
	int getPosition () const { return position; }
	void setPosition (int newPosition) { position = newPosition; }
	bool isStart () const { return position <= 0; }
	bool isEnd () const { return position >= length; }

	/** Get the code point after the current position without advancing, 0 at the end. */
	char32_t peekNext () const
	{
		if(isEnd ())
			return 0;
		int units = 1;
		return getCodePoint (text, length, position, units);
	}

	/** Get the code point before the current position without moving back, 0 at the start. */
	char32_t peekPrevious () const
	{
		UTF16CodePointIterator iterator (*this);
		return iterator.previous ();
	}

	/** Get the code point after the current position and advance behind it, 0 at the end. */
	char32_t next ()
	{
		if(isEnd ())
			return 0;
		int units = 1;
		char32_t c = getCodePoint (text, length, position, units);
		position += units;
		return c;
	}

	/** Move back one code point and return it, 0 at the start. */
	char32_t previous ()
	{
		if(position > length)
			position = length;
		if(isStart ())
			return 0;
		int units = 1;
		char32_t c = getCodePointBefore (text, position, units);
		position -= units;
		return c;
	}

protected:
	const CharType* text;
	int length;
	int position;
};

} // namespace CrossPlatformUnicode

#endif // _unicodecodec_h
