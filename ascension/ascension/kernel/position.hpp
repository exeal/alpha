/**
 * @file position.hpp
 * @author exeal
 * @date 2011-01-21 separated from document.hpp
 * @date 2011-2013
 */

#ifndef ASCENSION_POSITION_HPP
#define ASCENSION_POSITION_HPP
#include <ascension/corelib/basic-types.hpp>	// Index
#include <ascension/corelib/memory.hpp>			// FastArenaObject
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <locale>	// std.ctype, std.use_facet
#include <stdexcept>
#include <ostream>	// std.basic_ostream
#include <sstream>	// std.ostringstream
#include <string>

namespace ascension {
	namespace kernel {
		/**
		 * @c Position represents a position in the document by a line number and an offset in the line.
		 * @note This class is not intended to be subclassed.
		 * @see Region, Point, viewer#VisualPoint, viewer#Caret
		 */
		class Position : public FastArenaObject<Position>, private boost::totally_ordered<Position> {
		public:
			/// Line number. Zero means that the position is the first line in the document.
			Index line;
			/// Offset in the line (column) in UTF-16 code units.
			/// Zero means that the position is the beginning of the line.
			Index offsetInLine;
		public:
			/// Default constructor does not initialize the members at all.
			Position() BOOST_NOEXCEPT {}
			/**
			 * Constructor.
			 * @param line The initial value for @c #line
			 * @param offsetInLine The initial value for @c #offsetInLine
			 */
			Position(Index line, Index offsetInLine) BOOST_NOEXCEPT : line(line), offsetInLine(offsetInLine) {}
			/// Equality operator.
			bool operator==(const Position& other) const BOOST_NOEXCEPT {
				return line == other.line && offsetInLine == other.offsetInLine;
			}
			/// Relational operator.
			bool operator<(const Position& other) const BOOST_NOEXCEPT {
				return line < other.line || (line == other.line && offsetInLine < other.offsetInLine);
			}
			/// Returns the result of @c Position(line, 0).
			static BOOST_CONSTEXPR Position bol(Index line) BOOST_NOEXCEPT {
				return Position(line, 0);
			}
			/// Returns the result of @c #bol(p.line).
			static BOOST_CONSTEXPR Position bol(const Position& p) BOOST_NOEXCEPT {
				return bol(p.line);
			}
			/// Returns the result of @c Position(0, 0).
			static BOOST_CONSTEXPR Position zero() BOOST_NOEXCEPT {
				return Position(0, 0);
			}
		};

		/**
		 * Writes a @c Position into the output stream.
		 * @tparam Element The element type of the output stream
		 * @tparam Traits The character traits of the output stream
		 * @param out The output stream
		 * @param value The position to write
		 * @return @a out
		 */
		template<typename Element, typename Traits>
		inline std::basic_ostream<Element, Traits>& operator<<(
				std::basic_ostream<Element, Traits>& out, const Position& value) {
			const std::ctype<Element>& ct = std::use_facet<std::ctype<Element>>(out.getloc());
			std::basic_ostringstream<Element, Traits> s;
			s.flags(out.flags());
			s.imbue(out.getloc());
			s.precision(out.precision());
			s << ct.widen('(') << value.line << ct.widen(',') << value.offsetInLine << ct.widen(')');
			return out << s.str().c_str();
		}

		/**
		 * Thrown when the specified line or character position is outside of the document.
		 * @see BadRegionException
		 */
		class BadPositionException : public std::invalid_argument {
		public:
			/// Default constructor.
			BadPositionException() BOOST_NOEXCEPT : std::invalid_argument(
				"the position <not-initialized> is outside of the document or invalid.") {}
			/**
			 * Constructor.
			 * @param requested The requested position in the document
			 */
			explicit BadPositionException(const Position& requested) : std::invalid_argument(
				static_cast<std::ostringstream&>(std::ostringstream() << "the position " << requested
				<< " is outside of the document.").str()), requestedPosition_(requested) {}
			/**
			 * Constructor.
			 * @param requested The requested position in the document
			 * @param message The message @c #what returns
			 */
			BadPositionException(const Position& requested, const std::string& message)
				: std::invalid_argument(message), requestedPosition_(requested) {}
			/// Returns the requested position in the document.
			const boost::optional<Position>& requestedPosition() const BOOST_NOEXCEPT {
				return requestedPosition_;
			}
		private:
			const boost::optional<Position> requestedPosition_;
		};
	}
} // namespace ascension.kernel

#endif // !ASCENSION_POSITION_HPP
