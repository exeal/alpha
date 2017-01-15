/**
 * @file rendering-context-gdi.cpp
 * @author exeal
 * @date 2012-05-30 created
 * @date 2012-2014
 */

#include <ascension/graphics/rendering-context.hpp>
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
#include <ascension/corelib/native-conversion.hpp>
#include <ascension/graphics/font/font.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/paint.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/math/constants/constants.hpp>		// boost.math.constants.pi
#include <boost/math/special_functions/round.hpp>	// boost.iround

namespace ascension {
	namespace graphics {
		/**
		 * Constructor.
		 * @param nativeObject The Win32 @c HDC value to hold
		 */
		RenderingContext2D::RenderingContext2D(win32::Handle<HDC> nativeObject) : nativeObject_(nativeObject), hasCurrentSubpath_(false) {
			setFillStyle(std::make_shared<SolidColor>(Color::OPAQUE_BLACK));
			setStrokeStyle(std::make_shared<SolidColor>(Color::OPAQUE_BLACK));

			auto fontHandle(win32::borrowed(static_cast<HFONT>(::GetCurrentObject(nativeObject.get(), OBJ_FONT))));
			if(fontHandle.get() == nullptr)
				fontHandle.reset(static_cast<HFONT>(::GetStockObject(DEVICE_DEFAULT_FONT)), boost::null_deleter());
			assert(fontHandle.get() != nullptr);
			setFont(std::make_shared<const font::Font>(fontHandle));

			::SetBkMode(nativeObject_.get(), TRANSPARENT);
			::SetGraphicsMode(nativeObject_.get(), GM_ADVANCED);
			::SetPolyFillMode(nativeObject_.get(), WINDING);
			::SetTextAlign(nativeObject_.get(), TA_LEFT | TA_BASELINE | TA_NOUPDATECP);
		}

		namespace {
			inline FLOAT radianToDegree(double radian) BOOST_NOEXCEPT {
				return static_cast<FLOAT>(radian * 180. / boost::math::constants::pi<double>());
			}
		}

		RenderingContext2D& RenderingContext2D::arc(const Point& p,
				Scalar radius, double startAngle, double endAngle, bool counterClockwise /* = false */) {
			if(radius < 0)
				throw std::invalid_argument("radius");
			if(!hasCurrentSubpath_) {
				::AngleArc(nativeObject_.get(), static_cast<int>(geometry::x(p)), static_cast<int>(geometry::y(p)), static_cast<DWORD>(radius), radianToDegree(startAngle), 0.0);
				hasCurrentSubpath_ = true;
			}
			if(!win32::boole(::AngleArc(nativeObject_.get(), static_cast<int>(geometry::x(p)), static_cast<int>(geometry::y(p)),
					static_cast<DWORD>(radius), radianToDegree(startAngle), counterClockwise ? radianToDegree(startAngle - endAngle) : radianToDegree(endAngle - startAngle))))
				throw makePlatformError();
			return *this;
		}

		RenderingContext2D& RenderingContext2D::arcTo(const Point& p1, const Point& p2, Scalar radius) {
			if(radius < 0)
				throw std::invalid_argument("radius");
			ensureThereIsASubpathFor(p1);
			// TODO: not implemented.
			return *this;
		}

		font::FontCollection RenderingContext2D::availableFonts() const {
			return font::FontCollection(nativeObject_);
		}

		RenderingContext2D& RenderingContext2D::beginPath() {
			if(!win32::boole(::BeginPath(nativeObject_.get())))
				throw makePlatformError();
			hasCurrentSubpath_ = false;
			return *this;
		}

		RenderingContext2D& RenderingContext2D::bezierCurveTo(const Point& cp1, const Point& cp2, const Point& to) {
			ensureThereIsASubpathFor(cp1);
			const POINT points[3] = {toNative<POINT>(cp1), toNative<POINT>(cp2), toNative<POINT>(to)};
			if(!win32::boole(::PolyBezierTo(nativeObject_.get(), points, 3)))
				throw makePlatformError();
			return *this;
		}

		RenderingContext2D& RenderingContext2D::changePen(win32::Handle<HPEN> newPen) {
			assert(newPen.get() != nullptr);
			auto oldPen(win32::borrowed(static_cast<HPEN>(::SelectObject(nativeObject_.get(), newPen.get()))));
			if(oldPen.get() == nullptr)
				throw makePlatformError();
			currentState_.pen = newPen;
			std::swap(currentState_.previousPen, oldPen);
			return *this;
		}

		RenderingContext2D& RenderingContext2D::clearRectangle(const graphics::Rectangle& rectangle) {
			const RECT temp(toNative<RECT>(rectangle));
			if(::FillRect(nativeObject_.get(), &temp, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH))) == 0)
				throw makePlatformError();
			return *this;
		}

		RenderingContext2D& RenderingContext2D::clip() {
			closePath();
			if(!win32::boole(::SelectClipPath(nativeObject_.get(), RGN_AND))) {
				const auto e(makePlatformError());
				if(e.code().value() != ERROR_CAN_NOT_COMPLETE)
					throw e;
			}
			return *this;
		}

		RenderingContext2D& RenderingContext2D::closePath() {
			if(endPath()) {
//				if(!win32::boole(::CloseFigure(nativeObject_.get())))
//					throw makePlatformError();
			}
			return *this;
		}

		std::unique_ptr<ImageData> RenderingContext2D::createImageData(const Dimension& dimensions) const {
			const std::size_t dx = static_cast<std::size_t>(geometry::dx(dimensions));
			const std::size_t dy = static_cast<std::size_t>(geometry::dy(dimensions));
			const std::size_t pixels = dx * dy /*geometry::area(dimensions)*/;
			std::unique_ptr<std::uint8_t[]> bytes(new std::uint8_t[pixels]);
			for(std::size_t i = 0; i < pixels; i += 4) {
				bytes[i + 0] = Color::TRANSPARENT_BLACK.red();
				bytes[i + 1] = Color::TRANSPARENT_BLACK.green();
				bytes[i + 2] = Color::TRANSPARENT_BLACK.blue();
				bytes[i + 3] = Color::TRANSPARENT_BLACK.alpha();
			}
			return std::unique_ptr<ImageData>(new ImageData(std::move(bytes), dx, dy));
		}

		win32::Handle<HPEN> RenderingContext2D::createModifiedPen(const LOGBRUSH* patternBrush,
				boost::optional<Scalar> lineWidth, boost::optional<LineCap> lineCap, boost::optional<LineJoin> lineJoin) const {
			if(HGDIOBJ oldPen = ::GetCurrentObject(nativeObject_.get(), OBJ_PEN)) {
				DWORD style = PS_GEOMETRIC | PS_SOLID;
				Scalar width;
				LOGBRUSH brush;
				switch(::GetObjectType(oldPen)) {
					case 0:
						throw makePlatformError();
					case OBJ_PEN: {
						LOGPEN lp;
						if(::GetObjectW(oldPen, sizeof(LOGPEN), &lp) == 0)
							throw makePlatformError();
						width = static_cast<Scalar>(lp.lopnWidth.x);
						brush.lbStyle = BS_SOLID;
						brush.lbColor = lp.lopnColor;
						break;
					}
					case OBJ_EXTPEN: {
						EXTLOGPEN elp;
						if(::GetObjectW(oldPen, sizeof(EXTLOGPEN), &elp) == 0)
							throw makePlatformError();
						style |= elp.elpPenStyle & PS_ENDCAP_MASK;
						style |= elp.elpPenStyle & PS_JOIN_MASK;
						width = static_cast<Scalar>(elp.elpWidth);
						brush.lbStyle = elp.elpBrushStyle;
						brush.lbColor = elp.elpColor;
						brush.lbHatch = elp.elpHatch;
						break;
					}
				}

				if(patternBrush != nullptr)
					brush = *patternBrush;
				width = boost::get_optional_value_or(lineWidth, width);
				if(lineCap) {
					style &= ~PS_ENDCAP_MASK;
					switch(boost::native_value(*lineCap)) {
						case LineCap::BUTT:
							style |= PS_ENDCAP_FLAT;
							break;
						case LineCap::ROUND:
							style |= PS_ENDCAP_ROUND;
							break;
						case LineCap::SQUARE:
							style |= PS_ENDCAP_SQUARE;
							break;
						default:
							throw UnknownValueException("lineCap");
					}
				}
				if(lineJoin) {
					style &= ~PS_JOIN_MASK;
					switch(boost::native_value(*lineJoin)) {
						case LineJoin::BEVEL:
							style |= PS_JOIN_BEVEL;
							break;
						case LineJoin::MITER:
							style |= PS_JOIN_MITER;
							break;
						case LineJoin::ROUND:
							style |= PS_JOIN_ROUND;
							break;
						default:
							throw UnknownValueException("lineJoin");
					}
				}

				return win32::makeHandle(::ExtCreatePen(style, static_cast<DWORD>(width), &brush, 0, nullptr), &::DeleteObject);
			}
			throw makePlatformError();
		}
#if 1
		RenderingContext2D& RenderingContext2D::drawFocusIfNeeded(const void* element) {
			// TODO: not implemented.
			return *this;
		}
#else
		void RenderingContext2D::drawSystemFocusRing(/*const NativeRectangle& bounds*/) {
			// TODO: not implemented.
		}

		bool RenderingContext2D::drawCustomFocusRing(/*const NativeRectangle& bounds*/) {
			// TODO: not implemented.
			return true;
		}
#endif
		inline bool RenderingContext2D::endPath() {
			if(!hasCurrentSubpath_)
				return false;
			if(!win32::boole(::EndPath(nativeObject_.get()))) {
				const auto e(makePlatformError());
				assert(e.code().value() != ERROR_INVALID_PARAMETER);
				throw e;
			}
			hasCurrentSubpath_ = false;
			return true;
		}

		inline bool RenderingContext2D::ensureThereIsASubpathFor(const Point& p) {
			const bool hadCurrentSubpath = hasCurrentSubpath_;
			if(!hasCurrentSubpath_)
				moveTo(p);
			assert(hasCurrentSubpath_);
			return !hadCurrentSubpath;
		}

		RenderingContext2D& RenderingContext2D::fill() {
			if(endPath()) {
				updatePenAndBrush();
				if(!win32::boole(::FillPath(nativeObject_.get())))
					throw makePlatformError();
			}
			return *this;
		}

		RenderingContext2D& RenderingContext2D::fillRectangle(const graphics::Rectangle& rectangle) {
			updatePenAndBrush();
			if(HBRUSH currentBrush = static_cast<HBRUSH>(::GetCurrentObject(nativeObject_.get(), OBJ_BRUSH))) {
				RECT temp(toNative<RECT>(rectangle));
				if(::FillRect(nativeObject_.get(), &temp, currentBrush) != 0)
					return *this;
			}
			throw makePlatformError();
		}

		std::shared_ptr<const Paint> RenderingContext2D::fillStyle() const {
			return currentState_.fillStyle.first;
		}

		std::shared_ptr<const font::Font> RenderingContext2D::font() const {
			return currentState_.font;
		}

		namespace {
			class SubpathsSaver : private boost::noncopyable {
			public:
				explicit SubpathsSaver(win32::Handle<HDC> deviceContext)
						: deviceContext_(deviceContext), numberOfPoints_(::GetPath(deviceContext.get(), nullptr, nullptr, 0)) {
					// backup subpaths
					if(numberOfPoints_ == 0)
						throw makePlatformError();
					points_.reset(new POINT[numberOfPoints_]);
					types_.reset(new BYTE[numberOfPoints_]);
					if(::GetPath(deviceContext_.get(), points_.get(), types_.get(), numberOfPoints_) == 0)
						throw makePlatformError();
					if(!win32::boole(::AbortPath(deviceContext_.get()))) {
						points_.reset();
						throw makePlatformError();
					}
				}
				~SubpathsSaver() BOOST_NOEXCEPT {
					if(points_.get() != nullptr) {
						// restore subpaths
						::BeginPath(deviceContext_.get());
						::PolyDraw(deviceContext_.get(), points_.get(), types_.get(), numberOfPoints_);
					}
				}
			private:
				const win32::Handle<HDC> deviceContext_;
				const int numberOfPoints_;
				std::unique_ptr<POINT[]> points_;
				std::unique_ptr<BYTE[]> types_;
			};

			class FontSaver : private boost::noncopyable {
			public:
				explicit FontSaver(win32::Handle<HDC> deviceContext) :
						deviceContext_(deviceContext),
						savedFont_(static_cast<HFONT>(::GetCurrentObject(deviceContext_.get(), OBJ_FONT))) {
					if(savedFont_.get() == nullptr)
						throw makePlatformError();
				}
				~FontSaver() BOOST_NOEXCEPT {
					::SelectObject(deviceContext_.get(), savedFont_.get());
				}
				win32::Handle<HFONT> savedFont() const BOOST_NOEXCEPT {
					return savedFont_;
				}
			private:
				const win32::Handle<HDC> deviceContext_;
				win32::Handle<HFONT> savedFont_;
			};

			RenderingContext2D& paintText(RenderingContext2D& context, const StringPiece& text,
					const Point& origin, boost::optional<Scalar> maximumMeasure, bool onlyStroke) {
				const SubpathsSaver sb(context.native());
				const auto dc(context.native());
				std::unique_ptr<const FontSaver> fontSaver;
				win32::Handle<HFONT> condensedFont;
				if(maximumMeasure) {
					fontSaver.reset(new FontSaver(context.native()));
					const Scalar measure = geometry::dx(context.measureText(text));
					if(measure > *maximumMeasure) {
						LOGFONTW lf;
						if(::GetObjectW(fontSaver->savedFont().get(), sizeof(LOGFONTW), &lf) == 0)
							throw makePlatformError();
						lf.lfWidth = static_cast<LONG>(static_cast<Scalar>(lf.lfWidth) * *maximumMeasure / measure);
//						lf.lfWidth = ::MulDiv(lf.lfWidth, *maximumMeasure, measure);
						condensedFont.reset(::CreateFontIndirectW(&lf), &::DeleteObject);
						if(condensedFont.get() == nullptr || ::SelectObject(dc.get(), condensedFont.get()) == nullptr)
							throw makePlatformError();
					}
				}

				if(onlyStroke) {
					if(!win32::boole(::BeginPath(dc.get())))
						throw makePlatformError();
				}
				if(!win32::boole(::ExtTextOutW(dc.get(),
						static_cast<int>(geometry::x(origin)), static_cast<int>(geometry::y(origin)),
						ETO_NUMERICSLOCAL, nullptr, win32::wideString<const WCHAR>(text.cbegin()), text.length(), nullptr)))
					throw makePlatformError();
				if(onlyStroke) {
					if(!win32::boole(::EndPath(dc.get())) || !win32::boole(::StrokePath(dc.get())))
						throw makePlatformError();
				}

				return context;
			}
		}

		RenderingContext2D& RenderingContext2D::fillText(const StringPiece& text,
				const Point& origin, boost::optional<Scalar> maximumMeasure /* = boost::none */) {
			updatePenAndBrush();
			return paintText(*this, text, origin, maximumMeasure, false);
		}

		namespace {
			class GdiFontMetrics : public font::FontMetrics<Scalar> {
			public:
				explicit GdiFontMetrics(win32::Handle<HDC> dc, win32::Handle<HFONT> font) {
					const int cookie = ::SaveDC(dc.get());
					if(font.get() != nullptr)
						::SelectObject(dc.get(), font.get());
					if(::SetGraphicsMode(dc.get(), GM_ADVANCED) == 0)
						fail(dc, cookie);
//					const double xdpi = ::GetDeviceCaps(dc.get(), LOGPIXELSX);
//					const double ydpi = ::GetDeviceCaps(dc.get(), LOGPIXELSY);

					// generic font metrics
					auto otm(win32::makeZeroSize<OUTLINETEXTMETRICW, UINT>());
					TEXTMETRICW tm;
					if(::GetOutlineTextMetricsW(dc.get(), sizeof(decltype(otm)), &otm) == 0) {
						tm = otm.otmTextMetrics;
						unitsPerEm_ = otm.otmEMSquare;
					} else if(win32::boole(::GetTextMetricsW(dc.get(), &tm)))
						unitsPerEm_ = 1;	// hmm...
					else
						fail(dc, cookie);
					ascent_ = static_cast<Unit>(tm.tmAscent)/* * defaultDpiY() / ydpi*/;
					descent_ = static_cast<Unit>(tm.tmDescent)/* * 96defaultDpiY()0 / ydpi*/;
					internalLeading_ = static_cast<Unit>(tm.tmInternalLeading)/* * defaultDpiY() / ydpi*/;
					externalLeading_ = static_cast<Unit>(tm.tmExternalLeading)/* * defaultDpiY() / ydpi*/;
					averageCharacterWidth_ = ((tm.tmAveCharWidth > 0) ?
						static_cast<Unit>(tm.tmAveCharWidth) : static_cast<Unit>(tm.tmHeight) * 0.56f)/* * defaultDpiY() / xdpi*/;
					averageCharacterWidth_ = std::max(averageCharacterWidth_, static_cast<Unit>(1));

					// x-height
					GLYPHMETRICS gm;
					const MAT2 temp = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
					xHeight_ = (::GetGlyphOutlineW(dc.get(), L'x', GGO_METRICS, &gm, 0, nullptr, nullptr) != GDI_ERROR
						&& gm.gmptGlyphOrigin.y > 0) ? static_cast<Unit>(gm.gmptGlyphOrigin.y) : boost::math::iround(static_cast<double>(ascent_) * 0.56);
					if(xHeight_ == GDI_ERROR)
						fail(dc, cookie);
				}
				Unit ascent() const BOOST_NOEXCEPT override {return ascent_;}
				Unit averageCharacterWidth() const BOOST_NOEXCEPT override {return averageCharacterWidth_;}
				Unit descent() const BOOST_NOEXCEPT override {return descent_;}
				Unit externalLeading() const BOOST_NOEXCEPT override {return externalLeading_;}
				Unit internalLeading() const BOOST_NOEXCEPT override {return internalLeading_;}
				std::uint16_t unitsPerEm() const BOOST_NOEXCEPT override {return unitsPerEm_;}
				Unit xHeight() const BOOST_NOEXCEPT override {return xHeight_;}

			private:
				static void fail(win32::Handle<HDC> dc, int savedContext) {
					::RestoreDC(dc.get(), savedContext);
					throw makePlatformError();
				}
				Unit ascent_, descent_, internalLeading_, externalLeading_, averageCharacterWidth_, xHeight_;
				std::uint16_t unitsPerEm_;
			};
		}

		std::unique_ptr<const font::FontMetrics<Scalar>> RenderingContext2D::fontMetrics(std::shared_ptr<const font::Font> font /* = nullptr */) const {
			if(font.get() == nullptr)
				font = this->font();
			return std::unique_ptr<const font::FontMetrics<Scalar>>(new GdiFontMetrics(nativeObject_, font->native()));
		}

		font::FontRenderContext RenderingContext2D::fontRenderContext() const {
			XFORM xf;
			if(!win32::boole(::GetWorldTransform(nativeObject_.get(), &xf)))
				throw makePlatformError();
			const geometry::AffineTransform tx(fromNative<geometry::AffineTransform>(xf));

			bool antiAliased;
			LOGFONTW lf;
			if(::GetObjectW(font()->native().get(), sizeof(decltype(lf)), &lf) == 0)
				throw makePlatformError();
			if(lf.lfQuality == ANTIALIASED_QUALITY || lf.lfQuality == CLEARTYPE_QUALITY)
				antiAliased = true;
			else if(lf.lfQuality == NONANTIALIASED_QUALITY)
				antiAliased = false;
			else {
				BOOL temp;
				if(!win32::boole(::SystemParametersInfoW(SPI_GETFONTSMOOTHING, 0, &temp, 0)))
					throw makePlatformError();
				antiAliased = win32::boole(temp);
			}

			return font::FontRenderContext(tx, antiAliased, false);
		}

		std::unique_ptr<ImageData> RenderingContext2D::getImageData(const graphics::Rectangle& bounds) const {
			// TODO: not implemented.
			return nullptr;
		}

		double RenderingContext2D::globalAlpha() const {
			return 1.0;	// not supported in Win32 GDI...
		}

		bool RenderingContext2D::isPointInPath(const Point& point) const {
			// this is tough fight...

			const SubpathsSaver sb(nativeObject_);
			if(!win32::boole(::EndPath(nativeObject_.get())))
				throw makePlatformError();
			auto region(win32::makeHandle(::PathToRegion(nativeObject_.get()), &::DeleteObject));
			if(region.get() == nullptr)
				throw makePlatformError();
			return win32::boole(::PtInRegion(region.get(), static_cast<int>(geometry::x(point)), static_cast<int>(geometry::y(point))));
		}

		LineCap RenderingContext2D::lineCap() const {
			if(HPEN currentPen = static_cast<HPEN>(::GetCurrentObject(nativeObject_.get(), OBJ_PEN))) {
				switch(::GetObjectType(currentPen)) {
					case 0:
						break;
					case OBJ_PEN:
						return LineCap::BUTT;
					case OBJ_EXTPEN: {
						EXTLOGPEN elp;
						if(::GetObjectW(currentPen, sizeof(EXTLOGPEN), &elp) == 0)
							break;
						switch(elp.elpPenStyle & PS_ENDCAP_MASK) {
							case PS_ENDCAP_FLAT:
								return LineCap::BUTT;
							case PS_ENDCAP_ROUND:
								return LineCap::ROUND;
							case PS_ENDCAP_SQUARE:
								return LineCap::SQUARE;
							default:
								ASCENSION_ASSERT_NOT_REACHED();
						}
					}
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
			throw makePlatformError();
		}

		LineJoin RenderingContext2D::lineJoin() const {
			if(HPEN currentPen = static_cast<HPEN>(::GetCurrentObject(nativeObject_.get(), OBJ_PEN))) {
				switch(::GetObjectType(currentPen)) {
					case 0:
						break;
					case OBJ_PEN:
						return LineJoin::BEVEL;
					case OBJ_EXTPEN: {
						EXTLOGPEN elp;
						if(::GetObjectW(currentPen, sizeof(EXTLOGPEN), &elp) == 0)
							break;
						switch(elp.elpPenStyle & PS_JOIN_MASK) {
							case PS_JOIN_BEVEL:
								return LineJoin::BEVEL;
							case PS_JOIN_MITER:
								return LineJoin::MITER;
							case PS_JOIN_ROUND:
								return LineJoin::ROUND;
							default:
								ASCENSION_ASSERT_NOT_REACHED();
						}
					}
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
			throw makePlatformError();
		}

		RenderingContext2D& RenderingContext2D::lineTo(const Point& to) {
			if(!ensureThereIsASubpathFor(to)) {
				if(!win32::boole(::LineTo(nativeObject_.get(), static_cast<int>(geometry::x(to)), static_cast<int>(geometry::y(to)))))
					throw makePlatformError();
			}
			return *this;
		}

		Scalar RenderingContext2D::lineWidth() const {
			if(HPEN currentPen = static_cast<HPEN>(::GetCurrentObject(nativeObject_.get(), OBJ_PEN))) {
				switch(::GetObjectType(currentPen)) {
					case 0:
						break;
					case OBJ_PEN: {
						LOGPEN lp;
						if(::GetObjectW(currentPen, sizeof(LOGPEN), &lp) == 0)
							break;
						return static_cast<Scalar>(lp.lopnWidth.x);
					}
					case OBJ_EXTPEN: {
						EXTLOGPEN elp;
						if(::GetObjectW(currentPen, sizeof(EXTLOGPEN), &elp) == 0)
							break;
						return static_cast<Scalar>(elp.elpWidth);
					}
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
			throw makePlatformError();
		}

		Dimension RenderingContext2D::measureText(const StringPiece& text) const {
			SIZE s;
			if(!win32::boole(::GetTextExtentPoint32W(nativeObject_.get(), win32::wideString<const WCHAR>(text.cbegin()), text.length(), &s)))
				throw makePlatformError();
			const geometry::BasicDimension<LONG> temp(fromNative<geometry::BasicDimension<LONG>>(s));
			return Dimension(geometry::_dx = static_cast<Scalar>(geometry::dx(temp)), geometry::_dy = static_cast<Scalar>(geometry::dy(temp)));
		}

		double RenderingContext2D::miterLimit() const {
			FLOAT temp;
			if(!win32::boole(::GetMiterLimit(nativeObject_.get(), &temp)))
				throw makePlatformError();
			return temp;
		}

		RenderingContext2D& RenderingContext2D::moveTo(const Point& to) {
			if(!win32::boole(::MoveToEx(nativeObject_.get(), static_cast<int>(geometry::x(to)), static_cast<int>(geometry::y(to)), nullptr)))
				throw makePlatformError();
			hasCurrentSubpath_ = true;
			return *this;
		}

		/// Returns the internal Win32 @c HDC value.
		win32::Handle<HDC> RenderingContext2D::native() const BOOST_NOEXCEPT {
			return nativeObject_;
		}

		RenderingContext2D& RenderingContext2D::putImageData(
				const ImageData& image, const Point& destination, const graphics::Rectangle& dirtyRectangle) {
			auto dc(win32::makeHandle(::CreateCompatibleDC(nativeObject_.get()), &::DeleteDC));
			if(dc.get() != nullptr) {
				static_assert(sizeof(DWORD) == 4, "");
				const std::size_t dx = std::min(static_cast<std::size_t>(geometry::dx(dirtyRectangle)), image.width());
				const std::size_t dy = std::min(static_cast<std::size_t>(geometry::dy(dirtyRectangle)), image.height());
				auto header(win32::makeZeroSize<BITMAPV5HEADER>());
				header.bV5Width = dx;
				header.bV5Height = dy;
				header.bV5Planes = 1;
				header.bV5BitCount = 32;
				header.bV5Compression = BI_BITFIELDS;
				header.bV5RedMask = 0x00ff0000u;
				header.bV5GreenMask = 0x0000ff00u;
				header.bV5BlueMask = 0x000000ffu;
				header.bV5AlphaMask = 0xff000000u;
				void* pixels;
				auto bitmap(win32::makeHandle(::CreateDIBSection(nativeObject_.get(),
					reinterpret_cast<BITMAPINFO*>(&header), DIB_RGB_COLORS, &pixels, nullptr, 0), &::DeleteObject));
				if(bitmap.get() != nullptr) {
					const std::uint8_t* const imageData = boost::begin(image.data());
					DWORD* pixel = static_cast<DWORD*>(pixels);
					for(std::size_t y = 0; y < dy; ++y) {
						for(std::size_t x = 0; x < dx; ++x, ++pixel) {
							// write premultiplied components
							const std::uint8_t* const sourcePixel = imageData + x + y * dx;
							*pixel = ::MulDiv(sourcePixel[0], sourcePixel[3], 255);		// R
							*pixel |= ::MulDiv(sourcePixel[1], sourcePixel[3], 255);	// G
							*pixel |= ::MulDiv(sourcePixel[2], sourcePixel[3], 255);	// B
							*pixel |= 0xff000000u;										// A
						}
					}
				}
				if(const HGDIOBJ oldBitmap = ::SelectObject(dc.get(), bitmap.get())) {
					const bool succeeded = win32::boole(::BitBlt(nativeObject_.get(),
						static_cast<int>(geometry::x(destination) + geometry::left(dirtyRectangle)),
						static_cast<int>(geometry::y(destination) + geometry::top(dirtyRectangle)),
						dx, dy, dc.get(),
						static_cast<int>(geometry::left(dirtyRectangle)),
						static_cast<int>(geometry::top(dirtyRectangle)), SRCCOPY));
					::SelectObject(dc.get(), oldBitmap);
					if(succeeded)
						return *this;
				}
			}
			throw makePlatformError();
		}

		RenderingContext2D& RenderingContext2D::quadraticCurveTo(const Point& cp, const Point& to) {
			return bezierCurveTo(cp, to, to);
		}

		RenderingContext2D& RenderingContext2D::rectangle(const graphics::Rectangle& bounds) {
			updatePenAndBrush();
			if(!win32::boole(::Rectangle(nativeObject_.get(),
					static_cast<int>(geometry::left(bounds)), static_cast<int>(geometry::top(bounds)),
					static_cast<int>(geometry::right(bounds)), static_cast<int>(geometry::bottom(bounds)))))
				throw makePlatformError();
			return moveTo(geometry::origin(bounds));
		}

		RenderingContext2D& RenderingContext2D::restore() {
			if(!savedStates_.empty()) {
				if(!win32::boole(::RestoreDC(nativeObject_.get(), savedStates_.top().cookie)))
					throw makePlatformError();
				currentState_ = savedStates_.top().state;
				savedStates_.pop();
				updatePenAndBrush();
				auto currentPen(win32::borrowed(static_cast<HPEN>(::GetCurrentObject(nativeObject_.get(), OBJ_PEN))));
				auto currentBrush(win32::borrowed(static_cast<HBRUSH>(::GetCurrentObject(nativeObject_.get(), OBJ_BRUSH))));
				if(currentPen.get() != currentState_.pen.get())
					::SelectObject(nativeObject_.get(), currentState_.pen.get());
				if(currentBrush.get() != currentState_.brush.get())
					::SelectObject(nativeObject_.get(), currentState_.brush.get());
			}
//			else
//				throw IllegalStateException("there is no state to back to.");
			return *this;
		}

		RenderingContext2D& RenderingContext2D::save() {
			const int cookie = ::SaveDC(nativeObject_.get());
			if(cookie == 0)
				throw makePlatformError();
			savedStates_.push(SavedState(currentState_, cookie));
			return *this;
		}
#if 0
		RenderingContext2D& RenderingContext2D::scrollPathIntoView() {
			// TODO: not implemented.
			return *this;
		}
#endif
		RenderingContext2D& RenderingContext2D::setFillStyle(std::shared_ptr<const Paint> fillStyle) {
			if(fillStyle.get() == nullptr)
				throw NullPointerException("fillStyle");
			auto newBrush(win32::makeHandle(::CreateBrushIndirect(&fillStyle->native()), &::DeleteObject));
			if(newBrush.get() != nullptr) {
				auto oldBrush(win32::borrowed(static_cast<HBRUSH>(::SelectObject(nativeObject_.get(), newBrush.get()))));
				if(oldBrush.get() != nullptr) {
					std::swap(currentState_.brush, newBrush);
					std::swap(currentState_.previousBrush, oldBrush);
					currentState_.fillStyle = std::make_pair(fillStyle, fillStyle->revisionNumber());
					return *this;
				}
			}
			throw makePlatformError();
		}

		RenderingContext2D& RenderingContext2D::setFont(std::shared_ptr<const font::Font> font) {
			if(font.get() == nullptr || font->native().get() == nullptr)
				throw NullPointerException("font");
			::SelectObject(nativeObject_.get(), font->native().get());
			currentState_.font = font;
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setGlobalAlpha(double) {
			return *this;	// not supported in Win32 GDI...
		}

		RenderingContext2D& RenderingContext2D::setGlobalCompositeOperation(CompositeOperation compositeOperation) {
			switch(compositeOperation) {
				case COPY:
					::SetROP2(nativeObject_.get(), R2_COPYPEN);
					break;
				case XOR:
					::SetROP2(nativeObject_.get(), R2_XORPEN);
				default:
					ASCENSION_ASSERT_NOT_REACHED();
			}
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setLineCap(LineCap lineCap) {
			return changePen(createModifiedPen(nullptr, boost::none, lineCap, boost::none));
		}

		RenderingContext2D& RenderingContext2D::setLineJoin(LineJoin lineJoin) {
			return changePen(createModifiedPen(nullptr, boost::none, boost::none, lineJoin));
		}

		RenderingContext2D& RenderingContext2D::setLineWidth(Scalar lineWidth) {
			return changePen(createModifiedPen(nullptr, lineWidth, boost::none, boost::none));
		}

		RenderingContext2D& RenderingContext2D::setMiterLimit(double miterLimit) {
			if(!win32::boole(::SetMiterLimit(nativeObject_.get(), static_cast<FLOAT>(miterLimit), nullptr)))
				throw makePlatformError();
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setShadowBlur(Scalar) {
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setShadowColor(const Color&) {
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setShadowOffset(const Dimension&) {
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setStrokeStyle(std::shared_ptr<const Paint> strokeStyle) {
			if(strokeStyle.get() == nullptr)
				throw NullPointerException("strokeStyle");
			changePen(createModifiedPen(&strokeStyle->native(), boost::none, boost::none, boost::none));
			currentState_.strokeStyle = std::make_pair(strokeStyle, strokeStyle->revisionNumber());
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setTextAlignment(TextAlignment textAlignment) {
			UINT v = ::GetTextAlign(nativeObject_.get());
			if(v == GDI_ERROR)
				throw makePlatformError();
			const bool rtl = (v & TA_RTLREADING) != 0;
			v &= ~(TA_LEFT | TA_CENTER | TA_RIGHT);
			switch(boost::native_value(textAlignment)) {
				case TextAlignment::START:
					v |= rtl ? TA_RIGHT : TA_LEFT;
					break;
				case TextAlignment::END:
					v |= rtl ? TA_LEFT : TA_RIGHT;
					break;
				case TextAlignment::LEFT:
					v |= TA_LEFT;
					break;
				case TextAlignment::RIGHT:
					v |= TA_RIGHT;
					break;
				case TextAlignment::CENTER:
					v |= TA_CENTER;
					break;
				default:
					throw UnknownValueException("textAlignment");
			}
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setTextBaseline(font::AlignmentBaseline textBaseline) {
			UINT v = ::GetTextAlign(nativeObject_.get());
			if(v == GDI_ERROR)
				throw makePlatformError();
			v &= ~(TA_BASELINE | TA_BOTTOM | TA_TOP);
			switch(boost::native_value(textBaseline)) {
				case font::AlignmentBaseline::OVER_EDGE:
				case font::AlignmentBaseline::TEXT_OVER_EDGE:
					v |= TA_TOP;
					break;
				case font::AlignmentBaseline::UNDER_EDGE:
				case font::AlignmentBaseline::TEXT_UNDER_EDGE:
					v |= TA_BOTTOM;
					break;
				default:
					v |= TA_BASELINE;
					break;
			}
			return *this;
		}

		RenderingContext2D& RenderingContext2D::setTransform(const AffineTransform& matrix) {
			const XFORM native(toNative<XFORM>(matrix));
			if(!win32::boole(::SetWorldTransform(nativeObject_.get(), &native)))
				throw makePlatformError();
			return *this;
		}

		Scalar RenderingContext2D::shadowBlur() const {
			return 0;
		}

		Color RenderingContext2D::shadowColor() const {
			return Color::TRANSPARENT_BLACK;
		}

		Dimension RenderingContext2D::shadowOffset() const {
			return Dimension(geometry::_dx = 0.0f, geometry::_dy = 0.0f);
		}

		RenderingContext2D& RenderingContext2D::stroke() {
			if(endPath()) {
				updatePenAndBrush();
				if(!win32::boole(::StrokePath(nativeObject_.get())))
					throw makePlatformError();
			}
			return *this;
		}

		RenderingContext2D& RenderingContext2D::strokeRectangle(const graphics::Rectangle& rectangle) {
			updatePenAndBrush();
			if(HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(nativeObject_.get(), ::GetStockObject(NULL_BRUSH)))) {
				const bool succeeded = win32::boole(::Rectangle(nativeObject_.get(),
					static_cast<int>(geometry::left(rectangle)), static_cast<int>(geometry::top(rectangle)),
					static_cast<int>(geometry::right(rectangle)), static_cast<int>(geometry::bottom(rectangle))));
				::SelectObject(nativeObject_.get(), oldBrush);
				if(succeeded)
					return *this;
			}
			throw makePlatformError();
		}

		std::shared_ptr<const Paint> RenderingContext2D::strokeStyle() const {
			return currentState_.strokeStyle.first;
		}

		RenderingContext2D& RenderingContext2D::strokeText(const StringPiece& text,
				const Point& origin, boost::optional<Scalar> maximumMeasure /* = boost::none */) {
			updatePenAndBrush();
			return paintText(*this, text, origin, maximumMeasure, true);
		}

		TextAlignment RenderingContext2D::textAlignment() const {
			const UINT v = ::GetTextAlign(nativeObject_.get());
			if(v == GDI_ERROR)
				throw makePlatformError();
			switch(v & (TA_LEFT | TA_CENTER | TA_RIGHT)) {
				case TA_CENTER:
					return TextAlignment::CENTER;
				case TA_RIGHT:
					return TextAlignment::RIGHT;
				case TA_LEFT:
				default:
					return TextAlignment::LEFT;
			}
		}

		font::AlignmentBaseline RenderingContext2D::textBaseline() const {
			const UINT v = ::GetTextAlign(nativeObject_.get());
			if(v == GDI_ERROR)
				throw makePlatformError();
			switch(v & (TA_BASELINE | TA_BOTTOM | TA_TOP)) {
				case TA_BOTTOM:
					return font::AlignmentBaseline::TEXT_UNDER_EDGE;
				case TA_TOP:
					return font::AlignmentBaseline::TEXT_OVER_EDGE;
				case TA_BASELINE:
				default:
					return font::AlignmentBaseline::ALPHABETIC;
			}
		}

		RenderingContext2D& RenderingContext2D::transform(const AffineTransform& matrix) {
			const XFORM native(toNative<XFORM>(matrix));
			if(!win32::boole(::ModifyWorldTransform(nativeObject_.get(), &native, MWT_RIGHTMULTIPLY)))
				throw makePlatformError();
			return *this;
		}

		void RenderingContext2D::updatePenAndBrush() {
			win32::Handle<HPEN> newPen;
			win32::Handle<HBRUSH> newBrush;
			if(currentState_.strokeStyle.second != currentState_.strokeStyle.first->revisionNumber())
				newPen = createModifiedPen(nullptr, boost::none, boost::none, boost::none);
			if(currentState_.fillStyle.second != currentState_.fillStyle.first->revisionNumber())
				newBrush.reset(::CreateBrushIndirect(&currentState_.fillStyle.first->native()), &::DeleteObject);

			win32::Handle<HPEN> oldPen;
			win32::Handle<HBRUSH> oldBrush;
			if(newPen.get() != nullptr) {
				oldPen.reset(static_cast<HPEN>(::SelectObject(nativeObject_.get(), newPen.get())), boost::null_deleter());
				if(oldPen.get() == nullptr)
					throw makePlatformError();
			}
			if(newBrush.get() != nullptr) {
				oldBrush.reset(static_cast<HBRUSH>(::SelectObject(nativeObject_.get(), newBrush.get())), boost::null_deleter());
				if(oldBrush.get() == nullptr) {
					if(oldPen.get() != nullptr)
						::SelectObject(nativeObject_.get(), oldPen.get());
					throw makePlatformError();
				}
			}

			if(oldPen.get() != nullptr) {
				currentState_.strokeStyle.second = currentState_.strokeStyle.first->revisionNumber();
				std::swap(currentState_.pen, newPen);
				std::swap(currentState_.previousPen, oldPen);
			}
			if(oldBrush.get() != nullptr) {
				currentState_.fillStyle.second = savedStates_.top().state.fillStyle.first->revisionNumber();
				std::swap(currentState_.brush, newBrush);
				std::swap(currentState_.previousBrush, oldBrush);
			}
		}
	}
}

#endif // ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
