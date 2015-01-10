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
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
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
			template<typename T, int _initialValue>	// decltype(_initialValue) should be T...
			struct Enumerated : public boost::mpl::identity<T> {
				static type initialValue() {return static_cast<type>(_initialValue);}
			};

			template<int _initialValue, Length::Unit initialUnit>
			struct Lengthed : public boost::mpl::identity<Length> {
				static type initialValue() {return Length(_initialValue, initialUnit);}
			};

			template<typename T, T(*initialValueGenerator)(void) = nullptr>
			struct Complex : public boost::mpl::identity<T> {
				static type initialValue() {return (initialValueGenerator != nullptr) ? (*initialValueGenerator)() : type();}
			};

			template<typename Variant, typename InitialType, int _initialValue>	// decltype(_initialValue) should be InitialType...
			struct Multiple : public boost::mpl::identity<Variant> {
				static InitialType initialValue() {return static_cast<InitialType>(_initialValue);}
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
#if 0
			/**
			 * @tparam TransformedSequence
			 * @tparam DefinedType
			 */
			template<typename TransformedSequence, typename DefinedType>
			class FindByDefinedType {
				typedef typename boost::fusion::result_of::find<typename TransformedSequence::Definition, DefinedType>::type FoundInDefinition;
				typedef typename boost::fusion::result_of::distance<
					typename boost::fusion::result_of::begin<typename TransformedSequence::Definition>::type, FoundInDefinition
				>::type Position;
				typedef typename boost::fusion::result_of::begin<TransformedSequence>::type Begin;
			public:
				typedef typename boost::fusion::result_of::advance<Begin, Position>::type type;
			};

			template<typename DefinedType, typename TransformedSequence>
			inline typename FindByDefinedType<TransformedSequence, DefinedType>::type findByDefinedType(TransformedSequence& sequence) {
				return FindByDefinedType<TransformedSequence, DefinedType>::type(sequence);
			}

			template<typename DefinedType, typename TransformedSequence>
			inline const typename FindByDefinedType<const TransformedSequence, DefinedType>::type findByDefinedType(const TransformedSequence& sequence) {
				return FindByDefinedType<const TransformedSequence, DefinedType>::type(sequence);
			}
#endif
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
				boost::fusion::at_key<Property>(computedValues) = boost::fusion::at_key<Property>(specifiedValues);
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
#if 0
				/**
				 * Default constructor does the following:
				 * - If this property is 'inherited', set the inherit flag to @c true.
				 * - Otherwise, initializes the property value with the initial value.
				 */
				DeclaredValue() : value_(initialValue()), defaultingKeyword_(INHERITED ? &styles::INHERIT : nullptr) {}
#else
				/// Default constructor sets 'unset' keyword.
				DeclaredValue() : defaultingKeyword_(&UNSET) {}
#endif
				/**
				 * Constructor initializes the property value with the given value, ignores property's initial value
				 * and 'inherited' attribute.
				 */
				DeclaredValue(const value_type& value) : value_(value), defaultingKeyword_(nullptr) {}
				/// Constructor sets 'initial' keyword.
				DeclaredValue(const InitialTag&) : defaultingKeyword_(&INITIAL) {}
				/// Constructor sets 'inherit' keyword.
				DeclaredValue(const InheritTag&) : defaultingKeyword_(&INHERIT) {}
				/// Constructor sets 'unset' keyword.
				DeclaredValue(const UnsetTag&) : defaultingKeyword_(&UNSET) {}

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
					return defaultingKeyword_ != nullptr;
				}

			private:
				boost::optional<value_type> value_;
				const styles::DefaultingKeyword* defaultingKeyword_;
			};

			template<typename Property>
			class DeclaredValue<FlowRelativeFourSides<Property>>
				: ValueBase<Property, FlowRelativeFourSides<typename DeclaredValue<Property>::type>> {};

			/// @defgroup cascading_and_defaulting Cascading and Defaulting of Style Values
			/// @{
			/**
			 * Implements "Cascading" describe by CSS Cascading and Inheritance Level 3.
			 * @tparam Property @c StyleProperty class template
			 * @param declaredValue The "Declared Value" to process
			 * @param[out] cascadedValue The "Cascaded Value"
			 */
			template<typename Property>
			inline void cascade(const boost::optional<Property>& declaredValue, boost::optional<Property>& cascadedValue) {
				// TODO: This code is temporary.
				cascadedValue = declaredValue;
			}

			/**
			 * Implements "Inheritance" process, as root element.
			 * @tparam Property @c StyleProperty class template
			 * @param[out] specifiedValue The calculated "Specified Value"
			 */
			template<typename Property>
			inline void inherit(std::nullptr_t, typename SpecifiedValue<Property>::type& specifiedValue) {
				specifiedValue = Property::initialValue();
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
				specifiedValue = parentComputedValueGenerator();
			}

			/**
			 * Calculates a "Specified Value" from the given "Cascaded Value" with the defaulting process.
			 * @tparam Property @c StyleProperty class template
			 * @tparam ParentComputedValue The type of @a parentComputedValue
			 * @param cascadedValue The "Cascaded Value" to process or any defaulting ketyword
			 * @param parentComputedValue Either of the followings give the "Computed Value" of the parent element: (a)
			 *                            A literal value of type @c ComputedValue&lt;Property&gt;::type. (b) A
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
