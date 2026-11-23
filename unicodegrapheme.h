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
// Filename    : unicodegrapheme.h
// Description : Grapheme cluster boundaries (UAX #29)
//
//************************************************************************************************

#ifndef _unicodegrapheme_h
#define _unicodegrapheme_h

#include "unicodecodec.h"

namespace CrossPlatformUnicode {

//************************************************************************************************
// Tables
//************************************************************************************************

/** Grapheme_Cluster_Break property values, Extended_Pictographic is an additional class.
	The order must match tools/generate-tables.py. */
enum GraphemeBreakClass
{
	kGraphemeBreakOther,
	kGraphemeBreakCR,
	kGraphemeBreakLF,
	kGraphemeBreakControl,
	kGraphemeBreakExtend,
	kGraphemeBreakZWJ,
	kGraphemeBreakRegionalIndicator,
	kGraphemeBreakPrepend,
	kGraphemeBreakSpacingMark,
	kGraphemeBreakL,
	kGraphemeBreakV,
	kGraphemeBreakT,
	kGraphemeBreakLV,
	kGraphemeBreakLVT,
	kGraphemeBreakExtendedPictographic
};

struct GraphemeBreakRange
{
	char32_t first;
	char32_t last;
	unsigned char graphemeBreakClass;
};

namespace Tables {

inline constexpr GraphemeBreakRange kGraphemeBreak[] =
{
	#include "tables/unicode-grapheme-break.inc"
};

} // namespace Tables

//////////////////////////////////////////////////////////////////////////////////////////////////

inline GraphemeBreakClass getGraphemeBreakClass (char32_t c)
{
	if(c >= 0x20 && c < 0x7F)
		return kGraphemeBreakOther;
	const GraphemeBreakRange* range = findRange (Tables::kGraphemeBreak, c);
	return range ? GraphemeBreakClass (range->graphemeBreakClass) : kGraphemeBreakOther;
}

//************************************************************************************************
// GraphemeIterator
/** Iterates extended grapheme cluster boundaries (UAX #29) of a UTF-16 string.
	Emoji with modifiers, ZWJ sequences, flags and combining marks are atomic units.
	The position is given in code units. The Indic conjunct rule (GB9c) is not implemented. */
//************************************************************************************************

template <typename CharType>
class GraphemeIterator
{
public:
	GraphemeIterator (const CharType text[], int length, int position = 0)
	: codePoints (text, length, position)
	{}

	int getPosition () const { return codePoints.getPosition (); }
	void setPosition (int position) { codePoints.setPosition (position); }
	bool isStart () const { return codePoints.isStart (); }
	bool isEnd () const { return codePoints.isEnd (); }

	/** Move to the next grapheme cluster boundary, returns the new position. */
	int next ()
	{
		if(codePoints.isEnd ())
			return codePoints.getPosition ();

		GraphemeBreakClass previousClass = getGraphemeBreakClass (codePoints.next ());
		State state = kStateNormal;
		if(previousClass == kGraphemeBreakRegionalIndicator)
			state = kStateRegionalIndicator;
		else if(previousClass == kGraphemeBreakExtendedPictographic)
			state = kStatePictographicExtend;

		while(!codePoints.isEnd ())
		{
			GraphemeBreakClass currentClass = getGraphemeBreakClass (codePoints.peekNext ());
			if(isBreak (previousClass, currentClass, state))
				break;

			updateState (state, currentClass);
			codePoints.next ();
			previousClass = currentClass;
		}

		return codePoints.getPosition ();
	}

	/** Move to the previous grapheme cluster boundary, returns the new position. */
	int previous ()
	{
		int position = codePoints.getPosition ();
		int length = codePoints.getLength ();
		if(position > length)
			position = length;
		if(position <= 0)
			return position;

		// back up to a position that is a boundary regardless of the preceding context
		codePoints.setPosition (position);
		GraphemeBreakClass currentClass = getGraphemeBreakClass (codePoints.previous ());
		while(!codePoints.isStart ())
		{
			GraphemeBreakClass previousClass = getGraphemeBreakClass (codePoints.peekPrevious ());
			if(isContextFreeBreak (previousClass, currentClass))
				break;

			codePoints.previous ();
			currentClass = previousClass;
		}

		// scan forward from there to the last boundary before the position
		int lastBoundary = codePoints.getPosition ();
		while(codePoints.getPosition () < position)
		{
			int nextBoundary = next ();
			if(nextBoundary >= position)
				break;
			lastBoundary = nextBoundary;
		}

		codePoints.setPosition (lastBoundary);
		return lastBoundary;
	}

protected:
	enum State
	{
		kStateNormal,
		kStatePictographicExtend,		///< after Extended_Pictographic Extend*, for GB11
		kStatePictographicExtendZWJ,	///< after Extended_Pictographic Extend* ZWJ
		kStateRegionalIndicator			///< after an odd number of Regional_Indicator, for GB12/GB13
	};

	UTF16CodePointIterator<CharType> codePoints;

	static bool isBreak (GraphemeBreakClass previous, GraphemeBreakClass current, State state)
	{
		// GB3: CR x LF
		if(previous == kGraphemeBreakCR && current == kGraphemeBreakLF)
			return false;

		// GB4: (Control | CR | LF) break after
		if(previous == kGraphemeBreakControl || previous == kGraphemeBreakCR || previous == kGraphemeBreakLF)
			return true;

		// GB5: break before (Control | CR | LF)
		if(current == kGraphemeBreakControl || current == kGraphemeBreakCR || current == kGraphemeBreakLF)
			return true;

		// GB6: L x (L | V | LV | LVT)
		if(previous == kGraphemeBreakL && (current == kGraphemeBreakL || current == kGraphemeBreakV || current == kGraphemeBreakLV || current == kGraphemeBreakLVT))
			return false;

		// GB7: (LV | V) x (V | T)
		if((previous == kGraphemeBreakLV || previous == kGraphemeBreakV) && (current == kGraphemeBreakV || current == kGraphemeBreakT))
			return false;

		// GB8: (LVT | T) x T
		if((previous == kGraphemeBreakLVT || previous == kGraphemeBreakT) && current == kGraphemeBreakT)
			return false;

		// GB9: x (Extend | ZWJ)
		if(current == kGraphemeBreakExtend || current == kGraphemeBreakZWJ)
			return false;

		// GB9a: x SpacingMark
		if(current == kGraphemeBreakSpacingMark)
			return false;

		// GB9b: Prepend x
		if(previous == kGraphemeBreakPrepend)
			return false;

		// GB11: Extended_Pictographic Extend* ZWJ x Extended_Pictographic
		if(state == kStatePictographicExtendZWJ && current == kGraphemeBreakExtendedPictographic)
			return false;

		// GB12, GB13: pairs of Regional_Indicator
		if(state == kStateRegionalIndicator && current == kGraphemeBreakRegionalIndicator)
			return false;

		// GB999: break everywhere else
		return true;
	}

	static bool isContextFreeBreak (GraphemeBreakClass previous, GraphemeBreakClass current)
	{
		// GB11, GB12 and GB13 depend on the context before previous
		if(previous == kGraphemeBreakZWJ && current == kGraphemeBreakExtendedPictographic)
			return false;
		if(previous == kGraphemeBreakRegionalIndicator && current == kGraphemeBreakRegionalIndicator)
			return false;

		return isBreak (previous, current, kStateNormal);
	}

	static void updateState (State& state, GraphemeBreakClass current)
	{
		if(current == kGraphemeBreakRegionalIndicator)
			state = state == kStateRegionalIndicator ? kStateNormal : kStateRegionalIndicator;
		else if(current == kGraphemeBreakExtendedPictographic)
			state = kStatePictographicExtend;
		else if(current == kGraphemeBreakZWJ)
			state = state == kStatePictographicExtend ? kStatePictographicExtendZWJ : kStateNormal;
		else if(current != kGraphemeBreakExtend || state != kStatePictographicExtend)
			state = kStateNormal;
	}
};

} // namespace CrossPlatformUnicode

#endif // _unicodegrapheme_h
