/**
 * @file presentation.hpp
 * Provides classes define appearance and presentation of a text editor user interface.
 * @author exeal
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 */

#ifndef ASCENSION_PRESENTATION_HPP
#define ASCENSION_PRESENTATION_HPP

#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION, ...
#include <ascension/kernel/document.hpp>
#include <ascension/graphics/color.hpp>	// graphics.Color
#include <ascension/graphics/font.hpp>	// graphics.font.FontProperties, ...

namespace ascension {

	namespace graphics {
		namespace font{class TextRenderer;}
	}

	namespace rules {class URIDetector;}

	namespace presentation {

		struct Length {
			double value;	///< Value of the length.
			enum Unit {
				// relative length units
				EM_HEIGHT,		///< The font size of the relevant font.
				X_HEIGHT,		///< The x-height of the relevant font.
				PIXELS,			///< Pixels, relative to the viewing device.
				// relative length units introduced by CSS 3
				GRIDS,				///< The grid.
				REMS,				///< The font size of the primary font.
				VIEWPORT_WIDTH,		///< The viewport's width.
				VIEWPORT_HEIGHT,	///< The viewport's height.
				VIEWPORT_MINIMUM,	///< The viewport's height or width, whichever is smaller of the two.
				/**
				 * The width of the "0" (ZERO, U+0030) glyph found in the font for the font size
				 * used to render. If the "0" glyph is not found in the font, the average character
				 * width may be used.
				 */
				CHARACTERS,
				// absolute length units
				INCHES,			///< Inches -- 1 inch is equal to 2.54 centimeters.
				CENTIMETERS,	///< Centimeters.
				MILLIMETERS,	///< Millimeters.
				POINTS,			///< Points -- the point used by CSS 2.1 are equal to 1/72nd of an inch.
				PICAS,			///< Picas -- 1 pica is equal to 12 points.
				// used in DirectWrite
				DIPS,			///< Device independent pixels. 1 DIP is equal to 1/96th of an inch.
				// percentages (exactly not a length)
				PERCENTAGE,		///< Percentage.
				INHERIT			///< Inherits from the parent.
			} unit;	///< Unit of the length.

			/// Default constructor.
			Length() /*throw()*/ : unit(INHERIT) {}
			/// Constructor.
			explicit Length(double value, Unit unit = PIXELS) : value(value), unit(unit) {}
		};

		struct Border {
			enum Style {
				NONE, HIDDEN, DOTTED, DASHED, SOLID,
				DOT_DASH, DOT_DOT_DASH,
				DOUBLE, GROOVE, RIDGE, INSET, OUTSET,
				INHERIT
			};
			static const Length THIN, MEDIUM, THICK;
			struct Part {
				/**
				 * The foreground color of the border. Default value is Color() which means from
				 * the parent content. This value is ignored if @c #usesCurrentColor member is
				 * @c true.
				 */
				graphics::Color color;
				/**
				 * Set @c true if use the value of @c TextRunStyle#foreground, instead of the one
				 * of @c #color member.
				 */
				bool usesCurrentColor;
				/// Style of the border. Default value is @c NONE.
				Style style;
				/// Thickness of the border. Default value is @c MEDIUM.
				Length width;

				/// Default constructor.
				Part() : color(), style(NONE), width(MEDIUM) {}
			} before, after, start, end;
		};

		struct BaselineAlignment {
			enum Identifier {};
/*			enum DominantBaseline {
				DOMINANT_BASELINE_AUTO,
				DOMINANT_BASELINE_USE_SCRIPT,
				DOMINANT_BASELINE_NO_CHANGE,
				DOMINANT_BASELINE_RESET_SIZE,
				DOMINANT_BASELINE_ALPHABETIC,
				DOMINANT_BASELINE_HANGING,
				DOMINANT_BASELINE_IDEOGRAPHIC,
				DOMINANT_BASELINE_MATHEMATICAL,
				DOMINANT_BASELINE_CENTRAL,
				DOMINANT_BASELINE_MIDDLE,
				DOMINANT_BASELINE_TEXT_AFTER_EDGE,
				DOMINANT_BASELINE_TEXT_DEFORE_EDGE
			} dominantBaseline;
			enum AlignmentBaseline {
				ALIGNMENT_BASELINE_BASELINE,
				ALIGNMENT_BASELINE_USE_SCRIPT,
				ALIGNMENT_BASELINE_BEFORE_EDGE,
				ALIGNMENT_BASELINE_TEXT_BEFORE_EDGE,
				ALIGNMENT_BASELINE_AFTER_EDGE,
				ALIGNMENT_BASELINE_TEXT_AFTER_EDGE,
				ALIGNMENT_BASELINE_CENTRAL,
				ALIGNMENT_BASELINE_MIDDLE,
				ALIGNMENT_BASELINE_IDEOGRAPHIC,
				ALIGNMENT_BASELINE_ALPHABETIC,
				ALIGNMENT_BASELINE_HANGING,
				ALIGNMENT_BASELINE_MATHEMATICAL
			} alignmentBaseline;
			enum AlignmentAdjustEnums {
				ALIGNMENT_ADJUST_AUTO,
				ALIGNMENT_ADJUST_BASELINE,
				ALIGNMENT_ADJUST_BEFORE_EDGE,
				ALIGNMENT_ADJUST_TEXT_BEFORE_EDGE,
				ALIGNMENT_ADJUST_MIDDLE,
				ALIGNMENT_ADJUST_CENTRAL,
				ALIGNMENT_ADJUST_AFTER_EDGE,
				ALIGNMENT_ADJUST_TEXT_AFTER_EDGE,
				ALIGNMENT_ADJUST_IDEOGRAPHIC,
				ALIGNMENT_ADJUST_ALPHABETIC,
				ALIGNMENT_ADJUST_HANGING,
				ALIGNMENT_ADJUST_MATHEMATICAL
			};
			boost::any alignmentAdjust;
			enum BaselineShiftEnums {
				BASELINE_SHIFT_BASELINE,
				BASELINE_SHIFT_SUB,
				BASELINE_SHIFT_SUPER
			};
			boost::any baselineShift;
*/		};

		struct Decorations {
			enum Style {NONE, SOLID, DOTTED, DAHSED, INHERIT};
			struct Part {
				graphics::Color color;	// if is Color(), same as the foreground
				Style style;	///< Default value is @c NONE.
				/// Default constructor.
				Part() : style(NONE) {}
			} overline, strikethrough, baseline, underline;
		};

		enum TextTransform {
			CAPITALIZE, UPPERCASE, LOWERCASE, NONE, TEXT_TRANSFORM_INHERIT
		};

		/**
		 * Visual style settings of a text run.
		 * @see StyledTextRun, IStyledTextRunIterator, TextLineStyle
		 */
		struct TextRunStyle : public FastArenaObject<TextRunStyle> {
			/// Foreground color.
			graphics::Color foreground;
			/// Background color.
			graphics::Color background;
			/// Border of the text run. See the description of @c Border.
			Border border;
			BaselineAlignment baselineAlignment;
			/// Font family name. An empty string means inherit the parent.
			String fontFamily;	// TODO: replace with graphics.font.FontFamilies.
			/// Font properties. See @c graphics#FontProperties.
			graphics::font::FontProperties fontProperties;
			/// 'font-size-adjust' property. 0.0 means 'none', negative value means 'inherit'.
			double fontSizeAdjust;
			std::locale locale;
			/// Typography features applied to the text. See the description of @c TypographyProperties.
			std::map<graphics::font::TrueTypeFontTag, uint32_t> typographyProperties;
			Decorations decorations;
			/// Letter spacing in DIP. Default is 0.
			Length letterSpacing;
			/// Word spacing in DIP. Default is 0.
			Length wordSpacing;
//			TextTransform textTransform;
//			RubyProperties rubyProperties;
//			Effects effects;
			/// Set @c false to disable shaping. Default is @c true.
			bool shapingEnabled;

			/// Default constructor initializes the members by their default values.
			TextRunStyle() : letterSpacing(0), wordSpacing(0), shapingEnabled(true) {}
			TextRunStyle& resolveInheritance(const TextRunStyle& base, bool baseIsRoot);
		};

		struct StyledTextRun {
			/// The beginning column in the line of the text range which the style applies.
			length_t column;
			/// The style of the text run.
			std::tr1::shared_ptr<const TextRunStyle> style;
			/// Default constructor.
			StyledTextRun() {}
			/// Constructor initializes the all members.
			StyledTextRun(length_t column, std::tr1::shared_ptr<const TextRunStyle> style) : column(column), style(style) {}
		};

		class IStyledTextRunIterator {
		public:
			/// Destructor.
			virtual ~IStyledTextRunIterator() /*throw()*/ {}
			/// Returns the current styled text run or throws @c IllegalStateException.
			virtual void current(StyledTextRun& run) const = 0;
			/// Returns @c false if the iterator addresses the end of the range.
			virtual bool hasNext() const = 0;
			/// Moves the iterator to the next styled run or throws @c IllegalStateException.
			virtual void next() = 0;
		};

		class StyledTextRunEnumerator {
		public:
			StyledTextRunEnumerator(std::auto_ptr<IStyledTextRunIterator> sourceIterator, length_t end);
			Range<length_t> currentRange() const;
			std::tr1::shared_ptr<const TextRunStyle> currentStyle() const;
			bool hasNext() const /*throw()*/;
			void next();
		private:
			std::auto_ptr<IStyledTextRunIterator> iterator_;
			std::pair<bool, StyledTextRun> current_, next_;
			const length_t end_;
		};

		/**
		 * @c TextAlignment describes horizontal alignment of a paragraph. This implements
		 * 'text-align' property in CSS 3
		 * (http://www.w3.org/TR/2007/WD-css3-text-20070306/#text-align).
		 * @see resolveTextAlignment, TextLineStyle#alignment, TextLineStyle#lastSublineAlignment
		 */
		enum TextAlignment {
			/// The text is aligned to the start edge of the paragraph.
			ALIGN_START,
			/// The text is aligned to the end edge of the paragraph.
			ALIGN_END,
			/// The text is aligned to the left of the paragraph.
			ALIGN_LEFT,
			/// The text is aligned to the right of the paragraph.
			ALIGN_RIGHT,
			/// The text is aligned to the center of the paragraph.
			/// Some methods which take @c ParagraphAlignment don't accept this value.
			ALIGN_CENTER,
			/// The text is justified according to the method specified @c Justification value.
			JUSTIFY,
			/// The alignment is automatically determined.
			/// Some methods which take @c TextAlignment don't accept this value.
			INHERIT_TEXT_ALIGNMENT
		};

		/**
		 * Orientation of the text layout.
		 * @see TextLineStyle#readingDirection
		 */
		enum ReadingDirection {
			LEFT_TO_RIGHT,				///< The text is left-to-right.
			RIGHT_TO_LEFT,				///< The text is right-to-left.
			INHERIT_READING_DIRECTION	///< 
		};

		struct NumberSubstitution {
			/// Specifies how to apply number substitution on digits and related punctuation.
			enum Method {
				/// Uses the user setting.
				USER_SETTING,
				/**
				 * The substitution method should be determined based on the system setting for
				 * the locale given in the text.
				 */
				FROM_LOCALE,
				/**
				 * The number shapes depend on the context (the nearest preceding strong character,
				 * or the reading direction if there is none).
				 */
				CONTEXTUAL,
				/**
				 * No substitution is performed. Characters U+0030..0039 are always rendered as
				 * nominal numeral shapes (European numbers, not Arabic-Indic digits).
				 */
				NONE,
				/// Numbers are rendered using the national number shapes.
				NATIONAL,
				/// Numbers are rendered using the traditional shapes for the specified locale.
				TRADITIONAL
			} method;	///< The substitution method. Default value is @c USER_SETTING.
			/// The name of the locale to be used.
			std::string localeName;
			/// Whether to ignore user override. Default value is @c false.
			bool ignoreUserOverride;

			/// Default constructor.
			NumberSubstitution() /*throw()*/ : method(USER_SETTING), ignoreUserOverride(false) {}
		};

		struct TextLineStyle {
			/// The reading direction of the line. Default value is @c INHERIT_READING_DIRECTION.
			ReadingDirection readingDirection;
			/// The text alignment of the line. Default value is @c ALIGN_START.
			TextAlignment alignment;
			/// The number substitution setting. Default value is built by the default constructor.
			NumberSubstitution numberSubstitution;

			TextLineStyle() /*throw()*/;
		};

		/**
		 * Interface for objects which direct style of a text line.
		 * @see Presentation#setTextLineStyleDirector
		 */
		class ITextLineStyleDirector {
		public:
			/// Destructor.
			virtual ~ITextLineStyleDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the style of the text line.
			 * @param line the line to be queried
			 * @return the style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::tr1::shared_ptr<const TextLineStyle> queryTextLineStyle(length_t line) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which direct style of text runs in a text line.
		 * @see Presentation#setTextRunStyleDirector
		 */
		class ITextRunStyleDirector {
		public:
			/// Destructor.
			virtual ~ITextRunStyleDirector() /*throw()*/ {}
		private:
			/**
			 * Queries the style of the text line.
			 * @param line the line to be queried
			 * @return the style of the line or @c null (filled by the presentation's default style)
			 * @throw BadPositionException @a line is outside of the document
			 */
			virtual std::auto_ptr<IStyledTextRunIterator> queryTextRunStyle(length_t line) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which direct color of a text line.
		 * @see Presentation#addTextLineColorDirector
		 */
		class ITextLineColorDirector {
		public:
			/// Priority.
			typedef uchar Priority;
			/// Destructor.
			virtual ~ITextLineColorDirector() /*throw()*/ {}
		private:
			/**
			 * Returns the foreground and background colors of the text line.
			 * @param line The line to be queried
			 * @param[out] foreground The foreground color of the text line. If this is invalid
			 *                        color, line color is not set
			 * @param[out] background The background color of the text line. If this is invalid
			 *                        color, line color is not set
			 * @return the priority
			 */
			virtual Priority queryTextLineColors(length_t line,
				graphics::Color& foreground, graphics::Color& background) const = 0;
			friend class Presentation;
		};

		/**
		 * Interface for objects which are interested in change of the default style of
		 * @c Presentation.
		 * @see Presentation#addDefaultStyleListener, Presentation#removeDefaultStyleListener
		 */
		class DefaultTextStyleListener {
		public:
			/// Destructor.
			virtual ~DefaultTextStyleListener() /*throw()*/ {}
			/**
			 * The default text line style of @c Presentation was changed.
			 * @param used The old style used previously
			 * @see Presentation#defaultTextLineStyle, Presentation#setDefaultTextLineStyle
			 */
			virtual void defaultTextLineStyleChanged(std::tr1::shared_ptr<const TextLineStyle> used) = 0;
			/**
			 * The default text run style of @c Presentation was changed.
			 * @param used The old style used previously
			 * @see Presentation#defaultTextRunStyle, Presentation#setDefaultTextRunStyle
			 */
			virtual void defaultTextRunStyleChanged(std::tr1::shared_ptr<const TextRunStyle> used) = 0;
		};

		/**
		 * Provides support for detecting and presenting hyperlinks in text editors. "Hyperlink" is
		 * invokable text segment in the document.
		 * @see Presentation#getHyperlinks, Presentation#setHyperlinkDetector
		 */
		namespace hyperlink {
			/// Represents a hyperlink.
			class IHyperlink {
			public:
				/// Destructor.
				virtual ~IHyperlink() /*throw()*/ {}
				/// Returns the descriptive text of the hyperlink.
				virtual String description() const /*throw()*/ = 0;
				/// Invokes the hyperlink.
				virtual void invoke() const /*throw()*/ = 0;
				/// Returns the columns of the region of the hyperlink.
				const Range<length_t>& region() const /*throw()*/ {return region_;}
			protected:
				/// Protected constructor takes the region of the hyperlink.
				explicit IHyperlink(const Range<length_t>& region) /*throw()*/ : region_(region) {}
			private:
				const Range<length_t> region_;
			};

			/// A @c HyperlinkDetector finds the hyperlinks in the document.
			class IHyperlinkDetector {
			public:
				/// Destructor.
				virtual ~IHyperlinkDetector() /*throw()*/ {}
				/**
				 * Returns the next hyperlink in the specified text line.
				 * @param document the document
				 * @param line the line number
				 * @param range the column range in the line to search. @a range.beginning() can be
				 * the beginning of the found hyperlink
				 * @return the found hyperlink, or @c null if not found
				 */
				virtual std::auto_ptr<IHyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/ = 0;
			};

			/**
			 * URI hyperlink detector.
			 * @see rules#URIDetector, rules#URIRule
			 * @note This class is not intended to be subclassed.
			 */
			class URIHyperlinkDetector : public IHyperlinkDetector {
			public:
				URIHyperlinkDetector(const rules::URIDetector& uriDetector, bool delegateOwnership) /*throw()*/;
				~URIHyperlinkDetector() /*throw()*/;
				// IHyperlinkDetector
				std::auto_ptr<IHyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/;
			private:
				detail::StrategyPointer<const rules::URIDetector> uriDetector_;
			};

			/**
			 * @note This class is not intended to be subclassed.
			 */
			class CompositeHyperlinkDetector : public hyperlink::IHyperlinkDetector {
			public:
				~CompositeHyperlinkDetector() /*throw()*/;
				void setDetector(kernel::ContentType contentType, std::auto_ptr<hyperlink::IHyperlinkDetector> detector);
				// hyperlink.IHyperlinkDetector
				std::auto_ptr<IHyperlink> nextHyperlink(
					const kernel::Document& document, length_t line, const Range<length_t>& range) const /*throw()*/;
			private:
				std::map<kernel::ContentType, hyperlink::IHyperlinkDetector*> composites_;
			};
		} // namespace hyperlink

		/**
		 * A bridge between the document and visual styled text.
		 * @note This class is not intended to be subclassed.
		 * @see kernel#Document, kernel#DocumentPartitioner
		 */
		class Presentation : public kernel::IDocumentListener {
			ASCENSION_NONCOPYABLE_TAG(Presentation);
		public:
			// constructors
			explicit Presentation(kernel::Document& document) /*throw()*/;
			~Presentation() /*throw()*/;
			// attributes
			kernel::Document& document() /*throw()*/;
			const kernel::Document& document() const /*throw()*/;
			const hyperlink::IHyperlink* const* getHyperlinks(length_t line, std::size_t& numberOfHyperlinks) const;
			// styles
			void addDefaultTextStyleListener(DefaultTextStyleListener& listener);
			std::tr1::shared_ptr<const TextLineStyle> defaultTextLineStyle() const /*throw()*/;
			std::tr1::shared_ptr<const TextRunStyle> defaultTextRunStyle() const /*throw()*/;
			void removeDefaultTextStyleListener(DefaultTextStyleListener& listener);
			void setDefaultTextLineStyle(std::tr1::shared_ptr<const TextLineStyle> newStyle);
			void setDefaultTextRunStyle(std::tr1::shared_ptr<const TextRunStyle> newStyle);
			void textLineColors(length_t line, graphics::Color& foreground, graphics::Color& background) const;
			std::tr1::shared_ptr<const TextLineStyle> textLineStyle(length_t line) const;
			std::auto_ptr<IStyledTextRunIterator> textRunStyles(length_t line) const;
			// strategies
			void addTextLineColorDirector(std::tr1::shared_ptr<ITextLineColorDirector> director);
			void removeTextLineColorDirector(ITextLineColorDirector& director) /*throw()*/;
			void setHyperlinkDetector(hyperlink::IHyperlinkDetector* newDetector, bool delegateOwnership) /*throw()*/;
			void setTextLineStyleDirector(std::tr1::shared_ptr<ITextLineStyleDirector> newDirector) /*throw()*/;
			void setTextRunStyleDirector(std::tr1::shared_ptr<ITextRunStyleDirector> newDirector) /*throw()*/;
		private:
			void clearHyperlinksCache() /*throw()*/;
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
		private:
			kernel::Document& document_;
			static std::tr1::shared_ptr<const TextLineStyle> DEFAULT_TEXT_LINE_STYLE;
			static std::tr1::shared_ptr<const TextRunStyle> DEFAULT_TEXT_RUN_STYLE;
			std::tr1::shared_ptr<const TextLineStyle> defaultTextLineStyle_;
			std::tr1::shared_ptr<const TextRunStyle> defaultTextRunStyle_;
			std::tr1::shared_ptr<ITextLineStyleDirector> textLineStyleDirector_;
			std::tr1::shared_ptr<ITextRunStyleDirector> textRunStyleDirector_;
			std::list<std::tr1::shared_ptr<ITextLineColorDirector> > textLineColorDirectors_;
			detail::Listeners<DefaultTextStyleListener> defaultTextStyleListeners_;
			detail::StrategyPointer<hyperlink::IHyperlinkDetector> hyperlinkDetector_;
			struct Hyperlinks;
			mutable std::list<Hyperlinks*> hyperlinks_;
		};

		/**
		 * Creates (reconstructs) styles of the document region. This is used by
		 * @c PresentationReconstructor class to manage the styles in the specified content type.
		 * @see PresentationReconstructor#setPartitionReconstructor
		 */
		class IPartitionPresentationReconstructor {
		public:
			/// Destructor.
			virtual ~IPartitionPresentationReconstructor() /*throw()*/ {}
		private:
			/**
			 * Returns the styled text segments for the specified document region.
			 * @param region the region to reconstruct the new presentation
			 * @return the presentation or @c null (filled by the presentation's default style)
			 */
			virtual std::auto_ptr<IStyledTextRunIterator> getPresentation(const kernel::Region& region) const /*throw()*/ = 0;
			friend class PresentationReconstructor;
		};

		/// Reconstructs document presentation with single text style.
		class SingleStyledPartitionPresentationReconstructor : public IPartitionPresentationReconstructor {
			ASCENSION_UNASSIGNABLE_TAG(SingleStyledPartitionPresentationReconstructor);
		public:
			explicit SingleStyledPartitionPresentationReconstructor(std::tr1::shared_ptr<const TextRunStyle> style) /*throw()*/;
		private:
			// IPartitionPresentationReconstructor
			std::auto_ptr<IStyledTextRunIterator>
				getPresentation(length_t line, const Range<length_t>& columnRange) const /*throw()*/;
		private:
			class StyledTextRunIterator;
			const std::tr1::shared_ptr<const TextRunStyle> style_;
		};

		/**
		 * 
		 */
		class PresentationReconstructor : public ITextRunStyleDirector {
			ASCENSION_UNASSIGNABLE_TAG(PresentationReconstructor);
		public:
			// constructors
			explicit PresentationReconstructor(Presentation& presentation) /*throw()*/;
			~PresentationReconstructor() /*throw()*/;
			// attribute
			void setPartitionReconstructor(kernel::ContentType contentType,
				std::auto_ptr<IPartitionPresentationReconstructor> reconstructor);
		private:
			// ITextRunStyleDirector
			std::auto_ptr<IStyledTextRunIterator> queryTextRunStyle(length_t line) const;
		private:
			class StyledTextRunIterator;
			Presentation& presentation_;
			std::map<kernel::ContentType, IPartitionPresentationReconstructor*> reconstructors_;
		};


		/**
		 * Registers the text line color director.
		 * This method does not call @c TextRenderer#invalidate and the layout is not updated.
		 * @param director the director to register
		 * @throw NullPointerException @a director is @c null
		 */
		inline void Presentation::addTextLineColorDirector(std::tr1::shared_ptr<ITextLineColorDirector> director) {
			if(director.get() == 0) throw NullPointerException("director"); textLineColorDirectors_.push_back(director);}

		/// Returns the default text line style this object gives.
		inline std::tr1::shared_ptr<const TextLineStyle> Presentation::defaultTextLineStyle() const /*throw()*/ {return defaultTextLineStyle_;}

		/// Returns the default text run style this object gives.
		inline std::tr1::shared_ptr<const TextRunStyle> Presentation::defaultTextRunStyle() const /*throw()*/ {return defaultTextRunStyle_;}

		/// Returns the style of the specified text line.
		inline std::tr1::shared_ptr<const TextLineStyle> Presentation::textLineStyle(length_t line) const /*throw()*/ {
			std::tr1::shared_ptr<const TextLineStyle> style;
			if(textLineStyleDirector_.get() != 0)
				style = textLineStyleDirector_->queryTextLineStyle(line);
			return (style.get() != 0) ? style : defaultTextLineStyle();
		}

		/**
		 * Removes the specified text line color director.
		 * @param director the director to remove
		 */
		inline void Presentation::removeTextLineColorDirector(ITextLineColorDirector& director) /*throw()*/ {
			for(std::list<std::tr1::shared_ptr<ITextLineColorDirector> >::iterator
					i(textLineColorDirectors_.begin()), e(textLineColorDirectors_.end()); i != e; ++i) {
				if(i->get() == &director) {textLineColorDirectors_.erase(i); return;}
			}
		}

		/// 
		inline TextAlignment defaultTextAlignment(const Presentation& presentation) {
			std::tr1::shared_ptr<const TextLineStyle> style(presentation.defaultTextLineStyle());
			return (style.get() != 0
				&& style->alignment != INHERIT_TEXT_ALIGNMENT) ? style->alignment : ASCENSION_DEFAULT_TEXT_ALIGNMENT;
		}

		///
		inline ReadingDirection defaultReadingDirection(const Presentation& presentation) {
			std::tr1::shared_ptr<const TextLineStyle> style(presentation.defaultTextLineStyle());
			return (style.get() != 0
				&& style->readingDirection != INHERIT_READING_DIRECTION) ? style->readingDirection : ASCENSION_DEFAULT_TEXT_READING_DIRECTION;
		}

		/**
		 * Resolve an ambiguous text alignment value (@c ALIGN_START and @c ALIGN_END).
		 * @param value the text alignment to compute
		 * @param direction the reading direction
		 * @return the resolved text alignment
		 * @note This function does not resolve @c INHERIT_ALIGNMENT.
		 */
		inline TextAlignment resolveTextAlignment(TextAlignment value, ReadingDirection direction) {
			switch(value) {
				case ALIGN_START:
					return (direction == LEFT_TO_RIGHT) ? ALIGN_LEFT : ALIGN_RIGHT;
				case ALIGN_END:
					return (direction == LEFT_TO_RIGHT) ? ALIGN_RIGHT : ALIGN_LEFT;
				default:
					return value;
			}
		}

}} // namespace ascension.presentation

#endif // !ASCENSION_PRESENTATION_HPP
