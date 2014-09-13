/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2014
 */

#include <ascension/corelib/range.hpp>
#include <ascension/graphics/font/text-layout-styles.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/presentation/text-style.hpp>
#include <ascension/rules.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/foreach.hpp>
#ifdef BOOST_OS_WINDOWS
#include <shellapi.h>	// ShellExecuteW
#endif // BOOST_OS_WINDOWS
//using graphics::font::TextRenderer;


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
		// TODO: these value are changed later.
		const Length Border::THIN(0.05f, Length::EM_HEIGHT);
		const Length Border::MEDIUM(0.10f, Length::EM_HEIGHT);
		const Length Border::THICK(0.20f, Length::EM_HEIGHT);

		/**
		 * @param base The base style
		 * @param baseIsRoot Set @c true if @a base is the root style
		 * @return This object
		 */
		TextRunStyle& TextRunStyle::resolveInheritance(const TextRunStyle& base, bool baseIsRoot) {
			// TODO: not implemented.
			return *this;
		}

		/**
		 * Returns the default line style of the specified line style.
		 * @param lineStyle The line style
		 * @return lineStyle.defaultRunStyle, of a default-constructed @c TextRunStyle instance if @c null
		 * @see defaultTextLineStyle
		 */
		std::shared_ptr<const TextRunStyle> defaultTextRunStyle(const TextLineStyle& lineStyle) {
			static const TextRunStyle defaultInstance;
			return (lineStyle.defaultRunStyle.get() != nullptr) ?
				lineStyle.defaultRunStyle : std::shared_ptr<const TextRunStyle>(&defaultInstance, boost::null_deleter());
		}

		/**
		 * Returns the default line style of the specified toplevel style.
		 * @param toplevelStyle The toplevel style
		 * @return toplevelStyle.defaultLineStyle, of a default-constructed @c TextLineStyle instance if @c null
		 * @see defaultTextRunStyle
		 */
		std::shared_ptr<const TextLineStyle> defaultTextLineStyle(const TextToplevelStyle& toplevelStyle) {
			static const TextLineStyle defaultInstance;
			return (toplevelStyle.defaultLineStyle.get() != nullptr) ?
				toplevelStyle.defaultLineStyle : std::shared_ptr<const TextLineStyle>(&defaultInstance, boost::null_deleter());
		}


		// Presentation ///////////////////////////////////////////////////////////////////////////////////////////////

		std::shared_ptr<const TextToplevelStyle> Presentation::DEFAULT_TEXT_TOPLEVEL_STYLE;

		struct Presentation::Hyperlinks {
			Index lineNumber;
			std::unique_ptr<hyperlink::Hyperlink*[]> hyperlinks;	// TODO: This should be container class.
			std::size_t numberOfHyperlinks;
		};

		/**
		 * Constructor.
		 * @param document The target document
		 */
		Presentation::Presentation(kernel::Document& document) BOOST_NOEXCEPT : document_(document) {
			if(DEFAULT_TEXT_TOPLEVEL_STYLE.get() == nullptr) {
				std::unique_ptr<TextToplevelStyle> temp(new TextToplevelStyle);
				temp->defaultLineStyle = defaultTextLineStyle(*temp);
				DEFAULT_TEXT_TOPLEVEL_STYLE = std::move(temp);
			}
			setTextToplevelStyle(std::shared_ptr<const TextToplevelStyle>());
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
		
		/**
		 * Registers the text toplevel style listener.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 * @see #removeTextToplevelStyleListener, #textToplevelStyle
		 */
		void Presentation::addTextToplevelStyleListener(TextToplevelStyleListener& listener) {
			textToplevelStyleListeners_.add(listener);
		}

		void Presentation::clearHyperlinksCache() BOOST_NOEXCEPT {
			BOOST_FOREACH(Hyperlinks* p, hyperlinks_) {
				for(size_t j = 0; j < p->numberOfHyperlinks; ++j)
					delete p->hyperlinks[j];
				delete p;
			}
			hyperlinks_.clear();
		}

		namespace {
//			template<typename PropertyType>
//			inline typename sp::IntrinsicType<PropertyType>::Type resolveProperty(PropertyType TextLineStyle::*pointerToMember, const TextLineStyle& declared, const TextLineStyle& toplevel) {
//				return !(declared.*pointerToMember).inherits() ? (declared.*pointerToMember).get() : (toplevel.*pointerToMember).getOrInitial();
//			}
			template<typename PropertyType>
			inline void resolveProperty(PropertyType TextLineStyle::*pointerToMember, const TextLineStyle& toplevel, TextLineStyle& property) {
				if((property.*pointerToMember).inherits())
					property.*pointerToMember = (toplevel.*pointerToMember).getOrInitial();
			}
		}

		/**
		 * Returns the style of the specified text line.
		 * @param line The line number
		 * @param lengthContext 
		 * @param globalSwitch 
		 * @return The computed text line style
		 * @throw BadPositionException @a line is outside of the document
		 * @throw NullPointerException Internal @c Length#value call may throw this exception
		 */
		graphics::font::ComputedTextLineStyle&& Presentation::computeTextLineStyle(Index line,
				const Length::Context& lengthContext, const GlobalTextStyleSwitch* globalSwitch) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));
		
			TextToplevelStyle toplevel(textToplevelStyle());
			if(toplevel.writingMode.inherits() && globalSwitch != nullptr)
				toplevel.writingMode = globalSwitch->writingMode();
		
			std::shared_ptr<const TextLineStyle> declared;
			if(textLineStyleDeclarator_.get() != nullptr)
				declared = textLineStyleDeclarator_->declareTextLineStyle(line);
		
			TextLineStyle precomputed((declared.get() != nullptr) ? *declared : TextLineStyle());
			if(globalSwitch != nullptr) {
				if(!precomputed.direction.inherits())
					precomputed.direction = globalSwitch->direction();
				if(!precomputed.textAlignment.inherits())
					precomputed.textAlignment = globalSwitch->textAlignment();
				if(!precomputed.textOrientation.inherits())
					precomputed.textOrientation = globalSwitch->textOrientation();
				if(!precomputed.whiteSpace.inherits())
					precomputed.whiteSpace = globalSwitch->whiteSpace();
			}
			const std::shared_ptr<const TextLineStyle> defaultStyle(defaultTextLineStyle(toplevel));
			assert(defaultStyle != nullptr);
			resolveProperty(&TextLineStyle::direction, *defaultStyle, precomputed);
//			resolveProperty(&TextLineStyle::unicodeBidi, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::textOrientation, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::lineBoxContain, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::inlineBoxAlignment, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::whiteSpace, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::tabSize, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::lineBreak, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::wordBreak, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::overflowWrap, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::textAlignment, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::textAlignmentLast, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::textJustification, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::textIndent, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::hangingPunctuation, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::dominantBaseline, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::lineHeight, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::measure, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::numberSubstitutionLocaleOverride, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::numberSubstitutionLocaleSource, *defaultStyle, precomputed);
			resolveProperty(&TextLineStyle::numberSubstitutionMethod, *defaultStyle, precomputed);
		
			graphics::font::ComputedTextLineStyle computed;
			computed.writingMode = WritingMode(precomputed.direction.getOrInitial(), toplevel.writingMode.getOrInitial(), precomputed.textOrientation.getOrInitial());
			computed.lineBoxContain = precomputed.lineBoxContain.getOrInitial();
			computed.whiteSpace = precomputed.whiteSpace.getOrInitial();
			precomputed.tabSize.getOrInitial();
			computed.lineBreak = precomputed.lineBreak.getOrInitial();
			computed.wordBreak = precomputed.wordBreak.getOrInitial();
			computed.overflowWrap = precomputed.overflowWrap.getOrInitial();
			computed.alignment = precomputed.textAlignment.getOrInitial();
			computed.alignmentLast = precomputed.textAlignmentLast.getOrInitial();
			computed.justification = precomputed.textJustification.getOrInitial();
			computed.indent.length = static_cast<graphics::Scalar>(precomputed.textIndent.getOrInitial().length.value(lengthContext));
			computed.indent.hanging = precomputed.textIndent.getOrInitial().hanging;
			computed.indent.eachLine = precomputed.textIndent.getOrInitial().eachLine;
			computed.hangingPunctuation = precomputed.hangingPunctuation.getOrInitial();
			computed.dominantBaseline = precomputed.dominantBaseline.getOrInitial();
			{
				// TODO: This code is temporary.
				auto precomputedLineHeight(precomputed.lineHeight.getOrInitial());
				if(LineHeightEnums* const keyword = boost::get<LineHeightEnums>(&precomputedLineHeight)) {
					if(*keyword == LineHeightEnums::NONE)
						*keyword = LineHeightEnums::NORMAL;
					if(*keyword == LineHeightEnums::NORMAL || true)
						precomputedLineHeight = Length(1.15f, Length::EM_HEIGHT);
				} else if(const graphics::Scalar* const number = boost::get<graphics::Scalar>(&precomputedLineHeight))
					precomputedLineHeight = Length(*number, Length::EM_HEIGHT);
				if(const Length* const length = boost::get<Length>(&precomputedLineHeight))
					computed.lineHeight = static_cast<graphics::Scalar>(length->value(lengthContext));
				else
					ASCENSION_ASSERT_NOT_REACHED();
			}
			{
				const boost::optional<Length> value(precomputed.measure.getOrInitial());
				if(value != boost::none)
					computed.measure = static_cast<graphics::Scalar>(value->value(lengthContext));
				else
					computed.measure = Length(100, Length::PERCENTAGE, isHorizontal(computed.writingMode.blockFlowDirection) ? Length::WIDTH : Length::HEIGHT).value(lengthContext);
			}
			computed.numberSubstitution.localeOverride = precomputed.numberSubstitutionLocaleOverride.getOrInitial();
			computed.numberSubstitution.localeSource = precomputed.numberSubstitutionLocaleSource.getOrInitial();
			computed.numberSubstitution.method = precomputed.numberSubstitutionMethod.getOrInitial();
		
			return std::move(computed);
		}

		namespace {
			template<typename PropertyType, typename TextStyle>
			inline void computeDefaultTextRunStyle(PropertyType TextStyle::*pointerToMember, const TextStyle& defaultStyle, TextStyle& style) {
				if((style.*pointerToMember).inherits())
					style.*pointerToMember = (defaultStyle.*pointerToMember).getOrInitial();
			}
			inline void computeDefaultTextRunStyle(Background TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				computeDefaultTextRunStyle(&Background::color, defaultStyle.background, style.background);
			}
			inline void computeDefaultTextRunStyle(Border TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				BOOST_FOREACH(Border::Side& side, (static_cast<std::array<Border::Side, 4>&>(style.border.sides))) {
					const FlowRelativeDirection direction = static_cast<FlowRelativeDirection>(&side - &*std::begin(style.border.sides));
					computeDefaultTextRunStyle(&Border::Side::color, defaultStyle.border.sides[direction], style.border.sides[direction]);
					computeDefaultTextRunStyle(&Border::Side::style, defaultStyle.border.sides[direction], style.border.sides[direction]);
					computeDefaultTextRunStyle(&Border::Side::width, defaultStyle.border.sides[direction], style.border.sides[direction]);
				}
			}
			template<typename PropertyType>
			inline void computeDefaultTextRunStyle(FlowRelativeFourSides<PropertyType> TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				BOOST_FOREACH(FlowRelativeFourSides<PropertyType>::value_type& side, (static_cast<std::array<PropertyType, 4>&>(style.*pointerToMember))) {
					const FlowRelativeDirection direction = static_cast<FlowRelativeDirection>(&side - &*std::begin(style.*pointerToMember));
					if((style.*pointerToMember)[direction].inherits())
						(style.*pointerToMember)[direction] = (defaultStyle.*pointerToMember)[direction].getOrInitial();
				}
			}
			template<typename PropertyType>
			inline void computeDefaultTextRunStyle(SpacingLimit<PropertyType> TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				computeDefaultTextRunStyle(&SpacingLimit<PropertyType>::optimum, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&SpacingLimit<PropertyType>::minimum, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&SpacingLimit<PropertyType>::maximum, defaultStyle.*pointerToMember, style.*pointerToMember);
			}
			inline void computeDefaultTextRunStyle(TextDecoration TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				computeDefaultTextRunStyle(&TextDecoration::lines, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&TextDecoration::color, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&TextDecoration::style, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&TextDecoration::skip, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&TextDecoration::underlinePosition, defaultStyle.*pointerToMember, style.*pointerToMember);
			}
			inline void computeDefaultTextRunStyle(TextEmphasis TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				computeDefaultTextRunStyle(&TextEmphasis::style, defaultStyle.*pointerToMember, style.*pointerToMember);
				computeDefaultTextRunStyle(&TextEmphasis::position, defaultStyle.*pointerToMember, style.*pointerToMember);
			}
			inline void computeDefaultTextRunStyle(TextShadow TextRunStyle::*pointerToMember, const TextRunStyle& defaultStyle, TextRunStyle& style) {
				// TODO: Write the code.
			}
		
			class ComputedStyledTextRunIteratorImpl : public graphics::font::ComputedStyledTextRunIterator {
			public:
				ComputedStyledTextRunIteratorImpl(std::unique_ptr<StyledTextRunIterator>&& declaration,
						TextRunStyle&& defaultStyle) : declaration_(std::move(declaration)), defaultStyle_(defaultStyle) {
				}
				// font.ComputedStyledTextRunIterator
				boost::integer_range<Index> currentRange() const {
					return declaration_->currentRange();
				}
				void currentStyle(graphics::font::ComputedTextRunStyle& style) const {
					const std::shared_ptr<const TextRunStyle> declared(declaration_->currentStyle());
					declared->color;
				}
				bool isDone() const BOOST_NOEXCEPT {
					return declaration_->isDone();
				}
				void next() {
					return declaration_->next();
				}
			private:
				std::unique_ptr<StyledTextRunIterator> declaration_;
				const TextRunStyle defaultStyle_;
			};
		}

		/**
		 * Returns the styles of the text runs in the specified line.
		 * @param line The line
		 * @param lengthContext 
		 * @return An iterator enumerates the styles of the text runs in the line, or @c null if the line
		 *         has no styled text runs
		 * @throw BadPositionException @a line is outside of the document
		 */
		std::unique_ptr<graphics::font::ComputedStyledTextRunIterator> Presentation::computeTextRunStyles(Index line, const Length::Context& lengthContext) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));
			std::unique_ptr<StyledTextRunIterator> declaration(
				(textRunStyleDeclarator_.get() != nullptr) ?
					textRunStyleDeclarator_->declareTextRunStyle(line) : std::unique_ptr<StyledTextRunIterator>());
			if(declaration.get() == nullptr)
				return std::unique_ptr<graphics::font::ComputedStyledTextRunIterator>();
		
			const std::shared_ptr<const TextLineStyle> declaredLineStyle(
				(textLineStyleDeclarator_ != nullptr) ? textLineStyleDeclarator_->declareTextLineStyle(line) : nullptr);
			TextRunStyle defaultStyle((declaredLineStyle.get() != nullptr) ? *declaredLineStyle->defaultRunStyle : TextRunStyle());
			const std::shared_ptr<const TextRunStyle> defaultStyleFromToplevel(defaultTextRunStyle(*defaultTextLineStyle(*textToplevelStyle_)));
			// TODO: Should I use Boost.Fusion?
			computeDefaultTextRunStyle(&TextRunStyle::color, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::background, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::border, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::padding, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::margin, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::fontFamily, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::fontWeight, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::fontStretch, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::fontStyle, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::fontSize, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::fontSizeAdjust, *defaultStyleFromToplevel, defaultStyle);
//			computeDefaultTextRunStyle(&TextRunStyle::fontFeatureSettings, *defaultStyleFromToplevel, defaultStyle);
//			computeDefaultTextRunStyle(&TextRunStyle::fontLanguageOverride, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::textHeight, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::lineHeight, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::dominantBaseline, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::alignmentBaseline, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::alignmentAdjust, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::baselineShift, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::textTransform, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::hyphens, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::wordSpacing, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::letterSpacing, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::textDecoration, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::textEmphasis, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::textShadow, *defaultStyleFromToplevel, defaultStyle);
			computeDefaultTextRunStyle(&TextRunStyle::shapingEnabled, *defaultStyleFromToplevel, defaultStyle);
		
			return std::unique_ptr<graphics::font::ComputedStyledTextRunIterator>(
				new ComputedStyledTextRunIteratorImpl(std::move(declaration), std::move(defaultStyle)));
		}

		/**
		 * Computes the writing mode. This method does not call @c TextLineStyleDeclarator.
		 * @param globalSwitch
		 * @return The computed writing mode value
		 */
		WritingMode&& Presentation::computeWritingMode(const GlobalTextStyleSwitch* globalSwitch) const {
			const TextToplevelStyle& toplevel = textToplevelStyle();
			boost::optional<BlockFlowDirection> writingMode(toplevel.writingMode.getOrNone());
			if(writingMode == boost::none) {
				if(globalSwitch != nullptr)
					writingMode = globalSwitch->writingMode().getOrInitial();
				else
					writingMode = toplevel.writingMode.initialValue();
		
			}
			assert(writingMode != boost::none);
		
			boost::optional<ReadingDirection> direction;
			boost::optional<TextOrientation> textOrientation;
			if(globalSwitch != nullptr) {
				if(writingMode == boost::none)
					writingMode = globalSwitch->writingMode().getOrNone();
				direction = globalSwitch->direction().getOrNone();
				textOrientation = globalSwitch->textOrientation().getOrNone();
			}
			if(direction == boost::none)
				direction = defaultTextLineStyle(toplevel)->direction.getOrInitial();
			if(textOrientation == boost::none)
				textOrientation = defaultTextLineStyle(toplevel)->textOrientation.getOrInitial();
			assert(direction != boost::none);
			assert(textOrientation != boost::none);
		
			return WritingMode(*direction, *writingMode, *textOrientation);
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
		 * Removes the text toplevel style listener.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 * @see #addTextToplevelStyleListener, #textToplevelStyle
		 */
		void Presentation::removeTextToplevelStyleListener(TextToplevelStyleListener& listener) {
			textToplevelStyleListeners_.remove(listener);
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
		 * Sets the text toplevel line style.
		 * @param newStyle The style to set
		 * @see #textToplevelStyle
		 */
		void Presentation::setTextToplevelStyle(std::shared_ptr<const TextToplevelStyle> newStyle) {
			const std::shared_ptr<const TextToplevelStyle> used(textToplevelStyle_);
			textToplevelStyle_ = (newStyle.get() != nullptr) ? newStyle : DEFAULT_TEXT_TOPLEVEL_STYLE;
			textToplevelStyleListeners_.notify<std::shared_ptr<const TextToplevelStyle>>(&TextToplevelStyleListener::textToplevelStyleChanged, used);
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


		// SingleStyledPartitionPresentationReconstructor.Iterator ////////////////////////////////////////////////////

		class SingleStyledPartitionPresentationReconstructor::Iterator : public presentation::StyledTextRunIterator {
		public:
			Iterator(const boost::integer_range<Index>& range, std::shared_ptr<const TextRunStyle> style) BOOST_NOEXCEPT : range_(range), style_(style), done_(false) {
			}
		private:
			// StyledTextRunIterator
			boost::integer_range<Index> currentRange() const {
				if(done_)
					throw NoSuchElementException();
				return range_;
			}
			std::shared_ptr<const TextRunStyle> currentStyle() const {
				if(done_)
					throw NoSuchElementException();
				return style_;
			}
			bool isDone() const BOOST_NOEXCEPT {
				return done_;
			}
			void next() {
				if(done_)
					throw NoSuchElementException();
				done_ = true;
			}
		private:
			const boost::integer_range<Index> range_;
			const std::shared_ptr<const TextRunStyle> style_;
			bool done_;
		};


		// SingleStyledPartitionPresentationReconstructor /////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param style The style
		 */
		SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(std::shared_ptr<const TextRunStyle> style) BOOST_NOEXCEPT : style_(style) {
		}
		
		/// @see PartitionPresentationReconstructor#presentation
		std::unique_ptr<StyledTextRunIterator> SingleStyledPartitionPresentationReconstructor::presentation(Index, const boost::integer_range<Index>& rangeInLine) const {
			return std::unique_ptr<presentation::StyledTextRunIterator>(new Iterator(rangeInLine, style_));
		}


		// PresentationReconstructor.Iterator /////////////////////////////////////////////////////////////////////////

		class PresentationReconstructor::Iterator : public presentation::StyledTextRunIterator {
		public:
			Iterator(const Presentation& presentation,
				const std::map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors, Index line);
		private:
			void updateSubiterator();
			// StyledTextRunIterator
			boost::integer_range<Index> currentRange() const;
			std::shared_ptr<const TextRunStyle> currentStyle() const;
			bool isDone() const BOOST_NOEXCEPT;
			void next();
		private:
			const Presentation& presentation_;
			const std::map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
			const Index line_;
			kernel::DocumentPartition currentPartition_;
			std::unique_ptr<presentation::StyledTextRunIterator> subiterator_;
			boost::integer_range<Index> currentRange_;
			std::shared_ptr<const TextRunStyle> currentStyle_;
		};

		/**
		 * Constructor.
		 * @param presentation
		 * @param reconstructors
		 * @param line
		 */
		PresentationReconstructor::Iterator::Iterator(
				const Presentation& presentation, const std::map<kernel::ContentType,
				PartitionPresentationReconstructor*> reconstructors, Index line)
				: presentation_(presentation), reconstructors_(reconstructors), line_(line), currentRange_(0, 0) {
			const kernel::DocumentPartitioner& partitioner = presentation_.document().partitioner();
			Index offsetInLine = 0;
			for(const Index lineLength = presentation_.document().lineLength(line);;) {
				partitioner.partition(kernel::Position(line, offsetInLine), currentPartition_);	// this may throw BadPositionException
				if(!currentPartition_.region.isEmpty())
					break;
				if(++offsetInLine >= lineLength) {	// rare case...
					currentPartition_.contentType = kernel::DEFAULT_CONTENT_TYPE;
					currentPartition_.region = kernel::Region(line, boost::irange(static_cast<Index>(0), lineLength));
					break;
				}
			}
			updateSubiterator();
		}
		
		/// @see StyledTextRunIterator#currentRange
		boost::integer_range<Index> PresentationReconstructor::Iterator::currentRange() const {
			if(subiterator_.get() != nullptr)
				return subiterator_->currentRange();
			else if(!isDone())
				return currentRange_;
			throw NoSuchElementException();
		}
		
		/// @see StyledTextRunIterator#currentStyle
		std::shared_ptr<const TextRunStyle> PresentationReconstructor::Iterator::currentStyle() const {
			if(subiterator_.get() != nullptr)
				return subiterator_->currentStyle();
			else if(!isDone())
				return currentStyle_;
			throw NoSuchElementException();
		}

		/// @see StyledTextRunIterator#isDone
		bool PresentationReconstructor::Iterator::isDone() const BOOST_NOEXCEPT {
			return currentPartition_.region.isEmpty();
		}
		
		/// @see StyledTextRunIterator#next
		void PresentationReconstructor::Iterator::next() {
			if(subiterator_.get() != nullptr) {
				subiterator_->next();
				if(subiterator_->isDone())
					subiterator_.reset();
			}
			if(subiterator_.get() == nullptr) {
				const kernel::Document& document = presentation_.document();
				const Index lineLength = document.lineLength(line_);
				if(currentPartition_.region.end() >= kernel::Position(line_, lineLength)) {
					// done
					currentPartition_.region = kernel::Region(currentPartition_.region.end());
					return;
				}
				// find the next partition
				const kernel::DocumentPartitioner& partitioner = document.partitioner();
				for(Index offsetInLine = currentPartition_.region.end().offsetInLine; ; ) {
					partitioner.partition(kernel::Position(line_, offsetInLine), currentPartition_);
					if(!currentPartition_.region.isEmpty())
						break;
					if(++offsetInLine >= lineLength) {	// rare case...
						currentPartition_.contentType = kernel::DEFAULT_CONTENT_TYPE;
						currentPartition_.region = kernel::Region(line_,  boost::irange(offsetInLine, lineLength));
					}
				}
				updateSubiterator();
			}
		}

		inline void PresentationReconstructor::Iterator::updateSubiterator() {
			std::map<kernel::ContentType, PartitionPresentationReconstructor*>::const_iterator r(reconstructors_.find(currentPartition_.contentType));
			if(r != std::end(reconstructors_))
				subiterator_ = r->second->presentation(currentPartition_.region);
			else
				subiterator_.reset();
			if(subiterator_.get() == nullptr) {
				const std::shared_ptr<const TextLineStyle> lineStyle(defaultTextLineStyle(presentation_.textToplevelStyle()));
				assert(lineStyle.get() != nullptr);
				std::shared_ptr<const TextRunStyle> runStyle(defaultTextRunStyle(*lineStyle));
				assert(runStyle.get() != nullptr);
				currentRange_ = boost::irange(currentPartition_.region.beginning().offsetInLine, currentPartition_.region.end().offsetInLine);
				currentStyle_ = runStyle;
			}
		}


		// PresentationReconstructor //////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param presentation The presentation
		 */
		PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
			presentation_.setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator>(this));	// TODO: danger call (may delete this).
		}
		
		/// Destructor.
		PresentationReconstructor::~PresentationReconstructor() BOOST_NOEXCEPT {
		//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<LineStyleDirector>());
			typedef std::pair<kernel::ContentType, PartitionPresentationReconstructor*> Temp;
			BOOST_FOREACH(const Temp& p, reconstructors_)
				delete p.second;
		}

		/// @see LineStyleDeclarator#declareTextRunStyle
		std::unique_ptr<StyledTextRunIterator> PresentationReconstructor::declareTextRunStyle(Index line) const {
			return std::unique_ptr<StyledTextRunIterator>(new Iterator(presentation_, reconstructors_, line));
		}
		
		/**
		 * Sets the partition presentation reconstructor for the specified content type.
		 * @param contentType The content type. If a reconstructor for this content type was already be
		 *                    set, the old will be deleted
		 * @param reconstructor The partition presentation reconstructor to set. Can't be @c null. The
		 *                      ownership will be transferred to the callee
		 * @throw NullPointerException @a reconstructor is @c null
		 */
		void PresentationReconstructor::setPartitionReconstructor(
				kernel::ContentType contentType, std::unique_ptr<PartitionPresentationReconstructor> reconstructor) {
			if(reconstructor.get() == nullptr)
				throw NullPointerException("reconstructor");
			const std::map<kernel::ContentType, PartitionPresentationReconstructor*>::iterator old(reconstructors_.find(contentType));
			if(old != reconstructors_.end()) {
				delete old->second;
				reconstructors_.erase(old);
			}
			reconstructors_.insert(std::make_pair(contentType, reconstructor.release()));
		}


		// hyperlink.URIHyperlinkDetector /////////////////////////////////////////////////////////////////////////////

		namespace {
			class URIHyperlink : public hyperlink::Hyperlink {
			public:
				explicit URIHyperlink(const boost::integer_range<Index>& region, const String& uri) BOOST_NOEXCEPT : hyperlink::Hyperlink(region), uri_(uri) {}
				String description() const BOOST_NOEXCEPT {
					static const Char PRECEDING[] = {0x202au, 0};
					static const Char FOLLOWING[] = {0x202cu, 0x0a,
						0x43, 0x54, 0x52, 0x4c, 0x20, 0x2b, 0x20, 0x63, 0x6c, 0x69, 0x63, 0x6b, 0x20,
						0x74, 0x6f, 0x20, 0x66, 0x6f, 0x6c, 0x6c, 0x6f, 0x77, 0x20, 0x74, 0x68, 0x65,
						0x20, 0x6c, 0x69, 0x6e, 0x6b, 0x2e, 0
					};	// "\x202c\nCTRL + click to follow the link."
					return PRECEDING + uri_ + FOLLOWING;
				}
				void invoke() const BOOST_NOEXCEPT {
#ifdef BOOST_OS_WINDOWS
					::ShellExecuteW(nullptr, nullptr, uri_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
#endif
				}
			private:
				const String uri_;
			};
		} // namespace @0

		namespace hyperlink {
			/**
			 * Constructor.
			 * @param uriDetector Can't be @c null
			 * @throw NullPointerException @a uriDetector is @c null
			 */
			URIHyperlinkDetector::URIHyperlinkDetector(std::shared_ptr<const rules::URIDetector> uriDetector) : uriDetector_(uriDetector) {
				if(uriDetector.get() == nullptr)
					throw NullPointerException("uriDetector");
			}
			
			/// Destructor.
			URIHyperlinkDetector::~URIHyperlinkDetector() BOOST_NOEXCEPT {
			}
			
			/// @see HyperlinkDetector#nextHyperlink
			std::unique_ptr<Hyperlink> URIHyperlinkDetector::nextHyperlink(
					const kernel::Document& document, Index line, const boost::integer_range<Index>& range) const {
				const String& s = document.line(line);
				if(*range.end() > s.length())
					throw std::out_of_range("range");
				const Char* bol = s.data();
				const StringPiece result(uriDetector_->search(StringPiece(bol + range.front(), range.size())));
				if(result.begin() != nullptr)
					return std::unique_ptr<Hyperlink>(new URIHyperlink(
						boost::irange<Index>(result.begin() - bol, result.end() - bol), String(result.begin(), result.end())));
				else
					return std::unique_ptr<Hyperlink>();
			}


			// hyperlink.CompositeHyperlinkDetector ///////////////////////////////////////////////////////////////////

			/// Destructor.
			CompositeHyperlinkDetector::~CompositeHyperlinkDetector() BOOST_NOEXCEPT {
				typedef std::pair<kernel::ContentType, HyperlinkDetector*> Temp;
				BOOST_FOREACH(const Temp& p, composites_)
					delete p.second;
			}
			
			/// @see HyperlinkDetector#nextHyperlink
			std::unique_ptr<Hyperlink> CompositeHyperlinkDetector::nextHyperlink(
					const kernel::Document& document, Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT {
				const kernel::DocumentPartitioner& partitioner = document.partitioner();
				kernel::DocumentPartition partition;
				for(kernel::Position p(line, *range.begin()), e(line, *range.end()); p < e;) {
					partitioner.partition(p, partition);
					assert(partition.region.includes(p));
					std::map<kernel::ContentType, HyperlinkDetector*>::const_iterator detector(composites_.find(partition.contentType));
					if(detector != composites_.end()) {
						std::unique_ptr<Hyperlink> found = detector->second->nextHyperlink(
							document, line, boost::irange(p.offsetInLine, std::min(partition.region.end(), e).offsetInLine));
						if(found.get() != nullptr)
							return found;
					}
					p = partition.region.end();
				}
				return std::unique_ptr<Hyperlink>();
			}
			
			/**
			 * Sets the hyperlink detector for the specified content type.
			 * @param contentType The content type. if a detector for this content type was already be set, the
			 *                    old will be deleted
			 * @param detector The hyperlink detector to set. Can't be @c null. The ownership will be
			 *                 transferred to the callee
			 * @throw NullPointerException @a detector is @c null
			 */
			void CompositeHyperlinkDetector::setDetector(kernel::ContentType contentType, std::unique_ptr<HyperlinkDetector> detector) {
				if(detector.get() == nullptr)
					throw NullPointerException("detector");
				std::map<kernel::ContentType, HyperlinkDetector*>::iterator old(composites_.find(contentType));
				if(old != composites_.end()) {
					composites_.erase(old);
					delete old->second;
				}
				composites_.insert(std::make_pair(contentType, detector.release()));
			}
		}
	}

	namespace rules {
		// LexicalPartitionPresentationReconstructor.StyledTextRunIterator ////////////////////////////////////////////

		/// @internal
		class LexicalPartitionPresentationReconstructor::StyledTextRunIterator : public presentation::StyledTextRunIterator {
		public:
			StyledTextRunIterator(const kernel::Document& document, TokenScanner& tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>& styles,
				std::shared_ptr<const presentation::TextRunStyle> defaultStyle, const kernel::Region& region);
		private:
			void nextRun();
			// StyledTextRunIterator
			boost::integer_range<Index> currentRange() const;
			std::shared_ptr<const presentation::TextRunStyle> currentStyle() const;
			bool isDone() const BOOST_NOEXCEPT;
			void next();
		private:
//			const LexicalPartitionPresentationReconstructor& reconstructor_;
			TokenScanner& tokenScanner_;
			const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>& styles_;
			std::shared_ptr<const presentation::TextRunStyle> defaultStyle_;
			const kernel::Region region_;
			kernel::Region currentRegion_;
			std::shared_ptr<const presentation::TextRunStyle> currentStyle_;
			std::unique_ptr<Token> next_;
		};

		LexicalPartitionPresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
				const kernel::Document& document, TokenScanner& tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>& styles,
				std::shared_ptr<const presentation::TextRunStyle> defaultStyle, const kernel::Region& region) :
				tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), currentRegion_(region.beginning()) {
			tokenScanner_.parse(document, region);
			nextRun();
		}
		
		/// @see StyledTextRunIterator#currentRange
		boost::integer_range<Index> LexicalPartitionPresentationReconstructor::StyledTextRunIterator::currentRange() const {
			if(isDone())
				throw NoSuchElementException();
			if(currentRegion_.lines().size() == 1)
				return boost::irange(currentRegion_.beginning().offsetInLine, currentRegion_.end().offsetInLine);
			return boost::irange(currentRegion_.beginning().offsetInLine, currentRegion_.end().offsetInLine);
		}
		
		/// @see StyledTextRunIterator#current
		std::shared_ptr<const presentation::TextRunStyle> LexicalPartitionPresentationReconstructor::StyledTextRunIterator::currentStyle() const {
			if(isDone())
				throw NoSuchElementException();
			return currentStyle_;
		}
		
		/// @see StyledTextRunIterator#isDone
		bool LexicalPartitionPresentationReconstructor::StyledTextRunIterator::isDone() const {
			return currentRegion_.end() == region_.end();
		}
		
		/// @see StyledTextRunIterator#next
		void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::next() {
			if(isDone())
				throw NoSuchElementException();
			nextRun();
		}

		inline void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::nextRun() {
			if(next_.get() != nullptr) {
				std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>::const_iterator style(styles_.find(next_->id));
				currentRegion_ = next_->region;
				currentStyle_ = (style != styles_.end()) ? style->second : defaultStyle_;
				next_.reset();
			} else if(tokenScanner_.hasNext()) {
				next_ = move(tokenScanner_.nextToken());
				assert(next_.get() != nullptr);
				assert(next_->region.beginning() >= currentRegion_.end());
				if(next_->region.beginning() != currentRegion_.end()) {
					// 
					currentRegion_ = kernel::Region(currentRegion_.end(), next_->region.beginning());
					currentStyle_ = defaultStyle_;
				} else {
					std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>::const_iterator style(styles_.find(next_->id));
					currentRegion_ = next_->region;
					currentStyle_ = (style != styles_.end()) ? style->second : defaultStyle_;
					next_.reset();
				}
			} else if(currentRegion_.end() != region_.end()) {
				//
				currentRegion_ = kernel::Region(currentRegion_.end(), region_.end());
				currentStyle_ = defaultStyle_;
			}
		}


		// LexicalPartitionPresentationReconstructor //////////////////////////////////////////////////////////////////

		/// @see presentation#PartitionPresentationReconstructor#getPresentation
		std::unique_ptr<presentation::StyledTextRunIterator> LexicalPartitionPresentationReconstructor::getPresentation(const kernel::Region& region) const BOOST_NOEXCEPT {
			return std::unique_ptr<presentation::StyledTextRunIterator>(
				new StyledTextRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
		}
	}
}
