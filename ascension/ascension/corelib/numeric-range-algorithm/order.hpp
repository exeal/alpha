/**
 * @file order.hpp
 * @author exeal
 * @date 2015-03-28 Separated from numeric-range.hpp and range.hpp.
 */

#ifndef ASCENSION_ORDER_HPP
#define ASCENSION_ORDER_HPP
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>
#include <functional>	// std.less
#include <utility>		// std.get, std.minmax

namespace ascension {
	namespace algorithm {
		/**
		 * Returns @c true if the given numeric range is ordered (sorted).
		 * @tparam NumericRange The type of @a range
		 * @param range The numeric range to test
		 * @return @c true if @a range is ordered
		 */
		template<typename NumericRange>
		inline bool isOrdered(const NumericRange& range) {
			return isOrdered(range, std::less<typename boost::range_value<NumericRange>::type>());
		}

		/**
		 * Returns @c true if the given numeric range is ordered (sorted).
		 * @tparam NumericRange The type of @a range
		 * @tparam BinaryPredicate The type of @a pred
		 * @param range The numeric range to test
		 * @param pred The binary predicate to compare the values
		 * @return @c true if @a range is ordered
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline bool isOrdered(const NumericRange& range, BinaryPredicate pred) {
			return !pred(*boost::const_end(range), *boost::const_begin(range));
		}

		/**
		 * Generates an ordered numeric range.
		 * @tparam NumericRange The type of @a range1 and @a range2. This should have the constructor which takes the
		 *                      two parameters to initialize the result range
		 * @param range The original range
		 * @return The ordered range
		 */
		template<typename NumericRange>
		inline NumericRange order(const NumericRange& range) {
			return order(range, std::less<typename boost::range_value<NumericRange>::type>());
		}

		/**
		 * Generates an ordered numeric range.
		 * @tparam NumericRange The type of @a range1 and @a range2. This should have the constructor which takes the
		 *                      two parameters to initialize the result range
		 * @tparam BinaryPredicate The type of @a pred
		 * @param range The original range
		 * @param pred The binary predicate to compare the values
		 * @return The ordered range
		 */
		template<typename NumericRange, typename BinaryPredicate>
		inline NumericRange order(const NumericRange& range, BinaryPredicate pred) {
			const auto temp(std::minmax(*boost::const_begin(range), *boost::const_end(range), pred));
			return NumericRange(std::get<0>(temp), std::get<1>(temp));
		}
	}

	using algorithm::isOrdered;
	using algorithm::order;

	namespace detail {
		struct OrderForwarder0 {};

		template<typename BinaryPredicate>
		struct OrderForwarder1 : OrderForwarder0 {
			explicit OrderForwarder1(BinaryPredicate pred) : pred(pred) {}
			BinaryPredicate pred;
		};

		template<typename NumericRange>
		inline NumericRange operator|(const NumericRange& range, const OrderForwarder0&) {
			return order(range);
		}

		template<typename NumericRange, typename BinaryPredicate>
		inline NumericRange operator|(const NumericRange& range, const OrderForwarder1<BinaryPredicate>& forwarder) {
			return order(range, forwarder.pred);
		}
	}

	namespace adaptors {
		/// Returns an adaptor which applies @c order function.
		inline detail::OrderForwarder0 ordered() {
			return detail::OrderForwarder0();
		}

		/// Returns an adaptor which applies @c order function.
		template<typename BinaryPredicate>
		inline detail::OrderForwarder1<BinaryPredicate> ordered(BinaryPredicate pred) {
			return detail::OrderForwarder1<BinaryPredicate>(pred);
		}
	}
}

#endif // !ASCENSION_ORDER_HPP
