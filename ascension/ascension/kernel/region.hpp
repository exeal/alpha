/**
 * @file position.hpp
 * @author exeal
 * @date 2011-01-21 separated from document.hpp
 * @date 2015-05-02 Separated from position.hpp.
 */

#ifndef ASCENSION_REGION_HPP
#define ASCENSION_REGION_HPP
#include <ascension/kernel/position.hpp>
#include <boost/range/irange.hpp>
#include <utility>	// std.max, std.min, std.pair, std.swap

namespace ascension {
	namespace kernel {
		/**
		 * A region consists of two positions and represents a linear range in a document. There are no restriction
		 * about greater/less relationship between the two positions, but the region is called "normalized" when the
		 * first position is less than or equal to the second.
		 * @note This class is not intended to be subclassed.
		 */
		class Region : public std::pair<Position, Position>, public FastArenaObject<Region> {
		public:
			/// Default constructor does not initialize the two positions.
			Region() BOOST_NOEXCEPT {}
			/// Constructor creates an empty region.
			explicit Region(const Position& p) BOOST_NOEXCEPT : std::pair<Position, Position>(p, p) {}
			/// Constructor.
			Region(const Position& first, const Position& second) BOOST_NOEXCEPT
				: std::pair<Position, Position>(first, second) {}
			/// Constructor creates a region in a line.
			Region(Index line, const boost::integer_range<Index>& rangeInLine) BOOST_NOEXCEPT
				: std::pair<Position, Position>(Position(line, *rangeInLine.begin()), Position(line, *rangeInLine.end())) {}
			/// Returns an intersection of the two regions. Same as @c #getIntersection.
			Region operator&(const Region& other) const BOOST_NOEXCEPT {
				return getIntersection(other);
			}
			/// Returns a union of the two regions. Same as @c #getUnion.
			Region operator|(const Region& other) const {return getUnion(other);}
			/// Returns the result of Region(Position::zero()).
			static BOOST_CONSTEXPR Region zero() BOOST_NOEXCEPT {
				return Region(Position::zero());
			}
			/// Returns the beginning of the region.
			Position& beginning() BOOST_NOEXCEPT {return (first < second) ? first : second;}
			/// Returns the beginning of the region.
			const Position& beginning() const BOOST_NOEXCEPT {
				return (first < second) ? first : second;
			}
			/// Returns @c true if the region encompasses the other region.
			bool encompasses(const Region& other) const BOOST_NOEXCEPT {
				return beginning() <= other.beginning() && end() >= other.end();
			}
			/// Returns the end of the region.
			Position& end() BOOST_NOEXCEPT {return (first > second) ? first : second;}
			/// Returns the end of the region.
			const Position& end() const BOOST_NOEXCEPT {return (first > second) ? first : second;}
			/// Returns an intersection of the two regions. If the regions don't intersect, returns @c Region().
			Region getIntersection(const Region& other) const BOOST_NOEXCEPT {
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
			bool includes(const Position& p) const BOOST_NOEXCEPT {
				return p >= beginning() && p <= end();
			}
			/// Returns @c true if the region intersects with the other region.
			bool intersectsWith(const Region& other) const BOOST_NOEXCEPT {
				return includes(other.first) || includes(other.second);
			}
			/// Returns @c true if the region is empty.
			bool isEmpty() const BOOST_NOEXCEPT {return first == second;}
			/// Returns @c true if the region is normalized.
			bool isNormalized() const BOOST_NOEXCEPT {return first <= second;}
			/// Returns a range of lines.
			boost::integer_range<Index> lines() const BOOST_NOEXCEPT {
				return boost::irange(line(beginning()), line(end()) + 1);
			}
			/// Normalizes the region.
			Region& normalize() BOOST_NOEXCEPT {
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
		 * Thrown when the specified region intersects outside of the document.
		 * @see BadPositionException
		 */
		class BadRegionException : public std::invalid_argument {
		public:
			/// Default constructor.
			BadRegionException() BOOST_NOEXCEPT : std::invalid_argument(
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
			const boost::optional<Region>& requestedRegion() const BOOST_NOEXCEPT {
				return requestedRegion_;
			}
		private:
			const boost::optional<Region> requestedRegion_;
		};
	}
} // namespace ascension.kernel

#endif // !ASCENSION_REGION_HPP
