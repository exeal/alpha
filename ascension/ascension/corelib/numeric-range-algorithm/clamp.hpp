/**
 * @file clamp.hpp
 * @author exeal
 * @date 2015-03-28 Separated from numeric-range.hpp.
 */

#ifndef ASCENSION_CLAMP_HPP
#define ASCENSION_CLAMP_HPP
#include <boost/algorithm/clamp.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace ascension {
	namespace algorithm {
		/**
		 * Clamps the given value into the numeric range.
		 * @tparam Value The type of @a value
		 * @tparam NumericRange The type of @a bounds
		 * @param value The value to clamp
		 * @param bounds The bounds range to clamp to
		 * @return A clamped value
		 * @note This function assumes that @a bounds is ordered.
		 * @note This function uses @c boost#algorithm#clamp.
		 */
		template<typename Value, typename NumericRange>
		inline Value clamp(Value value, const NumericRange& bounds) {
			return boost::algorithm::clamp(value, *boost::const_begin(bounds), *boost::const_end(bounds));
		}

		/**
		 * Clamps the given value into the numeric range.
		 * @tparam Value The type of @a value
		 * @tparam NumericRange The type of @a bounds
		 * @tparam BinaryPredicate The type of @a pred
		 * @param value The value to clamp
		 * @param bounds The bounds range to clamp to
		 * @param pred The binary predicate to compare the values
		 * @return A clamped value
		 * @note This function assumes that @a bounds is ordered.
		 * @note This function uses @c boost#algorithm#clamp.
		 */
		template<typename Value, typename NumericRange, typename BinaryPredicate>
		inline Value clamp(Value value, const NumericRange& bounds, BinaryPredicate pred) {
			return boost::algorithm::clamp(value, *boost::const_begin(bounds), *boost::const_end(bounds), pred);
		}

		/**
		 * Clamps the given numeric range into the other numeric range.
		 * @tparam NumericRange1 The type of @a range. This should have the constructor which takes the two parameters
		 * @tparam NumericRange2 The type of @a bounds
		 * @param range The range to clamp
		 * @param bounds The bounds range to clamp to
		 * @return A clamped range
		 * @note This function assumes that both @a range and @a bounds are ordered.
		 * @note This function uses @c boost#algorithm#clamp.
		 */
		template<typename NumericRange1, typename NumericRange2>
		inline NumericRange1 clampRange(const NumericRange1& range, const NumericRange2& bounds) {
			return NumericRange1(clamp(*boost::const_begin(range), bounds), clamp(*boost::const_end(range), bounds));
		}

		/**
		 * Clamps the given numeric range into the other numeric range.
		 * @tparam NumericRange1 The type of @a range. This should have the constructor which takes the two parameters
		 * @tparam NumericRange2 The type of @a bounds
		 * @tparam BinaryPredicate The type of @a pred
		 * @param range The range to clamp
		 * @param bounds The bounds range to clamp to
		 * @param pred The binary predicate to compare the values
		 * @return A clamped range
		 * @note This function assumes that both @a range and @a bounds are ordered.
		 * @note This function uses @c boost#algorithm#clamp.
		 */
		template<typename NumericRange1, typename NumericRange2, typename BinaryPredicate>
		inline NumericRange1 clampRange(const NumericRange1& range, const NumericRange2& bounds, BinaryPredicate pred) {
			return NumericRange1(clamp(*boost::const_begin(range), bounds, pred), clamp(*boost::const_end(range), bounds, pred));
		}
	}

	using algorithm::clamp;

	namespace detail {
		template<typename NumericRange>
		struct ClampForwarder1 {
			explicit ClampForwarder1(const NumericRange& bounds) : bounds(bounds) {}
			const NumericRange bounds;
		};

		template<typename NumericRange, typename BinaryPredicate>
		struct ClampForwarder2 : ClampForwarder1<NumericRange> {
			explicit ClampForwarder2(const NumericRange& bounds, BinaryPredicate pred) : ClampForwarder1<NumericRange>(bounds), pred(pred) {}
			BinaryPredicate pred;
		};

		template<typename NumericRange>
		inline NumericRange operator|(const NumericRange& range, const ClampForwarder1<NumericRange>& forwarder) {
			return clamp(range, forwarder.bounds);
		}

		template<typename NumericRange, typename BinaryPredicate>
		inline NumericRange operator|(const NumericRange& range, const ClampForwarder2<NumericRange, BinaryPredicate>& forwarder) {
			return clamp(range, forwarder.bounds, forwarder.pred);
		}
	}

	namespace adaptors {
		/// Returns an adaptor which applies @c clamp function.
		template<typename NumericRange>
		inline detail::ClampForwarder1<NumericRange> clamped(const NumericRange& bounds) {
			return detail::ClampForwarder1<NumericRange>(bounds);
		}

		/// Returns an adaptor which applies @c clamp function.
		template<typename NumericRange, typename BinaryPredicate>
		inline detail::ClampForwarder2<NumericRange, BinaryPredicate> clamped(const NumericRange& bounds, BinaryPredicate pred) {
			return detail::ClampForwarder2<NumericRange, BinaryPredicate>(bounds, pred);
		}
	}
}

#endif // !ASCENSION_CLAMP_HPP
