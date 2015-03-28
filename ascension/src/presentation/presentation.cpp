/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2015
 */

#include <ascension/corelib/numeric-range-algorithm/includes.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/hyperlink/hyperlink-detector.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/flyweight.hpp>
#include <boost/foreach.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>


namespace ascension {
	namespace graphics {
		/// A transparent object whose all components are zero.
		const Color Color::TRANSPARENT_BLACK(0, 0, 0, 0);
		/// An opaque object whose all components are zero.
		const Color Color::OPAQUE_BLACK(0, 0, 0, 0xff);
		/// An opaque object whose all components are 255.
		const Color Color::OPAQUE_WHITE(0xff, 0xff, 0xff, 0xff);

	}

	namespace presentation {
		namespace styles {
/*			namespace linewidthkeywords {
				// TODO: these value are changed later.
				const Length THIN(0.05f, Length::EM_HEIGHT);
				const Length MEDIUM(0.10f, Length::EM_HEIGHT);
				const Length THICK(0.20f, Length::EM_HEIGHT);
			}
*/		}


		// Presentation ///////////////////////////////////////////////////////////////////////////////////////////////

		struct Presentation::ComputedStyles {
			boost::flyweight<ComputedTextRunStyle> forRuns;
			boost::flyweight<ComputedTextLineStyle> forLines;
			boost::flyweight<ComputedTextToplevelStyle> forToplevel;
		};

		struct Presentation::Hyperlinks {
			Index lineNumber;
			std::unique_ptr<hyperlink::Hyperlink*[]> hyperlinks;	// TODO: This should be container class.
			std::size_t numberOfHyperlinks;
		};

		/**
		 * @class ascension::presentation::Presentation
		 * A bridge between the document and visual styled text.
		 *
		 * <h3>The Default (Reading) Direction</h3>
		 *
		 * This value is used only when the all other rules declared @c styles#Direction style property as 'unset'. All
		 * other rules take precedence over the default direction. You can set this value by calling
		 * @c #setDefaultDirection method. The default value is @c ASCENSION_DEFAULT_TEXT_READING_DIRECTION.
		 *
		 * @note This class is not intended to be subclassed.
		 * @see kernel#Document, kernel#DocumentPartitioner
		 */

		/**
		 * @typedef ascension::presentation::Presentation::ComputedTextToplevelStyleChangedSignal
		 * The signal which gets emitted when the computed text toplevel style of @c Presentation was changed.
		 * @param presentation The @c Presentation object
		 * @param previouslyDeclared The declared style used before the change
		 * @param previouslyComputed The computed style used before the change
		 * @see #computedTextToplevelStyle, #computedTextToplevelStyleChangedSignal, #declaredTextToplevelStyle,
		 *      #setDeclaredTextToplevelStyle
		 */

		/**
		 * Constructor.
		 * @param document The target document
		 */
		Presentation::Presentation(kernel::Document& document) BOOST_NOEXCEPT : document_(document), defaultDirection_(ASCENSION_DEFAULT_TEXT_READING_DIRECTION) {
			setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle>());
			document_.addListener(*this);
		}
		
		/// Destructor.
		Presentation::~Presentation() BOOST_NOEXCEPT {
			document_.removeListener(*this);
			clearHyperlinksCache();
		}

		/**
		 * Registers the text line color specifier.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param specifier The specifier to register
		 * @throw NullPointerException @a specifier is @c null
		 */
		void Presentation::addTextLineColorSpecifier(std::shared_ptr<TextLineColorSpecifier> specifier) {
			if(specifier.get() == nullptr)
				throw NullPointerException("specifier");
			textLineColorSpecifiers_.push_back(specifier);
		}

		void Presentation::clearHyperlinksCache() BOOST_NOEXCEPT {
			BOOST_FOREACH(Hyperlinks* p, hyperlinks_) {
				for(size_t j = 0; j < p->numberOfHyperlinks; ++j)
					delete p->hyperlinks[j];
				delete p;
			}
			hyperlinks_.clear();
		}

		/// Returns the "Computed Value" of @c TextLineStyle in the toplevel style declared by
		/// @c #setDeclaredTextToplevelStyle method.
		const ComputedTextLineStyle& Presentation::computedTextLineStyle() const BOOST_NOEXCEPT {
			return computedStyles_->forLines.get();
		}

		/// Returns the "Computed Value" of @c TextRunStyle in the toplevel style declared by
		/// @c #setDeclaredTextToplevelStyle method.
		const ComputedTextRunStyle& Presentation::computedTextRunStyle() const BOOST_NOEXCEPT {
			return computedStyles_->forRuns.get();
		}

		/// Returns the "Computed Value" of the toplevel style declared by @c #setDeclaredTextToplevelStyle method.
		const ComputedTextToplevelStyle& Presentation::computedTextToplevelStyle() const BOOST_NOEXCEPT {
			return computedStyles_->forToplevel.get();
		}

		/// Returns @c ComputedTextToplevelStyleChanged signal.
		SignalConnector<Presentation::ComputedTextToplevelStyleChanged> Presentation::computedTextToplevelStyleChangedSignal() BOOST_NOEXCEPT {
			return makeSignalConnector(computedTextToplevelStyleChangedSignal_);
		}

		namespace {
			template<typename CascadedValues, typename ParentComputedValues, typename SpecifiedValues>
			struct SpecifiedValuesFromCascadedValues {
				SpecifiedValuesFromCascadedValues(const CascadedValues& cascadedValues,
					const ParentComputedValues& parentComputedValues, typename SpecifiedValues& specifiedValues) :
					cascadedValues(cascadedValues), parentComputedValues(parentComputedValues), specifiedValues(specifiedValues) {}
				template<typename Property, typename SpecifiedValue>
				void operator()(boost::fusion::pair<Property, SpecifiedValue>& p) const {
					handle<Property, SpecifiedValue, ParentComputedValues>(p);
				}
				template<typename Property, typename SpecifiedValue>
				void operator()(boost::fusion::pair<FlowRelativeFourSides<Property>, FlowRelativeFourSides<SpecifiedValue>>& p) const {
					handle<Property, SpecifiedValue, ParentComputedValues>(p);
				}
				const CascadedValues& cascadedValues;
				const ParentComputedValues& parentComputedValues;
				mutable SpecifiedValues& specifiedValues;	// mutable because of boost.fusion.for_each
			private:
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<Property, SpecifiedValue>& p,
						typename std::enable_if<!std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					styles::specifiedValueFromCascadedValue<Property>(
						boost::fusion::at_key<Property>(cascadedValues),
						boost::fusion::at_key<Property>(parentComputedValues), p.second);
				}
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<Property, SpecifiedValue>& p,
						typename std::enable_if<std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					styles::specifiedValueFromCascadedValue<Property>(
						boost::fusion::at_key<Property>(cascadedValues), styles::HANDLE_AS_ROOT, p.second);
				}
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<FlowRelativeFourSides<Property>, FlowRelativeFourSides<SpecifiedValue>>& p,
						typename std::enable_if<!std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					auto& cascadedSideValues = boost::fusion::at_key<FlowRelativeFourSides<Property>>(cascadedValues);
					auto& parentComputedSideValues = boost::fusion::at_key<FlowRelativeFourSides<Property>>(parentComputedValues);
					for(std::size_t i = 0; i < cascadedSideValues.size(); ++i)
						styles::specifiedValueFromCascadedValue<Property>(cascadedSideValues[i], parentComputedSideValues[i], p.second[i]);
				}
				template<typename Property, typename SpecifiedValue, typename P>
				void handle(boost::fusion::pair<FlowRelativeFourSides<Property>, FlowRelativeFourSides<SpecifiedValue>>& p,
						typename std::enable_if<std::is_same<P, styles::HandleAsRoot>::value>::type* = nullptr) const {
					auto& cascadedSideValues = boost::fusion::at_key<FlowRelativeFourSides<Property>>(cascadedValues);
					for(std::size_t i = 0; i < cascadedSideValues.size(); ++i)
						styles::specifiedValueFromCascadedValue<Property>(cascadedSideValues[i], styles::HANDLE_AS_ROOT, p.second[i]);
				}
			};

			template<typename CascadedValues, typename ParentComputedValues, typename SpecifiedValues>
			inline void specifiedValuesFromCascadedValues(const CascadedValues& cascadedValues,
					const ParentComputedValues& parentComputedValues, SpecifiedValues& specifiedValues) {
				static_assert(boost::fusion::traits::is_sequence<CascadedValues>::value, "'cascadedValues' is not boost.fusion sequence.");
				static_assert(boost::fusion::traits::is_sequence<SpecifiedValues>::value, "'specifiedValues' is not boost.fusion sequence.");
				static_assert(boost::fusion::traits::is_sequence<ParentComputedValues>::value
					|| std::is_same<typename std::decay<ParentComputedValues>::type, styles::HandleAsRoot>::value,
					"'parentComputedValues' is neither boost.fusion sequence nor 'styles.handleAsRoot'.");
//				static_assert(std::is_same<ParentComputedValues, typename styles::ComputedValue<Styles>::type>::value
//					|| std::is_same<typename std::decay<ParentComputedValues>::type, styles::HandleAsRoot>::value, "");
				const SpecifiedValuesFromCascadedValues<CascadedValues, ParentComputedValues, SpecifiedValues> compressor(cascadedValues, parentComputedValues, specifiedValues);
				boost::fusion::for_each(specifiedValues, compressor);
			}
		}

		/**
		 * Returns the style of the specified text line.
		 * @param line The line number
		 * @param[out] result The computed text line style
		 * @throw BadPositionException @a line is outside of the document
		 * @throw NullPointerException Internal @c Length#value call may throw this exception
		 */
		void Presentation::computeTextLineStyle(Index line, ComputedTextLineStyle& result) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));

			const std::shared_ptr<const DeclaredTextLineStyle> declared(declaredTextLineStyle(line));
			const std::shared_ptr<const DeclaredTextLineStyle> cascaded(*styles::cascade(boost::make_iterator_range_n(&declared, 1)));
			SpecifiedTextLineStyle specified;
			specifiedValuesFromCascadedValues(*cascaded, computedTextLineStyle(), specified);
		}

		namespace {		
			class ComputedStyledTextRunIteratorImpl : public ComputedStyledTextRunIterator {
			public:
				ComputedStyledTextRunIteratorImpl(
						std::unique_ptr<DeclaredStyledTextRunIterator> declaration, const styles::ComputedValue<TextRunStyle>::type& computedParentStyles)
						: declaration_(std::move(declaration)), computedParentStyles_(computedParentStyles) {
				}
				// ComputedStyledTextRunIterator
				bool isDone() const override BOOST_NOEXCEPT {
					return declaration_->isDone();
				}
				void next() override {
					return declaration_->next();
				}
				kernel::Position position() const override {
					return declaration_->position();
				}
				boost::flyweight<ComputedTextRunStyle> style() const override {
					const std::shared_ptr<const DeclaredTextRunStyle> declared(declaration_->style());
					const std::shared_ptr<const DeclaredTextRunStyle> cascaded(*styles::cascade(boost::make_iterator_range_n(&declared, 1)));
					styles::SpecifiedValue<TextRunStyle>::type specified;
					specifiedValuesFromCascadedValues(cascaded->colors, computedParentStyles_.colors, specified.colors);
					specifiedValuesFromCascadedValues(cascaded->backgroundsAndBorders, computedParentStyles_.backgroundsAndBorders, specified.backgroundsAndBorders);
					specifiedValuesFromCascadedValues(cascaded->basicBoxModel, computedParentStyles_.basicBoxModel, specified.basicBoxModel);
					specifiedValuesFromCascadedValues(cascaded->fonts, computedParentStyles_.fonts, specified.fonts);
					specifiedValuesFromCascadedValues(cascaded->inlineLayout, computedParentStyles_.inlineLayout, specified.inlineLayout);
					specifiedValuesFromCascadedValues(cascaded->text, computedParentStyles_.text, specified.text);
					specifiedValuesFromCascadedValues(cascaded->textDecoration, computedParentStyles_.textDecoration, specified.textDecoration);
					specifiedValuesFromCascadedValues(cascaded->writingModes, computedParentStyles_.writingModes, specified.writingModes);
					specifiedValuesFromCascadedValues(cascaded->auxiliary, computedParentStyles_.auxiliary, specified.auxiliary);
					return compute(specified, boost::fusion::at_key<styles::Color>(computedParentStyles_.colors));
				}

			private:
				std::unique_ptr<DeclaredStyledTextRunIterator> declaration_;
				const styles::ComputedValue<TextRunStyle>::type& computedParentStyles_;
			};
		}

		/**
		 * Returns the styles of the text runs in the specified line.
		 * @param line The line 
		 * @return An iterator enumerates the styles of the text runs in the line, or @c null if the line has no styled
		 *         text runs
		 * @throw BadPositionException @a line is outside of the document
		 */
		std::unique_ptr<ComputedStyledTextRunIterator> Presentation::computeTextRunStyles(Index line) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));
			if(textRunStyleDeclarator_.get() != nullptr) {
				std::unique_ptr<DeclaredStyledTextRunIterator> declaredRunStyles(textRunStyleDeclarator_->declareTextRunStyle(line));
				if(declaredRunStyles.get() != nullptr) {
					// compute parent's styles
					const std::shared_ptr<const DeclaredTextLineStyle> declaredLineStyles(declaredTextLineStyle(line));
					const std::shared_ptr<const DeclaredTextRunStyle> declaredParentStyles(declaredLineStyles->runsStyle());
					assert(declaredParentStyles.get() != nullptr);
//					styles::cascade(declaredParentStyles);
					// TODO: Wow, ugly code...
					styles::SpecifiedValue<TextRunStyle>::type specifiedParentStyles;
					const styles::ComputedValue<TextRunStyle>::type& computedToplevelStyles = computedTextRunStyle();
					specifiedValuesFromCascadedValues(declaredParentStyles->colors, computedToplevelStyles.colors, specifiedParentStyles.colors);
					specifiedValuesFromCascadedValues(declaredParentStyles->backgroundsAndBorders, computedToplevelStyles.backgroundsAndBorders, specifiedParentStyles.backgroundsAndBorders);
					specifiedValuesFromCascadedValues(declaredParentStyles->basicBoxModel, computedToplevelStyles.basicBoxModel, specifiedParentStyles.basicBoxModel);
					specifiedValuesFromCascadedValues(declaredParentStyles->fonts, computedToplevelStyles.fonts, specifiedParentStyles.fonts);
					specifiedValuesFromCascadedValues(declaredParentStyles->inlineLayout, computedToplevelStyles.inlineLayout, specifiedParentStyles.inlineLayout);
					specifiedValuesFromCascadedValues(declaredParentStyles->text, computedToplevelStyles.text, specifiedParentStyles.text);
					specifiedValuesFromCascadedValues(declaredParentStyles->textDecoration, computedToplevelStyles.textDecoration, specifiedParentStyles.textDecoration);
					specifiedValuesFromCascadedValues(declaredParentStyles->writingModes, computedToplevelStyles.writingModes, specifiedParentStyles.writingModes);
					specifiedValuesFromCascadedValues(declaredParentStyles->auxiliary, computedToplevelStyles.auxiliary, specifiedParentStyles.auxiliary);
					const boost::flyweight<styles::ComputedValue<TextRunStyle>::type> computedParentStyles(
						compute(specifiedParentStyles, boost::fusion::at_key<styles::Color>(computedToplevelStyles.colors)));
					return std::unique_ptr<ComputedStyledTextRunIterator>(
						new ComputedStyledTextRunIteratorImpl(std::move(declaredRunStyles), computedParentStyles));
				}
			}
			return std::unique_ptr<ComputedStyledTextRunIterator>();	// TODO: Is this ok?
		}

		/**
		 * Computes the writing mode.
		 * @param line The line to test. If this is @c boost#none, this method returns the entire writing mode
		 * @return The computed writing mode value
		 * @throw IndexOutOfBoundsException @a line is invalid
		 */
		WritingMode Presentation::computeWritingMode(boost::optional<Index> line /* = boost::none */) const {
			const BlockFlowDirection writingMode = boost::fusion::at_key<styles::WritingMode>(computedTextToplevelStyle());
			if(line == boost::none)
				return WritingMode(boost::fusion::at_key<styles::Direction>(computedTextLineStyle()),
					writingMode, boost::fusion::at_key<styles::TextOrientation>(computedTextLineStyle()));
			else {
				// compute 'direction' and 'text-orientation' properties
				const std::shared_ptr<const DeclaredTextLineStyle> declared(declaredTextLineStyle(boost::get(line)));
#if 0
				const styles::DeclaredValue<TextLineStyle>::type& cascaded = styles::cascade(declared);
#else
				const styles::DeclaredValue<TextLineStyle>::type& cascaded = *declared;
#endif

				styles::SpecifiedValue<styles::Direction>::type specifiedDirection;
				const auto& cascadedDirection(boost::fusion::at_key<styles::Direction>(cascaded));
				const auto& computedParentDirection(boost::fusion::at_key<styles::Direction>(computedTextLineStyle()));
				static_assert(!std::is_same<std::decay<decltype(cascadedDirection)>::type, boost::mpl::void_>::value, "");
				static_assert(!std::is_same<std::decay<decltype(computedParentDirection)>::type, boost::mpl::void_>::value, "");
				styles::specifiedValueFromCascadedValue<styles::Direction>(
					cascadedDirection,
					[&computedParentDirection]() {
						return computedParentDirection;
					},
					specifiedDirection);

				styles::SpecifiedValue<styles::TextOrientation>::type specifiedTextOrientation;
				const auto& cascadedTextOrientation(boost::fusion::at_key<styles::TextOrientation>(cascaded));
				const auto& computedParentTextOrientation(boost::fusion::at_key<styles::TextOrientation>(computedTextLineStyle()));
				static_assert(!std::is_same<std::decay<decltype(cascadedTextOrientation)>::type, boost::mpl::void_>::value, "");
				static_assert(!std::is_same<std::decay<decltype(computedParentTextOrientation)>::type, boost::mpl::void_>::value, "");
				styles::specifiedValueFromCascadedValue<styles::TextOrientation>(
					cascadedTextOrientation,
					[&computedParentTextOrientation]() {
						return computedParentTextOrientation;
					},
					specifiedTextOrientation);

				styles::ComputedValue<styles::Direction>::type computedDirection(
					styles::computeAsSpecified<styles::Direction>(specifiedDirection));
				styles::ComputedValue<styles::TextOrientation>::type computedTextOrientation(
					styles::computeAsSpecified<styles::TextOrientation>(specifiedTextOrientation));
				return WritingMode(computedDirection, writingMode, computedTextOrientation);
			}		
		}

		/// @internal
		inline std::shared_ptr<const DeclaredTextLineStyle> Presentation::declaredTextLineStyle(Index line) const {
			std::shared_ptr<const DeclaredTextLineStyle> p;
			if(textLineStyleDeclarator_.get() != nullptr)
				p = textLineStyleDeclarator_->declareTextLineStyle(line);
			if(p.get() == nullptr)
				p = std::shared_ptr<const DeclaredTextLineStyle>(&DeclaredTextLineStyle::unsetInstance(), boost::null_deleter());
			assert(p.get() != nullptr);
			return p;
		}

		/// Returns the document to which the presentation connects.
		const kernel::Document& Presentation::document() const BOOST_NOEXCEPT {
			return document_;
		}

		/// Returns the document to which the presentation connects.
		kernel::Document& Presentation::document() BOOST_NOEXCEPT {
			return document_;
		}
		
		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void Presentation::documentAboutToBeChanged(const kernel::Document& document) {
			// TODO: not implemented.
		}
		
		/// @see kernel#DocumentListener#documentChanged
		void Presentation::documentChanged(const kernel::Document&, const kernel::DocumentChange& change) {
			const boost::integer_range<Index>
				erasedLines(change.erasedRegion().first.line, change.erasedRegion().second.line),
				insertedLines(change.insertedRegion().first.line, change.insertedRegion().second.line);
			for(std::list<Hyperlinks*>::iterator i(std::begin(hyperlinks_)), e(std::end(hyperlinks_)); i != e; ) {
				const Index line = (*i)->lineNumber;
				if(line == insertedLines.front() || includes(erasedLines, line)) {
					for(size_t j = 0; j < (*i)->numberOfHyperlinks; ++j)
						delete (*i)->hyperlinks[j];
					delete *i;
					i = hyperlinks_.erase(i);
					continue;
				} else {
					if(line >= *erasedLines.end() && !erasedLines.empty())
						(*i)->lineNumber -= erasedLines.size();
					if(line >= *insertedLines.end() && !insertedLines.empty())
						(*i)->lineNumber += insertedLines.size();
				}
				++i;
			}
		}

		/**
		 * Returns the hyperlinks in the specified line.
		 * @param line the line number
		 * @param[out] numberOfHyperlinks the number of the hyperlinks
		 * @return the hyperlinks or @c null
		 * @throw BadPositionException @a line is outside of the document
		 */
		const hyperlink::Hyperlink* const* Presentation::getHyperlinks(Index line, size_t& numberOfHyperlinks) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));
			else if(hyperlinkDetector_.get() == nullptr) {
				numberOfHyperlinks = 0;
				return nullptr;
			}
		
			// find in the cache
			for(std::list<Hyperlinks*>::iterator i(std::begin(hyperlinks_)), e(std::end(hyperlinks_)); i != e; ++i) {
				if((*i)->lineNumber == line) {
					Hyperlinks& result = **i;
					if(&result != hyperlinks_.front()) {
						// bring to the front
						hyperlinks_.erase(i);
						hyperlinks_.push_front(&result);
					}
					numberOfHyperlinks = result.numberOfHyperlinks;
					return result.hyperlinks.get();
				}
			}
		
			// not found in the cache
			if(hyperlinks_.size() == ASCENSION_HYPERLINKS_CACHE_SIZE) {
				delete hyperlinks_.back();
				hyperlinks_.pop_back();
			}
			std::vector<hyperlink::Hyperlink*> temp;
			for(Index offsetInLine = 0, eol = document_.lineLength(line); offsetInLine < eol; ) {
				std::unique_ptr<hyperlink::Hyperlink> h(hyperlinkDetector_->nextHyperlink(document_, line, boost::irange(offsetInLine, eol)));
				if(h.get() == nullptr)
					break;
				// check result
				const boost::integer_range<Index>& r(h->region());
				if(*r.begin() < offsetInLine)
					break;
				offsetInLine = *r.end();
				temp.push_back(h.release());
			}
			std::unique_ptr<Hyperlinks> newItem(new Hyperlinks);
			newItem->lineNumber = line;
			newItem->hyperlinks.reset(new hyperlink::Hyperlink*[numberOfHyperlinks = newItem->numberOfHyperlinks = temp.size()]);
			std::copy(temp.begin(), temp.end(), newItem->hyperlinks.get());
			hyperlinks_.push_front(newItem.release());
			return hyperlinks_.front()->hyperlinks.get();
		}

		/**
		 * Removes the specified text line color specifier.
		 * @param specifier The director to remove
		 */
		void Presentation::removeTextLineColorSpecifier(TextLineColorSpecifier& specifier) BOOST_NOEXCEPT {
			for(std::list<std::shared_ptr<TextLineColorSpecifier>>::iterator
					i(std::begin(textLineColorSpecifiers_)), e(std::end(textLineColorSpecifiers_)); i != e; ++i) {
				if(i->get() == &specifier) {
					textLineColorSpecifiers_.erase(i);
					return;
				}
			}
		}

		/**
		 * Sets the default direction.
		 */
		void Presentation::setDefaultDirection(ReadingDirection direction) {
			if(direction != defaultDirection_) {
				defaultDirection_ = direction;
			}
		}
		
		/**
		 * Sets the hyperlink detector.
		 * @param newDirector The director. Set @c null to unregister
		 */
		void Presentation::setHyperlinkDetector(std::shared_ptr<hyperlink::HyperlinkDetector> newDetector) BOOST_NOEXCEPT {
			hyperlinkDetector_ = newDetector;
			clearHyperlinksCache();
		}

		/**
		 * Sets the line style declarator.
		 * @param newDeclarator The declarator. @c null to unregister
		 */
		void Presentation::setTextLineStyleDeclarator(std::shared_ptr<TextLineStyleDeclarator> newDeclarator) BOOST_NOEXCEPT {
			textLineStyleDeclarator_ = newDeclarator;
		}
		
		/**
		 * Sets the text run style declarator.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param newDirector The declarator. @c null to unregister
		 */
		void Presentation::setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator> newDeclarator) BOOST_NOEXCEPT {
			textRunStyleDeclarator_ = newDeclarator;
		}

		/**
		 * Sets the declared text toplevel line style.
		 * @param newStyle The style to set
		 * @see #declaredTextToplevelStyle, ComputedTextToplevelStyleListener
		 */
		void Presentation::setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle> newStyle) {
			// compute TextToplevelStyle
			std::shared_ptr<const DeclaredTextToplevelStyle> newlyDeclaredToplevelStyles = (newStyle.get() != nullptr) ? newStyle
				: std::shared_ptr<const DeclaredTextToplevelStyle>(&DeclaredTextToplevelStyle::unsetInstance(), boost::null_deleter());
#if 0
			const DeclaredTextToplevelStyle& newlyCascadedToplevelStyles = styles::cascade(newlyDeclaredToplevelStyles);
#else
			const DeclaredTextToplevelStyle& newlyCascadedToplevelStyles = *newlyDeclaredToplevelStyles;
#endif
			styles::SpecifiedValue<TextToplevelStyle>::type newlySpecifiedToplevelStyles;
			specifiedValuesFromCascadedValues(newlyCascadedToplevelStyles, styles::HANDLE_AS_ROOT, newlySpecifiedToplevelStyles);
			boost::flyweight<ComputedTextToplevelStyle> newlyComputedToplevelStyles(compute(newlySpecifiedToplevelStyles));

			// compute TextLineStyle
#if 0
			const DeclaredTextLineStyle& newlyCascadedLineStyles = styles::cascade(newlyDeclaredToplevelStyles->linesStyle());
#else
			const DeclaredTextLineStyle& newlyCascadedLineStyles = *newlyDeclaredToplevelStyles->linesStyle();
#endif
			styles::SpecifiedValue<TextLineStyle>::type newlySpecifiedLineStyles;
			specifiedValuesFromCascadedValues(newlyCascadedLineStyles, styles::HANDLE_AS_ROOT, newlySpecifiedLineStyles);
			boost::flyweight<styles::ComputedValue<TextLineStyle>::type> newlyComputedLineStyles(compute(newlySpecifiedLineStyles));

			// compute TextRunStyle
#if 0
			const DeclaredTextRunStyle& newlyCascadedRunStyles = styles::cascade(newlyDeclaredToplevelStyles->linesStyle()->runsStyle());
#else
			const DeclaredTextRunStyle& newlyCascadedRunStyles = *newlyDeclaredToplevelStyles->linesStyle()->runsStyle();
#endif
			styles::SpecifiedValue<TextRunStyle>::type newlySpecifiedRunStyles;
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.colors, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.colors);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.backgroundsAndBorders, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.backgroundsAndBorders);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.basicBoxModel, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.basicBoxModel);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.fonts, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.fonts);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.inlineLayout, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.inlineLayout);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.text, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.text);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.textDecoration, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.textDecoration);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.writingModes, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.writingModes);
			specifiedValuesFromCascadedValues(newlyCascadedRunStyles.auxiliary, styles::HANDLE_AS_ROOT, newlySpecifiedRunStyles.auxiliary);
			boost::flyweight<styles::ComputedValue<TextRunStyle>::type> newlyComputedRunStyles(compute(newlySpecifiedRunStyles, styles::HANDLE_AS_ROOT));

			// commit
			std::shared_ptr<const DeclaredTextToplevelStyle> previouslyDeclared(declaredTextToplevelStyle_);
			boost::flyweight<ComputedTextToplevelStyle> previouslyComputed(computedStyles_->forToplevel);
			using boost::swap;
			using std::swap;
			swap(newlyDeclaredToplevelStyles, declaredTextToplevelStyle_);
			swap(newlyComputedToplevelStyles, computedStyles_->forToplevel);
			swap(newlyComputedLineStyles, computedStyles_->forLines);
			swap(newlyComputedRunStyles, computedStyles_->forRuns);
			if(previouslyDeclared.get() != nullptr)
				computedTextToplevelStyleChangedSignal_(*this, *previouslyDeclared, previouslyComputed.get());
		}
		
		/**
		 * Returns the colors of the specified text line.
		 * @param line The line
		 * @param[out] foreground The foreground color of the line. Unspecified if an invalid value
		 * @param[out] background The background color of the line. Unspecified if an invalid value
		 * @throw BadPositionException @a line is outside of the document
		 */
		void Presentation::textLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));
			TextLineColorSpecifier::Priority highestPriority = 0, p;
			boost::optional<graphics::Color> f, g;
			BOOST_FOREACH(const std::shared_ptr<TextLineColorSpecifier>& s, textLineColorSpecifiers_) {
				p = s->specifyTextLineColors(line, f, g);
				if(p > highestPriority) {
					highestPriority = p;
					foreground = f;
					background = g;
				}
			}
		}
	}
}
