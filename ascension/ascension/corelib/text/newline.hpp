/**
 * @file newline.hpp
 * Defines @c Newline class and some related functions.
 * @author exeal
 * @date 2003-2006 was EditDoc.h
 * @date 2006-2011 was document.hpp
 * @date 2011-05-02 separated from document.hpp
 * @date 2011-2013
 * @see kernel/document.hpp
 */

#ifndef ASCENSION_NEWLINE_HPP
#define ASCENSION_NEWLINE_HPP
#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/memory.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/character.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <boost/operators.hpp>	// boost.equality_comparable
#include <boost/optional.hpp>
#include <boost/range/algorithm/find_first_of.hpp>

namespace ascension {
	namespace text {
		/**
		 * Value represents a newline in document. @c #USE_INTRINSIC_VALUE and @c #USE_DOCUMENT_INPUT are special
		 * values indicate how to interpret newlines during any text I/O.
		 * @see kernel#Document, ASCENSION_DEFAULT_NEWLINE, NEWLINE_CHARACTERS
		 */
		class Newline : public FastArenaObject<Newline>, private boost::equality_comparable<Newline> {
		public:
			static const Newline LINE_FEED;
			static const Newline CARRIAGE_RETURN;
			static const Newline CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED;
			static const Newline NEXT_LINE;
			static const Newline LINE_SEPARATOR;
			static const Newline PARAGRAPH_SEPARATOR;
			static const Newline USE_INTRINSIC_VALUE;
			static const Newline USE_DOCUMENT_INPUT;
		public:
			/**
			 * Default-constructor creates a newline which refers to platform-native NLF.
			 * @see ASCENSION_DEFAULT_NEWLINE
			 */
			Newline() BOOST_NOEXCEPT : value_(ASCENSION_DEFAULT_NEWLINE.value_) {}
			/// Copy-constructor.
			Newline(const Newline& other) BOOST_NOEXCEPT : value_(other.value_) {}
			/// Copy-assignment operator.
			Newline& operator=(const Newline& other) BOOST_NOEXCEPT {
				return (value_ = other.value_), *this;
			}
			/// Equality operator.
			bool operator==(const Newline& other) const BOOST_NOEXCEPT {return value_ == other.value_;}
			/**
			 * Returns a string represents the newline.
			 * @throw std#logic_error The newline is not literal
			 */
			String asString() const {
				if(!isLiteral())
					throw std::logic_error("The newline is not literal.");
				String s;
				s.reserve(8);
				auto inserter(std::back_inserter(s));
				if(value_ < 0x10000u)
					text::utf::encode(static_cast<Char>(value_ & 0xffffu), inserter);
				else {
					text::utf::encode(text::CARRIAGE_RETURN, inserter);
					text::utf::encode(text::LINE_FEED, inserter);
				}
				return s;
			}
			/// Returns @c true if the given newline value is a literal.
			bool isLiteral() const BOOST_NOEXCEPT {
				return (value_ & 0x80000000u) == 0;
			}

		private:
			Newline(std::uint32_t value) BOOST_NOEXCEPT;
			std::uint32_t value_;
		};
		
		/**
		 * Returns the number of lines in the specified UTF-16 or UTF-32 character sequence.
		 * This method is exception-neutral (does not throw if @a ForwardIterator does not).
		 * @tparam ForwardIterator The type of @a first and @a last
		 * @param first The beginning of the character sequence
		 * @param last The end of the character sequence
		 * @param emptyCase If the character sequence is empty, this function returns this value
		 * @return The number of lines
		 */
		template<typename ForwardIterator>
		inline Index calculateNumberOfLines(ForwardIterator first, ForwardIterator last, Index emptyCase = 1) {
			static_assert(CodeUnitSizeOf<ForwardIterator>::value == 2, "ForwardIterator should addresses UTF-16 character sequence.");
			if(first == last)
				return emptyCase;
			Index lines = 1;
			while(true) {
				first = boost::find_first_of(boost::make_iterator_range(first, last), NEWLINE_CHARACTERS);
				if(first == last)
					break;
				++lines;
				if(*first == CARRIAGE_RETURN) {
					if(++first == last)
						break;
					else if(*first == LINE_FEED)
						++first;
				} else
					++first;
			}
			return lines;
		}

		/**
		 * Returns the number of lines in the specified UTF-16 or UTF-32 text.
		 * @tparam SinglePassReadableRange The type of @a range
		 * @param range The character range
		 * @param emptyCase If the character sequence is empty, this function returns this value
		 * @return The number of lines
		 */
		template<typename SinglePassReadableRange>
		inline Index calculateNumberOfLines(const SinglePassReadableRange& range, Index emptyCase = 1) {
			return calculateNumberOfLines(boost::const_begin(range), boost::const_end(range), emptyCase);
		}

		/**
		 * Returns the newline at the beginning of the specified buffer.
		 * @tparam ForwardIterator The type of @a first and @a last
		 * @param first The beginning of the buffer
		 * @param last The end of the buffer. This should greater than @a first
		 * @return The newline or @c boost#none if the beginning of the buffer is not newline
		 */
		template<typename ForwardIterator>
		inline boost::optional<Newline> eatNewline(ForwardIterator first, ForwardIterator last) {
			static_assert(CodeUnitSizeOf<ForwardIterator>::value == 2, "ForwardIterator should addresses UTF-16 character sequence.");
			switch(*first) {
				case LINE_FEED:
					return boost::make_optional(Newline::LINE_FEED);
				case CARRIAGE_RETURN:
					return boost::make_optional((++first != last && *first == LINE_FEED) ?
						Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED : Newline::CARRIAGE_RETURN);
				case NEXT_LINE:
					return boost::make_optional(Newline::NEXT_LINE);
				case LINE_SEPARATOR:
					return boost::make_optional(Newline::LINE_SEPARATOR);
				case PARAGRAPH_SEPARATOR:
					return boost::make_optional(Newline::PARAGRAPH_SEPARATOR);
				default:
					return boost::none;
			}
		}

		/**
		 * Returns the newline at the beginning of the specified range.
		 * @tparam SinglePassReadableRange The type of @a range
		 * @param range The character range. This can't be empty
		 * @return The newline or @c boost#none if the beginning of the buffer is not newline
		 */
		template<typename SinglePassReadableRange>
		inline boost::optional<Newline> eatNewline(const SinglePassReadableRange& range) {
			return eatNewline(boost::const_begin(range), boost::const_end(range));
		}
	}
} // namespace ascension.text

#endif // !ASCENSION_NEWLINE_HPP
