/**
 * @file collator.hpp
 * @author exeal
 * @date 2005-2011 was unicode.hpp
 * @date 2011-04-27 separated from unicode.hpp
 */

#ifndef ASCENSION_COLLATOR_HPP
#define ASCENSION_COLLATOR_HPP

#include <ascension/config.hpp>				// ASCENSION_NO_UNICODE_COLLATION

#ifndef ASCENSION_NO_UNICODE_COLLATION
#include <ascension/corelib/memory.hpp>		// AutoBuffer
#include <ascension/corelib/text/unicode.hpp>
#include <memory>							// std.unique_ptr

#if ASCENSION_UNICODE_VERSION > 0x0510
#	error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {

	namespace text {

		class CollationKey : public FastArenaObject<CollationKey> {
		public:
			CollationKey() /*throw()*/ : length_(0) {}
			CollationKey(AutoBuffer<const uint8_t> keyValues,
				std::size_t length) : keyValues_(keyValues), length_(length) {}
			CollationKey(const CollationKey& other);
			CollationKey&operator=(const CollationKey& other);
			bool operator==(const CollationKey& other) const /*throw()*/;
			bool operator!=(const CollationKey& other) const /*throw()*/;
			bool operator<(const CollationKey& other) const /*throw()*/;
			bool operator<=(const CollationKey& other) const /*throw()*/;
			bool operator>(const CollationKey& other) const /*throw()*/;
			bool operator>=(const CollationKey& other) const /*throw()*/;
		private:
			const AutoBuffer<const uint8_t> keyValues_;
			const std::size_t length_;
		};

		class CollationElementIterator {
		public:
			static const int NULL_ORDER;
		public:
			virtual ~CollationElementIterator() /*throw()*/;
			bool equals(const CollationElementIterator& other) const {
				return position() == other.position();
			}
			bool less(const CollationElementIterator &other) const {
				return position() < other.position();
			}
		public:
			virtual int current() const = 0;
			virtual void next() = 0;
			virtual void previous() = 0;
			virtual std::size_t position() const = 0;
		protected:
			CollationElementIterator() /*throw()*/;
		};

		class Collator {
		public:
			enum Strength {
				PRIMARY = 0,
				SECONDARY = 1,
				TERTIARY = 2,
				QUATERNARY = 3,
				IDENTICAL = 15
			};
			// constructor
			virtual ~Collator() /*throw()*/;
			// attributes
			Decomposition decomposition() const /*throw()*/;
			void setDecomposition(Decomposition newDecomposition);
			void setStrength(Strength newStrength);
			Strength strength() const /*throw()*/;
			// operations
			virtual std::unique_ptr<CollationKey> collationKey(const String& s) const = 0;
			int compare(const String& s1, const String& s2) const;
			virtual int compare(const CharacterIterator& s1, const CharacterIterator& s2) const = 0;
			std::unique_ptr<CollationElementIterator>
				createCollationElementIterator(const String& source) const;
			virtual std::unique_ptr<CollationElementIterator>
				createCollationElementIterator(const CharacterIterator& source) const = 0;
		protected:
			Collator() /*throw()*/ : strength_(IDENTICAL), decomposition_(NO_DECOMPOSITION) {}
		private:
			Strength strength_;
			Decomposition decomposition_;
		};

		/// @c NullCollator performs binary comparison.
		class NullCollator : public Collator {
		public:
			NullCollator() /*throw()*/;
			std::unique_ptr<CollationKey> collationKey(const String& s) const;
			int compare(const CharacterIterator& s1, const CharacterIterator& s2) const;
			std::unique_ptr<CollationElementIterator>
				createCollationElementIterator(const CharacterIterator& source) const;
		private:
			class ElementIterator : public CollationElementIterator {
			public:
				explicit ElementIterator(std::unique_ptr<CharacterIterator> source) /*throw()*/ : i_(std::move(source)) {}
				~ElementIterator() /*throw()*/ {}
				int current() const {return i_->hasNext() ? i_->current() : NULL_ORDER;}
				void next() {i_->next();}
				void previous() {i_->previous();}
				std::size_t position() const {return i_->offset();}
			private:
				std::unique_ptr<CharacterIterator> i_;
			};
		};

		/**
		 * @c DefaultCollator uses DUCET (Default Unicode Collation Element Table) to collate
		 * characters and strings.
		 */
		class DefaultCollator : public Collator {};

	}
} // namespace ascension.text

#endif // !ASCENSION_NO_UNICODE_COLLATION
#endif // !ASCENSION_UNICODE_HPP
