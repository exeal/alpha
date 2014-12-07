/**
 * @file presentation.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2014
 */

#include <ascension/corelib/range.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/hyperlink/hyperlink-detector.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <boost/core/null_deleter.hpp>
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
		 * Constructor.
		 * @param document The target document
		 */
		Presentation::Presentation(kernel::Document& document) BOOST_NOEXCEPT : document_(document) {
			setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle>());
			document_.addListener(*this);
		}
		
		/// Destructor.
		Presentation::~Presentation() BOOST_NOEXCEPT {
			document_.removeListener(*this);
			clearHyperlinksCache();
		}
		
		/**
		 * Registers the text toplevel style listener.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 * @see #removeTextToplevelStyleListener, #computedTextToplevelStyle
		 */
		void Presentation::addComputedTextToplevelStyleListener(ComputedTextToplevelStyleListener& listener) {
			computedTextToplevelStyleListeners_.add(listener);
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

		namespace {
			template<typename Styles>
			struct SpecifiedValuesFromCascadedValues {
				SpecifiedValuesFromCascadedValues(
					const typename styles::DeclaredValue<Styles>::type& cascadedValues,
					const typename styles::ComputedValue<Styles>::type& parentComputedValues,
					typename styles::SpecifiedValue<Styles>::type& specifiedValues) :
					cascadedValues(cascadedValues), parentComputedValues(parentComputedValues), specifiedValues(specifiedValues) {}
				template<typename SpecifiedValue>
				void operator()(SpecifiedValue& specifiedValue) const {
					typedef boost::fusion::result_of::find<
						typename styles::SpecifiedValue<Styles>::type, SpecifiedValue
					>::type I;
					static_assert(
						!std::is_same<
							I, boost::fusion::result_of::end<
								typename styles::SpecifiedValue<Styles>::type
							>::type
						>::value,
						"SpecifiedValue was not found in SpecifiedTextLineStyle.");
					typedef boost::fusion::result_of::distance<
						boost::fusion::result_of::begin<
							typename styles::SpecifiedValue<Styles>::type
						>::type, I
					>::type Index;
					styles::specifiedValueFromCascadedValue<boost::fusion::result_of::value_at<Styles, Index>::type>(
						boost::fusion::at<Index>(cascadedValues), boost::fusion::at<Index>(parentComputedValues), specifiedValue);
				}
				const typename styles::DeclaredValue<Styles>::type& cascadedValues;
				const typename styles::ComputedValue<Styles>::type& parentComputedValues;
				mutable typename styles::SpecifiedValue<Styles>::type& specifiedValues;	// mutable because of boost.fusion.for_each
			};

			template<typename Styles>
			void specifiedValuesFromCascadedValues(
					const typename styles::DeclaredValue<Styles>::type& cascadedValues,
					const typename styles::ComputedValue<Styles>::type& parentComputedValues,
					typename styles::SpecifiedValue<Styles>::type& specifiedValues) {
				const SpecifiedValuesFromCascadedValues<Styles> compressor(cascadedValues, parentComputedValues, specifiedValues);
//				boost::fusion::for_each(specifiedValues, compressor);
			}
		}

		/**
		 * Returns the style of the specified text line.
		 * @param line The line number
		 * @param lengthContext 
		 * @param[out] result The computed text line style
		 * @throw BadPositionException @a line is outside of the document
		 * @throw NullPointerException Internal @c Length#value call may throw this exception
		 */
		void Presentation::computeTextLineStyle(Index line,
				const styles::Length::Context& lengthContext, ComputedTextLineStyle& result) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));

			const std::shared_ptr<const DeclaredTextLineStyle> declared(
				(textLineStyleDeclarator_.get() != nullptr) ? textLineStyleDeclarator_->declareTextLineStyle(line)
					: std::shared_ptr<const DeclaredTextLineStyle>(&DeclaredTextLineStyle::unsetInstance(), boost::null_deleter()));

//			styles::cascade(declared);

			SpecifiedTextLineStyle specified;
			specifiedValuesFromCascadedValues<TextLineStyle>(*declared, computedStyles_->forLines.get(), specified);
		}

		namespace {		
			class ComputedStyledTextRunIteratorImpl : public ComputedStyledTextRunIterator {
			public:
				ComputedStyledTextRunIteratorImpl(std::unique_ptr<DeclaredStyledTextRunIterator> declaration,
						const styles::Length::Context& context, const ComputedTextRunStyle& parentComputedStyle)
						: declaration_(std::move(declaration)), context_(context), parentComputedStyle_(parentComputedStyle) {
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
#if 0
					const DeclaredTextRunStyle& cascaded = styles::cascade(declared);
#else
					const DeclaredTextRunStyle& cascaded = *declared;
#endif
					SpecifiedTextRunStyle specified;
					specifiedValuesFromCascadedValues<TextRunStyle>(cascaded, parentComputedStyle_, specified);
					return compute(specified, context_, parentComputedStyle_);
				}

			private:
				std::unique_ptr<DeclaredStyledTextRunIterator> declaration_;
				const styles::Length::Context& context_;
				const ComputedTextRunStyle& parentComputedStyle_;
			};
		}

		/**
		 * Returns the styles of the text runs in the specified line.
		 * @param line The line
		 * @param lengthContext 
		 * @return An iterator enumerates the styles of the text runs in the line, or @c null if the line has no styled
		 *         text runs
		 * @throw BadPositionException @a line is outside of the document
		 */
		std::unique_ptr<ComputedStyledTextRunIterator> Presentation::computeTextRunStyles(Index line, const styles::Length::Context& lengthContext) const {
			if(line >= document_.numberOfLines())
				throw kernel::BadPositionException(kernel::Position(line, 0));
			if(textRunStyleDeclarator_.get() != nullptr) {
				std::unique_ptr<DeclaredStyledTextRunIterator> declaration(textRunStyleDeclarator_->declareTextRunStyle(line));
				if(declaration.get() != nullptr)
					return std::unique_ptr<ComputedStyledTextRunIterator>(
						new ComputedStyledTextRunIteratorImpl(std::move(declaration), lengthContext, computedStyles_->forRuns.get()));
			}
			return std::unique_ptr<ComputedStyledTextRunIterator>();
		}

		/**
		 * Computes the writing mode.
		 * @param line The line to test. If this is @c boost#none, this method returns the entire writing mode
		 * @return The computed writing mode value
		 * @throw IndexOutOfBoundsException @a line is invalid
		 */
		WritingMode Presentation::computeWritingMode(boost::optional<Index> line /* = boost::none */) const {
			const BlockFlowDirection writingMode = *boost::fusion::find<BlockFlowDirection>(computedStyles_->forToplevel.get());
			if(line == boost::none)
				return WritingMode(*boost::fusion::find<ReadingDirection>(computedStyles_->forLines.get()),
					writingMode, *boost::fusion::find<TextOrientation>(computedStyles_->forLines.get()));
			else {
				// compute 'direction' and 'text-orientation' properties
				std::shared_ptr<const DeclaredTextLineStyle> declared;
				if(textLineStyleDeclarator_.get() != nullptr)
					declared = textLineStyleDeclarator_->declareTextLineStyle(boost::get(line));
#if 0
				const DeclaredTextLineStyle& cascaded = styles::cascade(declared);
#else
				const DeclaredTextLineStyle& cascaded = *declared;
#endif
				if(declared.get() == nullptr)
					declared.reset(&DeclaredTextLineStyle::unsetInstance(), boost::null_deleter());

				styles::SpecifiedValue<styles::Direction>::type specifiedDirection;
				const auto& cascadedDirection(*boost::fusion::find<styles::DeclaredValue<styles::Direction>::type>(cascaded));
				const auto& computedParentDirection(*boost::fusion::find<styles::ComputedValue<styles::Direction>::type>(computedStyles_->forLines.get()));
				static_assert(!std::is_same<std::decay<decltype(cascadedDirection)>::type, boost::mpl::void_>::value, "");
				static_assert(!std::is_same<std::decay<decltype(computedParentDirection)>::type, boost::mpl::void_>::value, "");
				styles::specifiedValueFromCascadedValue<styles::Direction>(
					cascadedDirection,
					[&computedParentDirection]() {
						return computedParentDirection;
					},
					specifiedDirection);

				styles::SpecifiedValue<styles::TextOrientation>::type specifiedTextOrientation;
				const auto& cascadedTextOrientation(*boost::fusion::find<styles::DeclaredValue<styles::TextOrientation>::type>(cascaded));
				const auto& computedParentTextOrientation(*boost::fusion::find<styles::ComputedValue<styles::TextOrientation>::type>(computedStyles_->forLines.get()));
				static_assert(!std::is_same<std::decay<decltype(cascadedTextOrientation)>::type, boost::mpl::void_>::value, "");
				static_assert(!std::is_same<std::decay<decltype(computedParentTextOrientation)>::type, boost::mpl::void_>::value, "");
				styles::specifiedValueFromCascadedValue<styles::TextOrientation>(
					cascadedTextOrientation,
					[&computedParentTextOrientation]() {
						return computedParentTextOrientation;
					},
					specifiedTextOrientation);

				styles::ComputedValue<styles::Direction>::type computedDirection(specifiedDirection);	// TODO: compute as specified
				styles::ComputedValue<styles::TextOrientation>::type computedTextOrientation(specifiedTextOrientation);	// TODO: compute as specified
				return WritingMode(computedDirection, writingMode, computedTextOrientation);
			}		
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
		 * Removes the computed text toplevel style listener.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 * @see #addComputedTextToplevelStyleListener, #computedTextToplevelStyle
		 */
		void Presentation::removeComputedTextToplevelStyleListener(ComputedTextToplevelStyleListener& listener) {
			computedTextToplevelStyleListeners_.remove(listener);
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
		 * Sets the declared text toplevel line style.
		 * @param newStyle The style to set
		 * @see #declaredTextToplevelStyle, ComputedTextToplevelStyleListener
		 */
		void Presentation::setDeclaredTextToplevelStyle(std::shared_ptr<const DeclaredTextToplevelStyle> newStyle) {
			std::shared_ptr<const DeclaredTextToplevelStyle> newlyDeclared = (newStyle.get() != nullptr) ? newStyle
				: std::shared_ptr<const DeclaredTextToplevelStyle>(&DeclaredTextToplevelStyle::unsetInstance(), boost::null_deleter());
#if 0
			const DeclaredTextToplevelStyle& newlyCascaded = styles::cascade(newlyDeclared);
#else
			const DeclaredTextToplevelStyle& newlyCascaded = *newlyDeclared;
#endif

			const auto& cascadedWritingMode(*boost::fusion::find<styles::DeclaredValue<styles::WritingMode>::type>(newlyCascaded));
			static_assert(!std::is_same<std::decay<decltype(cascadedWritingMode)>::type, boost::mpl::void_>::value, "");
			SpecifiedTextToplevelStyle newlySpecified;
			auto& specifiedWritingMode(*boost::fusion::find<styles::SpecifiedValue<styles::WritingMode>::type>(newlySpecified));
			static_assert(!std::is_same<std::decay<decltype(specifiedWritingMode)>::type, boost::mpl::void_>::value, "");
			styles::specifiedValueFromCascadedValue<styles::WritingMode>(cascadedWritingMode, nullptr, specifiedWritingMode);
			boost::flyweight<ComputedTextToplevelStyle> newlyComputed(compute(newlySpecified));

			// commit
			std::shared_ptr<const DeclaredTextToplevelStyle> previouslyDeclared(declaredTextToplevelStyle_);
			boost::flyweight<ComputedTextToplevelStyle> previouslyComputed(computedStyles_->forToplevel);
			using boost::swap;
			using std::swap;
			swap(newlyDeclared, declaredTextToplevelStyle_);
			swap(newlyComputed, computedStyles_->forToplevel);
			if(previouslyDeclared.get() != nullptr)
				computedTextToplevelStyleListeners_.notify
					<std::shared_ptr<const DeclaredTextToplevelStyle>, boost::flyweight<ComputedTextToplevelStyle>>(
						&ComputedTextToplevelStyleListener::computedTextToplevelStyleChanged, previouslyDeclared, previouslyComputed);
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
