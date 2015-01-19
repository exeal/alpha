/**
 * @file style-sequence.hpp
 * @author exeal
 * @date 2014-12-26 Created.
 */

#ifndef ASCENSION_STYLE_SEQUENCE_HPP
#define ASCENSION_STYLE_SEQUENCE_HPP
#ifndef FUSION_MAX_MAP_SIZE
#	define FUSION_MAX_MAP_SIZE 40
#endif
#ifndef FUSION_MAX_VECTOR_SIZE
#	define FUSION_MAX_VECTOR_SIZE 40
#endif

#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/fusion/algorithm/transformation/transform.hpp>
#include <boost/fusion/container/map/convert.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/support/pair.hpp>
#include <boost/fusion/view/iterator_range.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/identity.hpp>
#include <type_traits>

namespace ascension {
	namespace presentation {
		namespace detail {
			template<typename Sequence, std::size_t size>
			class FindDuplicateImpl;
			template<typename Sequence>
			class FindDuplicateImpl<Sequence, 0> : public boost::fusion::result_of::end<Sequence> {};
			template<typename Sequence>
			class FindDuplicateImpl<Sequence, 1> : public FindDuplicateImpl<Sequence, 0> {};
			template<typename Sequence, std::size_t size>
			class FindDuplicateImpl<Sequence, size> {
				typedef typename boost::fusion::result_of::value_at_c<Sequence, 0>::type Car;
				typedef typename boost::fusion::iterator_range<
					typename boost::fusion::result_of::next<
						typename boost::fusion::result_of::begin<Sequence>::type
					>::type,
					typename boost::fusion::result_of::end<Sequence>::type
				> Cdr;
				typedef typename boost::fusion::result_of::find<Cdr, Car>::type Found;
				static const bool found = !std::is_same<Found, boost::fusion::result_of::end<Cdr>::type>::value;
				template<bool enable, typename SubSequence>
				struct FindNext : boost::mpl::identity<typename boost::fusion::result_of::end<SubSequence>::type> {};
				template<typename SubSequence>
				struct FindNext<false, SubSequence> : FindDuplicateImpl<SubSequence, boost::fusion::result_of::size<SubSequence>::type::value> {};
			public:
				typedef typename std::conditional<found, typename boost::fusion::result_of::value_of<Found>::type, typename FindNext<found, Cdr>::type>::type type;
			};

			template<typename Sequence>
			struct FindDuplicate : FindDuplicateImpl<Sequence, boost::fusion::result_of::size<Sequence>::type::value> {};

#define ASCENSION_ASSERT_STYLE_SEQUECE_UNIQUE(Sequence)	\
	BOOST_MPL_ASSERT_MSG(	\
		(std::is_same<::ascension::presentation::detail::FindDuplicate<Sequence>::type, boost::fusion::result_of::end<Sequence>::type>::value),	\
		GIVEN_STYLE_SEQUENCE_IS_NOT_UNIQUE,	\
		(Sequence, ::ascension::presentation::detail::FindDuplicate<Sequence>::type))

			template<template<typename> class Metafunction>
			struct ValueConverter {
				template<typename Property>
				typename Metafunction<Property>::type operator()(const Property&) const BOOST_NOEXCEPT;	// only definition
			};

			template<template<typename> class Metafunction>
			struct KeyValueConverter {
				template<typename Property>
				boost::fusion::pair<Property, typename Metafunction<Property>::type> operator()(const Property&) const BOOST_NOEXCEPT;	// only definition
			};

			template<typename Sequence, typename Converter>
			struct TransformAsVector : boost::mpl::identity<
				typename boost::fusion::result_of::as_vector<
					typename boost::fusion::result_of::transform<Sequence, Converter>::type
				>::type
			> {};

			template<typename Sequence, typename Converter>
			struct TransformAsMap : boost::mpl::identity<
				typename boost::fusion::result_of::as_map<
					typename boost::fusion::result_of::transform<Sequence, Converter>::type
				>::type
			> {};
		}
	}
}

#endif // !ASCENSION_STYLE_SEQUENCE_HPP
