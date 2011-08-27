/**
 * @file japanese.cpp
 * Implements Japanese encodings. This includes:
 * - Shift_JIS
 * - Shift_JIS-2004
 * - EUC-JP
 * - EUC=JIS-2004
 * - ISO-2022-JP
 * - ISO-2022-JP-1
 * - ISO-2022-JP-2
 * - ISO-2022-JP-2004
 * - ISO-2022-JP-2004-Strict
 * - ISO-2022-JP-2004-Compatible
 * - MacJapanese
 * @author exeal
 * @date 2004-2011
 *
 * Following documentation is written in Japanese (HeHe).
 *
 * <h3>Implemented character sets and encodings</h3>
 *
 * This file implements the following encodings:
 *
 * - JIS X 0208 -- Shift_JIS, ISO-2022-JP
 * - JIS X 0208 and JIS X 0212 -- EUC-JP, ISO-2022-JP-1 and ISO-2022-JP-2
 * - JIS X 0213 -- Shift_JIS-2004, EUC-JIS-2004, ISO-2022-JP-3-* and ISO-2022-JP-2004-*
 * - CP932 -- Windows-932, EUC (Windows-51932) and ISO-2022-JP (Windows-50220)
 *
 * The encodings based on CP932 are implemented in terms of Windows NLS and does not contain
 * characters of JIS X 0212 character set.
 *
 * 各エンコードの簡単な解説と Ascension での取り扱いについてコード中にコメントした
 *
 * ISO-2022-JP* から UCS への変換において、
 * 不正なエスケープシーケンスとその後続データは 1 バイトずつ等価なコード値の UCS に変換する。
 * これはユーザが誤った変換を発見し易くなる効果があるが、そのまま同じエンコードで非
 * UCS に変換しても元のデータには戻らないので注意
 *
 * <h3>JIS X 0208 と UCS の対応で複数の解釈がある文字</h3>
 *
 * KUBOTA 氏の調査 (http://www.debian.or.jp/~kubota/unicode-symbols-map2.html) によれば
 * JIS X 0208 の 12 個の文字は UCS との対応について複数の変換表で解釈に違いがある。
 * Ascension では JISX0213 InfoCenter の表を JIS X 0208 、JIS X 0213 のテーブル作成に用いており、
 * これらの表も上記調査の比較対象に含まれている。私はこれらの表の中で解釈に揺れのある12文字を
 * libiconv EUC-JP のものに変更し、自分のテーブルを作成した (Jis ディレクトリ)
 *
 * <h3>Three variants of ISO-2022-JP-2004</h3>
 *
 * Emacs は ISO-2022-JP との互換性のために ISO-2022-JP-3 とその変種を合わせて 3 つ実装している。
 * これは JIS X 0208 と JIS X 0213 で漢字の包摂基準が異なるためである。
 * 詳細や各エンコードについては以下のページを参照:
 *
 * - JIS X 0213の特徴と、Emacs上での実装
 *   (http://www.m17n.org/m17n2000_all_but_registration/proceedings/kawabata/jisx0213.html)
 * - Becky! JIS X 0213 プラグイン
 *   (http://members.at.infoseek.co.jp/jisx0213/bk0213.html)
 *
 * <h3>Limitations</h3>
 *
 * JIS X 0213 には合成可能な発音記号が含まれており、UCS からの変換で
 * JIS 側に無い合成済み文字については基礎文字と発音記号に分解することで理論上は表現できる。
 * しかし Ascension ではこの分解を行わなず、現時点では変換は不可能である。
 * JIS X 0213 に現れる合成済み仮名については対応している
 *
 * <h3>声調記号の合字</h3>
 *
 * JIS X 0213 の 2 つの声調記号、上昇調 (1-11-69) と下降調 (1-11-70) に直接対応する UCS 文字は無く、
 * 2つのコードポイントの合字が対応すると考えられる。すなわち JIS から UCS への変換において、上昇調は
 * U+02E9+02E5、下降調は U+02E5+02E9 となる。しかしこのような単純な変換を行うと JIS と UCS
 * のコード交換性が失われてしまうようだ (http://wakaba-web.hp.infoseek.co.jp/table/jis-note.ja.html)。
 * Ascension では ZWNJ を使って合字と意図したコードポイントの組み合わせとそうでないものを区別する。
 * つまり、JIS 側で超高 (1-11-64) と超低 (1-11-68) が並んでいる場合は、それぞれの文字を UCS
 * に変換して間に ZWNJ を挟む。逆に UCS 側で U+02E5 と U+02E9 が並んでいる場合は JIS
 * の対応する 1 つの声調記号に変換し、間に ZWNJ がある場合は2つの声調記号に変換する
 */

#ifndef ASCENSION_NO_STANDARD_ENCODINGS

#include <ascension/corelib/encoder.hpp>
#include <ascension/corelib/text/utf-16.hpp>
#include <algorithm>	// std.binary_search
#include <cassert>
#include <cstring>		// std.memcpy
#include <memory>		// std.auto_ptr
#include <map>

using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::encoding::implementation;
using namespace ascension::encoding::implementation::dbcs;
using namespace ascension::text;
using namespace std;

// registry
namespace {
	struct EncodingState {
		enum G0 {
			ASCII, JIS_X_0201_ROMAN, /*JIS_X_0201_KANA,*/ JIS_X_0208,
			JIS_X_0212, JIS_X_0213_PLANE_1, JIS_X_0213_PLANE_2, GB2312, KS_C_5601
		} g0;
		enum G2 {
			UNDESIGNATED = KS_C_5601 + 1, ISO_8859_1, ISO_8859_7
		} g2;
		bool invokedG2;	// true if invoked by SS2
		EncodingState() /*throw()*/ {reset();}
		void reset() /*throw()*/ {g0 = ASCII; g2 = UNDESIGNATED; invokedG2 = false;}
	};
	template<typename Factory>
	class InternalEncoder : public Encoder {
	public:
		explicit InternalEncoder(const Factory& factory) /*throw()*/ : props_(factory) {}
	private:
		Result doFromUnicode(Byte* to, Byte* toEnd, Byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext);
		Result doToUnicode(Char* to, Char* toEnd, Char*& toNext,
			const Byte* from, const Byte* fromEnd, const Byte*& fromNext);
		const EncodingProperties& properties() const /*throw()*/ {return props_;}
		Encoder& resetDecodingState() /*throw()*/ {decodingState_.reset(); return *this;}
		Encoder& resetEncodingState() /*throw()*/ {encodingState_.reset(); return *this;}
	private:
		const EncodingProperties& props_;
		EncodingState encodingState_, decodingState_;
	};

	class SHIFT_JIS : public EncoderFactoryBase {
	public:
		SHIFT_JIS() /*throw()*/ : EncoderFactoryBase("Shift_JIS", standard::SHIFT_JIS, "Japanese (Shift_JIS)", 2, 1, "MS_Kanji|csShiftJIS", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<SHIFT_JIS>(*this));}
	} shiftjis;

	class SHIFT_JIS_2004 : public EncoderFactoryBase {
	public:
		SHIFT_JIS_2004() /*throw()*/ : EncoderFactoryBase("Shift_JIS-2004", MIB_OTHER, "Japanese (Shift_JIS-2004)", 2, 1, "", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<SHIFT_JIS_2004>(*this));}
	} shiftjis2004;

	class EUC_JP : public EncoderFactoryBase {
	public:
		EUC_JP() /*throw()*/ : EncoderFactoryBase("EUC-JP", standard::EUC_JP, "Japanese (EUC-JP)", 3, 1, "Extended_UNIX_Code_Packed_Format_for_Japanese|csEUCPkdFmtJapanese", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<EUC_JP>(*this));}
	} eucjp;

	class EUC_JIS_2004 : public EncoderFactoryBase {
	public:
		EUC_JIS_2004() /*throw()*/ : EncoderFactoryBase("EUC-JIS-2004", MIB_OTHER, "Japanese (EUC-JIS-2004)", 3, 1, "", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<EUC_JIS_2004>(*this));}
	} eucjis2004;

	class ISO_2022_JP : public EncoderFactoryBase {
	public:
		ISO_2022_JP() /*throw()*/ : EncoderFactoryBase("ISO-2022-JP", standard::ISO_2022_JP, "Japanese (ISO-2022-JP)", 8, 1, "csISO2022JP", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_JP>(*this));}
	} iso2022jp;

	class ISO_2022_JP_2 : public EncoderFactoryBase {
	public:
		ISO_2022_JP_2() /*throw()*/ : EncoderFactoryBase("ISO-2022-JP-2", standard::ISO_2022_JP_2, "Japanese (ISO-2022-JP-2)", 9, 1, "csISO2022JP2", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_JP_2>(*this));}
	} iso2022jp2;

	class ISO_2022_JP_2004 : public EncoderFactoryBase {
	public:
		ISO_2022_JP_2004() /*throw()*/ : EncoderFactoryBase("ISO-2022-JP-2004", MIB_OTHER, "Japanese (ISO-2022-JP-2004)", 9, 1, "", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_JP_2004>(*this));}
	} iso2022jp2004;

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
	class ISO_2022_JP_1 : public EncoderFactoryBase {
	public:
		ISO_2022_JP_1() /*throw()*/ : EncoderFactoryBase("ISO-2022-JP-1", MIB_OTHER, "Japanese (ISO-2022-JP-1)", 9, 1, "", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_JP_1>(*this));}
	} iso2022jp1;

	class ISO_2022_JP_2004_STRICT : public EncoderFactoryBase {
	public:
		ISO_2022_JP_2004_STRICT() /*throw()*/ : EncoderFactoryBase("ISO-2022-JP-2004-Strict", MIB_OTHER, "Japanese (ISO-2022-JP-2004-Strict)", 9, 1, "", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_JP_2004_STRICT>(*this));}
	} iso2022jp2004strict;

	class ISO_2022_JP_2004_COMPATIBLE : public EncoderFactoryBase {
	public:
		ISO_2022_JP_2004_COMPATIBLE() /*throw()*/ : EncoderFactoryBase("ISO-2022-JP-2004-Compatible", MIB_OTHER, "Japanese (ISO-2022-JP-2004-Compatible)", 9, 1, "", 0x3f) {}
	private:
		auto_ptr<Encoder> create() const /*throw()*/ {return auto_ptr<Encoder>(new InternalEncoder<ISO_2022_JP_2004_COMPATIBLE>(*this));}
	} iso2022jp2004compatible;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
	class JISAutoDetector : public EncodingDetector {
	public:
		JISAutoDetector() : EncodingDetector("JISAutoDetect") {}
	private:
		pair<MIBenum, string> doDetect(const Byte* first, const Byte* last, ptrdiff_t* convertibleBytes) const /*throw()*/;
	};

	struct Installer {
		Installer() {
			Encoder::registerFactory(shiftjis);
			Encoder::registerFactory(shiftjis2004);
			Encoder::registerFactory(eucjp);
			Encoder::registerFactory(eucjis2004);
			Encoder::registerFactory(iso2022jp);
			Encoder::registerFactory(iso2022jp2);
			Encoder::registerFactory(iso2022jp2004);
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			Encoder::registerFactory(iso2022jp1);
			Encoder::registerFactory(iso2022jp2004strict);
			Encoder::registerFactory(iso2022jp2004compatible);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
			EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new JISAutoDetector));
		}
	} installer;
} // namespace @0

// mapping between JIS and UCS
namespace {
	template<
		typename Line0, typename Line1, typename Line2, typename Line3,
		typename Line4, typename Line5, typename Line6, typename Line7,
		typename Line8, typename Line9, typename LineA, typename LineB,
		typename LineC, typename LineD, typename LineE, typename LineF>
	class CodePointWire : public CodeWire<CodePoint, Line0, Line1, Line2, Line3,
		Line4, Line5, Line6, Line7, Line8, Line9, LineA, LineB, LineC, LineD, LineE, LineF> {};
	template<
		CodePoint c0, CodePoint c1, CodePoint c2, CodePoint c3, CodePoint c4, CodePoint c5, CodePoint c6, CodePoint c7,
		CodePoint c8, CodePoint c9, CodePoint cA, CodePoint cB, CodePoint cC, CodePoint cD, CodePoint cE, CodePoint cF>
	struct CodePointLine : public CodeLine<CodePoint, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF> {};
	struct EmptyCodePointLine : public CodePointLine<0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0> {};
#include "generated/jis.ipp"

	// JIS X 0201 Roman
	inline Char convertROMANtoUCS(Byte c) /*throw()*/ {
		if(c == 0x5c)					return 0x00a5u;					// Yen Sign
		else if(c == 0x7e)				return 0x203eu;					// Overline
		else if(c >= 0x20 && c <= 0x7d)	return c;						// 7-bit
		else							return REPLACEMENT_CHARACTER;	// unmappable
	}
	inline Byte convertUCStoROMAN(Char c) /*throw()*/ {
		if(c >= 0x0020u && c <= 0x005bu)		return mask8Bit(c);	// 7-bit
		else if(c >= 0x005du && c <= 0x007du)	return mask8Bit(c);	// 7-bit
		else if(c == 0x00a5u)					return 0x5c;		// Yen Sign
		else if(c == 0x203eu)					return 0x7e;		// Overline
		else									return 0x00;		// unmappable
	}

	// JIS X 0201 Kana
	inline Char convertKANAtoUCS(Byte c) /*throw()*/ {
		return (c >= 0xa1 && c <= 0xdf) ? c + 0xfec0u : REPLACEMENT_CHARACTER;
	}
	inline Byte convertUCStoKANA(Char c) /*throw()*/ {
		return (c >= 0xff61u && c <= 0xff9fu) ? mask8Bit(c - 0xfec0u) : 0x00;
	}

	// JIS X 0208:1997
	inline Char convertX0208toUCS(uint16_t c) /*throw()*/ {
		if(const Char** const wire = JIS_X_0208_TO_UCS[mask8Bit(c >> 8)])
			return wireAt(wire, mask8Bit(c));
		else
			return REPLACEMENT_CHARACTER;
	}
	inline uint16_t convertUCStoX0208(Char c) /*throw()*/ {
		if(const uint16_t** const wire = UCS_TO_JIS_X_0208[mask8Bit(c >> 8)])
			return wireAt(wire, mask8Bit(c));
		else
			return 0x0000;
	}

	// JIS X 0212:1990
	inline Char convertX0212toUCS(uint16_t c) /*throw()*/ {
		if(const Char** const wire = JIS_X_0212_TO_UCS[mask8Bit(c >> 8)])
			return wireAt(wire, mask8Bit(c));
		else
			return REPLACEMENT_CHARACTER;
	}
	inline uint16_t convertUCStoX0212(Char c) /*throw()*/ {
		if(const uint16_t** const wire = UCS_TO_JIS_X_0212[mask8Bit(c >> 8)])
			return wireAt(wire, mask8Bit(c));
		else
			return 0x0000;
	}
	// JIS X 0213:2004 plane 1 to UCS
	inline CodePoint convertX0213Plane1toUCS(uint16_t c) /*throw()*/ {
		const CodePoint ucs = convertX0208toUCS(c);
		if(ucs == REPLACEMENT_CHARACTER) {
			if(const CodePoint** const wire = JIS_X_0213_PLANE_1_TO_UCS[mask8Bit(c >> 8)])
				return wireAt(wire, mask8Bit(c));
		}
		return convertX0208toUCS(c);
	}
	// JIS X 0213:2000 plane 2 to UCS
	inline CodePoint convertX0213Plane2toUCS(uint16_t c) /*throw()*/ {
		if(const CodePoint** const wire = JIS_X_0213_PLANE_2_TO_UCS[mask8Bit(c >> 8)])
			return wireAt(wire, mask8Bit(c));
		else
			return REPLACEMENT_CHARACTER;
	}
	// UCS to JIS X 0213:2004
	Encoder::Result convertUCStoX0213(const Char* first, const Char* last,
			const Char*& next, bool eob, uint16_t& jis, bool& plane2) /*throw()*/ {
		jis = 0;
		if(binary_search(LEADING_BYTES_TO_JIS_X_0213, ASCENSION_ENDOF(LEADING_BYTES_TO_JIS_X_0213), first[0])) {
			if(first + 1 == last) {
				if(!eob) {
					// pending
					next = first;
					return Encoder::COMPLETED;
				}
			} else {
				if(first[1] == 0x309a) {	// <(kana), Combining Katakana-Hiragana Semi-Voiced Sound Mark>
					switch(first[0]) {
					case 0x304b: jis = 0x2477; break;	// ka -> bidakuon nga
					case 0x304d: jis = 0x2478; break;	// ki -> bidakuon ngi
					case 0x304f: jis = 0x2479; break;	// ku -> bidakuon ngu
					case 0x3051: jis = 0x247a; break;	// ke -> bidakuon nge
					case 0x3053: jis = 0x247b; break;	// ko -> bidakuon ngo
					case 0x30ab: jis = 0x2577; break;	// ka -> bidakuon nga
					case 0x30ad: jis = 0x2578; break;	// ki -> bidakuon ngi
					case 0x30af: jis = 0x2579; break;	// ku -> bidakuon ngu
					case 0x30b1: jis = 0x257a; break;	// ke -> bidakuon nge
					case 0x30b3: jis = 0x257b; break;	// ko -> bidakuon ngo
					case 0x30bb: jis = 0x257c; break;	// se -> ainu ce
					case 0x30c4: jis = 0x257d; break;	// tu -> ainu tu (tu)
					case 0x30c8: jis = 0x257e; break;	// to -> ainu to (tu)
					case 0x31f7: jis = 0x2678; break;	// small fu -> ainu p
					}
				} else if(first[1] == 0x0300) {	// <X, Combining Grave Accent>
					switch(first[0]) {
					case 0x00e6: jis = 0x2b44; break;	// ae
					case 0x0254: jis = 0x2b48; break;	// open o
					case 0x0259: jis = 0x2b4c; break;	// schwa
					case 0x025a: jis = 0x2b4e; break;	// schwa with hook
					case 0x028c: jis = 0x2b4a; break;	// turned v
					}
				} else if(first[1] == 0x0301) {	// <X, Combining Acute Accent>
					switch(first[0]) {
					case 0x0254: jis = 0x2b49; break;	// open o
					case 0x0259: jis = 0x2b4d; break;	// schwa
					case 0x025a: jis = 0x2b4f; break;	// schwa with hook
					case 0x028c: jis = 0x2b4b; break;	// turned v
					}
				} else if(first[0] == 0x02e9) {
					if(first[1] == 0x02e5)
						jis = 0x2b65;	// <Extra-Low Tone Bar, Extra-High Tone Bar> -> rising symbol
					else if(first[1] == ZERO_WIDTH_NON_JOINER && first + 2 < last && first[2] == 0x02e5)
						jis = 0x2b64;	// just dependent Extra-Low Tone Bar
				} else if(first[0] == 0x02e5) {
					if(first[1] == 0x02e9)
						jis = 0x2b66;	// <Extra-High Tone Bar, Extra-Low Tone Bar> -> falling symbol
					else if(first[1] == ZERO_WIDTH_NON_JOINER && first + 2 < last && first[2] == 0x02e9)
						jis = 0x2b60;	// just dependent Extra-High Tone Bar
				}
				if(jis != 0) {
					next = first + 2;
					plane2 = false;
					return Encoder::COMPLETED;
				}
			}
		}

		// one-to-one mapping
		if(surrogates::isHighSurrogate(first[0])) {
			if(first + 1 == last) {
				next = first;
				return eob ? Encoder::MALFORMED_INPUT : Encoder::COMPLETED;
			}
			const CodePoint c = utf16::decodeFirst(first, last);
			if(c < 0x10000ul) {
				next = first;
				return Encoder::MALFORMED_INPUT;
			} else if(c >= 0x20000ul && c < 0x30000ul) {
				const uint16_t** wire;
				if(0 != (wire = UCS_SIP_TO_JIS_X_0213_PLANE_1[mask8Bit((c - 0x20000ul) >> 8)])) {
					if(0 != (jis = wireAt(wire, mask8Bit(c - 0x20000ul))))
						plane2 = false;
				}
				if(jis == 0 && 0 != (wire = UCS_SIP_TO_JIS_X_0213_PLANE_2[mask8Bit((c - 0x20000ul) >> 8)])) {
					if(0 != (jis = wireAt(wire, mask8Bit(c - 0x20000ul))))
						plane2 = true;
				}
				if(jis != 0) {
					next = first + 2;
					return Encoder::COMPLETED;
				}
			}
			if(jis == 0) {
				next = first;
				return Encoder::UNMAPPABLE_CHARACTER;
			}
		} else {
			const uint16_t** wire;
			if(0 != (wire = UCS_BMP_TO_JIS_X_0213_PLANE_1[mask8Bit(first[0] >> 8)])) {
				if(0 != (jis = wireAt(wire, mask8Bit(first[0]))))
					plane2 = false;
			}
			if(jis == 0 && 0 != (wire = UCS_BMP_TO_JIS_X_0213_PLANE_2[mask8Bit(first[0] >> 8)])) {
				if(0 != (jis = wireAt(wire, mask8Bit(first[0]))))
					plane2 = true;
			}
			if(jis == 0 && 0 != (wire = UCS_TO_JIS_X_0208[mask8Bit(first[0] >> 8)])) {
				if(0 != (jis = wireAt(wire, mask8Bit(first[0]))))
					plane2 = false;
			}
			if(jis == 0) {
				next = first;
				return Encoder::UNMAPPABLE_CHARACTER;
			}
		}
		next = first + 1;
		return Encoder::COMPLETED;
	}
} // namespace @0

namespace {
	// makes JIS code from ku and ten numbers.
	inline uint16_t jk(Byte ku, Byte ten) /*throw()*/ {return ((ku << 8) | ten) + 0x2020;}

	// "禁止漢字" of ISO-2022-JP-3 (from JIS X 0213:2000 附属書 2 表 1)
	const uint16_t PROHIBITED_IDEOGRAPHS_2000[] = {
		jk( 3,26), jk( 3,27), jk( 3,28), jk( 3,29), jk( 3,30), jk( 3,31),
		jk( 3,32),
		jk( 3,59), jk( 3,60), jk( 3,61), jk( 3,62), jk( 3,63), jk( 3,64),
		jk( 3,91), jk( 3,92), jk( 3,93), jk( 3,94),
		jk( 4,84), jk( 4,85), jk( 4,86), jk( 8,87), jk( 4,88), jk( 4,89),
		jk( 4,90), jk( 4,91),
		jk( 5,87), jk( 5,88), jk( 5,89), jk( 5,90), jk( 5,91), jk( 5,92),
		jk( 5,93), jk( 5,94),
		jk( 6,25), jk( 6,26), jk( 6,27), jk( 6,28), jk( 6,29), jk( 6,30),
		jk( 6,31), jk( 6,32),
													jk(13,83), jk(13,88),
		jk(13,89), jk(13,93), jk(13,94),
															   jk(16, 2),
		jk(16,19), jk(16,79), jk(17,58), jk(17,75), jk(17,79), jk(18, 3),
		jk(18, 9), jk(18,10), jk(18,11), jk(18,25), jk(18,50), jk(18,89),
		jk(19, 4), jk(19,20), jk(19,21), jk(19,34), jk(19,41), jk(19,69),
		jk(19,73), jk(19,76), jk(19,86), jk(19,90), jk(20,18), jk(20,33),
		jk(20,35), jk(20,50), jk(20,79), jk(20,91), jk(21, 7), jk(21,85),
		jk(22, 2), jk(22,31), jk(22,33), jk(22,38), jk(22,48), jk(22,64),
		jk(22,77), jk(23,16), jk(23,39), jk(23,59), jk(23,66), jk(24, 6),
		jk(24,20), jk(25,60), jk(25,77), jk(25,82), jk(25,85), jk(27, 6),
		jk(27,67), jk(27,75), jk(28,40), jk(28,41), jk(28,49), jk(28,50),
		jk(28,52), jk(29,11), jk(29,13), jk(29,43), jk(29,75), jk(29,77),
		jk(29,79), jk(29,80), jk(29,84), jk(30,36), jk(30,45), jk(30,53),
		jk(30,63), jk(30,85), jk(31,32), jk(31,57), jk(32, 5), jk(32,65),
		jk(32,70), jk(33, 8), jk(33,36), jk(33,46), jk(33,56), jk(33,63),
		jk(33,67), jk(33,93), jk(33,94), jk(34, 3), jk(34, 8), jk(34,45),
		jk(34,86), jk(35,18), jk(35,29), jk(35,86), jk(35,88), jk(36, 7),
		jk(36, 8), jk(36,45), jk(36,47), jk(36,59), jk(36,87), jk(37,22),
		jk(37,31), jk(37,52), jk(37,55), jk(37,78), jk(37,83), jk(37,88),
		jk(38,33), jk(38,34), jk(38,45), jk(38,81), jk(38,86), jk(39,25),
		jk(39,63), jk(39,72), jk(40,14), jk(40,16), jk(40,43), jk(40,53),
		jk(40,60), jk(40,74), jk(41,16), jk(41,48), jk(41,49), jk(41,50),
		jk(41,51), jk(41,78), jk(42, 1), jk(42,27), jk(42,29), jk(42,57),
		jk(42,66), jk(43,43), jk(43,47), jk(43,72), jk(43,74), jk(43,89),
		jk(44,40), jk(44,45), jk(44,65), jk(44,89), jk(45,20), jk(45,58),
		jk(45,73), jk(45,74), jk(45,83), jk(46,20), jk(46,26), jk(46,48),
		jk(46,62), jk(46,64), jk(46,81), jk(46,82), jk(46,93), jk(47, 3),
		jk(47,13), jk(47,15), jk(47,22), jk(47,25), jk(47,26), jk(47,31),
							  jk(48,54), jk(52,68), jk(57,88), jk(58,25),
		jk(59,56), jk(59,77), jk(62,25), jk(62,85), jk(63,70), jk(64,86),
		jk(66,72), jk(66,74), jk(67,62), jk(68,38), jk(73, 2), jk(73,14),
		jk(73,58), jk(74, 4), jk(75,61), jk(76,45), jk(77,78), jk(80,55),
		jk(80,84), jk(82,45), jk(82,84), jk(84, 1), jk(84, 2), jk(84, 3),
		jk(84, 4), jk(84, 5), jk(84, 6)
	};
	// "禁止漢字" of ISO-2022-JP-2004 (from JIS X0213:2004 附属書 2 表 2)
	const uint16_t PROHIBITED_IDEOGRAPHS_2004[] = {
		jk(14, 1), jk(15,94), jk(17,19), jk(22,70), jk(23,50), jk(28,24),
		jk(33,73), jk(38,61), jk(39,77), jk(47,52), jk(47,94), jk(53,11),
		jk(54, 2), jk(54,58), jk(84, 7), jk(94,90), jk(94,91), jk(94,92),
		jk(94,93), jk(94,94)
	};
	// returns true if is "禁止漢字" of ISO-2022-JP-3.
	inline bool isISO2022JP3ProhibitedIdeograph(uint16_t jis) {
		return (jis >= jk(6, 57) && jis <= jk(6, 94))
			|| (jis >= jk(7, 34) && jis <= jk(7, 48))
			|| (jis >= jk(7, 82) && jis <= jk(8, 62))
			|| (jis >= jk(8, 71) && jis <= jk(8, 92))
			|| (jis >= jk(9, 1) && jis <= jk(12, 83))
			|| (jis >= jk(12, 93) && jis <= jk(13, 55))
			|| (jis >= jk(13, 63) && jis <= jk(13, 79))
			|| (jis >= jk(14, 2) && jis <= jk(15, 93))
			|| (jis >= jk(47, 53) && jis <= jk(47, 93))
			|| (jis >= jk(84, 8) && jis <= jk(94, 89))
			|| binary_search(PROHIBITED_IDEOGRAPHS_2000, ASCENSION_ENDOF(PROHIBITED_IDEOGRAPHS_2000), jis);
	}
	// returns true if is "禁止漢字" added by JIS X 0213:2004.
	inline bool isISO2022JP2004ProhibitedIdeograph(uint16_t jis) {
		return binary_search(PROHIBITED_IDEOGRAPHS_2004, ASCENSION_ENDOF(PROHIBITED_IDEOGRAPHS_2004), jis);
	}

	// converts from ISO-2022-JP-X into UTF-16.
	Encoder::Result convertISO2022JPXtoUTF16(char x, Char* to, Char* toEnd, Char*& toNext,
			const Byte* from, const Byte* fromEnd, const Byte*& fromNext,
			EncodingState& state, bool eob, Encoder::SubstitutionPolicy substitutionPolicy) {
		// Acceptable character sets and designate sequence are following. G0, unless described:
		//
		// ISO-2022-JP
		//	ASCII					ESC ( B
		//	JIS X 0201:1976-Roman	ESC ( J
		//	JIS X 0208:1978			ESC $ @
		//	JIS X 0208:1983			ESC $ B
		//
		// ISO-2022-JP-1 (in addition to ISO-2022-JP)
		//	JIS X 0212:1990			ESC $ ( D
		//
		// ISO-2022-JP-2 (in addition to ISO-2022-JP-1)
		//	GB2312:1980				ESC $ A
		//	KSC5601:1987			ESC $ ( C
		//	ISO-8859-1				ESC . A		96 文字集合につき G2
		//	ISO-8859-7				ESC . F		96 文字集合につき G2
		//
		// ISO-2022-JP-3
		//	ASCII					ESC ( B
		//	JIS X 0213:2000 plane 1	ESC $ ( O
		//							ESC $ B		禁止漢字がある
		//	JIS X 0213:2000 plane 2	ESC $ ( P
		//
		// ISO-2022-JP-2004
		//	ASCII					ESC ( B
		//	JIS X 0213:2004 plane 1	ESC $ ( Q
		//							ESC $ B		禁止漢字がある
		//	JIS X 0213:2004 plane 2	ESC $ ( P
		//	JIS X 0213:2000 plane 1	ESC $ ( O	禁止漢字がある
		//
		// JIS X 0213 1 面の指示では JIS X 0208 を指示するために使用していた ESC $ B を代用することが認められているが、
		// 包摂基準の変更により、この互換シーケンスで JIS X 0213 1 面を指示した場合、JIS X 0208
		// に含まれる文字の中で使用が禁止されている文字がある
		//
		// JIS X 0213:2004 (追補 1) ではこの禁止漢字が幾つか追加された。これらの文字は ISO-2022-JP-2004 において
		// ESC $ ( O で JIS X 0213 1 面を指示した場合にも使用できない
		//
		// Ascension の制限、解釈は次の通り:
		//	- JIS X 0201-Kana を使える実装もあるが、Ascension ではこの文字集合を使わない。
		//	  ベンダ拡張文字も変換表から除外されており、使わない
		//	- JIS X 0208 は年代を区別せず、全て JIS X 0208:1997 を使う
		//	- 現時点では ISO-2022-JP-2 の中国語と韓国語文字集合の変換には Windows の変換表を使う
		//	- JIS X 0213 1面から UCS への変換において、指示シーケンスは年度を区別しない
		//	- ISO-2022-JP-*-Strict 、ISO-2022-JP-*-Compatible では JIS X 0208 で表現可能な文字は
		//	  "ESC $ B" で指示する
		//	- ISO-2022-JP-*-Compatible では禁止漢字も "ESC $ B" で指示する
		//	- ISO-2022-JP-*-Strict から UCS への変換では禁止漢字を考慮しない
		//	- ISO-2022-JP-3 系のエンコーディングでは JIS X 0213 1面の指示に ESC $ ( O を使う。
		//	  ISO-2022-JP-2004 系エンコーディングとの差異はそれだけで、ISO-2022-JP-3
		//	  系エンコーディングでも追補 1 で追加された文字を使うことができる (UCS への変換については区別が無い)
		//  - ISO-2022-JP-2004-Compatible の互換性は ISO-2022-JP に対するものであり、ISO-2022-JP-3 に対するものではない

		const bool jis2004 = x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			|| x == 's' || x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
			;
		const Char* const beginning = to;
		auto_ptr<Encoder> gb2312Encoder, ksc5601Encoder, iso88591Encoder, iso88597Encoder;
		bool checkedGB2312 = false, checkedKSC5601 = false;

#define ASCENSION_HANDLE_UNMAPPABLE()	{									\
	if(substitutionPolicy == Encoder::IGNORE_UNMAPPABLE_CHARACTERS)			\
		--to;																\
	else if(substitutionPolicy != Encoder::REPLACE_UNMAPPABLE_CHARACTERS) {	\
		toNext = to;														\
		fromNext = from;													\
		return Encoder::UNMAPPABLE_CHARACTER;								\
	}																		\
}

		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from == ESC) {	// expect esc. seq.
				if(from + 2 < fromEnd) {
					switch(from[1]) {
					case 'N': state.invokedG2 = true; ++from; continue;	// SS2
					case '(':
						switch(from[2]) {
						case 'B': state.g0 = EncodingState::ASCII; from += 2; continue;				// "(B" => ASCII
//						case 'I': state.g0 = EncodingState::JIS_X_0201_KANA; from += 2; continue;	// "(I" => JIS X 0201 Kana
						case 'J': state.g0 = EncodingState::JIS_X_0201_ROMAN; from += 2; continue;	// "(J" => JIS X 0201 Roman
						}
						break;
					case '$':
						switch(from[2]) {
						case '@': state.g0 = EncodingState::JIS_X_0208; from += 2; continue;	// "$@" => JIS X 0208
						case 'A':	// "$A" => GB2312
							if(x != '2') break;
							if(!checkedGB2312) {
								if(0 != (gb2312Encoder = Encoder::forMIB(standard::GB2312)).get())
									gb2312Encoder->setSubstitutionPolicy(substitutionPolicy);
							}
							if(gb2312Encoder.get() == 0) break;
							state.g0 = EncodingState::GB2312; from += 2; continue;
						case 'B': state.g0 = EncodingState::JIS_X_0208; from += 2; continue;	// "$B" => JIS X 0208
						case '(':
							if(from + 3 < fromEnd) {
								switch(from[3]) {
								case 'C':	// "$(C" => KSC5601
									if(x != '2') break;
									if(!checkedKSC5601) {
										if(0 != (ksc5601Encoder = Encoder::forMIB(36)).get())
											ksc5601Encoder->setSubstitutionPolicy(substitutionPolicy);
									}
									if(ksc5601Encoder.get() == 0) break;
									state.g0 = EncodingState::KS_C_5601; from += 3; continue;
								case 'D':	// "$(D" => JIS X 0212
									if(x != '2'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
										&& x != '1'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
										) break;
									state.g0 = EncodingState::JIS_X_0212; from += 3; continue;
								case 'O':	// "$(O" => JIS X 0213 plane 1
									if(!jis2004) break;
									state.g0 = EncodingState::JIS_X_0213_PLANE_1; from += 3; continue;
								case 'P':	// "$(P" => JIS X 0213 plane 2
									if(!jis2004) break;
									state.g0 = EncodingState::JIS_X_0213_PLANE_2; from += 3; continue;
								case 'Q':	// "$(Q" => JIS X 0213 plane 1
									if(!jis2004) break;
									state.g0 = EncodingState::JIS_X_0213_PLANE_1; from += 3; continue;
								}
							}
						}
						break;
					case '.':
						if(x == '2') {
							switch(from[1]) {
							case 'A': state.g2 = EncodingState::ISO_8859_1; from += 2; continue;	// ".A" => ISO-8859-1 (G2)
							case 'F': state.g2 = EncodingState::ISO_8859_7; from += 2; continue;	// ".F" => ISO-8859-7 (G2)
							}
						}
						break;
					}
				}

				// illegal or unsupported esc. seq.
				toNext = to;
				fromNext = from;
				return Encoder::MALFORMED_INPUT;
			}

			if(*from <= 0x20 || (*from >= 0x80 && *from < 0xa0)) {	// C0 or C1
				if(*from == 0x0a || *from == 0x0d) {
					state.g0 = EncodingState::ASCII;
					state.g2 = EncodingState::UNDESIGNATED;
				}
				*to = *from;	// SI, SO, SS2 (1 byte) and SS3 (1 byte) are ignored
			} else if(state.invokedG2) {	// G2
				const Byte c = *from | 0x80;
				if(state.g2 == EncodingState::ISO_8859_1) {	// ISO-8859-1
					if(iso88591Encoder.get() == 0)
						(iso88591Encoder = Encoder::forMIB(fundamental::ISO_8859_1))->setSubstitutionPolicy(substitutionPolicy);
					const Byte* next;
					const Encoder::Result r = iso88591Encoder->toUnicode(to, toEnd, toNext, &c, &c + 1, next);
					if(r != Encoder::COMPLETED) {
						fromNext = from;
						return r;
					}
				} else if(state.g2 == EncodingState::ISO_8859_7) {	// ISO-8859-7
					if(iso88597Encoder.get() == 0)
						(iso88597Encoder = Encoder::forMIB(standard::ISO_8859_7))->setSubstitutionPolicy(substitutionPolicy);
					const Byte* next;
					const Encoder::Result r = iso88597Encoder->toUnicode(to, toEnd, toNext, &c, &c + 1, next);
					if(r != Encoder::COMPLETED) {
						fromNext = from;
						return r;
					}
				} else {	// G2 is not designated
					toNext = to;
					fromNext = from;
					return Encoder::MALFORMED_INPUT;
				}
				state.invokedG2 = false;
			} else if(state.g0 == EncodingState::JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
				*to = convertROMANtoUCS(*from);
				if(*to == REPLACEMENT_CHARACTER)
					ASCENSION_HANDLE_UNMAPPABLE()
/*			} else if(state.g0 == EncodingState::JIS_X_0201_KANA) {	// JIS X 0201-Kana
				wchar_t	u;
				if(src[i] >= 0x80) {
					CONFIRM_ILLEGAL_CHAR(u);
				} else {
					u = jisX0201ToUCS(pszSrc[iSrc] + 0x80);
					if(u == 0)
						CONFIRM_ILLEGAL_CHAR(u);
				}
				dest[j++] = u;
				++i;
*/			} else if(state.g0 == EncodingState::ASCII) {	// ASCII
				if(*from >= 0x80)
					ASCENSION_HANDLE_UNMAPPABLE()
				*to = *from;
			} else if(from + 1 >= fromEnd) {	// the trail byte was not found
				toNext = to;
				fromNext = from;
				return Encoder::MALFORMED_INPUT;
			} else if(state.g0 == EncodingState::JIS_X_0208) {	// JIS X 0208:1978 or :1983
				const uint16_t jis = (*from << 8) | from[1];
				const Char ucs = convertX0208toUCS(jis);
				if(ucs == REPLACEMENT_CHARACTER)
					ASCENSION_HANDLE_UNMAPPABLE()
				++from;
			} else if(state.g0 == EncodingState::JIS_X_0212) {	// JIS X 0212:1990
				const uint16_t jis = (*from << 8) | from[1];
				const Char ucs = convertX0212toUCS(jis);
				if(ucs == REPLACEMENT_CHARACTER)
					ASCENSION_HANDLE_UNMAPPABLE()
				++from;
			} else if(state.g0 == EncodingState::GB2312 || state.g0 == EncodingState::KS_C_5601) {	// GB2312:1980 or KSC5601:1987
				const Byte buffer[2] = {*from | 0x80, from[1] | 0x80};
				const Byte* const bufferEnd = ASCENSION_ENDOF(buffer);
				const Byte* next;
				const Encoder::Result r = ((state.g0 == EncodingState::GB2312) ?
					gb2312Encoder : ksc5601Encoder)->toUnicode(to, toEnd, toNext, buffer, bufferEnd, next);
				if(r != Encoder::COMPLETED) {
					fromNext = from;
					return r;
				}
				from = next - 1;
			} else if(state.g0 == EncodingState::JIS_X_0213_PLANE_1
					|| state.g0 == EncodingState::JIS_X_0213_PLANE_2) {	// JIS X 0213:2004 or :2000
				const uint16_t jis = (*from << 8) | from[1];
				CodePoint ucs = (state.g0 == EncodingState::JIS_X_0213_PLANE_1) ?
					convertX0213Plane1toUCS(jis) : convertX0213Plane2toUCS(jis);

				if(ucs == REPLACEMENT_CHARACTER) {
					if(substitutionPolicy == Encoder::IGNORE_UNMAPPABLE_CHARACTERS) {
						--to;
						++from;
						continue;
					} else if(substitutionPolicy != Encoder::REPLACE_UNMAPPABLE_CHARACTERS) {
						toNext = to;
						fromNext = from;
						return Encoder::UNMAPPABLE_CHARACTER;
					}
				}
				if(ucs > 0xffffu) {
					if(to + 1 >= toEnd)
						break;	// INSUFFICIENT_BUFFER
					if(ucs > 0x0010fffful) {	// two UCS characters
						*to = maskUCS2(ucs >> 16);
						*++to = maskUCS2(ucs);
					} else
						utf16::uncheckedEncode(ucs, to++);
				} else {
					if(to > beginning && ((to[-1] == 0x02e9u && ucs == 0x02e5u) || (to[-1] == 0x02e5u && ucs == 0x02e9u))) {
						if(to + 1 >= toEnd)
							break;	// INSUFFICIENT_BUFFER
						*(to++) = ZERO_WIDTH_NON_JOINER;
					}
					*to = maskUCS2(ucs);
				}
				++from;
			}
		}
#undef ASCENSION_HANDLE_UNMAPPABLE
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
	}

	// converts from UTF-16 into ISO-2022-JP-X.
	Encoder::Result convertUTF16toISO2022JPX(char x, Byte* to, Byte* toEnd, Byte*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext,
			EncodingState& state, bool eob, Encoder::SubstitutionPolicy substitutionPolicy) {
		const bool jis2004 = x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
			|| x == 's' || x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
			;
		int charset = EncodingState::ASCII;
		auto_ptr<Encoder> iso88591Encoder, iso88597Encoder, gb2312Encoder, ksc5601Encoder;
		if(x == '2') {
			if(0 != (iso88591Encoder = Encoder::forMIB(fundamental::ISO_8859_1)).get())
				iso88591Encoder->setSubstitutionPolicy(substitutionPolicy);
			if(0 != (iso88597Encoder = Encoder::forMIB(standard::ISO_8859_7)).get())
				iso88597Encoder->setSubstitutionPolicy(substitutionPolicy);
			if(0 != (gb2312Encoder = Encoder::forMIB(standard::GB2312)).get())
				gb2312Encoder->setSubstitutionPolicy(substitutionPolicy);
			if(0 != (ksc5601Encoder = Encoder::forMIB(36)).get())
				ksc5601Encoder->setSubstitutionPolicy(substitutionPolicy);
		}

#define ASCENSION_HANDLE_UNMAPPABLE()										\
	if(substitutionPolicy == Encoder::REPLACE_UNMAPPABLE_CHARACTERS) {		\
		jis = mbcs[0] = 0x1a;												\
		mbcs[1] = 1;														\
		charset = EncodingState::ASCII;										\
	} else if(substitutionPolicy == Encoder::IGNORE_UNMAPPABLE_CHARACTERS) {	\
		--to;																\
		continue;															\
	} else {																\
		toNext = to;														\
		fromNext = from;													\
		return Encoder::UNMAPPABLE_CHARACTER;								\
	}

		uint16_t jis;
		Byte mbcs[2];
		Byte* dummy1;
		const Char* dummy2;
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			// first, convert '*from' into 'jis' or 'mbcs' buffer
			if(*from < 0x80) {
				mbcs[0] = mask8Bit(jis = *from);
				mbcs[1] = 0;
				charset = EncodingState::ASCII;
			} else if(0 != (jis = convertUCStoROMAN(*from)) && jis < 0x80)
				charset = /*(jis < 0x80) ?*/ EncodingState::JIS_X_0201_ROMAN /*: EncodingState::JIS_X_0201_KANA*/;
			else if(jis2004) {
				bool plane2;
				switch(convertUCStoX0213(from, fromEnd, fromNext, eob, jis, plane2)) {
				case Encoder::COMPLETED:
					if(fromNext == from) {
						toNext = to;	// pending...
						return Encoder::COMPLETED;
					}
					charset = EncodingState::UNDESIGNATED;
					if(!plane2) {
						// try JIS X 0208 compatible sequence
						if(x == 'c' && convertUCStoX0208(*from) != 0x00)
							charset = EncodingState::JIS_X_0208;
						else if(x == 's'
								&& !isISO2022JP3ProhibitedIdeograph(jis)
								&& !isISO2022JP2004ProhibitedIdeograph(jis))
							charset = EncodingState::JIS_X_0208;
					}
					if(charset == EncodingState::UNDESIGNATED)
						charset = plane2 ? EncodingState::JIS_X_0213_PLANE_2 : EncodingState::JIS_X_0213_PLANE_1;
					break;
				case Encoder::UNMAPPABLE_CHARACTER:
					ASCENSION_HANDLE_UNMAPPABLE()
					break;
				case Encoder::MALFORMED_INPUT:
					return Encoder::MALFORMED_INPUT;
				}
			} else if(0 != (jis = convertUCStoX0208(*from)))
				charset = EncodingState::JIS_X_0208;
			else if((x == '2'
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
					|| x == '1'
#endif // !ASCENSION_NO_EXTENDED_ENCODINGS
					) && (jis = convertUCStoX0212(*from)) != 0)
				charset = EncodingState::JIS_X_0212;
			else if(/*x == '2' &&*/ gb2312Encoder.get() != 0
					&& gb2312Encoder->fromUnicode(mbcs, ASCENSION_ENDOF(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = EncodingState::GB2312;
			else if(/*x == '2' &&*/ ksc5601Encoder.get() != 0
					&& ksc5601Encoder->fromUnicode(mbcs, ASCENSION_ENDOF(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = EncodingState::KS_C_5601;
			else if(x == '2'
					&& iso88591Encoder->fromUnicode(mbcs, ASCENSION_ENDOF(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = EncodingState::ISO_8859_1;
			else if(x == '2'
					&& iso88597Encoder->fromUnicode(mbcs, ASCENSION_ENDOF(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = EncodingState::ISO_8859_7;
			else 
				ASCENSION_HANDLE_UNMAPPABLE()

#define ASCENSION_DESIGNATE_TO_G0(escapeSequence, length)	\
	if(state.g0 != charset) {								\
		if(to + length > toEnd)								\
			break;	/* INSUFFICIENT_BUFFER */				\
		memcpy(to, escapeSequence, length);					\
		to += length;										\
		state.g0 = static_cast<EncodingState::G0>(charset);	\
	}
#define ASCENSION_DESIGNATE_TO_G2(escapeSequence, length)	\
	if(state.g2 != charset) {								\
		if(to + length > toEnd)								\
			break;	/* INSUFFICIENT_BUFFER */				\
		memcpy(to, escapeSequence, length);					\
		to += length;										\
		state.g2 = static_cast<EncodingState::G2>(charset);	\
	}

			if(charset == EncodingState::ASCII) {	// ASCII
				ASCENSION_DESIGNATE_TO_G0("\x1b(B", 3);
				*to = mask8Bit(jis);
			} else if(charset == EncodingState::JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
				ASCENSION_DESIGNATE_TO_G0("\x1b(J", 3);
				*to = mask8Bit(jis);
//			} else if(charset == EncodingState::JIS_X_0201_KANA) {	// JIS X 0201-Kana
//				ASCENSION_DESIGNATE_TO_G0("\x1b(I", 3);
//				*to = mask8Bit(jis);
			} else if(charset == EncodingState::JIS_X_0208) {	// JIS X 0208:1997 (:1990)
				ASCENSION_DESIGNATE_TO_G0("\x1b$B", 3);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == EncodingState::JIS_X_0212) {	// JIS X 0212:1990
				ASCENSION_DESIGNATE_TO_G0("\x1b$(D", 4);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == EncodingState::JIS_X_0213_PLANE_1) {	// JIS X 0213:2004 plane-1 /* or :2000 plane-1 */
				ASCENSION_DESIGNATE_TO_G0("\x1b$(Q" /*"\x1b$(O"*/, 4);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == EncodingState::JIS_X_0213_PLANE_2) {	// JIS X 0213:2004 (:2000) plane-2
				ASCENSION_DESIGNATE_TO_G0("\x1b$(P", 4);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == EncodingState::GB2312) {	// GB2312:1980
				ASCENSION_DESIGNATE_TO_G0("\x1b$A", 3);
				*to = mask7Bit(mbcs[0]);
				if(mbcs[1] != 0)
					*++to = mask7Bit(mbcs[1]);
			} else if(charset == EncodingState::KS_C_5601) {	// KSC5601:1987
				ASCENSION_DESIGNATE_TO_G0("\x1b$(C", 4);
				*to = mask7Bit(mbcs[0]);
				if(mbcs[1] != 0)
					*++to = mask7Bit(mbcs[1]);
			} else if(charset == EncodingState::ISO_8859_1) {	// ISO-8859-1
				ASCENSION_DESIGNATE_TO_G2("\x1b.A", 3);
				if(to + 3 >= toEnd)
					break;	// INSUFFICIENT_BUFFER
				*to = ESC;	// SS2
				*++to = 'N';
				*++to = mask8Bit(mbcs[0]);
			} else if(charset == EncodingState::ISO_8859_7) {	// ISO-8859-7

				ASCENSION_DESIGNATE_TO_G2("\x1b.F", 3);
				if(to + 3 >= toEnd)
					break;	// INSUFFICIENT_BUFFER
				*to = ESC;	// SS2
				*++to = 'N';
				*++to = mask8Bit(mbcs[0]);
			}
		}

		// restore G0 into ASCII and end (if sufficient buffer is)
		if(from == fromEnd && state.g0 != EncodingState::ASCII && to + 3 <= toEnd) {
			memcpy(to, "\x1b(B", 3);
			to += 3;
			state.g0 = EncodingState::ASCII;
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
#undef ASCENSION_HANDLE_UNMAPPABLE
#undef ASCENSION_DESIGNATE_TO_G0
#undef ASCENSION_DESIGNATE_TO_G2
	}

	// JIS X 0208 or JIS X 0213 <-> シフト JIS 2 バイト文字の変換
	inline void shiftCode(uint16_t jis, Byte* dbcs, bool plane2) {
		assert(dbcs != 0);
		const Byte jk = mask8Bit((jis - 0x2020) >> 8);	// ku
		const Byte jt = mask8Bit((jis - 0x2020) >> 0);	// ten

		assert(jk >= 1 && jk <= 94 && jt >= 1 && jt <= 94);
		if(!plane2)	// plane 1
//			dbcs[0] = (jk - 1) / 2 + ((jk <= 62) ? 0x81 : 0xc1);
			dbcs[0] = (jk + ((jk <= 62) ? 0x101 : 0x181)) / 2;
		else	// plane 2
			dbcs[0] = (jk >= 78) ? ((jk + 0x19b) / 2) : ((jk + 0x1df) / 2 - jk / 8 * 3);
		if((jk & 0x1) == 0)	dbcs[1] = jt + 0x9e;
		else				dbcs[1] = jt + ((jt <= 63) ? 0x3f : 0x40);
	}
	inline uint16_t unshiftCodeX0208(const Byte dbcs[]) {
		assert(dbcs != 0);
		Byte jk, jt;

		if(dbcs[0] >= 0x81 && dbcs[0] <= 0x9f)	// ku: 01..62
			jk = (dbcs[0] - 0x81) * 2 + ((dbcs[1] > 0x9e) ? 2 : 1);	// < leadbyte = (jk - 1) / 2 + 0x81
		else	// ku: 63..94
			jk = (dbcs[0] - 0xc1) * 2 + ((dbcs[1] > 0x9e) ? 2 : 1);	// < leadbyte = (jk - 1) / 2 + 0xc1
		if((jk & 0x1) == 0)
			jt = dbcs[1] - 0x9e;	// < trailbyte = jt + 0x9E
		else if(dbcs[1] <= 0x3f + 63)	// ten: 01..63
			jt = dbcs[1] - 0x3f;	// < trailbyte = jt + 0x3F
		else	// ten: 64..94
			jt = dbcs[1] - 0x40;	// < trailbyte = jt + 0x40
		return ((jk << 8) | jt) + 0x2020;
	}
	inline uint16_t unshiftCodeX0213(const Byte dbcs[], bool& plane2) {
		Byte jk, jt;
		const bool kuIsEven = dbcs[1] > 0x9e;

		plane2 = dbcs[0] >= 0xf0;
		if(dbcs[0] >= 0x81 && dbcs[0] <= 0x9f)
			jk = dbcs[0] * 2 - 0x101 + (kuIsEven ? 1 : 0);
		else if(dbcs[0] >= 0xe0 && dbcs[0] <= 0xef)
			jk = dbcs[0] * 2 - 0x181 + (kuIsEven ? 1 : 0);
		else if((dbcs[0] == 0xf4 && kuIsEven) || (dbcs[0] >= 0xf5 && dbcs[0] <= 0xfc))
			jk = dbcs[0] * 2 - 0x19b + (kuIsEven ? 1 : 0);
		else if((dbcs[0] >= 0xf0 && dbcs[0] <= 0xf3) || (dbcs[0] == 0xf4 && !kuIsEven)) {
			switch(dbcs[0]) {
			case 0xf0:	jk = kuIsEven ? 8 : 1; break;
			case 0xf1:	jk = kuIsEven ? 4 : 3; break;
			case 0xf2:	jk = kuIsEven ? 12 : 5; break;
			case 0xf3:	jk = kuIsEven ? 14 : 13; break;
			case 0xf4:	jk = 15; break;
			}
		}
		if((jk & 0x1) == 0)
			jt = dbcs[1] - 0x9e;	// < trailbyte = jt + 0x9E
		else if(dbcs[1] <= 0x3f + 63)	// ten: 01..63
			jt = dbcs[1] - 0x3f;	// < trailbyte = jt + 0x3F
		else	// ten: 64..94
			jt = dbcs[1] - 0x40;	// < trailbyte = jt + 0x40
		return ((jk << 8) | jt) + 0x2020;
	}

} // namespace @0


// Shift_JIS //////////////////////////////////////////////////////////////////////////////////////

namespace {
	template<> Encoder::Result InternalEncoder<SHIFT_JIS>::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x80)
				*to = mask8Bit(*from);
			else {
				uint16_t jis = convertUCStoX0208(*from);	// try JIS X 0208
				if(jis == 0x00) {
					if(const Byte kana = convertUCStoKANA(*from)) {	// try JIS X 0201 kana
						*to = kana;
						continue;
					} else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
						*to = properties().substitutionCharacter();
					else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
						--to;
					else {
						toNext = to;
						fromNext = from;
						return UNMAPPABLE_CHARACTER;
					}
				} else if(to + 1 >= toEnd)
					break;	// INSUFFICIENT_BUFFER
				shiftCode(jis, to, false);
				++to;	// DBCS			
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}

	template<> Encoder::Result InternalEncoder<SHIFT_JIS>::doToUnicode(
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x80)	// ascii
				*to = *from;
			else if(*from >= 0xa1 && *from <= 0xdf)	// 1-byte kana
				*to = convertKANAtoUCS(*from);
			else if(*from == 0xa0) {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			} else {	// DBCS leading byte
				if(from + 1 < fromEnd && from[1] >= 0x40 && from[1] <= 0xfc && from[1] != 0x7f) {
					*to = convertX0208toUCS(unshiftCodeX0208(from));
					if(*to == REPLACEMENT_CHARACTER) {
						if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
							--to;
						else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
							toNext = to;
							fromNext = from;
							return UNMAPPABLE_CHARACTER;
						}
					}
					++from;
				} else {
					toNext = to;
					fromNext = from;
					return (from + 1 == fromEnd && (flags() & END_OF_BUFFER) == 0) ? COMPLETED : MALFORMED_INPUT;
				}
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}
}


// Shift_JIS-2004 /////////////////////////////////////////////////////////////////////////////////

namespace {
	template<> Encoder::Result InternalEncoder<SHIFT_JIS_2004>::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		uint16_t jis;
		bool plane2;
		while(to < toEnd && from < fromEnd) {
			if(*from < 0x0080) {
				*to++ = mask8Bit(*from++);
				continue;
			}
			switch(convertUCStoX0213(from, fromEnd, fromNext, (flags() & END_OF_BUFFER) != 0, jis, plane2)) {
			case COMPLETED:
				if(fromNext == from) {
					assert((flags() & END_OF_BUFFER) == 0);	// pending...
					toNext = to;
					return COMPLETED;
				}
				break;
			case UNMAPPABLE_CHARACTER:
				if(0 == (jis = convertUCStoKANA(*from))) {
					if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
						*to = properties().substitutionCharacter();
					else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
						--to;
					else {
						toNext = to;
						fromNext = from;
						return UNMAPPABLE_CHARACTER;
					}
				} else {
					assert(jis < 0x0100);	// kana
					*to = mask8Bit(jis);
				}
				++to;
				++from;
				continue;
			case MALFORMED_INPUT:
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			}

			// double-byte
			if(to + 1 == toEnd)
				break;	// insufficient buffer
			shiftCode(jis, to, plane2);
			to += 2;
			from = fromNext;
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}

	template<> Encoder::Result InternalEncoder<SHIFT_JIS_2004>::doToUnicode(
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		const Char* const beginning = to;
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x80)	// ASCII
				*to = *from;
			else if(*from >= 0xa1 && *from <= 0xdf)	// kana
				*to = convertKANAtoUCS(*from);
			else if(*from == 0xa0) {	// illegal byte
				if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
					*to = properties().substitutionCharacter();
				else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
					--to;
				else {
					toNext = to;
					fromNext = from;
					return UNMAPPABLE_CHARACTER;
				}
			} else {
				if(from + 1 < fromEnd && (from[1] >= 0x40 && from[1] <= 0xfc && from[1] != 0x7f)) {	// double byte
					bool plane2;
					const uint16_t jis = unshiftCodeX0213(from, plane2);
					const CodePoint ucs = !plane2 ? convertX0213Plane1toUCS(jis) : convertX0213Plane2toUCS(jis);

					if(ucs == REPLACEMENT_CHARACTER) {	// unmappable
						if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
							--to;
						else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
							toNext = to;
							fromNext = from;
							return UNMAPPABLE_CHARACTER;
						}
						continue;
					} else if(ucs >= 0x010000ul && to + 1 >= toEnd)
						break;	// INSUFFICIENT_BUFFER

					if(ucs > 0x0010fffful) {	// a character uses two code points
						*to = maskUCS2(ucs >> 16);
						*++to = maskUCS2(ucs);
					} else if(ucs >= 0x00010000ul)	// out of BMP
						utf16::uncheckedEncode(ucs, to++);
					else {
						if(to > beginning && (to[-1] == 0x02e9u && ucs == 0x02e5u) || (to[-1] == 0x02e5u && ucs == 0x02e9u)) {
							if(to + 1 >= toEnd)
								break;	// INSUFFICIENT_BUFFER
							*(to++) = ZERO_WIDTH_NON_JOINER;
						}
						*to = maskUCS2(ucs);
					}
					++from;
				} else {
					toNext = to;
					fromNext = from;
					return MALFORMED_INPUT;
				}
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}
}


// EUC-JP /////////////////////////////////////////////////////////////////////////////////////////

namespace {
	template<> Encoder::Result InternalEncoder<EUC_JP>::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x0080) {	// ASCII
				*to = mask8Bit(*from);
				continue;
			}

			bool x0212 = false;
			uint16_t jis = convertUCStoX0208(*from);
			if(jis == 0x00) {
				if((jis = convertUCStoX0212(*from)) != 0x00)
					// JIS X 0212
					x0212 = true;
				else if(const Byte kana = convertUCStoKANA(*from)) {
					// JIS X 0201 Kana
					if(to + 1 >= toEnd) {
						toNext = to;
						fromNext = from;
						return INSUFFICIENT_BUFFER;
					}
					*to = SS2_8BIT;
					*++to = kana;
					continue;
				} else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
					*to = properties().substitutionCharacter();
				else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
					--to;
				else {
					toNext = to;
					fromNext = from;
					return UNMAPPABLE_CHARACTER;
				}
			} else if(to + 1 >= toEnd) {
				toNext = to;
				fromNext = from;
				return INSUFFICIENT_BUFFER;
			}

			jis |= 0x8080;	// jis -> euc-jp
			if(!x0212) {	// JIS X 0208
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(to + 2 >= toEnd) {
				toNext = to;
				fromNext = from;
				return INSUFFICIENT_BUFFER;
			} else {	// JIS X 0212
				*to = SS3_8BIT;
				*++to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}

	template<> Encoder::Result InternalEncoder<EUC_JP>::doToUnicode(
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x80)
				*to = *from;
			else {
				const size_t bytes = (*from != SS3_8BIT) ? 2 : 3;
				if(from + bytes > fromEnd) {
					toNext = to;
					fromNext = from;
					return MALFORMED_INPUT;
				} else if(*from == SS2_8BIT)	// SS2 -> JIS X 0201 Kana
					*to = convertKANAtoUCS(from[1]);
				else if(*from == SS3_8BIT) {	// SS3 -> JIS X 0212
					const uint16_t jis = ((from[1] << 8) | from[2]) - 0x8080;
					*to = convertX0212toUCS(jis);
				} else {	// JIS X 0208
					const uint16_t jis = ((*from << 8) | from[1]) - 0x8080;
					*to = convertX0208toUCS(jis);
				}

				if(*to == REPLACEMENT_CHARACTER) {	// unmappable
					if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
						--to;
					else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
						toNext = to;
						fromNext = from;
						return UNMAPPABLE_CHARACTER;
					}
				}
				from += bytes - 1;
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}
}


// EUC-JIS-2004 ///////////////////////////////////////////////////////////////////////////////////

namespace {
	template<> Encoder::Result InternalEncoder<EUC_JIS_2004>::doFromUnicode(
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {
		uint16_t jis;
		bool plane2 = false;
		while(to < toEnd && from < fromEnd) {
			if(*from < 0x0080) {	// ASCII
				*to++ = mask8Bit(*from++);
				continue;
			} else if(to + 1 == toEnd)	// insufficient buffer
				break;

			// UCS -> JIS
			switch(convertUCStoX0213(from, fromEnd, fromNext, (flags() & END_OF_BUFFER) != 0, jis, plane2)) {
			case COMPLETED:
				if(fromNext == from) {
					assert((flags() & END_OF_BUFFER) == 0);	// pending...
					toNext = to;
					return COMPLETED;
				}
			case UNMAPPABLE_CHARACTER:
				if(0 == (jis = convertUCStoKANA(*from))) {
					if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
						*to = properties().substitutionCharacter();
					else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
						--to;
					else {
						toNext = to;
						fromNext = from;
						return UNMAPPABLE_CHARACTER;
					}
				} else {
					assert(jis < 0x0100);	// kana
					*to = SS2_8BIT;
					*++to = mask8Bit(jis);
				}
				++to;
				++from;
				continue;
			case MALFORMED_INPUT:
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			}

			// JIS -> EUC-JIS
			jis += 0x8080;
			if(!plane2) {	// plane 1
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis >> 0);
			} else {	// plane 2
				if(to + 2 == toEnd)
					break;	// insufficient buffer
				*to = SS3_8BIT;
				*++to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis >> 0);
			}
			++to;
			from = fromNext;
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}

	template<> Encoder::Result InternalEncoder<EUC_JIS_2004>::doToUnicode(
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {
		const Char* const beginning = to;
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from < 0x80)
				*to = *from;
			else {
				const ptrdiff_t bytes = (*from != SS3_8BIT) ? 2 : 3;
				if(from + bytes > fromEnd) {
					toNext = to;
					fromNext = from;
					return MALFORMED_INPUT;
				} else if(*from == SS2_8BIT)	// SS2 -> JIS X 0201 Kana
					*to = convertKANAtoUCS(from[1]);
				else if(*from == SS3_8BIT) {	// SS3 -> plane-2
					const uint16_t jis = ((from[1] << 8) | from[2]) - 0x8080;
					const CodePoint ucs = convertX0213Plane2toUCS(jis);
					if(ucs != REPLACEMENT_CHARACTER) {
						if(ucs > 0x010000ul && to + 1 >= toEnd)
							break;	// INSUFFICIENT_BUFFER
						if(ucs > 0x0010fffful) {	// a character uses two code points
							*to = maskUCS2(ucs >> 16);
							*++to = maskUCS2(ucs >> 0);
						} else if(ucs >= 0x00010000ul)	// out of BMP
							utf16::uncheckedEncode(ucs, to++);
						else
							*to = maskUCS2(ucs);
					}
				} else {	// plane-1
					const uint16_t jis = ((*from << 8) | from[1]) - 0x8080;
					const CodePoint ucs = convertX0213Plane1toUCS(jis);
					if(ucs != REPLACEMENT_CHARACTER) {
						if(ucs > 0x0010fffful) {	// a character uses two code points
							*to = maskUCS2(ucs >> 16);
							*++to = maskUCS2(ucs >> 0);
						} else if(ucs >= 0x00010000ul)	// out of BMP
							utf16::uncheckedEncode(ucs, to++);
						else {
							if(to > beginning
									&& ((to[-1] == 0x02e9u && ucs == 0x02e5u)
									|| (to[-1] == 0x02e5u && ucs == 0x02e9u))) {
								if(to + 1 >= toEnd)
									break;	// INSUFFICIENT_BUFFER
								*(to++) = ZERO_WIDTH_NON_JOINER;
							}
							*to = maskUCS2(ucs);
						}
					}
				}
				if(*to == REPLACEMENT_CHARACTER) {	// unmappable
					if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
						--to;
					else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS) {
						toNext = to;
						fromNext = from;
						return UNMAPPABLE_CHARACTER;
					}
				}
				from += bytes - 1;
			}
		}
		toNext = to;
		fromNext = from;
		return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
	}
}


#define ASCENSION_IMPLEMENT_ISO_2022_JP_X(x, suffix)																\
	template<> Encoder::Result InternalEncoder<ISO_2022_##suffix>::doFromUnicode(									\
			Byte* to, Byte* toEnd, Byte*& toNext, const Char* from, const Char* fromEnd, const Char*& fromNext) {	\
		return convertUTF16toISO2022JPX(x, to, toEnd, toNext, from, fromEnd, fromNext,								\
			encodingState_,	(flags() & END_OF_BUFFER) != 0, substitutionPolicy());									\
	}																												\
	template<> Encoder::Result InternalEncoder<ISO_2022_##suffix>::doToUnicode(										\
			Char* to, Char* toEnd, Char*& toNext, const Byte* from, const Byte* fromEnd, const Byte*& fromNext) {	\
		return convertISO2022JPXtoUTF16(x, to, toEnd, toNext, from, fromEnd, fromNext,								\
			decodingState_, (flags() & END_OF_BUFFER) != 0, substitutionPolicy());									\
	}

namespace {

	// ISO-2022-JP ////////////////////////////////////////////////////////////////////////////////

	ASCENSION_IMPLEMENT_ISO_2022_JP_X('0', JP)


	// ISO-2022-JP-2 //////////////////////////////////////////////////////////////////////////////

	ASCENSION_IMPLEMENT_ISO_2022_JP_X('2', JP_2)


	// ISO-2022-JP-2004 ///////////////////////////////////////////////////////////////////////////

	ASCENSION_IMPLEMENT_ISO_2022_JP_X('4', JP_2004)


#ifndef ASCENSION_NO_MINORITY_ENCODINGS

	// ISO-2022-JP-1 //////////////////////////////////////////////////////////////////////////////

	ASCENSION_IMPLEMENT_ISO_2022_JP_X('1', JP_1)


	// ISO-2022-JP-2004-Strict ////////////////////////////////////////////////////////////////////

	ASCENSION_IMPLEMENT_ISO_2022_JP_X('s', JP_2004_STRICT)


	// ISO-2022-JP-2004-Compatible ////////////////////////////////////////////////////////////////

	ASCENSION_IMPLEMENT_ISO_2022_JP_X('c', JP_2004_COMPATIBLE)

}

#endif // !ASCENSION_NO_MINORITY_ENCODINGS


// JISAutoDetector ////////////////////////////////////////////////////////////////////////////////

namespace {
	inline const EncodingProperties& detectShiftJIS(const Byte* from, const Byte* last, ptrdiff_t& convertibleBytes, bool& foundKana) {
		bool jis2004 = false;
		foundKana = false;
		const Byte* p;
		for(p = from; p < last; ++p) {
			if(*p == ESC)	// Shift_JIS can't have an ESC
				break;
			else if(*p < 0x80)	// ASCII is ok
				continue;
			else if(*p >= 0xa1 && *p <= 0xdf)	// JIS X 0201 kana
				foundKana = true;
			else if(p < last - 1) {	// 2-byte character?
				if(*p < 0x81 || *p > 0xfc || (*p > 0x9f && *p < 0xe0))
					break;	// illegal lead byte
				else if(p[1] < 0x40 || p[1] > 0xfc || p[1] == 0x7f)
					break;	// illegal trail byte

				bool plane2;
				if(!jis2004) {
					if(convertX0208toUCS(unshiftCodeX0208(p)) == REPLACEMENT_CHARACTER) {
						const uint16_t jis = unshiftCodeX0213(p, plane2);
						if(!plane2 && convertX0213Plane1toUCS(jis) == REPLACEMENT_CHARACTER)
							break;	// unmappable
						jis2004 = true;
					}
				} else {	// Shift_JIS-2004
					if(unshiftCodeX0213(p, plane2) == 0x00)
						break;
				}
				++p;
			} else
				break;
		}
		convertibleBytes = p - from;
		return jis2004 ? static_cast<EncodingProperties&>(shiftjis2004) : static_cast<EncodingProperties&>(shiftjis);
	}
	inline const EncodingProperties& detectEUCJP(const Byte* from, const Byte* last, ptrdiff_t& convertibleBytes, bool& foundKana) {
		bool jis2004 = false;
		foundKana = false;
		const Byte* p;
		for(p = from; p < last; ++p) {
			if(*p == ESC)	// EUC-JP can't have an ESC
				break;
			else if(*p < 0x80)	// ASCII is ok
				continue;
			else if(*p == SS2_8BIT) {	// SS2 introduces JIS X 0201 kana
				if(p + 1 >= last || p[1] < 0xa0 || p[1] > 0xe0)
					break;
				foundKana = true;
				++p;
			} else if(*p == SS3_8BIT) {	// SS3 introduces JIS X 0212 or JIS X 0213 plane2
				if(p + 2 >= last)
					break;
				uint16_t jis = p[1] << 8 | p[2];
				if(jis < 0x8080)
					break;	// unmappable
				jis -= 0x8080;
				if(convertX0212toUCS(jis) != REPLACEMENT_CHARACTER) {
					if(jis2004)
						break;
//					cp = CPEX_JAPANESE_EUC;
				} else if(convertX0213Plane2toUCS(jis) != REPLACEMENT_CHARACTER) {
					if(!jis2004)
						break;
					jis2004 = true;
				} else
					break;
				from += 2;
			} else if(from < last - 1) {	// 2-byte character
				uint16_t jis = *p << 8 | p[1];
				if(jis <= 0x8080)
					break;
				jis -= 0x8080;
				if(convertX0208toUCS(jis) == REPLACEMENT_CHARACTER) {
					if(convertX0213Plane1toUCS(jis) != REPLACEMENT_CHARACTER) {
//						if(cp == CPEX_JAPANESE_EUC)
//							break;
						jis2004 = true;
					} else
						break;
				}
				++from;
			} else
				break;
		}
		convertibleBytes = p - from;
		return jis2004 ? static_cast<EncodingProperties&>(eucjis2004) : static_cast<EncodingProperties&>(eucjp);
	}
	inline const EncodingProperties& detectISO2022JP(const Byte* from, const Byte* last, ptrdiff_t& convertibleBytes, bool& foundKana) {
		char x = '0';	// ISO-2022-JP-X
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
		bool x0208 = false;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
		foundKana = false;
		const Byte* p;
		for(p = from; p < last; ++p) {
			if(*p >= 0x80)	// 8-bit
				break;
			else if(*p == ESC) {
				if(p + 2 >= last)
					break;
				if(memcmp(p + 1, "(J", 2) == 0 || memcmp(p + 1, "(I", 2) == 0) {	// JIS X 0201
					p += 2;
					foundKana = true;
				} else if(memcmp(p + 1, "$@", 2) == 0 || memcmp(p + 1, "$B", 2) == 0) {	// JIS X 0208
					p += 2;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
					x0208 = true;
					if(x == '4')
						x = 'c';
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
				} else if(memcmp(p + 1, "$A", 2) == 0		// GB2312
						|| memcmp(p + 1, ".A", 2) == 0		// ISO-8859-1
						|| memcmp(p + 1, ".F", 2) == 0) {	// ISO-8859-7
					if(x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							|| x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						)
						break;
					x = '2';
					p += 2;
				} else if(p + 3 < last) {
					if(memcmp(p + 1, "$(D", 3) == 0) {	// JIS X 0212
						if(x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
								|| x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
							)
							break;
						else if(x != '2')
							x =
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
								'1'
#else
								'2'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
								;
						p += 3;
					} else if(memcmp(p + 1, "$(C", 3) == 0) {	// KS C 5601
						if(x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
								|| x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
							)
							break;
						x = '2';
						p += 3;
					} else if(memcmp(p + 1, "$(", 2) == 0
							&& (p[3] == 'O'		// JIS X 0213:2000 plane1
							|| p[3] == 'P'		// JIS X 0213:2000 plane2
							|| p[3] == 'Q')) {	// JIS X 0213:2004 plane1
						if(x == '2')
							break;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
						if(x == '1')
							break;
						else if(x0208)
							x = 'c';
						else
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
							x = '4';
						p += 3;
					}
				} else
					break;
			}
		}

		convertibleBytes = p - from;
		const EncodingProperties* result = 0;
		switch(x) {
		case '0': result = &iso2022jp; break;
		case '2': result = &iso2022jp2; break;
		case '4': result = &iso2022jp2004; break;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
		case '1': result = &iso2022jp1; break;
		case 'c': result = &iso2022jp2004compatible; break;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
		}
		assert(result != 0);
		return *result;
	}

}

/// @see EncodingDetector#doDetector
pair<MIBenum, string> JISAutoDetector::doDetect(const Byte* first, const Byte* last, ptrdiff_t* convertibleBytes) const /*throw()*/ {
	pair<MIBenum, string> result(make_pair(fundamental::UTF_8, "UTF-8"));
	ptrdiff_t cb = 0;

	// first, test Unicode
	if(const EncodingDetector* unicodeDetector = EncodingDetector::forName("UnicodeAutoDetect")) {
		result = unicodeDetector->detect(first, last, &cb);
		if(cb == last - first) {
			if(convertibleBytes != 0)
				*convertibleBytes = cb;
			return result;
		}
	}

	bool foundKana;
	ptrdiff_t cb2;
	const EncodingProperties* result2 = &detectShiftJIS(first, last, cb2, foundKana);
	if(cb2 > cb) {
		result = make_pair(result2->mibEnum(), result2->name());
		cb = cb2;
	}
	if(cb < last - first || foundKana) {
		result2 = &detectEUCJP(first, last, cb2, foundKana);
		if(cb2 > cb) {
			result = make_pair(result2->mibEnum(), result2->name());
			cb = cb2;
		}
		if(cb < last - first || foundKana) {
			result2 = &detectISO2022JP(first, last, cb2, foundKana);
			if(cb2 > cb) {
				result = make_pair(result2->mibEnum(), result2->name());
				cb = cb2;
			}
		}
	}

	if(convertibleBytes != 0)
		*convertibleBytes = cb;
	return result;
}

#endif // !ASCENSION_NO_STANDARD_ENCODINGS
