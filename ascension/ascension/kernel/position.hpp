/**
 * @file position.hpp
 * @author exeal
 * @date 2011-01-21 separated from document.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_POSITION_HPP
#define ASCENSION_POSITION_HPP

#include <ascension/corelib/basic-types.hpp>	// Index
#include <ascension/corelib/memory.hpp>			// FastArenaObject
#include <ascension/corelib/range.hpp>
#include <locale>	// std.use_facet, ...
#include <sstream>	// std.basic_ostream, std.ostringstream
#include <utility>	// std.pair
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace kernel {

		/**
		 * @c Position represents a position in the document by a line number and an offset in the
		 * line.
		 * @note This class is not intended to be subclassed.
		 * @see Region, Point, viewers#VisualPoint, viewers#Caret
		 */
		class Position : public FastArenaObject<Position>,
				private boost::totally_ordered<Position> {
		public:
			/// Line number. Zero means that the position is the first line in the document.
			Index line;
			/// Offset in the line (column) in UTF-16 code units.
			/// Zero means that the position is the beginning of the line.
			Index offsetInLine;
		public:
			/// Default constructor does not initialize the members at all.
			Position() /*throw()*/ {}
			/**
			 * Constructor.
			 * @param line The initial value for @c #line
			 * @param offsetInLine The initial value for @c #offsetInLine
			 */
			Position(Index line, Index offsetInLine) /*throw()*/ : line(line), offsetInLine(offsetInLine) {
			}
			/// Equality operator.
			bool operator==(const Position& other) const /*throw()*/ {
				return line == other.line && offsetInLine == other.offsetInLine;
			}
			/// Relational operator.
			bool operator<(const Position& other) const /*throw()*/ {
				return line < other.line || (line == other.line && offsetInLine < other.offsetInLine);
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
		 * A region consists of two positions and represents a linear range in a document. There
		 * are no restriction about greater/less relationship between the two positions, but the
		 * region is called "normalized" when the first position is less than or equal to the second.
		 * @note This class is not intended to be subclassed.
		 * @see Range
		 */
		class Region : public std::pair<Position, Position>, public FastArenaObject<Region> {
		public:
			/// Default constructor does not initialize the two positions.
			Region() /*throw()*/ {}
			/// Constructor creates an empty region.
			explicit Region(const Position& p) /*throw()*/
				: std::pair<Position, Position>(p, p) {}
			/// Constructor.
			Region(const Position& first, const Position& second) /*throw()*/
				: std::pair<Position, Position>(first, second) {}
			/// Constructor creates a region in a line.
			Region(Index line, const std::pair<Index, Index>& columns) /*throw()*/
				: std::pair<Position, Position>(Position(line, columns.first), Position(line, columns.second)) {}
			/// Returns an intersection of the two regions. Same as @c #getIntersection.
			Region operator&(const Region& other) const /*throw()*/ {
				return getIntersection(other);
			}
			/// Returns a union of the two regions. Same as @c #getUnion.
			Region operator|(const Region& other) const {return getUnion(other);}
			/// Returns the beginning of the region.
			Position& beginning() /*throw()*/ {return (first < second) ? first : second;}
			/// Returns the beginning of the region.
			const Position& beginning() const /*throw()*/ {
				return (first < second) ? first : second;
			}
			/// Returns @c true if the region encompasses the other region.
			bool encompasses(const Region& other) const /*throw()*/ {
				return beginning() <= other.beginning() && end() >= other.end();
			}
			/// Returns the end of the region.
			Position& end() /*throw()*/ {return (first > second) ? first : second;}
			/// Returns the end of the region.
			const Position& end() const /*throw()*/ {return (first > second) ? first : second;}
			/// Returns an intersection of the two regions. If the regions don't intersect, returns @c Region().
			Region getIntersection(const Region& other) const /*throw()*/ {
				return intersectsWith(other) ?
					Region(std::max(beginning(), other.beginning()), std::min(end(), other.end())) : Region();
			}
			/// Returns a union of the two regions. If the two regions don't intersect, throws @c std#invalid_argument.
			Region getUnion(const Region& other) const {
				if(!intersectsWith(other))
					throw std::invalid_argument("can't make a union.");
				return Region(beginning(), other.end());
			}
			/// Returns @c true if @a p is contained by the region.
			bool includes(const Position& p) const /*throw()*/ {
				return p >= beginning() && p <= end();
			}
			/// Returns @c true if the region intersects with the other region.
			bool intersectsWith(const Region& other) const /*throw()*/ {
				return includes(other.first) || includes(other.second);
			}
			/// Returns @c true if the region is empty.
			bool isEmpty() const /*throw()*/ {return first == second;}
			/// Returns @c true if the region is normalized.
			bool isNormalized() const /*throw()*/ {return first <= second;}
			/// Returns a range of lines.
			Range<Index> lines() const /*throw()*/ {return makeRange(beginning().line, end().line + 1);}
			/// Normalizes the region.
			Region& normalize() /*throw()*/ {
				if(!isNormalized())
					std::swap(first, second);
				return *this;
			}
		};

		/**
		 * Writes a @c Region into the output stream.
		 * @tparam Element The element type of the output stream
		 * @tparam Traits The character traits of the output stream
		 * @param out The output stream
		 * @param value The region to write
		 * @return @a out
		 */
		template<typename Element, typename Traits>
		inline std::basic_ostream<Element, Traits>& operator<<(
				std::basic_ostream<Element, Traits>& out, const Region& value) {
			const std::ctype<Element>& ct = std::use_facet<std::ctype<Element>>(out.getloc());
			std::basic_ostringstream<Element, Traits> s;
			s.flags(out.flags());
			s.imbue(out.getloc());
			s.precision(out.precision());
			s << value.first << ct.widen('-') << value.second;
			return out << s.str().c_str();
		}

		/**
		 * Thrown when the specified line or character position is outside of the document.
		 * @see BadRegionException
		 */
		class BadPositionException : public std::invalid_argument {
		public:
			/// Default constructor.
			BadPositionException() /*throw()*/ : std::invalid_argument(
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
			const boost::optional<Position>& requestedPosition() const /*throw()*/ {
				return requestedPosition_;
			}
		private:
			const boost::optional<Position> requestedPosition_;
		};

		/**
		 * Thrown when the specified region intersects outside of the document.
		 * @see BadPositionException
		 */
		class BadRegionException : public std::invalid_argument {
		public:
			/// Default constructor.
			BadRegionException() /*throw()*/ : std::invalid_argument(
				"the region <not-initialized> intersects outside of the document or invalid") {}
			/**
			 * Constructor.
			 * @param requested The requested region in the document
			 * @param message The exception message
			 */
			explicit BadRegionException(const Region& requested) : std::invalid_argument(
				static_cast<std::ostringstream&>(std::ostringstream() << "the region " << requested
				<< ") intersects with the outside of the document.").str()), requestedRegion_(requested) {}
			/**
			 * Constructor.
			 * @param requested The requested region in the document
			 * @param message The exception message
			 */
			BadRegionException(const Region& requested, const std::string& message)
				: std::invalid_argument(message), requestedRegion_(requested) {}
			/// Returns the requested region in the document.
			const boost::optional<Region>& requestedRegion() const /*throw()*/ {
				return requestedRegion_;
			}
		private:
			const boost::optional<Region> requestedRegion_;
		};

	}
} // namespace ascension.kernel

#endif // !ASCENSION_POSITION_HPP
