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
// Filename    : unicodecompare.h
// Description : String comparison and search
//
//************************************************************************************************

#ifndef _unicodecompare_h
#define _unicodecompare_h

#include "unicodecase.h"
#include "unicodenormalization.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// CaselessIterator
/** Iterates the code points of NFD (toCasefold (NFD (s))), see "canonical caseless match"
	in the Unicode standard, optionally without combining marks. Without ignoreCase,
	the code points of NFD (s) are returned. */
//************************************************************************************************

template <typename CharType>
class CaselessIterator
{
public:
	CaselessIterator (const CharType s[], int length, bool ignoreCase, bool ignoreDiacritics)
	: s (s),
	  length (length > 0 ? length : 0),
	  position (0),
	  ignoreCase (ignoreCase),
	  ignoreDiacritics (ignoreDiacritics),
	  pending {},
	  pendingCount (0),
	  segment {},
	  segmentCount (0),
	  segmentIndex (0)
	{}

	/** Returns 0 at the end. */
	char32_t next ()
	{
		if(segmentIndex >= segmentCount && !fillSegment ())
			return 0;
		return segment[segmentIndex++];
	}

protected:
	static constexpr int kMaxSegment = 64;

	const CharType* s;
	int length;
	int position;
	bool ignoreCase;
	bool ignoreDiacritics;
	char32_t pending[kMaxDecompositionLength];
	int pendingCount;
	char32_t segment[kMaxSegment];
	int segmentCount;
	int segmentIndex;

	bool readNext ()
	{
		// canonical decomposition of the next character
		pendingCount = 0;
		if(position >= length || s[position] == 0)
			return false;

		int units = 1;
		char32_t c = getCodePoint (s, length, position, units);
		position += units;
		pendingCount = decompose (pending, c, false);
		return true;
	}

	void foldSegment ()
	{
		// full case folding and canonical decomposition of the folded characters
		char32_t folded[kMaxSegment] = {};
		int foldedCount = 0;
		for(int i = 0; i < segmentCount; i++)
		{
			char32_t foldings[kMaxFullCaseFoldingLength] = {};
			int foldingCount = foldCaseFull (foldings, segment[i]);
			for(int j = 0; j < foldingCount; j++)
			{
				char32_t decomposed[kMaxDecompositionLength] = {};
				int count = decompose (decomposed, foldings[j], false);
				for(int k = 0; k < count && foldedCount < kMaxSegment; k++)
					folded[foldedCount++] = decomposed[k];
			}
		}

		segmentCount = foldedCount;
		for(int i = 0; i < foldedCount; i++)
			segment[i] = folded[i];
	}

	bool fillSegment ()
	{
		segmentCount = 0;
		segmentIndex = 0;
		while(segmentCount == 0)
		{
			// collect a starter and the following non-starters
			if(pendingCount == 0 && !readNext ())
				return false;
			do
			{
				if(segmentCount > 0 && (getCombiningClass (pending[0]) == 0 || segmentCount + pendingCount > kMaxSegment))
					break;
				for(int i = 0; i < pendingCount; i++)
					segment[segmentCount++] = pending[i];
				pendingCount = 0;
			} while(readNext ());

			reorder (segment, segmentCount);
			if(ignoreCase)
			{
				foldSegment ();
				reorder (segment, segmentCount);
			}

			if(ignoreDiacritics)
			{
				int count = 0;
				for(int i = 0; i < segmentCount; i++)
					if(getCombiningClass (segment[i]) == 0)
						segment[count++] = segment[i];
				segmentCount = count;
			}
		}
		return true;
	}
};

//************************************************************************************************
// Comparison
//************************************************************************************************

/** Compare strings, returns -1, 0 or 1. Ignoring case or diacritics compares canonically equivalent strings
	as equal, ignoring case uses full case folding. Otherwise, code units are compared. */
template <typename CharType>
int compareStrings (const CharType s1[], int length1, const CharType s2[], int length2, bool ignoreCase, bool ignoreDiacritics = false)
{
	// compare ASCII characters directly
	int i = 0;
	for(;; i++)
	{
		char32_t c1 = i < length1 ? getCodeUnit (s1[i]) : 0;
		char32_t c2 = i < length2 ? getCodeUnit (s2[i]) : 0;
		if((ignoreCase || ignoreDiacritics) && (c1 >= 0x80 || c2 >= 0x80))
			break;

		if(ignoreCase)
		{
			c1 = toLowercase (c1);
			c2 = toLowercase (c2);
		}

		if(c1 != c2)
			return c1 < c2 ? -1 : 1;
		if(c1 == 0)
			return 0;
	}

	// the remaining strings start after an ASCII character (a starter), compare them separately
	CaselessIterator<CharType> iterator1 (s1 + i, length1 - i, ignoreCase, ignoreDiacritics);
	CaselessIterator<CharType> iterator2 (s2 + i, length2 - i, ignoreCase, ignoreDiacritics);
	while(true)
	{
		char32_t c1 = iterator1.next ();
		char32_t c2 = iterator2.next ();
		if(c1 != c2)
			return c1 < c2 ? -1 : 1;
		if(c1 == 0)
			return 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Compare null-terminated strings, sequences of decimal digits are compared by their numeric value. */
template <typename CharType>
int compareStringsNumerically (const CharType a[], const CharType b[], bool ignoreCase)
{
	auto digitAt = [] (const CharType* s)
	{
		return isDigit (getCodeUnit (*s));
	};

	auto compareRight = [&] (const CharType* a, const CharType* b)
	{
		int bias = 0;
		for(;; a++, b++)
		{
			bool aIsDigit = digitAt (a);
			bool bIsDigit = digitAt (b);
			if(!aIsDigit && !bIsDigit)
				return bias;
			else if(!aIsDigit)
				return -1;
			else if(!bIsDigit)
				return +1;
			else if(getCodeUnit (*a) < getCodeUnit (*b))
			{
				if(!bias)
					bias = -1;
			}
			else if(getCodeUnit (*a) > getCodeUnit (*b))
			{
				if(!bias)
					bias = +1;
			}
		}
	};

	auto compareLeft = [&] (const CharType* a, const CharType* b)
	{
		for(;; a++, b++)
		{
			bool aIsDigit = digitAt (a);
			bool bIsDigit = digitAt (b);
			if(!aIsDigit && !bIsDigit)
				return 0;
			else if(!aIsDigit)
				return -1;
			else if(!bIsDigit)
				return +1;
			else if(getCodeUnit (*a) < getCodeUnit (*b))
				return -1;
			else if(getCodeUnit (*a) > getCodeUnit (*b))
				return +1;
		}
	};

	int aIndex = 0;
	int bIndex = 0;
	while(true)
	{
		char32_t ca = getCodeUnit (a[aIndex]);
		char32_t cb = getCodeUnit (b[bIndex]);

		while(isWhitespace (ca))
			ca = getCodeUnit (a[++aIndex]);

		while(isWhitespace (cb))
			cb = getCodeUnit (b[++bIndex]);

		if(isDigit (ca) && isDigit (cb))
		{
			bool fractional = ca == '0' || cb == '0';
			int result = fractional ? compareLeft (a + aIndex, b + bIndex) : compareRight (a + aIndex, b + bIndex);
			if(result != 0)
				return result;
		}

		if(ca == 0 && cb == 0)
			return 0;

		if(ignoreCase)
		{
			ca = toUppercase (ca);
			cb = toUppercase (cb);
		}

		if(ca < cb)
			return -1;
		if(ca > cb)
			return +1;

		++aIndex;
		++bIndex;
	}
}

//************************************************************************************************
// Search
/** Ignoring case uses simple case folding. */
//************************************************************************************************

template <typename CharType>
bool matchesAt (const CharType source[], int sourceLength, const CharType value[], int valueLength, bool ignoreCase)
{
	if(valueLength > sourceLength)
		return false;

	for(int i = 0; i < valueLength;)
	{
		if(!ignoreCase)
		{
			if(source[i] != value[i])
				return false;
			i++;
			continue;
		}

		int sourceUnits = 1;
		int valueUnits = 1;
		char32_t c1 = getCodePoint (source, sourceLength, i, sourceUnits);
		char32_t c2 = getCodePoint (value, valueLength, i, valueUnits);
		if(sourceUnits != valueUnits || (c1 != c2 && foldCase (c1) != foldCase (c2)))
			return false;
		i += sourceUnits;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Find the first occurrence of value in source, returns nullptr if not found. */
template <typename CharType>
const CharType* findString (const CharType source[], int sourceLength, const CharType value[], int valueLength, bool ignoreCase)
{
	for(int offset = 0; offset < sourceLength && offset + valueLength <= sourceLength; offset++)
		if(matchesAt (source + offset, sourceLength - offset, value, valueLength, ignoreCase))
			return source + offset;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Find the last occurrence of value in source, returns nullptr if not found. */
template <typename CharType>
const CharType* findStringReverse (const CharType source[], int sourceLength, const CharType value[], int valueLength, bool ignoreCase)
{
	int start = sourceLength - valueLength < sourceLength - 1 ? sourceLength - valueLength : sourceLength - 1;
	for(int offset = start; offset >= 0; offset--)
		if(matchesAt (source + offset, sourceLength - offset, value, valueLength, ignoreCase))
			return source + offset;
	return nullptr;
}

} // namespace CrossPlatformUnicode

#endif // _unicodecompare_h
