/**
 * @file style-property.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 text-style.hpp separated from presentation.hpp
 * @date 2011-07-24 separated from text-style.hpp
 * @date 2011-12-25 separated from writing-mode.hpp
 * @date 2012-07-15 renamed from inheritable.hpp
 */

#ifndef ASCENSION_STYLE_PROPERTY_HPP
#define ASCENSION_STYLE_PROPERTY_HPP
#include <ascension/corelib/basic-exceptions.hpp>	// UnknownValueException, std.logic_error
#include <ascension/corelib/future/type-traits.hpp>	// detail.Type2Type

namespace ascension {
	namespace presentation {

		template<typename T>
		struct InitializedByDefaultConstructor : public detail::Type2Type<T> {
			static const Type INITIAL_VALUE;
		};

		template<typename T, T initialValue>
		struct InitializedByLiteralValue : public detail::Type2Type<T> {
			static const Type INITIAL_VALUE = initialValue;
		};

		typedef std::true_type Inherited;
		typedef std::false_type NotInherited;

		/**
		 * @tparam TypeSpec
		 * @tparam InheritedOrNot
		 */
		template<typename TypeSpec, typename InheritedOrNot>
		class StyleProperty : public TypeSpec {
		public:
			/// The type of property value.
			typedef typename TypeSpec::Type value_type;
			/// @c true if this property is 'inherited'.
			static const bool INHERITED = InheritedOrNot::value;
		public:
			/**
			 * Default constructor does the following:
			 * - If this property is 'inherited', set the inherit flag to @c true.
			 * - Otherwise, initializes the property value with the initial value.
			 */
			StyleProperty() : value_(INITIAL_VALUE), inherits_(INHERITED) {}
			/**
			 * Constructor initializes the property value with the given value, ignores property's
			 * initial value and 'inherited' attribute.
			 */
			StyleProperty(const value_type& value) : value_(value), inherits_(false) {}
			/**
			 * Returns the property value.
			 * @throw std#logic_error If @c #inherits() returned @c true
			 */
			value_type& get() {
				if(inherits())
					throw std::logic_error("");
				return value_;
			}
			/**
			 * Returns the property value.
			 * @throw std#logic_error If @c #inherits() returned @c true
			 */
			const value_type& get() const {
				if(inherits())
					throw std::logic_error("");
				return value_;
			}
			/**
			 * Lets this object to inherit other property.
			 * @return This object
			 * @see #inherits
			 */
			StyleProperty& inherit() /*noexcept*/ {
				inherits_ = true;
				return *this;
			}
			/**
			 * Returns @c true if this object inherits other property.
			 * @see #inherit
			 */
			bool inherits() const /*noexcept*/ {
				return inherits_;
			}
			/**
			 * Lets this object to hold the given value without inheritance.
			 * @param value The property value to set
			 * @return This object
			 */
			StyleProperty& set(const value_type& value) {
				value_ = value;
				inherits_ = false;
				return *this;
			}
		private:
			value_type value_;
			bool inherits_;
		};
#if 0
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
#endif
	}
} // namespace ascension.presentation

#endif // !ASCENSION_STYLE_PROPERTY_HPP
