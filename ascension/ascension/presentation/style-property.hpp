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
#include <ascension/presentation/length.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {

		namespace sp {
			template<typename T, int _initialValue>	// decltype(_initialValue) should be T...
			struct Enumerated : public detail::Type2Type<T> {
				static Type initialValue() {return static_cast<Type>(_initialValue);}
			};

			template<int _initialValue, Length::Unit initialUnit>
			struct Lengthed : public detail::Type2Type<Length> {
				static Type initialValue() {return Length(_initialValue, initialUnit);}
			};

			template<typename T>
			struct Complex : public detail::Type2Type<T> {
				static Type initialValue() {return Type();}
			};

			template<typename Variant, typename InitialType, int _initialValue>	// decltype(_initialValue) should be InitialType...
			struct Multiple : public detail::Type2Type<Variant> {
				static InitialType initialValue() {return static_cast<InitialType>(_initialValue);}
			};

			typedef std::true_type Inherited;
			typedef std::false_type NotInherited;
		}

		/**
		 * @tparam TypeSpec One of class templates in @c sp namespace
		 * @tparam InheritedOrNot Either @c sp#Inherited or @c sp#NotInherited
		 */
		template<typename TypeSpec, typename InheritedOrNot>
		class StyleProperty : public TypeSpec,
			private boost::equality_comparable<StyleProperty<TypeSpec, InheritedOrNot>> {
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
			StyleProperty() : value_(initialValue()), inherits_(INHERITED) {}
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
			/// Returns the property value, or the @a defaultValue if inherits the parent.
			const value_type& getOr(const value_type& defaultValue) const {
				return inherits() ? defaultValue : value_;
			}
			value_type getOr(const StyleProperty* parent, const StyleProperty& ancestor) const {
				if(!inherits())
					return value_;
				else if(parent != nullptr && !parent->inherits())
					return parent->value_;
				return ancestor.getOrInitial();
			}
			/// Returns the property value, or the initial value if inherits the parent.
			value_type getOrInitial() const {
				return inherits() ? initialValue() : value_;
			}
			/// Returns the property value, or @c boost#none if inherits the parent.
			boost::optional<value_type> getOrNone() const {
				return inherits() ? boost::none : boost::make_optional(value_);
			}
			/**
			 * Lets this object to inherit other property.
			 * @return This object
			 * @see #inherits
			 */
			StyleProperty& inherit() BOOST_NOEXCEPT {
				inherits_ = true;
				return *this;
			}
			/**
			 * Returns @c true if this object inherits other property.
			 * @see #inherit
			 */
			bool inherits() const BOOST_NOEXCEPT {
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

		/// Equality operator for @c StyleProperty class template.
		template<typename TypeSpec, typename InheritedOrNot1, typename InheritedOrNot2>
		inline bool operator==(
				const StyleProperty<TypeSpec, InheritedOrNot1>& lhs,
				const StyleProperty<TypeSpec, InheritedOrNot2>& rhs) {
			if(lhs.inherits())
				return rhs.inherits();
			else if(rhs.inherits())
				return false;
			return lhs.get() == rhs.get();
		}

		namespace sp {
			template<typename Property>
			class IntrinsicType : public detail::Type2Type<typename Property::value_type> {};
		}
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
