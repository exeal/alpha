/**
 * @file lexical-partitioner.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 */

#ifndef ASCENSION_LEXICAL_PARTITIONER_HPP
#define ASCENSION_LEXICAL_PARTITIONER_HPP
#include <ascension/corelib/detail/gap-vector.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/kernel/partition.hpp>
#include <ascension/kernel/region.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <forward_list>
#include <memory>
#include <tuple>

namespace ascension {
	namespace rules {
		class TransitionRule;

		/**
		 * @c LexicalPartitioner makes document partitions by using the specified lexical rules.
		 * @note This class is not derivable.
		 * @see kernel#Document
		 */
		class LexicalPartitioner : public kernel::DocumentPartitioner, private boost::noncopyable {
		public:
			// constructor
			LexicalPartitioner() BOOST_NOEXCEPT;
			~LexicalPartitioner() BOOST_NOEXCEPT;
			// attribute
			template<typename SinglePassReadableRange>
			void setRules(const SinglePassReadableRange& rules);

		private:
			struct Partition {
				kernel::ContentType contentType;
				kernel::Position start, tokenStart;
				Index tokenLength;
				Partition(const kernel::ContentType& type, const kernel::Position& p,
					const kernel::Position& startOfToken, Index lengthOfToken) BOOST_NOEXCEPT
					: contentType(type), start(p), tokenStart(startOfToken), tokenLength(lengthOfToken) {}
				kernel::Position getTokenEnd() const BOOST_NOEXCEPT {
					return kernel::Position(kernel::line(tokenStart), kernel::offsetInLine(tokenStart) + tokenLength);
				}
			};
		private:
			void computePartitioning(const kernel::Position& start,
				const kernel::Position& minimalLast, kernel::Region& changedRegion);
//			static void deleteRules(std::list<const TransitionRule*>& rules) BOOST_NOEXCEPT;
			void dump() const;
			void erasePartitions(const kernel::Position& first, const kernel::Position& last);
			ascension::detail::GapVector<Partition*>::const_iterator partitionAt(const kernel::Position& at) const BOOST_NOEXCEPT;
			kernel::ContentType transitionStateAt(const kernel::Position& at) const BOOST_NOEXCEPT;
			boost::optional<std::tuple<Index, kernel::ContentType, bool>> tryTransition(
				const StringPiece& line, Index offsetInLine, const kernel::ContentType& contentType) const BOOST_NOEXCEPT;
			void verify() const;
			// DocumentPartitioner
			void documentAboutToBeChanged(const kernel::DocumentChange& change) BOOST_NOEXCEPT override;
			void documentChanged(const kernel::DocumentChange& change) BOOST_NOEXCEPT override;
			void doGetPartition(const kernel::Position& at, kernel::DocumentPartition& partition) const BOOST_NOEXCEPT override;
			void doInstall() BOOST_NOEXCEPT override;
		private:
			ascension::detail::GapVector<Partition*> partitions_;
			std::forward_list<std::unique_ptr<const TransitionRule>> rules_;
		};

		/**
		 * Sets the new transition rules.
		 * @tparam SinglePassReadableRange The type of @a rules
		 * @param rules The new rules to set
		 * @throw IllegalStateException This partitioner had already been connected to a document
		 */
		template<typename SinglePassReadableRange>
		inline void LexicalPartitioner::setRules(const SinglePassReadableRange& rules) {
			if(document() != nullptr)
				throw IllegalStateException("The partitioner is already connected to document.");
//			std::forward_list<std::unique_ptr<const TransitionRule>> newRules(
//				std::make_move_iterator(std::begin(rules)), std::make_move_iterator(std::end(rules)));
			std::forward_list<std::unique_ptr<const TransitionRule>> newRules;
			boost::for_each(rules, [&newRules](const std::unique_ptr<const TransitionRule>& rule) mutable {
				newRules.push_front(std::move(rule->clone()));
			});
//			std::swap(rules_, newRules);
		}
	}
} // namespace ascension.rules

#endif // !ASCENSION_LEXICAL_PARTITIONER_HPP
