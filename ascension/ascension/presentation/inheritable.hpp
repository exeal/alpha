/**
 * @file inheritable.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 text-style.hpp separated from presentation.hpp
 * @date 2011-07-24 separated from text-style.hpp
 * @date 2011-12-25 separated from writing-mode.hpp
 */

#ifndef ASCENSION_INHERITABLE_HPP
#define ASCENSION_INHERITABLE_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException, std.logic_error
#include <ascension/corelib/type-traits.hpp>		// detail.Select

namespace ascension {
	namespace presentation {

		/**
		 * 
		 * @note This class template has some known specializations. See text-line-style.hpp,
		 *       text-style.hpp and writing-mode.hpp header files.
		 * @tparam T The property type.
		 */
		template<typename T>
		class Inheritable {
		public:
			/// The type of property value.
			typedef T value_type;
			/// This type.
			typedef Inheritable<T> Type;
		public:
			/// Default constructor makes an object inherits other property.
			Inheritable() /*throw()*/ : inherits_(true) {}

			/**
			 * Constructor makes an object holds the specified property value without inheritance.
			 * @param value The property value to set
			 */
			Inheritable(value_type value) /*throw()*/ : value_(value), inherits_(false) {}

			/// Returns @c #get().
			operator value_type() const {return get();}

			/**
			 * Returns the property value.
			 * @throw std#logic_error If @c #inherits() returned @c true
			 */
			value_type get() const {
				if(inherits())
					throw std::logic_error("");
				return value_;
			}

			/**
			 * Lets this object to inherit other property.
			 * @return This object
			 */
			Inheritable<value_type>& inherit() /*throw()*/ {
				inherits_ = true;
				return *this;
			}

			/// Returns @c true if this object inherits other property.
			bool inherits() const /*throw()*/ {return inherits_;}

			/**
			 * Lets this object to hold the specified property value without inheritance.
			 * @param value The property value to set
			 * @return This object
			 */
			Inheritable<value_type>& set(value_type value) /*throw()*/ {
				value_ = value;
				inherits_ = false;
				return *this;
			}
		private:
			value_type value_;
			bool inherits_;
		};

		/**
		 * Generates @c Inheritable type according to the given condition.
		 * @tparam condition The boolean condition
		 * @tpatam T The base property type
		 */
		template<bool condition, typename T>
		struct InheritableIf {
			/// @c Inheritable&lt;T&gt; if @c condition is @c true. Otherwise, @c T.
			typedef typename std::conditional<condition, Inheritable<T>, T>::type Type;
		};

		/**
		 * Resolves the inheritance of the given property value.
		 * @tparam T The property type.
		 * @param inheritable The property value to test
		 * @param defaultValue The default value used to resolve inheritance 
		 * @retval @a inheritable.get() If @c inheritable.inherits() is @c false
		 * @retval @a defaultValue If @c inheritable.inherits() is @c true
		 */
		template<typename T>
		inline T resolveInheritance(const Inheritable<T>& inheritable, const T& defaultValue) {
			 return inheritable.inherits() ? defaultValue : inheritable.get();
		}

	}
} // namespace ascension.presentation

#endif // !ASCENSION_INHERITABLE_HPP
