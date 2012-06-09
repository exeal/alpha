/**
 * @file rendering-context-gdi.cpp
 * @author exeal
 * @date 2012-05-30 created
 */

#include <ascension/graphics/rendering-context.hpp>
#ifdef ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI

using namespace ascension;
using namespace ascension::graphics;
using namespace std;

/**
 * Constructor.
 * @param nativeObject The Win32 @c HDC value to move
 */
RenderingContext2D::RenderingContext2D(win32::Handle<HDC>&& nativeObject) : nativeObject_(move(nativeObject)), hasCurrentSubpath_(false) {
	::SetGraphicsMode(nativeObject_.get(), GM_ADVANCED);
	::SetTextAlign(nativeObject_.get(), TA_LEFT | TA_BASELINE | TA_UPDATECP);
}

/**
 * Constructor.
 * @param nativeObject
 */
RenderingContext2D::RenderingContext2D(const win32::Handle<HDC>& nativeObject) : nativeObject_(nativeObject_.get()), hasCurrentSubpath_(false) {
	::SetGraphicsMode(nativeObject_.get(), GM_ADVANCED);
	::SetTextAlign(nativeObject_.get(), TA_LEFT | TA_BASELINE | TA_UPDATECP);
}

namespace {
	inline FLOAT radianToDegree(double radian) /*noexcept*/ {
		return static_cast<FLOAT>(radian * 180 / 3.14159);
	}
}

RenderingContext2D& RenderingContext2D::arc(const NativePoint& p,
		Scalar radius, double startAngle, double endAngle, bool counterClockwise /* = false */) {
	if(radius < 0)
		throw invalid_argument("radius");
	if(!hasCurrentSubpath_) {
		::AngleArc(nativeObject_.get(), geometry::x(p), geometry::y(p), radius, radianToDegree(startAngle), 0.0);
		hasCurrentSubpath_ = true;
	}
	if(!win32::boole(::AngleArc(nativeObject_.get(), geometry::x(p), geometry::y(p), radius,
			radianToDegree(startAngle), counterClockwise ? radianToDegree(startAngle - endAngle) : radianToDegree(endAngle - startAngle))))
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::arcTo(const NativePoint& p1, const NativePoint& p2, Scalar radius) {
	if(radius < 0)
		throw invalid_argument("radius");
	ensureThereIsASubpathFor(p1);
	// TODO: not implemented.
	return *this;
}

/// Returns the internal Win32 @c HDC value.
const win32::Handle<HDC>& RenderingContext2D::asNativeObject() const /*noexcept*/ {
	return nativeObject_;
}

RenderingContext2D& RenderingContext2D::beginPath() {
	if(!win32::boole(::BeginPath(nativeObject_.get())))
		throw makePlatformError();
	hasCurrentSubpath_ = false;
	return *this;
}

RenderingContext2D& RenderingContext2D::bezierCurveTo(const NativePoint& cp1, const NativePoint& cp2, const NativePoint& to) {
	ensureThereIsASubpathFor(cp1);
	const POINT points[3] = {cp1, cp2, to};
	if(!win32::boole(::PolyBezierTo(nativeObject_.get(), points, 3)))
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::clearRectangle(const NativeRectangle& rectangle) {
	if(::FillRect(nativeObject_.get(), &rectangle, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH))) == 0)
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::clip() {
	if(!win32::boole(::SelectClipPath(nativeObject_.get(), RGN_AND)))
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::closePath() {
	if(endPath()) {
		if(!win32::boole(::CloseFigure(nativeObject_.get())))
			throw makePlatformError();
	}
	return *this;
}

inline bool RenderingContext2D::endPath() {
	if(!hasCurrentSubpath_)
		return false;
	if(!win32::boole(::EndPath(nativeObject_.get())))
		throw makePlatformError();
	hasCurrentSubpath_ = false;
	return true;
}

inline bool RenderingContext2D::ensureThereIsASubpathFor(const NativePoint& p) {
	const bool hadCurrentSubpath = hasCurrentSubpath_;
	if(!hasCurrentSubpath_)
		moveTo(p);
	assert(hasCurrentSubpath_);
	return !hadCurrentSubpath;
}

RenderingContext2D& RenderingContext2D::fill() {
	if(endPath()) {
		if(!win32::boole(::FillPath(nativeObject_.get())))
			throw makePlatformError();
	}
	return *this;
}

RenderingContext2D& RenderingContext2D::fillRectangle(const NativeRectangle& rectangle) {
	if(HBRUSH currentBrush = static_cast<HBRUSH>(::GetCurrentObject(nativeObject_.get(), OBJ_BRUSH))) {
		if(::FillRect(nativeObject_.get(), &rectangle, currentBrush) != 0)
			return *this;
	}
	throw makePlatformError();
}

bool RenderingContext2D::isPointInPath(const NativePoint& point) const {
	// this is tough fight...

	// backup current subpath
	const int numberOfPoints = ::GetPath(nativeObject_.get(), nullptr, nullptr, 0);
	if(numberOfPoints == 0)
		throw makePlatformError();
	const unique_ptr<POINT[]> points(new POINT[numberOfPoints]);
	const unique_ptr<BYTE[]> types(new BYTE[numberOfPoints]);
	if(::GetPath(nativeObject_.get(), points.get(), types.get(), numberOfPoints) == 0)
		throw makePlatformError();

	// test the point
	if(!win32::boole(::EndPath(nativeObject_.get())))
		throw makePlatformError();
	win32::Handle<HRGN> region(::PathToRegion(nativeObject_.get()), &::DeleteObject);
	if(region.get() == nullptr)
		throw makePlatformError();
	const bool result = win32::boole(::PtInRegion(region.get(), geometry::x(point), geometry::y(point)));

	// restore subpath
	::BeginPath(nativeObject_.get());
	::PolyDraw(nativeObject_.get(), points.get(), types.get(), numberOfPoints);

	return result;
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

RenderingContext2D& RenderingContext2D::lineTo(const NativePoint& to) {
	if(!ensureThereIsASubpathFor(to)) {
		if(!win32::boole(::LineTo(nativeObject_.get(), geometry::x(to), geometry::y(to))))
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
				return lp.lopnWidth.x;
			}
			case OBJ_EXTPEN: {
				EXTLOGPEN elp;
				if(::GetObjectW(currentPen, sizeof(EXTLOGPEN), &elp) == 0)
					break;
				return elp.elpWidth;
			}
			default:
				ASCENSION_ASSERT_NOT_REACHED();
		}
	}
	throw makePlatformError();
}

double RenderingContext2D::miterLimit() const {
	FLOAT temp;
	if(!win32::boole(::GetMiterLimit(nativeObject_.get(), &temp)))
		throw makePlatformError();
	return temp;
}

RenderingContext2D& RenderingContext2D::moveTo(const NativePoint& to) {
	if(!win32::boole(::MoveToEx(nativeObject_.get(), geometry::x(to), geometry::y(to), nullptr)))
		throw makePlatformError();
	hasCurrentSubpath_ = true;
	return *this;
}

RenderingContext2D& RenderingContext2D::quadraticCurveTo(const NativePoint& cp, const NativePoint& to) {
	return bezierCurveTo(cp, to, to);
}

RenderingContext2D& RenderingContext2D::restore() {
	if(!savedStates_.empty()) {
		if(!win32::boole(::RestoreDC(nativeObject_.get(), savedStates_.top())))
			throw makePlatformError();
		savedStates_.pop();
	}
//	else
//		throw IllegalStateException("there is no state to back to.");
	return *this;
}

RenderingContext2D& RenderingContext2D::rectangle(const NativeRectangle& bounds) {
	if(!win32::boole(::Rectangle(nativeObject_.get(),
			geometry::left(bounds), geometry::top(bounds), geometry::right(bounds), geometry::bottom(bounds))))
		throw makePlatformError();
	return moveTo(geometry::origin(bounds));
}

RenderingContext2D& RenderingContext2D::rotate(double angle) {
	return transform(geometry::rotationTransform<NativeAffineTransform>(angle));
}

RenderingContext2D& RenderingContext2D::save() {
	const int cookie = ::SaveDC(nativeObject_.get());
	if(cookie == 0)
		throw makePlatformError();
	savedStates_.push(cookie);
	return *this;
}

RenderingContext2D& RenderingContext2D::scale(
		geometry::Coordinate<NativeAffineTransform>::Type sx, geometry::Coordinate<NativeAffineTransform>::Type sy) {
	return transform(geometry::scalingTransform<NativeAffineTransform>(sx, sy));
}

namespace {
	void modifyLineStyle(const win32::Handle<HDC>& dc,
			boost::optional<Scalar> lineWidth, boost::optional<LineCap> lineCap, boost::optional<LineJoin> lineJoin) {
		if(HGDIOBJ oldPen = ::GetCurrentObject(dc.get(), OBJ_PEN)) {
			DWORD style = PS_GEOMETRIC | PS_SOLID, width;
			LOGBRUSH brush;
			switch(::GetObjectType(oldPen)) {
				case 0:
					throw makePlatformError();
				case OBJ_PEN: {
					LOGPEN lp;
					if(::GetObjectW(oldPen, sizeof(LOGPEN), &lp) == 0)
						throw makePlatformError();
					width = lp.lopnWidth.x;
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
					width = elp.elpWidth;
					brush.lbStyle = elp.elpBrushStyle;
					brush.lbColor = elp.elpColor;
					brush.lbHatch = elp.elpHatch;
					break;
				}
			}

			width = lineWidth.get_value_or(width);
			if(lineCap) {
				style &= ~PS_ENDCAP_MASK;
				switch(*lineCap) {
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
				switch(*lineJoin) {
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

			if(HPEN pen = ::ExtCreatePen(style, width, &brush, 0, nullptr)) {
				if((oldPen = ::SelectObject(dc.get(), pen)) == 0)
					::DeleteObject(pen);
				else {
					::DeleteObject(oldPen);
					return;
				}
			}
		}
		throw makePlatformError();
	}
}

RenderingContext2D& RenderingContext2D::setLineCap(LineCap lineCap) {
	modifyLineStyle(nativeObject_, boost::none, lineCap, boost::none);
	return *this;
}

RenderingContext2D& RenderingContext2D::setLineJoin(LineJoin lineJoin) {
	modifyLineStyle(nativeObject_, boost::none, boost::none, lineJoin);
	return *this;
}

RenderingContext2D& RenderingContext2D::setLineWidth(Scalar lineWidth) {
	modifyLineStyle(nativeObject_, lineWidth, boost::none, boost::none);
	return *this;
}

RenderingContext2D& RenderingContext2D::setMiterLimit(double miterLimit) {
	if(!win32::boole(::SetMiterLimit(nativeObject_.get(), static_cast<FLOAT>(miterLimit), nullptr)))
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::setTextAlignment(TextAlignment textAlignment) {
	UINT v = ::GetTextAlign(nativeObject_.get());
	if(v == GDI_ERROR)
		throw makePlatformError();
	const bool rtl = (v & TA_RTLREADING) != 0;
	v &= ~(TA_LEFT | TA_CENTER | TA_RIGHT);
	switch(textAlignment) {
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

RenderingContext2D& RenderingContext2D::setTextBaseline(presentation::AlignmentBaseline textBaseline) {
	UINT v = ::GetTextAlign(nativeObject_.get());
	if(v == GDI_ERROR)
		throw makePlatformError();
	v &= ~(TA_BASELINE | TA_BOTTOM | TA_TOP);
	switch(textBaseline) {
		case presentation::ALIGNMENT_BASELINE_BEFORE_EDGE:
		case presentation::ALIGNMENT_BASELINE_TEXT_BEFORE_EDGE:
			v |= TA_TOP;
			break;
		case presentation::ALIGNMENT_BASELINE_AFTER_EDGE:
		case presentation::ALIGNMENT_BASELINE_TEXT_AFTER_EDGE:
			v |= TA_BOTTOM;
			break;
		default:
			v |= TA_BASELINE;
			break;
	}
	return *this;
}

RenderingContext2D& RenderingContext2D::setTransform(const NativeAffineTransform& matrix) {
	if(!win32::boole(::SetWorldTransform(nativeObject_.get(), &matrix)))
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::stroke() {
	if(endPath()) {
		if(!win32::boole(::StrokePath(nativeObject_.get())))
			throw makePlatformError();
	}
	return *this;
}

RenderingContext2D& RenderingContext2D::strokeRectangle(const NativeRectangle& rectangle) {
	if(HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(nativeObject_.get(), ::GetStockObject(NULL_BRUSH)))) {
		const bool succeeded = win32::boole(::Rectangle(nativeObject_.get(),
			geometry::left(rectangle), geometry::top(rectangle), geometry::right(rectangle), geometry::bottom(rectangle)));
		::SelectObject(nativeObject_.get(), oldBrush);
		if(succeeded)
			return *this;
	}
	throw makePlatformError();
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

presentation::AlignmentBaseline RenderingContext2D::textBaseline() const {
	const UINT v = ::GetTextAlign(nativeObject_.get());
	if(v == GDI_ERROR)
		throw makePlatformError();
	switch(v & (TA_BASELINE | TA_BOTTOM | TA_TOP)) {
		case TA_BOTTOM:
			return presentation::ALIGNMENT_BASELINE_TEXT_AFTER_EDGE;
		case TA_TOP:
			return presentation::ALIGNMENT_BASELINE_TEXT_BEFORE_EDGE;
		case TA_BASELINE:
		default:
			return presentation::ALIGNMENT_BASELINE_ALPHABETIC;
	}
}

RenderingContext2D& RenderingContext2D::transform(const NativeAffineTransform& matrix) {
	if(!win32::boole(::ModifyWorldTransform(nativeObject_.get(), &matrix, MWT_RIGHTMULTIPLY)))
		throw makePlatformError();
	return *this;
}

RenderingContext2D& RenderingContext2D::translate(const NativeSize& delta) {
	return translate(static_cast<FLOAT>(geometry::dx(delta)), static_cast<FLOAT>(geometry::dy(delta)));
}

RenderingContext2D& RenderingContext2D::translate(
		geometry::Coordinate<NativeAffineTransform>::Type dx, geometry::Coordinate<NativeAffineTransform>::Type dy) {
	return transform(geometry::translationTransform<NativeAffineTransform>(dx, dy));
}

#endif // ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI
