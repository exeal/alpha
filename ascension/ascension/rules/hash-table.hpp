/**
 * @file hash-table.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Separated from token-rules.cpp
 * @date 2014-11-17 Separated from uri-detector.cpp
 */

#ifndef ASCENSION_HASH_TABLE_HPP
#define ASCENSION_HASH_TABLE_HPP

#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/ustring.hpp>	// umemcmp
#include <boost/core/noncopyable.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/range/iterator.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace ascension {
	namespace rules {
		namespace detail {
			class HashTable : private boost::noncopyable {
			public:
				/**
				 * Constructor.
				 * @tparam StringSequence A type of @a first and @a last
				 * @param first The start of the strings
				 * @param last The end of the strings
				 * @param caseSensitive Set @c true to enable case sensitive match
				 */
				template<typename StringSequence>
				inline HashTable(StringSequence first, StringSequence last, bool caseSensitive)
						: entries_(std::distance(first, last)), maxLength_(0), caseSensitive_(caseSensitive) {
					while(first != last) {
						const String folded(caseSensitive_ ? *first : text::CaseFolder::fold(*first));
						const std::size_t h = hashCode(folded);
						std::unique_ptr<Entry> newEntry(new Entry(folded));
						if(folded.length() > maxLength_)
							maxLength_ = folded.length();
						if(entries_[h % entries_.size()] != nullptr)
							newEntry->next = std::move(entries_[h % entries_.size()]);
						entries_[h % entries_.size()] = std::move(newEntry);
						++first;
					}
				}

				/**
				 * Returns the hash value of the specified string.
				 * @tparam SinglePassReadableRange UTF-16 character sequence
				 * @param characterSequence The character sequence
				 * @return The hash value
				 */
				template<typename SinglePassReadableRange>
				static inline std::uint32_t hashCode(const SinglePassReadableRange& characterSequence) {
					static_assert(
						text::CodeUnitSizeOf<boost::range_iterator<const SinglePassReadableRange>::type>::value == 2,
						"characterSequence should be 16-bit character sequence.");
					return boost::hash_range(boost::begin(characterSequence), boost::end(characterSequence));
				}

				/**
				 * Searches the specified string.
				 * @param textString The text string
				 * @return @c true if the specified string is found
				 */
				inline bool matches(const StringPiece& textString) const {
					if(caseSensitive_) {
						if(textString.length() > maxLength_)
							return false;
						const std::size_t h = hashCode(textString);
						for(const std::unique_ptr<Entry>* entry = &entries_[h % entries_.size()]; *entry != nullptr; entry = &(*entry)->next) {
							if((*entry)->data.length() == textString.length() && umemcmp((*entry)->data.data(), textString.cbegin(), (*entry)->data.length()) == 0)
								return true;
						}
					} else {
						const String folded(text::CaseFolder::fold(textString));
						const std::size_t h = hashCode(folded);
						for(const std::unique_ptr<Entry>* entry = &entries_[h % entries_.size()]; *entry != nullptr; entry = &(*entry)->next) {
							if((*entry)->data.length() == folded.length() && umemcmp((*entry)->data.data(), folded.data(), folded.length()) == 0)
								return true;
						}
					}
					return false;
				}

				BOOST_CONSTEXPR inline std::size_t maximumLength() const BOOST_NOEXCEPT {return maxLength_;}

			private:
				struct Entry {
					String data;
					std::unique_ptr<Entry> next;
					explicit Entry(const String& str) BOOST_NOEXCEPT : data(str) {}
				};
				std::vector<std::unique_ptr<Entry>> entries_;
				std::size_t maxLength_;	// the length of the longest keyword
				const bool caseSensitive_;
			};
		}
	}
}

#endif // !ASCENSION_HASH_TABLE_HPP
