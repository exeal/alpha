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
			CollationKey() throw() : length_(0) {}
			CollationKey(AutoBuffer<const uchar> keyValues, std::size_t length) : keyValues_(keyValues), length_(length) {}
			bool	operator==(const CollationKey& rhs) const throw();
			bool	operator!=(const CollationKey& rhs) const throw();
			bool	operator<(const CollationKey& rhs) const throw();
			bool	operator<=(const CollationKey& rhs) const throw();
			bool	operator>(const CollationKey& rhs) const throw();
			bool	operator>=(const CollationKey& rhs) const throw();
		private:
			const AutoBuffer<const uchar> keyValues_;
			const std::size_t length_;
		};

		class CollationElementIterator : public BidirectionalIteratorFacade<CollationElementIterator, const int> {
		public:
			static const NULL_ORDER;
			// operators
			bool operator<(const CollationElementIterator& rhs) const {return position() < rhs.position();}
			bool operator<=(const CollationElementIterator& rhs) const {return position() <= rhs.position();}
			bool operator>(const CollationElementIterator& rhs) const {return position() > rhs.position();}
			bool operator>=(const CollationElementIterator& rhs) const {return position() >= rhs.position();}
			std::ptrdiff_t operator-(const CollationElementIterator& rhs) const {return position() - rhs.position();}
		protected:
			CollationElementIterator() throw() {}
			virtual int			current() const = 0;
			virtual void		next() = 0;
			virtual void		previous() = 0;
			virtual std::size_t	position() const = 0;
		private:
			reference dereference() const {return current();}
			void increment() {next();}
			void decrement() {previous();}
			bool equals(const CollationElementIterator& rhs) const {return position() == rhs.position();}
			friend class Facade;
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
		protected:
			Collator() throw() : strength_(IDENTICAL), decomposition_(NO_DECOMPOSITION) {}
		private:
			Strength strength_;
			Decomposition decomposition_;
		};

		/// @c NullCollator performs binary comparison.
		class NullCollator : public Collator {
		public:
			NullCollator() throw() {}
			int compare(const CharacterIterator& s1, const CharacterIterator& s2) const {
				if(getStrength() == PRIMARY)
					return CaseFolder::compare(s1, s2);
				std::auto_ptr<CharacterIterator> i1(s1.clone()), i2(s2.clone());
				while(!i1.isLast() && !i2.isLast()) {
					if(i1.current() < i2.current())
						return -1;
					else if(i1.current() > i2.current())
						return +1;
				}
				if(!i2.isLast()) return -1;
				if(!i1.isLast()) return +1;
				return 0;
			}
			std::auto_ptr<CollationElementIterator> createCollationElementIterator(const CharacterIterator& source) const {
				return std::auto_ptr<CollationElementIterator>(new ElementIterator(source));
			}
			std::auto_ptr<CollationKey> getCollationKey(const String& s) const {
				return std::auto_ptr<CollationKey>(new CollationKey(reinterpret_cast<const uchar*>(s.data()), s.length() * sizeof(Char)));
			}
		private:
			class ElementIterator : public CollationElementIterator {
			public:
				int current() const {return i_->isLast() ? NULL_ORDER : i_->current();}
				void next() {i_->next();}
				void previous() {i_->previous();}
				std::size_t position() const {return i_->getOffset();}
			private:
				std::auto_ptr<CharacterIterator> i_;
			};
		};

		/**
		 * @c DefaultCollator uses DUCET (Default Unicode Collation Element Table) to collate
		 * characters and strings.
		 */
		class DefaultCollator : public Collator {};

	} // namespace unicode
} // namespace ascension

#endif /* !ASCENSION_COLLATOR_HPP */
