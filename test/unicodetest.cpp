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
// Filename    : test/unicodetest.cpp
// Description : Standalone test, intentionally without any library (not even the C++ standard library)
//
//************************************************************************************************

#include "../unicodecompare.h"
#include "../codepageascii.h"
#include "../codepageisolatin1.h"
#include "../codepagewindowslatin1.h"
#include "../codepagemacroman.h"
#include "../codepagedoslatinus.h"
#include "../codepageshiftjis.h"
#include "../unicodegrapheme.h"

using namespace CrossPlatformUnicode;

static int failures = 0;

#define TEST_ASSERT(condition) \
	if(!(condition)) { failures++; reportFailure (#condition, __LINE__); }

extern "C" int puts (const char* s);

//////////////////////////////////////////////////////////////////////////////////////////////////

static void reportFailure (const char* condition, int line)
{
	char lineText[16] = {};
	int length = 0;
	char digits[16] = {};
	int count = 0;
	do
	{
		digits[count++] = char('0' + line % 10);
		line /= 10;
	} while(line > 0);
	while(count > 0)
		lineText[length++] = digits[--count];
	puts ("FAILED:");
	puts (condition);
	puts (lineText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
static bool equals (const CharType a[], int lengthA, const char16_t b[])
{
	int lengthB = getLength (b);
	if(lengthA != lengthB)
		return false;
	for(int i = 0; i < lengthA; i++)
		if(getCodeUnit (a[i]) != b[i])
			return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
static int normalizeString (CharType result[], int resultSize, const char16_t source[], NormalizationForm form)
{
	char32_t codePoints[64] = {};
	int length = getLength (source);
	if(getDecompositionLength (source, length, form) > 64)
		return -1;
	int count = normalize (codePoints, source, length, form);
	if(encodeUTF16<CharType> (nullptr, codePoints, count) > resultSize)
		return -1;
	return encodeUTF16 (result, codePoints, count);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testProperties ()
{
	TEST_ASSERT (isAlpha ('a') && isAlpha (0x00E4) && isAlpha (0x4E00) && !isAlpha ('1'))
	TEST_ASSERT (isUppercase (0x00C4) && !isUppercase (0x00E4))
	TEST_ASSERT (isLowercase (0x00E4) && !isLowercase (0x00C4))
	TEST_ASSERT (isNumeric (0x0663) && !isNumeric ('a')) // Arabic-Indic digit three
	TEST_ASSERT (isWhitespace (0x3000) && isWhitespace (' ') && !isWhitespace ('x'))
	TEST_ASSERT (getCombiningClass (0x0301) == 230 && getCombiningClass ('a') == 0)
	TEST_ASSERT (isAlpha (0x10400)) // Deseret, outside of the BMP
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testCase ()
{
	TEST_ASSERT (toUppercase (0x00E4) == 0x00C4)
	TEST_ASSERT (toLowercase (0x0130) == 0x0069)
	TEST_ASSERT (toTitlecase (0x01C6) == 0x01C5)
	TEST_ASSERT (toUppercase (0x10428) == 0x10400)

	char16_t sigma[] = u"\u039F\u0394\u039F\u03A3 \u03A3";
	toLowercase (sigma, getLength (sigma));
	TEST_ASSERT (equals (sigma, getLength (sigma), u"\u03BF\u03B4\u03BF\u03C2 \u03C3"))

	unsigned short surrogates[] = {0xD801, 0xDC28, 'a', 0};
	toUppercase (surrogates, 3);
	TEST_ASSERT (surrogates[0] == 0xD801 && surrogates[1] == 0xDC00 && surrogates[2] == 'A')

	char16_t words[] = u"don't stop \u00E9t\u00E9";
	capitalize (words, getLength (words));
	TEST_ASSERT (equals (words, getLength (words), u"Don't Stop \u00C9t\u00E9"))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testNormalization ()
{
	char16_t result[64] = {};
	int length = normalizeString (result, 64, u"A\u030A", kNormalizationC);
	TEST_ASSERT (equals (result, length, u"\u00C5"))

	length = normalizeString (result, 64, u"\u00C5", kNormalizationD);
	TEST_ASSERT (equals (result, length, u"A\u030A"))

	length = normalizeString (result, 64, u"\uFB01", kNormalizationKC);
	TEST_ASSERT (equals (result, length, u"fi"))

	length = normalizeString (result, 64, u"\uAC01", kNormalizationD);
	TEST_ASSERT (equals (result, length, u"\u1100\u1161\u11A8"))

	length = normalizeString (result, 64, u"\u1100\u1161\u11A8", kNormalizationC);
	TEST_ASSERT (equals (result, length, u"\uAC01"))

	length = normalizeString (result, 64, u"a\u0323\u0302", kNormalizationC); // reordering not needed, both compose
	TEST_ASSERT (equals (result, length, u"\u1EAD"))

	length = normalizeString (result, 64, u"a\u0302\u0323", kNormalizationC); // canonical reordering
	TEST_ASSERT (equals (result, length, u"\u1EAD"))

	TEST_ASSERT (isTriviallyNormalized (u"abc", 3, kNormalizationC))
	TEST_ASSERT (!isTriviallyNormalized (u"\u00C5", 1, kNormalizationD))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testCompare ()
{
	const char16_t* a = u"Stra\u00DFe";
	const char16_t* b = u"STRASSE";
	TEST_ASSERT (compareStrings (a, getLength (a), b, getLength (b), true) == 0)
	TEST_ASSERT (compareStrings (a, getLength (a), b, getLength (b), false) != 0)

	const char16_t* composed = u"\u00C5ngstr\u00F6m";
	const char16_t* decomposed = u"A\u030Angstro\u0308m";
	TEST_ASSERT (compareStrings (composed, getLength (composed), decomposed, getLength (decomposed), true) == 0)
	TEST_ASSERT (compareStrings (composed, getLength (composed), decomposed, getLength (decomposed), false) != 0)

	const char16_t* plain = u"Angstrom";
	TEST_ASSERT (compareStrings (composed, getLength (composed), plain, getLength (plain), false, true) == 0)

	TEST_ASSERT (compareStringsNumerically (u"file9", u"file10", false) < 0)
	TEST_ASSERT (compareStringsNumerically (u"File10", u"file10", true) == 0)

	const char16_t* source = u"Hello W\u00F6rld, w\u00F6rld";
	const char16_t* value = u"W\u00D6RLD";
	const char16_t* found = findString (source, getLength (source), value, getLength (value), true);
	TEST_ASSERT (found == source + 6)
	found = findStringReverse (source, getLength (source), value, getLength (value), true);
	TEST_ASSERT (found == source + 13)
	TEST_ASSERT (findString (source, getLength (source), value, getLength (value), false) == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testCodepages ()
{
	const char16_t* text = u"\u00C4\u20AC\U0001F600";
	int length = getLength (text);
	char buffer[16] = {};

	TEST_ASSERT (encodeASCII (nullptr, 0, text, length) == 4)
	TEST_ASSERT (encodeASCII (buffer, 16, text, length) == 4)
	TEST_ASSERT (buffer[0] == 'A' && buffer[2] == '?' && buffer[3] == 0)

	TEST_ASSERT (encodeISOLatin1 (buffer, 16, text, length) == 4)
	TEST_ASSERT (static_cast<unsigned char> (buffer[0]) == 0xC4)

	TEST_ASSERT (encodeWindowsLatin1 (buffer, 16, text, length) == 4)
	TEST_ASSERT (static_cast<unsigned char> (buffer[1]) == 0x80)

	char16_t decoded[16] = {};
	TEST_ASSERT (decodeWindowsLatin1 (decoded, 16, "\x80\xC4", 2) == 3)
	TEST_ASSERT (decoded[0] == 0x20AC && decoded[1] == 0x00C4 && decoded[2] == 0)

	TEST_ASSERT (encodeMacRoman (buffer, 16, u"\u00C4", 1) == 2 && static_cast<unsigned char> (buffer[0]) == 0x80)
	TEST_ASSERT (decodeMacRoman (decoded, 16, "\x80", 1) == 2 && decoded[0] == 0x00C4)

	TEST_ASSERT (encodeDOSLatinUS (buffer, 16, u"\u00C7", 1) == 2 && static_cast<unsigned char> (buffer[0]) == 0x80)
	TEST_ASSERT (decodeDOSLatinUS (decoded, 16, "\x80", 1) == 2 && decoded[0] == 0x00C7)

	const char16_t* japanese = u"\u65E5\u672C\uFF71a";
	TEST_ASSERT (encodeShiftJIS (buffer, 16, japanese, 4) == 7)
	TEST_ASSERT (static_cast<unsigned char> (buffer[0]) == 0x93 && static_cast<unsigned char> (buffer[1]) == 0xFA)
	TEST_ASSERT (decodeShiftJIS (decoded, 16, buffer, 6) == 5)
	TEST_ASSERT (equals (decoded, 4, japanese))

	// buffer too small
	TEST_ASSERT (encodeASCII (buffer, 3, u"abcd", 4) == 3 && buffer[2] == 0)
	TEST_ASSERT (decodeASCII (decoded, 3, "abcd", 4) == 3 && decoded[2] == 'c') // like Core::Text::UTF16Writer, no room for the terminator
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testUTF ()
{
	// U+00E4, U+20AC, U+1F600
	const char* utf8 = "a\xC3\xA4\xE2\x82\xAC\xF0\x9F\x98\x80";
	char16_t decoded[16] = {};
	TEST_ASSERT (decodeUTF8 (decoded, 16, utf8, 10) == 6)
	TEST_ASSERT (decoded[0] == 'a' && decoded[1] == 0x00E4 && decoded[2] == 0x20AC && decoded[3] == 0xD83D && decoded[4] == 0xDE00 && decoded[5] == 0)
	TEST_ASSERT (decodeUTF8<char16_t> (nullptr, 0, utf8, 10) == 6)

	char encoded[16] = {};
	TEST_ASSERT (encodeUTF8 (encoded, 16, decoded, 5) == 11)
	for(int i = 0; i < 10; i++)
		TEST_ASSERT (encoded[i] == utf8[i])
	TEST_ASSERT (encoded[10] == 0)

	char32_t c = 0;
	const unsigned char overlong[] = {0xC0, 0x80};
	TEST_ASSERT (decodeUTF8 (c, overlong, 2) == kIllegalInput)
	const unsigned char truncated[] = {0xE2, 0x82};
	TEST_ASSERT (decodeUTF8 (c, truncated, 2) == kBufferTooSmall)

	unsigned char bytes[4] = {};
	TEST_ASSERT (encodeUTF16 (0x1F600, bytes, 4, true) == 4 && bytes[0] == 0xD8 && bytes[1] == 0x3D && bytes[2] == 0xDE && bytes[3] == 0x00)
	TEST_ASSERT (decodeUTF16 (c, bytes, 4, true) == 4 && c == 0x1F600)
	TEST_ASSERT (decodeUTF16 (c, bytes, 2, true) == kBufferTooSmall)
	TEST_ASSERT (encodeUTF16 (0xD800, bytes, 4, false) == kIllegalInput)

	// unpaired surrogate stops encoding like Core did
	const char16_t unpaired[] = {'a', 0xDC00, 'b', 0};
	TEST_ASSERT (encodeUTF8 (encoded, 16, unpaired, 3) == 2)

	const char16_t* text = u"a\U0001F600b";
	UTF16CodePointIterator<char16_t> iterator (text, 4);
	TEST_ASSERT (iterator.next () == 'a' && iterator.next () == 0x1F600 && iterator.getPosition () == 3)
	TEST_ASSERT (iterator.previous () == 0x1F600 && iterator.getPosition () == 1)
	TEST_ASSERT (iterator.peekPrevious () == 'a')
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename CharType>
static int countGraphemes (const CharType text[], int length)
{
	GraphemeIterator<CharType> iterator (text, length);
	int count = 0;
	while(!iterator.isEnd ())
	{
		iterator.next ();
		count++;
	}

	// backwards must find the same boundaries
	int backwardCount = 0;
	while(!iterator.isStart ())
	{
		iterator.previous ();
		backwardCount++;
	}
	return count == backwardCount ? count : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void testGraphemes ()
{
	TEST_ASSERT (countGraphemes (u"abc", 3) == 3)
	TEST_ASSERT (countGraphemes (u"a\u0301e", 3) == 2)							// combining mark
	TEST_ASSERT (countGraphemes (u"\r\n", 2) == 1)								// CR LF
	TEST_ASSERT (countGraphemes (u"\U0001F44D\U0001F3FD", 4) == 1)				// emoji with skin tone
	TEST_ASSERT (countGraphemes (u"\U0001F468\u200D\U0001F469\u200D\U0001F467", 8) == 1) // ZWJ sequence
	TEST_ASSERT (countGraphemes (u"\U0001F1E9\U0001F1EA\U0001F1EB", 6) == 2)	// flag and a single regional indicator
	TEST_ASSERT (countGraphemes (u"\u1100\u1161\u11A8", 3) == 1)				// Hangul jamo
	TEST_ASSERT (countGraphemes (u"\u0915\u093F", 2) == 1)						// Devanagari spacing mark
	TEST_ASSERT (countGraphemes (u"\u3000\u3001", 2) == 2)						// CJK punctuation is not pictographic
	TEST_ASSERT (countGraphemes (u"\u2764\uFE0F", 2) == 1)						// variation selector

	TEST_ASSERT (getGraphemeBreakClass (0x1F600) == kGraphemeBreakExtendedPictographic)
	TEST_ASSERT (getGraphemeBreakClass (0x0301) == kGraphemeBreakExtend)
	TEST_ASSERT (getGraphemeBreakClass (0xAC01) == kGraphemeBreakLVT)

	TEST_ASSERT (isFullWidth (0x4E00) && isFullWidth (0xFF21) && isFullWidth (0x1F600))
	TEST_ASSERT (!isFullWidth ('A') && !isFullWidth (0xFF71)) // halfwidth katakana
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int main ()
{
	testProperties ();
	testCase ();
	testNormalization ();
	testCompare ();
	testCodepages ();
	testUTF ();
	testGraphemes ();

	puts (failures == 0 ? "All tests passed." : "Tests failed.");
	return failures == 0 ? 0 : 1;
}
