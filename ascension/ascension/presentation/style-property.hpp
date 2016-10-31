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
#include <ascension/presentation/flow-relative-four-sides.hpp>
#include <ascension/presentation/styles/length.hpp>
#include <boost/fusion/algorithm/query/find.hpp>
#include <boost/fusion/iterator/advance.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/distance.hpp>
#include <boost/fusion/sequence/intrinsic/at_key.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			template<typename T, T initialIntegralValue>
			struct Enumerated : public boost::mpl::identity<T> {
				static type initialValue() {return initialIntegralValue;}
			};

			template<int _initialValue, Length::Unit initialUnit>
			struct Lengthed : public boost::mpl::identity<Length> {
				static type initialValue() {return Length(_initialValue, initialUnit);}
			};

			template<typename T, T(*initialValueGenerator)(void) = nullptr>
			struct Complex : public boost::mpl::identity<T> {
				static type initialValue() {return (initialValueGenerator != nullptr) ? (*initialValueGenerator)() : type();}
			};

			template<typename Variant, typename InitialType>
			struct Multiple : public boost::mpl::identity<Variant> {
				static InitialType initialValue() {return InitialType();}
			};

			template<typename Variant, typename InitialType, InitialType initialIntegralValue>
			struct MultipleWithInitialInteger : public boost::mpl::identity<Variant> {
				static InitialType initialValue() {return initialIntegralValue;}
			};

			template<typename Variant, typename N>
			struct MultipleWithInitialIndex : public boost::mpl::identity<Variant> {
				typedef typename boost::mpl::at<typename Variant::types, N>::type InitialType;
				static InitialType initialValue() {return InitialType();}
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
			struct InitialTag : std::integral_constant<int, 0> {typedef int isDefaultingKeyword;};
			struct InheritTag : std::integral_constant<int, 1> {typedef int isDefaultingKeyword;};
			struct UnsetTag : std::integral_constant<int, 2> {typedef int isDefaultingKeyword;};

			/// Represents 'initial' keyword.
			BOOST_STATIC_CONSTEXPR InitialTag INITIAL;
			/// Represents 'inherit' keyword.
			BOOST_STATIC_CONSTEXPR InheritTag INHERIT;
			/// Represents 'unset' keyword.
			BOOST_STATIC_CONSTEXPR UnsetTag UNSET;
			/// @}

			/// @defgroup style_properties_metafunctions Style Properties Metafunctions
			/// @{
			/**
			 * Base type of other metafunctions.
			 * @tparam The property type
			 * @tparam ValueType The value type
			 */
			template<typename Property, typename ValueType>
			struct ValueBase : boost::mpl::identity<ValueType> {
				static_assert(!std::is_pointer<Property>::value, "'Property' can't be a pointer type.");
				static_assert(!std::is_reference<Property>::value, "'Property' can't be a reference type.");
#if 0
				typedef Property Definition;	///< The defined property type. Usually @c StyleProperty class template.
#endif
			};

			/**
			 * Returns "Specified Value" type of the given property.
			 * @tparam Property @c StyleProperty class template
			 */
			template<typename Property>
			struct SpecifiedValue : ValueBase<Property, typename Property::value_type> {
				static_assert(!std::is_same<typename std::remove_cv<type>::type, boost::mpl::void_>::value, "");
			};

			template<typename Property>
			struct SpecifiedValue<FlowRelativeFourSides<Property>>
					: ValueBase<FlowRelativeFourSides<Property>, FlowRelativeFourSides<typename SpecifiedValue<Property>::type>> {
				static_assert(!std::is_same<typename std::remove_cv<type>::type, boost::mpl::void_>::value, "");
			};

			/**
			 * Returns "Computed Value" type of the given property.
			 * @tparam Property @c StyleProperty class template
			 */
			template<typename Property>
			struct ComputedValue : ValueBase<Property, typename Property::_ComputedValueType> {};

			template<typename Property>
			struct ComputedValue<FlowRelativeFourSides<Property>>
				: ValueBase<FlowRelativeFourSides<Property>, FlowRelativeFourSides<typename ComputedValue<Property>::type>> {};

			/**
			 * Computes the given "Specified Value" as specified.
			 * @tparam Property The style property
			 * @param specifiedValue The "Specified Value" of the style property to compute
			 * @return The "Computed Value"
			 */
			template<typename Property>
			inline typename ComputedValue<Property>::type computeAsSpecified(const typename SpecifiedValue<Property>::type& specifiedValue) {
				return specifiedValue;
			}

			/**
			 * Computes the given "Specified Value" as specified.
			 * @tparam Property The style property
			 * @tparam SpecifiedStyles The type of @a specifiedValues
			 * @tparam ComputedStyles The type of @a computedValues
			 * @param specifiedValues The set of the "Specified Value"s which contains the style property to compute
			 * @param[out] computedValues The set of the "Computed Value"s
			 */
			template<typename Property, typename SpecifiedStyles, typename ComputedStyles>
			inline void computeAsSpecified(const SpecifiedStyles& specifiedValues, ComputedStyles& computedValues) {
				boost::fusion::at_key<Property>(computedValues) = computeAsSpecified<Property>(boost::fusion::at_key<Property>(specifiedValues));
			}
			/// @}
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
		class StyleProperty : public TypeSpec {
		public:
			typedef typename TypeSpec::type value_type;	///< The type of property value.
//			static_assert(std::is_same<value_type, typename styles::SpecifiedValue<StyleProperty>::type>::value, "");
			typedef typename std::conditional<
				!std::is_same<ComputedValueType, void>::value,
				ComputedValueType, value_type>::type _ComputedValueType;
			/// @c true if this property is 'inherited'.
			static const bool INHERITED = InheritedTag::value;
		};

		namespace styles {
			/**
			 * Represents a "Declared Value" and returns its type of the given property.
			 * @tparam Property
			 * @ingroup style_properties_metafunctions
			 */
			template<typename Property>
			class DeclaredValue :
				public ValueBase<Property, DeclaredValue<Property>>,
				private boost::equality_comparable<DeclaredValue<Property>>,
				private boost::equality_comparable<DeclaredValue<Property>, InitialTag>,
				private boost::equality_comparable<DeclaredValue<Property>, InheritTag>,
				private boost::equality_comparable<DeclaredValue<Property>, UnsetTag> {
			public:
				static_assert(!std::is_same<typename std::remove_cv<type>::type, boost::mpl::void_>::value, "");
				typedef typename Property::value_type value_type;	///< The type of the contributed value.
			public:
				/// Default constructor sets 'unset' keyword.
				DeclaredValue() : defaultingKeyword_(UnsetTag::value) {}
				/**
				 * Constructor initializes the property value with the given value, ignores property's initial value
				 * and 'inherited' attribute.
				 */
				DeclaredValue(const value_type& value) : value_(value) {}
				/**
				 * Constructor sets defaulting keyword.
				 * @tparam DefaultingKeyword The type of @a defaultingKeyword
				 * @param defaultingKeyword The defaulting keyword to set
				 */
				template<typename DefaultingKeyword>
				DeclaredValue(const DefaultingKeyword&,
					typename DefaultingKeyword::isDefaultingKeyword* = nullptr) : defaultingKeyword_(DefaultingKeyword::value) {}

				/**
				 * Copy-assignment operator resets with the given "Specified Value" or "Computed Value".
				 * @param other The "Specified Value" or "Computed Value" to set
				 * @return This object
				 */
				DeclaredValue& operator=(const value_type& other) {
					return std::swap(*this, DeclaredValue(other)), *this;
				}
				/**
				 * Move-assignment operator resets with the given "Specified Value" or "Computed Value".
				 * @param other The "Specified Value" or "Computed Value" to set
				 * @return This object
				 */
				DeclaredValue& operator=(value_type&& other) {
					return std::swap(*this, DeclaredValue(other)), *this;
				}
				/// Sets 'initial' keyword.
				DeclaredValue& operator=(const InitialTag&) {
					DeclaredValue temp(INITIAL);
					return std::swap(*this, temp), *this;
				}
				/// Sets 'inherit' keyword.
				DeclaredValue& operator=(const InheritTag&) {
					DeclaredValue temp(INHERIT);
					return std::swap(*this, temp), *this;
				}
				/// Sets 'unset' keyword.
				DeclaredValue& operator=(const UnsetTag&) {
					DeclaredValue temp(UNSET);
					return std::swap(*this, temp), *this;
				}

				/// Equality operator for @c StyleProperty class template.
				template<typename TypeSpec2, typename InheritedOrNot2>
				bool operator==(const DeclaredValue<StyleProperty<TypeSpec2, InheritedOrNot2>>& other) const {
					if(isDefaultingKeyword())
						return defaultingKeyword_ == other.defaultingKeyword_;
					else if(other.isDefaultingKeyword())
						return false;
					return get() == other.get();
				}
#if 1
				template<typename DefaultingKeyword>
				typename std::enable_if<std::is_same<typename DefaultingKeyword::isDefaultingKeyword, int>::value, bool>::type operator==(const DefaultingKeyword&) const BOOST_NOEXCEPT {
					return isDefaultingKeyword() && defaultingKeyword_ == DefaultingKeyword::value;
				}
#else
				/// Equality operator returns @c true if this specifies 'initial' keyword.
				bool operator==(const InitialTag&) const BOOST_NOEXCEPT {
					return defaultingKeyword_ == &INITIAL;
				}
				/// Equality operator returns @c true if this specifies 'inherit' keyword.
				bool operator==(const InheritTag&) const BOOST_NOEXCEPT {
					return defaultingKeyword_ == &INHERIT;
				}
				/// Equality operator returns @c true if this specifies 'unset' keyword.
				bool operator==(const UnsetTag&) const BOOST_NOEXCEPT {
					return defaultingKeyword_ == &UNSET;
				}
#endif

				/**
				 * Returns the contributed value.
				 * @throw std#logic_error If @c #isDefaultingKeyword() returned @c true
				 */
				value_type& get() {
					if(isDefaultingKeyword())
						throw std::logic_error("");
					return boost::get(value_);
				}
				/**
				 * Returns the contributed value.
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
					return value_ == boost::none;
				}

			private:
				boost::optional<value_type> value_;
				int defaultingKeyword_;
			};

			template<typename Property>
			class DeclaredValue<FlowRelativeFourSides<Property>>
				: ValueBase<Property, FlowRelativeFourSides<typename DeclaredValue<Property>::type>> {};

			/// @defgroup cascading_and_defaulting Cascading and Defaulting of Style Values
			/// @{
			/**
			 * Implements "Cascading" describe by CSS Cascading and Inheritance Level 3.
			 * @tparam Property @c StyleProperty class template
			 * @param declaredValues The "Declared Value" to process
			 * @return The "Cascaded Value"
			 */
			template<typename SinglePassReadableRange>
			inline typename boost::range_iterator<SinglePassReadableRange>::type cascade(const SinglePassReadableRange& declaredValues) {
				// TODO: This code is temporary.
				return boost::begin(declaredValues);
			}

			/**
			 * Converts the given "Computed Value" into a "Specified Value".
			 * This function is used to implement @c inherit functions.
			 * @tparam Property @c StyleProperty class template
			 * @param computedValue The "Computed Value" to uncompute
			 * @return The uncomputed "Specified Value"
			 * @note This is not described in CSS Cascading and Inheritance Level 3.
			 */
			template<typename Property>
			inline typename SpecifiedValue<Property>::type uncompute(const typename ComputedValue<Property>::type& computedValue) {
				static_assert(std::is_convertible<typename ComputedValue<Property>::type, typename SpecifiedValue<Property>::type>::value,
					"\"Computed Value\" of the style property must be convertible to \"Specified Value\".");
				return computedValue;
			}

			/// An empty tag type used to specify that the element is the root.
			struct HandleAsRoot : private boost::equality_comparable<HandleAsRoot> {
				/// Equality operator returns always @c true;
				BOOST_CONSTEXPR bool operator==(const HandleAsRoot&) const BOOST_NOEXCEPT {
					return true;
				}
			};

			/// Returns a hash value of the @c HandleAsRoot value.
			BOOST_CONSTEXPR inline std::size_t hash_value(const HandleAsRoot&) BOOST_NOEXCEPT {
				return 42;
			}

			/// An instance of an empty tag type @c HandleAsRoot.
			BOOST_STATIC_CONSTEXPR HandleAsRoot HANDLE_AS_ROOT;

			/**
			 * Implements "Inheritance" process, as root element.
			 * @tparam Property @c StyleProperty class template
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property>
			inline void inherit(HandleAsRoot, typename SpecifiedValue<Property>::type& specifiedValue) {
				static_assert(
					std::is_convertible<decltype(Property::initialValue()), typename SpecifiedValue<Property>::type>::value,
					"\"Initial Value\" of the style property must be convertible to \"Specified Value\".");
				specifiedValue = Property::initialValue();
			}

			/**
			 * Implements "Inheritance" process.
			 * @tparam Property @c StyleProperty class template
			 * @param parentComputedValue The "Computed Value" of the parent element
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property>
			inline void inherit(const typename ComputedValue<Property>::type& parentComputedValue, typename SpecifiedValue<Property>::type& specifiedValue) {
				 specifiedValue = uncompute<Property>(parentComputedValue);
			}

			/**
			 * Implements "Inheritance" process.
			 * @tparam Property @c StyleProperty class template
			 * @tparam Function The type of @a parentComputedValueGenerator
			 * @param parentComputedValueGenerator The function takes no parameter and returns the "Computed Value" of
			 *                                     the parent element
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property, typename Function>
			inline void inherit(Function parentComputedValueGenerator, typename SpecifiedValue<Property>::type& specifiedValue) {
				 specifiedValue = uncompute<Property>(parentComputedValueGenerator());
			}

			/**
			 * Calculates a "Specified Value" from the given "Cascaded Value" with the defaulting process.
			 * @tparam Property @c StyleProperty class template
			 * @tparam ParentComputedValue The type of @a parentComputedValue
			 * @param cascadedValue The "Cascaded Value" to process or any defaulting ketyword
			 * @param parentComputedValue Either of the followings give the "Computed Value" of the parent element: (a)
			 *                            A literal value of type @c ComputedValue&lt;Property&gt;#type. (b) A
			 *                            function takes no parameter and returns the. (c) An object of type
			 *                            @c HandleAsRoot if the element is the root
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property, typename ParentComputedValue>
			inline void specifiedValueFromCascadedValue(
					const typename DeclaredValue<Property>::type& cascadedValue,
					const ParentComputedValue& parentComputedValue, typename SpecifiedValue<Property>::type& specifiedValue) {
				if(cascadedValue == UNSET) {
					if(Property::INHERITED)
						inherit<Property>(parentComputedValue, specifiedValue);
					else
						specifiedValue = Property::initialValue();
				} else if(!cascadedValue.isDefaultingKeyword())
					specifiedValue = cascadedValue.get();
				else if(cascadedValue == INITIAL)
					specifiedValue = Property::initialValue();
				else if(cascadedValue == INHERIT)
					inherit<Property>(parentComputedValue, specifiedValue);
				else
					ASCENSION_ASSERT_NOT_REACHED();
			}
			/// @}
		}
	}
} // namespace ascension.presentation

#endif // !ASCENSION_STYLE_PROPERTY_HPP
