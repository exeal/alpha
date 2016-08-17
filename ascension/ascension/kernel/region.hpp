/**
 * @file region.hpp
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
		namespace detail {
			/// @internal
			class RegionIterator : public boost::iterator_facade<
				RegionIterator, const Position, boost::incrementable_traversal_tag, const Position&/*, void */
			> {
				typedef boost::iterator_facade<
					RegionIterator, const Position, boost::incrementable_traversal_tag, const Position&/*, void */
				> Super;
			public:
				typedef Super::value_type value_type;
//				typedef Super::difference_type void;
				typedef Super::reference reference;
				typedef std::input_iterator_tag iterator_category;
			public:
				RegionIterator() BOOST_NOEXCEPT {}
				explicit RegionIterator(const Position& value) BOOST_NOEXCEPT : value_(value) {}
			private:
				reference dereference() const BOOST_NOEXCEPT {return value_;}
				bool equal(const RegionIterator& other) const BOOST_NOEXCEPT {return value_ == other.value_;}
				void increment() BOOST_NOEXCEPT {assert(false);}
			private:
				friend class boost::iterator_core_access;
				Position value_;
			};
		}

		/**
		 * A region consists of two positions and represents a linear range in a document.
		 * @c Region satisfies Single Pass Range concept.
		 * @note This class is not intended to be subclassed by clients.
		 */
		class Region : public boost::iterator_range<detail::RegionIterator>, public FastArenaObject<Region> {
			typedef detail::RegionIterator Iterator;
			typedef boost::iterator_range<Iterator> Super;

		public:
			/// Default constructor does not initialize the two positions.
			Region() BOOST_NOEXCEPT {}
			/**
			 * Creates a region with the specified two positions.
			 * @param p1 A position
			 * @param p2 An another position
			 */
			Region(const Position& p1, const Position& p2) BOOST_NOEXCEPT : Super(Iterator(std::min(p1, p2)), Iterator(std::max(p1, p2))) {}
			/**
			 * Creates a region with the specified range.
			 * @tparam SinglePassReadableRange The type of @a range
			 * @param range Any range
			 */
			template<typename SinglePassReadableRange>
			static Region fromRange(const SinglePassReadableRange& range) {
				BOOST_CONCEPT_ASSERT((boost::SinglePassRangeConcept<SinglePassReadableRange>));
				return Region(*boost::const_begin(range), *boost::const_end(range));
			}
			/**
			 * Creates a region with the specified two positions.
			 * @tparam Positions The type of @a positions
			 * @param positions An array, tuple, ... whose size is 2
			 */
			template<typename Positions>
			static Region fromTuple(const Positions& positions) {
				static_assert(std::tuple_size<Positions>::value == 2, "");
				return Region(std::get<0>(positions), std::get<1>(positions));
			}
			/**
			 * Creates an empty region with the specified position.
			 * @param position The position
			 */
			static Region makeEmpty(const Position& position) BOOST_NOEXCEPT {
				return Region(position, position);
			}
			/**
			 * Creates a single-line region.
			 * @tparam SinglePassReadableRange The type of @a rangeInLine
			 * @param line The line number
			 * @param rangeInLine Offsets in the line
			 */
			template<typename SinglePassReadableRange>
			static Region makeSingleLine(Index line, const SinglePassReadableRange& rangeInLine) BOOST_NOEXCEPT {
				BOOST_CONCEPT_ASSERT((boost::SinglePassRangeConcept<SinglePassReadableRange>));
				return Region(Position(line, *boost::const_begin(rangeInLine)), Position(line, *boost::const_end(rangeInLine)));
			}
			/// Returns a range of lines. @c boost#size(lines()) returns the number of lines this range covers.
			auto lines() const BOOST_NOEXCEPT -> decltype(boost::irange<Index>(0, 0)) {
				return boost::irange(line(*boost::const_begin(*this)), line(*boost::const_end(*this)) + 1);
			}
			/// Returns the result of @c Region(Position::zero()).
			static Region zero() BOOST_NOEXCEPT {
				return makeEmpty(Position::zero());
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
			s << *boost::const_begin(value) << ct.widen('-') << *boost::const_end(value);
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
			 */
			explicit BadRegionException(const Region& requested) : std::invalid_argument(
				static_cast<std::ostringstream&&>(std::ostringstream() << "the region " << requested
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

		BOOST_CONCEPT_ASSERT((boost::SinglePassRangeConcept<Region>));
	}
} // namespace ascension.kernel

namespace boost {
	namespace range {
		inline bool equal(
				const iterator_range<::ascension::kernel::detail::RegionIterator>& lhs,
				const iterator_range<::ascension::kernel::detail::RegionIterator>& rhs) {
			return *const_begin(lhs) == *const_begin(rhs) && *const_end(lhs) == *const_end(rhs);
		}
		template<typename BinaryPredicate>
		inline bool equal(
				const iterator_range<::ascension::kernel::detail::RegionIterator>& lhs,
				const iterator_range<::ascension::kernel::detail::RegionIterator>& rhs,
				BinaryPredicate pred) {
			return pred(*const_begin(lhs), *const_begin(rhs)) && pred(*const_end(lhs), *const_end(rhs));
		}
	}

	using ::boost::range::equal;
}

#endif // !ASCENSION_REGION_HPP
