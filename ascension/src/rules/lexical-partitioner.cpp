/**
 * @file lexical-partitioner.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Separated from lexical-partitioning.cpp
 */

#include <ascension/corelib/utility.hpp>	// detail.searchBound
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/rules/lexical-partitioner.hpp>
#include <ascension/rules/transition-rules.hpp>
#include <boost/foreach.hpp>
#if defined(_DEBUG)
#	include <boost/log/trivial.hpp>
#endif


namespace ascension {
	namespace rules {
		/// Constructor.
		LexicalPartitioner::LexicalPartitioner() BOOST_NOEXCEPT {
		}
		
		/// Destructor.
		LexicalPartitioner::~LexicalPartitioner() BOOST_NOEXCEPT {
			BOOST_FOREACH(const Partition* partition, partitions_)
				delete partition;
		}

		/**
		 * Computes and constructs the partitions on the specified region.
		 * @param start The start of the region to compute
		 * @param minimalLast The partitioner must scan to this position at least
		 * @param[out] changedRegion The region whose content type was changed
		 */
		void LexicalPartitioner::computePartitioning(const kernel::Position& start, const kernel::Position& minimalLast, kernel::Region& changedRegion) {
			// TODO: see LexicalPartitioner.documentChanged.
		}
		
		/// @see kernel#DocumentPartitioner#documentAboutToBeChanged
		void LexicalPartitioner::documentAboutToBeChanged() BOOST_NOEXCEPT {
		}
		
		/// @see kernel#DocumentPartitioner#documentChanged
		void LexicalPartitioner::documentChanged(const kernel::DocumentChange& change) BOOST_NOEXCEPT {
			// this code reconstructs partitions in the region changed by the document modification using
			// the registered partitioning rules

//			assert(!change.erasedRegion().isEmpty() || !change.insertedRegion().isEmpty());
//			if(change.region().isEmpty())
//				return;
			// TODO: there is more efficient implementation using LexicalPartitioner.computePartitioning.
			const kernel::Document& doc = *document();
		
			// move the partitions adapting to the document change. this does not affect partitions out of
			// the deleted region
			if(!change.erasedRegion().isEmpty()) {
				for(std::size_t i = 1, c = partitions_.size(); i < c; ++i) {
					Partition& p = *partitions_[i];
					if(p.start < change.erasedRegion().beginning())
						continue;
					else if(p.start > change.erasedRegion().end()) {
						p.start = kernel::positions::updatePosition(p.start, change, Direction::FORWARD);
						p.tokenStart = kernel::positions::updatePosition(p.tokenStart, change, Direction::FORWARD);
					} else if(((i + 1 < c) ? partitions_[i + 1]->start : doc.region().end()) <= change.erasedRegion().end()) {
						// this partition is encompassed with the deleted region
						delete partitions_[i];
						partitions_.erase(std::begin(partitions_) + i);
						if(i < c - 1 && partitions_[i]->contentType == partitions_[i - 1]->contentType) {
							delete partitions_[i];
							partitions_.erase(std::begin(partitions_) + i);
							--c;
						}
						--i;
						if(--c == 1)
							break;
					} else
						// this partition will be erased later
						p.start = p.tokenStart = change.erasedRegion().beginning();
				}
			}
			if(!change.insertedRegion().isEmpty()) {
				for(std::size_t i = 1, c = partitions_.size(); i < c; ++i) {
					Partition& p = *partitions_[i];
					p.start = kernel::positions::updatePosition(p.start, change, Direction::FORWARD);
					p.tokenStart = kernel::positions::updatePosition(p.tokenStart, change, Direction::FORWARD);
				}
			}
			verify();
		
			// compute partitioning for the affected region using the registered rules
			std::vector<Partition*> newPartitions;	// newly computed partitions for the affected region
			kernel::DocumentCharacterIterator i(doc,	// the beginning of the region to parse ~ the end of the document
				kernel::Region(kernel::Position(std::min(change.erasedRegion().beginning().line, change.insertedRegion().beginning().line), 0), doc.region().end()));
			kernel::ContentType contentType, destination;
			contentType = (i.tell().line == 0) ? kernel::DEFAULT_CONTENT_TYPE
				: (*partitionAt(kernel::Position(i.tell().line - 1, doc.lineLength(i.tell().line - 1))))->contentType;
			for(const String* line = &doc.line(i.tell().line); ; ) {	// scan and tokenize into partitions...
				const bool atEOL = i.tell().offsetInLine == line->length();
				Index tokenLength = tryTransition(*line, i.tell().offsetInLine, contentType, destination);
				if(tokenLength != 0) {	// a token was found
					if(atEOL)
						tokenLength = 0;	// a line terminator is zero-length...
					const kernel::Position tokenEnd(i.tell().line, i.tell().offsetInLine + tokenLength);
					// insert the new partition behind the current
					assert(destination != contentType);
					newPartitions.push_back(new Partition(
						destination, (destination > contentType) ? i.tell() : tokenEnd, i.tell(), tokenLength));
					contentType = destination;
					// go to the end of the found token
					if(!atEOL)
						i.seek(tokenEnd);
				}
				// this loop can end at only EOL
				if(atEOL) {
					// the end of the document
					if(!i.hasNext())
						break;
					// if reached the end of the affected region and content types are same, we are done
					else if(i.tell() >= std::max(change.erasedRegion().second, change.insertedRegion().second) && transitionStateAt(i.tell()) == contentType)
						break;
				}
				// go to the next character if no transition occurred
				if(tokenLength == 0) {
					++i;
					if(i.tell().offsetInLine == 0)	// entered the next line
						line = &doc.line(i.tell().line);
				}
			}
		
			// replace partitions encompassed with the affected region
			erasePartitions(i.region().beginning(), i.tell());
			partitions_.insert(partitionAt(i.region().beginning()) + 1, std::begin(newPartitions), std::end(newPartitions));

#ifdef _DEBUG
			static bool trace = false;
			if(trace)
				dump();
#endif
			verify();
			notifyDocument(kernel::Region(kernel::Position(std::min(
				change.erasedRegion().beginning().line, change.insertedRegion().beginning().line), 0), i.tell()));
		}

		/// @see kernel#DocumentPartitioner#doGetPartition
		void LexicalPartitioner::doGetPartition(const kernel::Position& at, kernel::DocumentPartition& partition) const BOOST_NOEXCEPT {
			auto i(partitionAt(at));
			const Partition& p = **i;
			partition.contentType = p.contentType;
			partition.region.first = p.start;
			partition.region.second = (i < std::end(partitions_) - 1) ? (*++i)->start : document()->region().second;
		}

		/// @see kernel#DocumentPartitioner#doInstall
		void LexicalPartitioner::doInstall() BOOST_NOEXCEPT {
			BOOST_FOREACH(const Partition* partition, partitions_)
				delete partition;
			partitions_.clear();
			partitions_.insert(std::begin(partitions_), new Partition(kernel::DEFAULT_CONTENT_TYPE, kernel::Position::zero(), kernel::Position::zero(), 0));
			kernel::Region dummy;
			const kernel::Region entire(document()->region());
			computePartitioning(entire.first, entire.second, dummy);
		}

		/// Dumps the partitions information.
		void LexicalPartitioner::dump() const {
#if defined(_DEBUG)
			BOOST_LOG_TRIVIAL(debug) << "LexicalPartitioner dump start:\n";
			BOOST_FOREACH(auto i, partitions_)
				BOOST_LOG_TRIVIAL(debug)
					<< "\t" << i->contentType << " = ("
					<< static_cast<std::uint32_t>(i->start.line) << ", "
					<< static_cast<std::uint32_t>(i->start.offsetInLine) << ")\n";
#endif
		}

		// erases partitions encompassed with the region between the given two positions.
		void LexicalPartitioner::erasePartitions(const kernel::Position& first, const kernel::Position& last) {
			// locate the first partition to delete
			auto deletedFirst(partitionAt(first));
			if(first >= (*deletedFirst)->getTokenEnd())
				++deletedFirst;	// do not delete this partition
//			else if(deletedFirst < partitions_.getSize() - 1 && partitions_[deletedFirst + 1]->tokenStart < change.getRegion().getBottom())
//				++deletedFirst;	// delete from the next partition
			// locate the last partition to delete
			auto deletedLast(partitionAt(last) + 1);	// exclusive
			if(deletedLast < std::end(partitions_) && (*deletedLast)->tokenStart < last)
				++deletedLast;
//			else if(titions_[predeletedLast - 1]->start == change.getRegion().getBottom())
//				--deletedLast;
			if(deletedLast > deletedFirst) {
				if(deletedFirst > std::begin(partitions_) && deletedLast < std::end(partitions_)
						&& (*(deletedFirst - 1))->contentType == (*deletedLast)->contentType)
					++deletedLast;	// combine
				std::for_each(deletedFirst, deletedLast, std::default_delete<Partition>());
				partitions_.erase(deletedFirst, deletedLast);
			}

			// push a default partition if no partition includes the start of the document
			const kernel::Document& d = *document();
			if(partitions_.empty() || partitions_[0]->start != d.region().first) {
				if(partitions_.empty() || partitions_[0]->contentType != kernel::DEFAULT_CONTENT_TYPE) {
					const kernel::Position bob(d.region().first);
					partitions_.insert(std::begin(partitions_), new Partition(kernel::DEFAULT_CONTENT_TYPE, bob, bob, 0));
				} else {
					partitions_[0]->start = partitions_[0]->tokenStart = d.region().first;
					partitions_[0]->tokenLength = 0;
				}
			}

			// delete the partition whose start position is the end of the document
			if(partitions_.size() > 1 && partitions_.back()->start == d.region().second) {
				delete partitions_[partitions_.size() - 1];
				partitions_.erase(std::end(partitions_) - 1);
			}
		}

		namespace {
			template<typename Partition>
			struct PartitionPositionCompare {
				bool operator()(const kernel::Position& at, const Partition* p) const {
					return at < p->start;
				}
			};
		}

		// returns the index of the partition encompasses the given position.
		inline ascension::detail::GapVector<LexicalPartitioner::Partition*>::const_iterator
				LexicalPartitioner::partitionAt(const kernel::Position& at) const BOOST_NOEXCEPT {
			auto p(ascension::detail::searchBound(std::begin(partitions_), std::end(partitions_), at, PartitionPositionCompare<Partition>()));
			if(p == std::end(partitions_)) {
				assert(partitions_.front()->start != document()->region().first);	// twilight context
				return std::begin(partitions_);
			}
			if(at.line < document()->numberOfLines()
					&& (*p)->tokenStart == at && p != std::begin(partitions_) && at.offsetInLine == document()->lineLength(at.line))
				--p;
//			if(result > 0 && partitions_[result]->start == partitions_[result - 1]->start)
//				--p;
			while(p + 1 < std::end(partitions_) && (*(p + 1))->start == (*p)->start)
				++p;
			return p;
		}

		/**
		 * @fn void ascension::rules::LexicalPartitioner::setRules(InputIterator first, InputIterator last)
		 * @brief Sets the new transition rules.
		 * @tparam InputIterator Input iterator provides transition rules. The deference operator should return a value
		 *                       implicitly convertible to a pointer to @c TransitionRule. This method calls
		 *                       @c TransitionRule#clone to copy the values
		 * @param first, last The transition rules
		 * @throw IllegalStateException This partitioner had already been connected to a document
		 */
		
		// returns the transition state (corresponding content type) at the given position.
		inline kernel::ContentType LexicalPartitioner::transitionStateAt(const kernel::Position& at) const BOOST_NOEXCEPT {
			if(at.line == 0 && at.offsetInLine == 0)
				return kernel::DEFAULT_CONTENT_TYPE;
			auto i(partitionAt(at));
			if((*i)->start == at)
				--i;
			return (*i)->contentType;
		}
		
		/**
		 *
		 * @param line The scanning line text
		 * @param offsetInLine The offset in the line at which match starts
		 * @param contentType The current content type
		 * @param[out] destination The type of the transition destination content
		 * @return The length of the pattern matched or 0 if the all rules did not matched
		 */
		inline Index LexicalPartitioner::tryTransition(
				const String& line, Index offsetInLine, kernel::ContentType contentType, kernel::ContentType& destination) const BOOST_NOEXCEPT {
			BOOST_FOREACH(const std::unique_ptr<const TransitionRule>& rule, rules_) {
				if(rule->contentType() == contentType) {
					if(const Index c = rule->matches(line, offsetInLine)) {
						destination = rule->destination();
						return c;
					}
				}
			}
			destination = kernel::UNDETERMINED_CONTENT_TYPE;
			return 0;
		}

		/// Diagnoses the partitions.
		inline void LexicalPartitioner::verify() const {
#ifdef _DEBUG
			assert(!partitions_.empty());
			assert(partitions_.front()->start == document()->region().first);
			bool previousWasEmpty = false;
			for(std::size_t i = 0, e = partitions_.size(); i < e - 1; ++i) {
				assert(partitions_[i]->contentType != partitions_[i + 1]->contentType);
				if(partitions_[i]->start == partitions_[i + 1]->start) {
					if(previousWasEmpty)
						throw std::runtime_error("LexicalPartitioner.verify failed.");
					previousWasEmpty = true;
				} else {
					assert(partitions_[i]->start < partitions_[i + 1]->start);
					previousWasEmpty = false;
				}
			}
//			assert(partitions_.back()->start < getDocument()->getEndPosition(false) || partitions_.getSize() == 1);
#endif // _DEBUG
		}
	}
}
