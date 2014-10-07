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
#include <ascension/presentation/styles/length.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			template<typename T, int _initialValue>	// decltype(_initialValue) should be T...
			struct Enumerated : public boost::mpl::identity<T> {
				static type initialValue() {return static_cast<type>(_initialValue);}
			};

			template<int _initialValue, Length::Unit initialUnit>
			struct Lengthed : public boost::mpl::identity<Length> {
				static type initialValue() {return Length(_initialValue, initialUnit);}
			};

			template<typename T>
			struct Complex : public boost::mpl::identity<T> {
				static type initialValue() {return type();}
			};

			template<typename Variant, typename InitialType, int _initialValue>	// decltype(_initialValue) should be InitialType...
			struct Multiple : public boost::mpl::identity<Variant> {
				static InitialType initialValue() {return static_cast<InitialType>(_initialValue);}
			};

			/**
			 * A tag which specifies that the property is "Inherited Property" or not.
			 * @tparam yesOrNo Set @c true for "Inherited Property", otherwise @c false
			 * @see StyleProperty
			 */
			template<bool yesOrNo>
			struct Inherited : std::integral_constant<bool, yesOrNo> {};

			/**
			 * @defgroup explicit_defaulting_keywords Explicit Defaulting
			 * These values are used by constructors of @c StyleProperty class template.
			 * @{
			 */
			struct DefaultingKeyword {};
			struct InitialTag : DefaultingKeyword {};
			struct InheritTag : DefaultingKeyword {};
			struct UnsetTag : DefaultingKeyword {};

			/// Represents 'initial' keyword.
			BOOST_CONSTEXPR_OR_CONST InitialTag INITIAL;
			/// Represents 'inherit' keyword.
			BOOST_CONSTEXPR_OR_CONST InheritTag INHERIT;
			/// Represents 'unset' keyword.
			BOOST_CONSTEXPR_OR_CONST UnsetTag UNSET;
			/// @}

			/**
			 * Returns "Specified Value" type of the given property.
			 * @tparam Property @c StyleProperty class template
			 */
			template<typename Property>
			struct SpecifiedValueType : boost::mpl::identity<typename Property::value_type> {};
			/**
			 * Returns "Computed Value" type of the given property.
			 * @tparam Property @c StyleProperty class template
			 */
			template<typename Property>
			struct ComputedValueType : boost::mpl::identity<typename Property::_ComputedValueType> {};
		}

		/**
		 * Defines a style property. An instance represents a "Declared Value" or a "Cascaded Value".
		 * @tparam TypeSpec Specifies the type of "Specified Value" and "Computed Value". One of class templates in
		 *                  @c sp namespace
		 * @tparam InheritedTag Specifies if this is "Inherited Property" or not. Either
		 *                      @c styles#Inherited&lt;true&gt; or @c styles#Inherited&lt;false&gt;
		 * @tparam ComputedValueType
		 */
		template<typename TypeSpec, typename InheritedTag, typename ComputedValueType = void>
		class StyleProperty : public TypeSpec,
			private boost::equality_comparable<StyleProperty<TypeSpec, InheritedTag, ComputedValueType>>,
			private boost::equality_comparable<StyleProperty<TypeSpec, InheritedTag, ComputedValueType>, styles::InitialTag>,
			private boost::equality_comparable<StyleProperty<TypeSpec, InheritedTag, ComputedValueType>, styles::InheritTag>,
			private boost::equality_comparable<StyleProperty<TypeSpec, InheritedTag, ComputedValueType>, styles::UnsetTag> {
		public:
			typedef typename TypeSpec::type value_type;	///< The type of property value.
			static_assert(std::is_same<value_type, typename styles::SpecifiedValueType<StyleProperty>::type>::value, "");
			typedef typename std::conditional<
				!std::is_same<ComputedValueType, void>::value,
				ComputedValueType, value_type>::type _ComputedValueType;
			/// @c true if this property is 'inherited'.
			static const bool INHERITED = InheritedTag::value;
		public:
			/**
			 * Default constructor does the following:
			 * - If this property is 'inherited', set the inherit flag to @c true.
			 * - Otherwise, initializes the property value with the initial value.
			 */
			StyleProperty() : value_(initialValue()), inherits_(INHERITED ? &styles::INHERIT : nullptr) {}
			/**
			 * Constructor initializes the property value with the given value, ignores property's
			 * initial value and 'inherited' attribute.
			 */
			StyleProperty(const value_type& value) : value_(value), defaultingKeyword_(nullptr) {}

			/// Constructor sets 'initial' keyword.
			StyleProperty(const styles::InitialTag&) : defaultingKeyword_(&styles::INITIAL) {}
			/// Constructor sets 'inherit' keyword.
			StyleProperty(const styles::InheritTag&) : defaultingKeyword_(&styles::INHERIT) {}
			/// Constructor sets 'unset' keyword.
			StyleProperty(const styles::UnsetTag&) : defaultingKeyword_(&styles::UNSET) {}

			/**
			 * Copy-assignment operator resets with the given "Specified Value" or "Computed Value".
			 * @param other The "Specified Value" or "Computed Value" to set
			 * @return This object
			 */
			StyleProperty& operator=(const value_type& other) {
				return std::swap(*this, StyleProperty(other)), *this;
			}
			/**
			 * Move-assignment operator resets with the given "Specified Value" or "Computed Value".
			 * @param other The "Specified Value" or "Computed Value" to set
			 * @return This object
			 */
			StyleProperty& operator=(const value_type&& other) {
				return std::swap(*this, StyleProperty(other)), *this;
			}
			/// Sets 'initial' keyword.
			StyleProperty& operator=(const styles::InitialTag&) {
				StyleProperty temp(styles::INITIAL);
				return std::swap(*this, temp), *this;
			}
			/// Sets 'inherit' keyword.
			StyleProperty& operator=(const styles::InheritTag&) {
				StyleProperty temp(styles::INHERIT);
				return std::swap(*this, temp), *this;
			}
			/// Sets 'unset' keyword.
			StyleProperty& operator=(const styles::UnsetTag&) {
				StyleProperty temp(styles::UNSET);
				return std::swap(*this, temp), *this;
			}

			/// Equality operator for @c StyleProperty class template.
			template<typename InheritedOrNot2>
			bool operator==(const StyleProperty<TypeSpec, InheritedOrNot2>& other) const {
				if(isDefaultingKeyword())
					return defaultingKeyword_ == other.defaultingKeyword_;
				else if(other.isDefaultingKeyword())
					return false;
				return get() == other.get();
			}
			/// Equality operator returns @c true if this specifies 'initial' keyword.
			bool operator==(const styles::InitialTag&) {
				return defaultingKeyword_ == &styles::INITIAL;
			}
			/// Equality operator returns @c true if this specifies 'inherit' keyword.
			bool operator==(const styles::InheritTag&) {
				return defaultingKeyword_ == &styles::INHERIT;
			}
			/// Equality operator returns @c true if this specifies 'unset' keyword.
			bool operator==(const styles::UnsetTag&) {
				return defaultingKeyword_ == &styles::UNSET;
			}

			/**
			 * Returns the "Specified Value" or "Computed Value".
			 * @throw std#logic_error If @c #isDefaultingKeyword() returned @c true
			 */
			value_type& get() {
				if(isDefaultingKeyword())
					throw std::logic_error("");
				return boost::get(value_);
			}
			/**
			 * Returns the "Specified Value" or "Computed Value".
			 * @throw std#logic_error If @c #isDefaultingKeyword() returned @c true
			 */
			const value_type& get() const {
				if(isDefaultingKeyword())
					throw std::logic_error("");
				return boost::get(value_);
			}
			/// Returns the property value, or the @a defaultValue if this specifies defaulting keyword.
			const value_type& getOr(const value_type& defaultValue) const {
				return isDefaultingKeyword() ? defaultValue : boost::get(value_);
			}
			/// Returns the property value, or the initial value if this specifies defaulting keyword.
			value_type getOrInitial() const {
				return isDefaultingKeyword() ? initialValue() : boost::get(value_);
			}
			/// Returns the property value, or @c boost#none if inherits the parent.
			boost::optional<value_type> getOrNone() const {
				return isDefaultingKeyword() ? boost::none : value_;
			}
			/// Returns @c true if this is defaulting keyword.
			bool isDefaultingKeyword() const BOOST_NOEXCEPT {
				return defaultingKeyword_ != nullptr;
			}

		private:
			boost::optional<value_type> value_;
			const styles::DefaultingKeyword* defaultingKeyword_;
		};

		namespace styles {
			/**
			 * Calculates a "Specified Value" from the given "Cascaded Value" with the defaulting process.
			 * @tparam Property @c StyleProperty class template
			 * @param cascadedValue The "Cascaded Value" to process, or @c boost#none if not present
			 * @param parentComputedValue The "Computed Value" of the parent element for inheritance
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property>
			inline void specifiedValueFromCascadedValue(
					boost::optional<Property> cascadedValue,
					const typename ComputedValueType<Property>::type& parentComputedValue,
					typename SpecifiedValueType<Property>::type& specifiedValue) {
				if(cascadedValue == boost::none || boost::get(cascadedValue) == UNSET)
					specifiedValue = Property::INHERITED ? parentComputedValue : Property::initialValue();
				else if(!cascadedValue->isDefaultingKeyword())
					specifiedValue = cascadedValue.get();
				else if(boost::get(cascadedValue) == INITIAL)
					specifiedValue = Property::initialValue();
				else if(boost::get(cascadedValue) == INHERIT)
					specifiedValue = parentComputedValue;
				else
					ASCENSION_ASSERT_NOT_REACHED();
			}

			/**
			 * Calculates a "Specified Value" from the given "Cascaded Value" with the defaulting process.
			 * @tparam Property @c StyleProperty class template
			 * @tparam Function The type of @a parentComputedValueGenerator
			 * @param cascadedValue The "Cascaded Value" to process, or @c boost#none if not present
			 * @param parentComputedValueGenerator The function takes no parameter and returns the "Computed Value" of
			 *                                     the parent element for inheritance
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property, typename Function>
			inline void specifiedValueFromCascadedValue(
					boost::optional<Property> cascadedValue,
					Function parentComputedValueGenerator,
					typename SpecifiedValueType<Property>::type& specifiedValue) {
				if(cascadedValue == boost::none || boost::get(cascadedValue) == UNSET)
					specifiedValue = Property::INHERITED ? parentComputedValueGenerator() : Property::initialValue();
				else if(!cascadedValue->isDefaultingKeyword())
					specifiedValue = cascadedValue.get();
				else if(boost::get(cascadedValue) == INITIAL)
					specifiedValue = Property::initialValue();
				else if(boost::get(cascadedValue) == INHERIT)
					specifiedValue = parentComputedValueGenerator();
				else
					ASCENSION_ASSERT_NOT_REACHED();
			}
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_STYLE_PROPERTY_HPP
