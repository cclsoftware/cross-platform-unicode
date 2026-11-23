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
// Filename    : unicodenormalization.h
// Description : Unicode normalization (NFC, NFD, NFKC, NFKD)
//
//************************************************************************************************

#ifndef _unicodenormalization_h
#define _unicodenormalization_h

#include "unicodeproperties.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// Tables
//************************************************************************************************

constexpr int kMaxDecompositionLength = 18; // must match tools/generate-tables.py

struct Decomposition
{
	char32_t key;
	unsigned short offset;				///< offset into kDecompositionData
	unsigned char canonicalLength;		///< UTF-16 code units, 0 for compatibility decompositions only
	unsigned char compatibilityLength;	///< UTF-16 code units, 0 if equal to the canonical decomposition
};

struct Composition
{
	char32_t first;
	char32_t second;
	char32_t composite;
};

namespace Tables {

inline constexpr Decomposition kDecomposition[] =
{
	#include "tables/unicode-decomposition.inc"
};

inline constexpr char16_t kDecompositionData[] =
{
	#include "tables/unicode-decomposition-data.inc"
};

inline constexpr Composition kComposition[] =
{
	#include "tables/unicode-composition.inc"
};

} // namespace Tables

//************************************************************************************************
// Hangul
//************************************************************************************************

namespace Hangul {

constexpr char32_t kSBase = 0xAC00;
constexpr char32_t kLBase = 0x1100;
constexpr char32_t kVBase = 0x1161;
constexpr char32_t kTBase = 0x11A7;
constexpr char32_t kLCount = 19;
constexpr char32_t kVCount = 21;
constexpr char32_t kTCount = 28;
constexpr char32_t kNCount = kVCount * kTCount;
constexpr char32_t kSCount = kLCount * kNCount;

} // namespace Hangul

//************************************************************************************************
// Normalization of code points
//************************************************************************************************

/** Full canonical or compatibility decomposition, returns the number of code points written to result. */
inline int decompose (char32_t result[kMaxDecompositionLength], char32_t c, bool compatibility)
{
	if(c < 0xA0)
	{
		result[0] = c;
		return 1;
	}

	if(c >= Hangul::kSBase && c < Hangul::kSBase + Hangul::kSCount)
	{
		char32_t index = c - Hangul::kSBase;
		result[0] = Hangul::kLBase + index / Hangul::kNCount;
		result[1] = Hangul::kVBase + (index % Hangul::kNCount) / Hangul::kTCount;
		char32_t trailing = index % Hangul::kTCount;
		if(trailing == 0)
			return 2;
		result[2] = Hangul::kTBase + trailing;
		return 3;
	}

	const Decomposition* item = findKey (Tables::kDecomposition, c);
	if(item == nullptr)
	{
		result[0] = c;
		return 1;
	}

	const char16_t* data = Tables::kDecompositionData + item->offset;
	int length = item->canonicalLength;
	if(compatibility && item->compatibilityLength > 0)
	{
		data += item->canonicalLength;
		length = item->compatibilityLength;
	}
	if(length == 0) // compatibility decomposition only
	{
		result[0] = c;
		return 1;
	}

	int count = 0;
	for(int i = 0; i < length;)
	{
		int units = 1;
		result[count++] = getCodePoint (data, length, i, units);
		i += units;
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Canonical ordering: stable sort of non-starter sequences by combining class. */
inline void reorder (char32_t codePoints[], int count)
{
	for(int i = 1; i < count; i++)
	{
		char32_t c = codePoints[i];
		int combiningClass = getCombiningClass (c);
		if(combiningClass == 0)
			continue;

		int j = i;
		for(; j > 0 && getCombiningClass (codePoints[j - 1]) > combiningClass; j--)
			codePoints[j] = codePoints[j - 1];
		codePoints[j] = c;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Primary composite of two code points, 0 if there is none. */
inline char32_t composePair (char32_t first, char32_t second)
{
	using namespace Hangul;

	if(first >= kLBase && first < kLBase + kLCount && second >= kVBase && second < kVBase + kVCount)
		return kSBase + ((first - kLBase) * kVCount + (second - kVBase)) * kTCount;

	if(first >= kSBase && first < kSBase + kSCount && (first - kSBase) % kTCount == 0 &&
	   second > kTBase && second < kTBase + kTCount)
		return first + (second - kTBase);

	constexpr int kCount = sizeof(Tables::kComposition) / sizeof(Tables::kComposition[0]);
	int low = 0;
	int high = kCount - 1;
	while(low <= high)
	{
		int middle = (low + high) / 2;
		const Composition& item = Tables::kComposition[middle];
		if(first < item.first || (first == item.first && second < item.second))
			high = middle - 1;
		else if(first > item.first || second > item.second)
			low = middle + 1;
		else
			return item.composite;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Canonical composition of decomposed and reordered code points in place, returns the new count. */
inline int compose (char32_t codePoints[], int count)
{
	int starterIndex = -1;
	int lastClass = 0;
	int resultCount = 0;
	for(int i = 0; i < count; i++)
	{
		char32_t c = codePoints[i];
		int combiningClass = getCombiningClass (c);
		if(starterIndex >= 0)
		{
			bool blocked = resultCount - 1 != starterIndex && (lastClass == 0 || lastClass >= combiningClass);
			if(!blocked)
				if(char32_t composite = composePair (codePoints[starterIndex], c))
				{
					codePoints[starterIndex] = composite;
					continue;
				}
		}
		if(combiningClass == 0)
			starterIndex = resultCount;
		lastClass = combiningClass;
		codePoints[resultCount++] = c;
	}
	return resultCount;
}

//************************************************************************************************
// Normalization of strings
/** Strings are normalized in two steps without allocating memory:

	int count = getDecompositionLength (s, length, form);
	// allocate count code points
	count = normalize (codePoints, s, length, form);
	int units = encodeUTF16 (nullptr, codePoints, count);
	// allocate units code units
	encodeUTF16 (dest, codePoints, count);
*/
//************************************************************************************************

enum NormalizationForm
{
	kNormalizationD,	///< canonical decomposition
	kNormalizationC,	///< canonical decomposition followed by canonical composition
	kNormalizationKD,	///< compatibility decomposition
	kNormalizationKC	///< compatibility decomposition followed by canonical composition
};

constexpr bool isComposed (NormalizationForm form)
{
	return form == kNormalizationC || form == kNormalizationKC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr bool isCompatibility (NormalizationForm form)
{
	return form == kNormalizationKD || form == kNormalizationKC;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Fast check for strings that are not changed by the normalization form (e.g. ASCII). */
template <typename CharType>
bool isTriviallyNormalized (const CharType s[], int length, NormalizationForm form)
{
	// characters below this limit are not changed by the normalization form
	char32_t limit = isCompatibility (form) ? 0xA0 : isComposed (form) ? 0x300 : 0xC0;
	for(int i = 0; i < length; i++)
		if(getCodeUnit (s[i]) >= limit)
			return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Number of code points of the decomposed string, the required size for normalize (). */
template <typename CharType>
int getDecompositionLength (const CharType s[], int length, NormalizationForm form)
{
	int count = 0;
	for(int i = 0; i < length;)
	{
		int units = 1;
		char32_t decomposed[kMaxDecompositionLength] = {};
		count += decompose (decomposed, getCodePoint (s, length, i, units), isCompatibility (form));
		i += units;
	}
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Normalize a string to code points, codePoints must have room for getDecompositionLength () items.
	Returns the number of code points. */
template <typename CharType>
int normalize (char32_t codePoints[], const CharType s[], int length, NormalizationForm form)
{
	int count = 0;
	for(int i = 0; i < length;)
	{
		int units = 1;
		count += decompose (codePoints + count, getCodePoint (s, length, i, units), isCompatibility (form));
		i += units;
	}

	reorder (codePoints, count);
	if(isComposed (form))
		count = compose (codePoints, count);
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Encode code points as UTF-16 (without null terminator), returns the number of code units.
	Without buffer, only the required length is calculated. */
template <typename CharType>
int encodeUTF16 (CharType dest[], const char32_t codePoints[], int count)
{
	int length = 0;
	for(int i = 0; i < count; i++)
	{
		if(dest)
			writeCodePoint (dest + length, codePoints[i]);
		length += getUTF16Length (codePoints[i]);
	}
	return length;
}

} // namespace CrossPlatformUnicode

#endif // _unicodenormalization_h
