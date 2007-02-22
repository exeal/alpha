/**
 * @file collator.hpp
 * @author exeal
 * @date 2007
 * @see unicode-utils.hpp
 */

#ifndef ASCENSION_COLLATOR_HPP
#define ASCENSION_COLLATOR_HPP
#include "unicode-utils.hpp"

namespace ascension {
	namespace unicode {

		class CollationKey : public manah::FastArenaObject<CollationKey> {
		public:
			bool	operator==(const CollationKey& rhs) const throw();
			bool	operator!=(const CollationKey& rhs) const throw();
			bool	operator<(const CollationKey& rhs) const throw();
			bool	operator<=(const CollationKey& rhs) const throw();
			bool	operator>(const CollationKey& rhs) const throw();
			bool	operator>=(const CollationKey& rhs) const throw();
			String	getSourceString() const throw();
		};

		class CollationElementIterator : public std::iterator<std::bidirectional_iterator_tag, int> {
		public:
			static const NULL_ORDER;
			// operators
			int operator*() const {return current();}
			CollationElementIterator& operator++() {increment(); ++position_; return *this;}
			CollationElementIterator& operator--() {decrement(); --position_; return *this;}
			bool operator==(const CollationElementIterator& rhs) const throw() {return position_ == rhs.position_;}
			bool operator!=(const CollationElementIterator& rhs) const throw() {return position_ != rhs.position_;}
			bool operator<(const CollationElementIterator& rhs) const throw() {return position_ < rhs.position_;}
			bool operator<=(const CollationElementIterator& rhs) const throw() {return position_ <= rhs.position_;}
			bool operator>(const CollationElementIterator& rhs) const throw() {return position_ > rhs.position_;}
			bool operator>=(const CollationElementIterator& rhs) const throw() {return position_ >= rhs.position_;}
			std::ptrdiff_t operator-(const CollationElementIterator& rhs) const throw() {return position_ - rhs.position_;}
		protected:
			CollationElementIterator(std::ptrdiff_t initialPosition) throw() : position_(initialPosition) {}
			virtual int		current() const = 0;
			virtual void	increment() = 0;
			virtual void	decrement() = 0;
		private:
			std::ptrdiff_t position_;
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
			virtual ~Collator() throw();
			// attributes
			Decomposition	getDecomposition() const throw();
			Strength		getStrength() const throw();
			void			setDecomposition(Decomposition newDecomposition);
			void			setStrength(Strength newStrength);
			// operations
			int												compare(const String& s1, const String& s2) const;
			virtual int										compare(const CharacterIterator& s1, const CharacterIterator& s2) const = 0;
			std::auto_ptr<CollationElementIterator>			createCollationElementIterator(const String& source) const;
			virtual std::auto_ptr<CollationElementIterator>	createCollationElementIterator(const CharacterIterator& source) const = 0;
			virtual std::auto_ptr<CollationKey>				getCollationKey(const String& s) const = 0;
		private:
			Strength strength_;
			Decomposition decomposition_;
		};

	} // namespace unicode
} // namespace ascension

#endif /* !ASCENSION_COLLATOR_HPP */
