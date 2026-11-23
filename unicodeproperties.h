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
// Filename    : unicodeproperties.h
// Description : Character properties
//
//************************************************************************************************

#ifndef _unicodeproperties_h
#define _unicodeproperties_h

#include "unicodebase.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// Tables
//************************************************************************************************

struct CombiningClassRange
{
	char32_t first;
	char32_t last;
	unsigned char combiningClass;
};

namespace Tables {

inline constexpr Range kAlphabetic[] =
{
	#include "tables/unicode-alphabetic.inc"
};

inline constexpr Range kLowercase[] =
{
	#include "tables/unicode-lowercase.inc"
};

inline constexpr Range kUppercase[] =
{
	#include "tables/unicode-uppercase.inc"
};

inline constexpr Range kCased[] =
{
	#include "tables/unicode-cased.inc"
};

inline constexpr Range kCaseIgnorable[] =
{
	#include "tables/unicode-case-ignorable.inc"
};

inline constexpr Range kDecimal[] =
{
	#include "tables/unicode-decimal.inc"
};

inline constexpr char16_t kWhitespace[] =
{
	#include "tables/charset-whitespace.inc"
};

inline constexpr CombiningClassRange kCombiningClass[] =
{
	#include "tables/unicode-combining-class.inc"
};

inline constexpr Range kEastAsianWide[] =
{
	#include "tables/unicode-east-asian-wide.inc"
};

} // namespace Tables

//************************************************************************************************
// Character properties
/** Properties follow the Unicode Character Database, see tools/generate-tables.py. */
//************************************************************************************************

inline bool isUppercase (char32_t c)
{
	if(c < 0x80)
		return c >= 'A' && c <= 'Z';
	return findRange (Tables::kUppercase, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isLowercase (char32_t c)
{
	if(c < 0x80)
		return c >= 'a' && c <= 'z';
	return findRange (Tables::kLowercase, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isAlpha (char32_t c)
{
	if(c < 0x80)
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	return findRange (Tables::kAlphabetic, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Decimal digits of all scripts. */
inline bool isNumeric (char32_t c)
{
	if(c < 0x80)
		return c >= '0' && c <= '9';
	return findRange (Tables::kDecimal, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isAlphaNumeric (char32_t c)
{
	return isAlpha (c) || isNumeric (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Decimal digits 0..9 only. */
constexpr bool isDigit (char32_t c)
{
	return c >= '0' && c <= '9';
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isWhitespace (char32_t c)
{
	return findValue (Tables::kWhitespace, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isCased (char32_t c)
{
	if(c < 0x80)
		return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
	return findRange (Tables::kCased, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool isCaseIgnorable (char32_t c)
{
	return findRange (Tables::kCaseIgnorable, c) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Canonical combining class, 0 for starters. */
inline int getCombiningClass (char32_t c)
{
	if(c < 0x300)
		return 0;
	const CombiningClassRange* range = findRange (Tables::kCombiningClass, c);
	return range ? range->combiningClass : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** East_Asian_Width Wide or Fullwidth, i.e. the character takes two columns in East Asian typography. */
inline bool isFullWidth (char32_t c)
{
	if(c < 0x1100)
		return false;
	return findRange (Tables::kEastAsianWide, c) != nullptr;
}

} // namespace CrossPlatformUnicode

#endif // _unicodeproperties_h
