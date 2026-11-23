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
// Filename    : unicodecase.h
// Description : Case mapping and case folding
//
//************************************************************************************************

#ifndef _unicodecase_h
#define _unicodecase_h

#include "unicodeproperties.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// Tables
//************************************************************************************************

constexpr int kMaxFullCaseFoldingLength = 3;

struct FullMapping
{
	char32_t key;
	char32_t values[kMaxFullCaseFoldingLength];
};

namespace Tables {

inline constexpr Mapping kToUpper[] =
{
	#include "tables/unicode-toupper.inc"
};

inline constexpr Mapping kToLower[] =
{
	#include "tables/unicode-tolower.inc"
};

inline constexpr Mapping kToTitle[] =
{
	#include "tables/unicode-totitle.inc"
};

inline constexpr Mapping kCaseFold[] =
{
	#include "tables/unicode-casefold.inc"
};

inline constexpr FullMapping kCaseFoldFull[] =
{
	#include "tables/unicode-casefold-full.inc"
};

} // namespace Tables

//************************************************************************************************
// Case mapping of code points
/** Simple (1:1) case mappings, they never change the number of UTF-16 code units. */
//************************************************************************************************

inline char32_t toUppercase (char32_t c)
{
	if(c < 0x80)
		return c >= 'a' && c <= 'z' ? c - 0x20 : c;
	return lookup (Tables::kToUpper, c, c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char32_t toLowercase (char32_t c)
{
	if(c < 0x80)
		return c >= 'A' && c <= 'Z' ? c + 0x20 : c;
	return lookup (Tables::kToLower, c, c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline char32_t toTitlecase (char32_t c)
{
	if(c < 0x80)
		return toUppercase (c);
	const Mapping* item = findKey (Tables::kToTitle, c);
	return item ? item->value : toUppercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Simple case folding. */
inline char32_t foldCase (char32_t c)
{
	if(c < 0x80)
		return toLowercase (c);
	const Mapping* item = findKey (Tables::kCaseFold, c);
	return item ? item->value : toLowercase (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Full case folding, returns the number of code points written to result. */
inline int foldCaseFull (char32_t result[kMaxFullCaseFoldingLength], char32_t c)
{
	if(c >= 0x80)
		if(const FullMapping* item = findKey (Tables::kCaseFoldFull, c))
		{
			int count = 0;
			for(char32_t value : item->values)
				if(value != 0)
					result[count++] = value;
			return count;
		}

	result[0] = foldCase (c);
	return 1;
}

//************************************************************************************************
// Case conversion of strings
/** Strings are converted in place with simple case mappings, the length doesn't change. */
//************************************************************************************************

template <typename CharType>
void toUppercase (CharType s[], int length)
{
	for(int i = 0; i < length;)
	{
		int units = 1;
		char32_t c = getCodePoint (s, length, i, units);
		char32_t mapped = toUppercase (c);
		if(mapped != c)
			writeCodePoint (s + i, mapped);
		i += units;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Final_Sigma condition: preceded by a cased letter and not followed by a cased letter, ignoring case-ignorable characters. */
template <typename CharType>
bool isFinalSigma (const CharType s[], int length, int index, int units)
{
	bool precededByCased = false;
	for(int i = index; i > 0;)
	{
		int previousUnits = 1;
		char32_t c = getCodePointBefore (s, i, previousUnits);
		i -= previousUnits;
		if(!isCaseIgnorable (c))
		{
			precededByCased = isCased (c);
			break;
		}
	}
	if(!precededByCased)
		return false;

	for(int i = index + units; i < length;)
	{
		int nextUnits = 1;
		char32_t c = getCodePoint (s, length, i, nextUnits);
		i += nextUnits;
		if(!isCaseIgnorable (c))
			return !isCased (c);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Converts to lowercase, handles the Greek final sigma. */
template <typename CharType>
void toLowercase (CharType s[], int length)
{
	constexpr char32_t kCapitalSigma = 0x03A3;
	constexpr char32_t kFinalSigma = 0x03C2;

	for(int i = 0; i < length;)
	{
		int units = 1;
		char32_t c = getCodePoint (s, length, i, units);
		char32_t mapped = c == kCapitalSigma && isFinalSigma (s, length, i, units) ? kFinalSigma : toLowercase (c);
		if(mapped != c)
			writeCodePoint (s + i, mapped);
		i += units;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Converts the first letter of each word to titlecase. */
template <typename CharType>
void capitalize (CharType s[], int length)
{
	auto isApostropheInWord = [&] (char32_t c, int index)
	{
		int nextUnits = 1;
		return (c == 0x0027 || c == 0x2019) && index + 1 < length && isAlpha (getCodePoint (s, length, index + 1, nextUnits));
	};

	bool inWord = false;
	for(int i = 0; i < length;)
	{
		int units = 1;
		char32_t c = getCodePoint (s, length, i, units);
		if(isAlphaNumeric (c))
		{
			if(!inWord && isLowercase (c))
				writeCodePoint (s + i, toTitlecase (c));
			inWord = true;
		}
		else if(!inWord || (getCombiningClass (c) == 0 && !isApostropheInWord (c, i)))
			inWord = false; // combining marks and apostrophes (e.g. "don't") continue a word
		i += units;
	}
}

} // namespace CrossPlatformUnicode

#endif // _unicodecase_h
