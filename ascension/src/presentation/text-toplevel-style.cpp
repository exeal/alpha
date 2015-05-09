/**
 * @file text-toplevel-style.cpp
 * @author exeal
 * @date 2014-10-14 Created.
 */

#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/functional/hash/hash.hpp>

namespace ascension {
	namespace presentation {
		/**
		 * Computes and creates a @c ComputedTextToplevelStyle.
		 * @param specifiedValues The "Specified Value"s of @c TextToplevelStyle properties
		 */
		ComputedTextToplevelStyle::ComputedTextToplevelStyle(const SpecifiedTextToplevelStyle& specifiedValues) {
			styles::computeAsSpecified<styles::WritingMode>(specifiedValues, *this);
		}

		/**
		 * Default constructor.
		 * @post @c *#linesStyle() == @c DeclaredTextLineStyle#unsetInstance()
		 */
		DeclaredTextToplevelStyle::DeclaredTextToplevelStyle() {
			setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle>());
		}

		/**
		 * Sets the default @c DeclaredTextLineStyle of this toplevel element.
		 * @param newStyle The style collection to set. If @c null, this @c DeclaredTextToplevelStyle holds the value
		 *                 of @c DeclaredTextLineStyle#unsetInstance()
		 */
		void DeclaredTextToplevelStyle::setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle> newStyle) BOOST_NOEXCEPT {
			if(newStyle.get() != nullptr)
				linesStyle_ = newStyle;
			else
				linesStyle_ = std::shared_ptr<const DeclaredTextLineStyle>(&DeclaredTextLineStyle::unsetInstance(), boost::null_deleter());
		}

		/// Returns a @c DeclaredTextToplevelStyle instance filled with @c styles#UNSET values.
		const DeclaredTextToplevelStyle& DeclaredTextToplevelStyle::unsetInstance() {
			static const DeclaredTextToplevelStyle SINGLETON;
			return SINGLETON;
		}

		namespace {
			template<template<typename> class Metafunction>
			inline std::size_t hashTextToplevelStyle(const typename Metafunction<TextToplevelStyle>::type& style) {
				return boost::hash_value(boost::fusion::at_key<styles::WritingMode>(style));
			}
		}

		/// @c boost#hash_value for @c styles#ComputedValue&lt;TextToplevelStyle&gt;type.
		std::size_t hash_value(const styles::ComputedValue<TextToplevelStyle>::type& style) {
			return hashTextToplevelStyle<styles::ComputedValue>(style);
		}

		/// @c boost#hash_value for @c styles#SpecifiedValue&lt;TextToplevelStyle&gt;type.
		std::size_t hash_value(const styles::SpecifiedValue<TextToplevelStyle>::type& style) {
			return hashTextToplevelStyle<styles::SpecifiedValue>(style);
		}
	}
}
