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
 * @date 2004-2012, 2014
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

#include <ascension/corelib/encoding/encoder.hpp>
#include <ascension/corelib/encoding/encoder-implementation.hpp>
#include <ascension/corelib/encoding/encoding-detector.hpp>
#include <ascension/corelib/text/utf.hpp>
#include <cassert>
#include <cstring>		// std.memcpy
#include <map>
#include <boost/range/algorithm/binary_search.hpp>

namespace ascension {
	namespace encoding {
		namespace implementation {
			namespace dbcs {
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
						EncodingState() BOOST_NOEXCEPT {reset();}
						void reset() BOOST_NOEXCEPT {
							g0 = ASCII;
							g2 = UNDESIGNATED;
							invokedG2 = false;
						}
					};

					template<typename Factory>
					class InternalEncoder : public Encoder {
					public:
						explicit InternalEncoder(const Factory& factory) BOOST_NOEXCEPT : properties_(factory) {}
					private:
						Result doFromUnicode(State& state,
							const boost::iterator_range<Byte*>& to, Byte*& toNext,
							const boost::iterator_range<const Char*>& from, const Char*& fromNext) override;
						Result doToUnicode(State& state,
							const boost::iterator_range<Char*>& to, Char*& toNext,
							const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) override;
						const EncodingProperties& properties() const override BOOST_NOEXCEPT {
							return properties_;
						}
					private:
						const EncodingProperties& properties_;
					};

					class ShiftJis : public EncoderFactoryImpl {
					public:
						ShiftJis() BOOST_NOEXCEPT : EncoderFactoryImpl("Shift_JIS", standard::SHIFT_JIS, "Japanese (Shift_JIS)", 2, 1, "MS_Kanji|csShiftJIS", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<ShiftJis>(*this));
						}
					};

					class ShiftJis2004 : public EncoderFactoryImpl {
					public:
						ShiftJis2004() BOOST_NOEXCEPT : EncoderFactoryImpl("Shift_JIS-2004", MIB_OTHER, "Japanese (Shift_JIS-2004)", 2, 1, "", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<ShiftJis2004>(*this));
						}
					};

					class EucJp : public EncoderFactoryImpl {
					public:
						EucJp() BOOST_NOEXCEPT : EncoderFactoryImpl("EUC-JP", standard::EUC_JP, "Japanese (EUC-JP)", 3, 1, "Extended_UNIX_Code_Packed_Format_for_Japanese|csEUCPkdFmtJapanese", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<EucJp>(*this));
						}
					};

					class EucJis2004 : public EncoderFactoryImpl {
					public:
						EucJis2004() BOOST_NOEXCEPT : EncoderFactoryImpl("EUC-JIS-2004", MIB_OTHER, "Japanese (EUC-JIS-2004)", 3, 1, "", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<EucJis2004>(*this));
						}
					};

					class Iso2022Jp : public EncoderFactoryImpl {
					public:
						Iso2022Jp() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-JP", standard::ISO_2022_JP, "Japanese (ISO-2022-JP)", 8, 1, "csISO2022JP", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Jp>(*this));
						}
					};

					class Iso2022Jp2 : public EncoderFactoryImpl {
					public:
						Iso2022Jp2() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-JP-2", standard::ISO_2022_JP_2, "Japanese (ISO-2022-JP-2)", 9, 1, "csISO2022JP2", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Jp2>(*this));
						}
					};

					class Iso2022Jp2004 : public EncoderFactoryImpl {
					public:
						Iso2022Jp2004() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-JP-2004", MIB_OTHER, "Japanese (ISO-2022-JP-2004)", 9, 1, "", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Jp2004>(*this));
						}
					};

#ifndef ASCENSION_NO_MINORITY_ENCODINGS
					class Iso2022Jp1 : public EncoderFactoryImpl {
					public:
						Iso2022Jp1() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-JP-1", MIB_OTHER, "Japanese (ISO-2022-JP-1)", 9, 1, "", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Jp1>(*this));
						}
					};

					class Iso2022Jp2004Strict : public EncoderFactoryImpl {
					public:
						Iso2022Jp2004Strict() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-JP-2004-Strict", MIB_OTHER, "Japanese (ISO-2022-JP-2004-Strict)", 9, 1, "", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Jp2004Strict>(*this));
						}
					};

					class Iso2022Jp2004Compatible : public EncoderFactoryImpl {
					public:
						Iso2022Jp2004Compatible() BOOST_NOEXCEPT : EncoderFactoryImpl("ISO-2022-JP-2004-Compatible", MIB_OTHER, "Japanese (ISO-2022-JP-2004-Compatible)", 9, 1, "", 0x3f) {}
					private:
						std::unique_ptr<Encoder> create() const override BOOST_NOEXCEPT {
							return std::unique_ptr<Encoder>(new InternalEncoder<Iso2022Jp2004Compatible>(*this));
						}
					};
#endif // !ASCENSION_NO_MINORITY_ENCODINGS

					class JisAutoDetector : public EncodingDetector {
					public:
						JisAutoDetector() : EncodingDetector("JISAutoDetect") {}
					private:
						std::tuple<MIBenum, std::string, std::size_t> doDetect(const boost::iterator_range<const Byte*>& bytes) const override BOOST_NOEXCEPT;
					};

					struct Installer {
						Installer() :
								SHIFT_JIS(std::make_shared<ShiftJis>()), SHIFT_JIS_2004(std::make_shared<ShiftJis2004>()),
								EUC_JP(std::make_shared<EucJp>()), EUC_JIS_2004(std::make_shared<EucJis2004>()),
								ISO_2022_JP(std::make_shared<Iso2022Jp>()), ISO_2022_JP_2(std::make_shared<Iso2022Jp2>()), ISO_2022_JP_2004(std::make_shared<Iso2022Jp2004>())
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
								, ISO_2022_JP_1(std::make_shared<Iso2022Jp1>()), ISO_2022_JP_2004_COMPATIBLE(std::make_shared<Iso2022Jp2004Compatible>())
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						{
							EncoderRegistry::instance().registerFactory(SHIFT_JIS);
							EncoderRegistry::instance().registerFactory(SHIFT_JIS_2004);
							EncoderRegistry::instance().registerFactory(EUC_JP);
							EncoderRegistry::instance().registerFactory(EUC_JIS_2004);
							EncoderRegistry::instance().registerFactory(ISO_2022_JP);
							EncoderRegistry::instance().registerFactory(ISO_2022_JP_2);
							EncoderRegistry::instance().registerFactory(ISO_2022_JP_2004);
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							EncoderRegistry::instance().registerFactory(ISO_2022_JP_1);
							EncoderRegistry::instance().registerFactory(std::make_shared<const Iso2022Jp2004Strict>());
							EncoderRegistry::instance().registerFactory(ISO_2022_JP_2004_COMPATIBLE);
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
							EncodingDetector::registerDetector(std::make_shared<const JisAutoDetector>());
						}

						const std::shared_ptr<const ShiftJis> SHIFT_JIS;
						const std::shared_ptr<const ShiftJis2004> SHIFT_JIS_2004;
						const std::shared_ptr<const EucJp> EUC_JP;
						const std::shared_ptr<const EucJis2004> EUC_JIS_2004;
						const std::shared_ptr<const Iso2022Jp> ISO_2022_JP;
						const std::shared_ptr<const Iso2022Jp2> ISO_2022_JP_2;
						const std::shared_ptr<const Iso2022Jp2004> ISO_2022_JP_2004;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
						const std::shared_ptr<const Iso2022Jp1> ISO_2022_JP_1;
						const std::shared_ptr<const Iso2022Jp2004Compatible> ISO_2022_JP_2004_COMPATIBLE;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
					} installer;

					// mapping between JIS and UCS
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
					inline Char convertROMANtoUCS(Byte c) BOOST_NOEXCEPT {
						if(c == 0x5c)					return 0x00a5u;						// Yen Sign
						else if(c == 0x7e)				return 0x203eu;						// Overline
						else if(c >= 0x20 && c <= 0x7d)	return c;							// 7-bit
						else							return text::REPLACEMENT_CHARACTER;	// unmappable
					}
					inline Byte convertUCStoROMAN(Char c) BOOST_NOEXCEPT {
						if(c >= 0x0020u && c <= 0x005bu)		return mask8Bit(c);	// 7-bit
						else if(c >= 0x005du && c <= 0x007du)	return mask8Bit(c);	// 7-bit
						else if(c == 0x00a5u)					return 0x5c;		// Yen Sign
						else if(c == 0x203eu)					return 0x7e;		// Overline
						else									return 0x00;		// unmappable
					}

					// JIS X 0201 Kana
					inline Char convertKANAtoUCS(Byte c) BOOST_NOEXCEPT {
						return (c >= 0xa1 && c <= 0xdf) ? c + 0xfec0u : text::REPLACEMENT_CHARACTER;
					}
					inline Byte convertUCStoKANA(Char c) BOOST_NOEXCEPT {
						return (c >= 0xff61u && c <= 0xff9fu) ? mask8Bit(c - 0xfec0u) : 0x00;
					}

					// JIS X 0208:1997
					inline Char convertX0208toUCS(std::uint16_t c) BOOST_NOEXCEPT {
						if(const Char** const wire = JIS_X_0208_TO_UCS[mask8Bit(c >> 8)])
							return wireAt(wire, mask8Bit(c));
						else
							return text::REPLACEMENT_CHARACTER;
					}
					inline std::uint16_t convertUCStoX0208(Char c) BOOST_NOEXCEPT {
						if(const std::uint16_t** const wire = UCS_TO_JIS_X_0208[mask8Bit(c >> 8)])
							return wireAt(wire, mask8Bit(c));
						else
							return 0x0000;
					}

					// JIS X 0212:1990
					inline Char convertX0212toUCS(std::uint16_t c) BOOST_NOEXCEPT {
						if(const Char** const wire = JIS_X_0212_TO_UCS[mask8Bit(c >> 8)])
							return wireAt(wire, mask8Bit(c));
						else
							return text::REPLACEMENT_CHARACTER;
					}
					inline std::uint16_t convertUCStoX0212(Char c) BOOST_NOEXCEPT {
						if(const std::uint16_t** const wire = UCS_TO_JIS_X_0212[mask8Bit(c >> 8)])
							return wireAt(wire, mask8Bit(c));
						else
							return 0x0000;
					}

					// JIS X 0213:2004 plane 1 to UCS
					inline CodePoint convertX0213Plane1toUCS(std::uint16_t c) BOOST_NOEXCEPT {
						const CodePoint ucs = convertX0208toUCS(c);
						if(ucs == text::REPLACEMENT_CHARACTER) {
							if(const CodePoint** const wire = JIS_X_0213_PLANE_1_TO_UCS[mask8Bit(c >> 8)])
								return wireAt(wire, mask8Bit(c));
						}
						return convertX0208toUCS(c);
					}

					// JIS X 0213:2000 plane 2 to UCS
					inline CodePoint convertX0213Plane2toUCS(std::uint16_t c) BOOST_NOEXCEPT {
						if(const CodePoint** const wire = JIS_X_0213_PLANE_2_TO_UCS[mask8Bit(c >> 8)])
							return wireAt(wire, mask8Bit(c));
						else
							return text::REPLACEMENT_CHARACTER;
					}

					// UCS to JIS X 0213:2004
					Encoder::Result convertUCStoX0213(const boost::iterator_range<const Char*>& ucs,
							const Char*& next, bool eob, std::uint16_t& jis, bool& plane2) BOOST_NOEXCEPT {
						jis = 0;
						if(boost::binary_search(LEADING_BYTES_TO_JIS_X_0213, ucs[0])) {
							if(boost::size(ucs) == 1) {
								if(!eob) {
									// pending
									next = boost::const_begin(ucs);
									return Encoder::COMPLETED;
								}
							} else {
								if(ucs[1] == 0x309a) {	// <(kana), Combining Katakana-Hiragana Semi-Voiced Sound Mark>
									switch(ucs[0]) {
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
								} else if(ucs[1] == 0x0300) {	// <X, Combining Grave Accent>
									switch(ucs[0]) {
										case 0x00e6: jis = 0x2b44; break;	// ae
										case 0x0254: jis = 0x2b48; break;	// open o
										case 0x0259: jis = 0x2b4c; break;	// schwa
										case 0x025a: jis = 0x2b4e; break;	// schwa with hook
										case 0x028c: jis = 0x2b4a; break;	// turned v
									}
								} else if(ucs[1] == 0x0301) {	// <X, Combining Acute Accent>
									switch(ucs[0]) {
										case 0x0254: jis = 0x2b49; break;	// open o
										case 0x0259: jis = 0x2b4d; break;	// schwa
										case 0x025a: jis = 0x2b4f; break;	// schwa with hook
										case 0x028c: jis = 0x2b4b; break;	// turned v
									}
								} else if(ucs[0] == 0x02e9) {
									if(ucs[1] == 0x02e5)
										jis = 0x2b65;	// <Extra-Low Tone Bar, Extra-High Tone Bar> -> rising symbol
									else if(ucs[1] == text::ZERO_WIDTH_NON_JOINER && boost::size(ucs) > 2 && ucs[2] == 0x02e5)
										jis = 0x2b64;	// just dependent Extra-Low Tone Bar
								} else if(ucs[0] == 0x02e5) {
									if(ucs[1] == 0x02e9)
										jis = 0x2b66;	// <Extra-High Tone Bar, Extra-Low Tone Bar> -> falling symbol
									else if(ucs[1] == text::ZERO_WIDTH_NON_JOINER && boost::size(ucs) > 2 && ucs[2] == 0x02e9)
										jis = 0x2b60;	// just dependent Extra-High Tone Bar
								}
								if(jis != 0) {
									next = boost::const_begin(ucs) + 2;
									plane2 = false;
									return Encoder::COMPLETED;
								}
							}
						}

						// one-to-one mapping
						if(text::surrogates::isHighSurrogate(ucs[0])) {
							if(boost::size(ucs) == 1) {
								next = boost::const_begin(ucs);
								return eob ? Encoder::MALFORMED_INPUT : Encoder::COMPLETED;
							}
							const CodePoint c = text::utf::decodeFirst(ucs);
							if(c < 0x10000ul) {
								next = boost::const_begin(ucs);
								return Encoder::MALFORMED_INPUT;
							} else if(c >= 0x20000ul && c < 0x30000ul) {
								const std::uint16_t** wire;
								if(nullptr != (wire = UCS_SIP_TO_JIS_X_0213_PLANE_1[mask8Bit((c - 0x20000ul) >> 8)])) {
									if(0 != (jis = wireAt(wire, mask8Bit(c - 0x20000ul))))
										plane2 = false;
								}
								if(jis == 0 && nullptr != (wire = UCS_SIP_TO_JIS_X_0213_PLANE_2[mask8Bit((c - 0x20000ul) >> 8)])) {
									if(0 != (jis = wireAt(wire, mask8Bit(c - 0x20000ul))))
										plane2 = true;
								}
								if(jis != 0) {
									next = boost::const_begin(ucs) + 2;
									return Encoder::COMPLETED;
								}
							}
							if(jis == 0) {
								next = boost::const_begin(ucs);
								return Encoder::UNMAPPABLE_CHARACTER;
							}
						} else {
							const std::uint16_t** wire;
							if(nullptr != (wire = UCS_BMP_TO_JIS_X_0213_PLANE_1[mask8Bit(ucs[0] >> 8)])) {
								if(0 != (jis = wireAt(wire, mask8Bit(ucs[0]))))
									plane2 = false;
							}
							if(jis == 0 && nullptr != (wire = UCS_BMP_TO_JIS_X_0213_PLANE_2[mask8Bit(ucs[0] >> 8)])) {
								if(0 != (jis = wireAt(wire, mask8Bit(ucs[0]))))
									plane2 = true;
							}
							if(jis == 0 && nullptr != (wire = UCS_TO_JIS_X_0208[mask8Bit(ucs[0] >> 8)])) {
								if(0 != (jis = wireAt(wire, mask8Bit(ucs[0]))))
									plane2 = false;
							}
							if(jis == 0) {
								next = boost::const_begin(ucs);
								return Encoder::UNMAPPABLE_CHARACTER;
							}
						}
						next = boost::const_begin(ucs) + 1;
						return Encoder::COMPLETED;
					}

					// makes JIS code from ku and ten numbers.
					inline std::uint16_t jk(Byte ku, Byte ten) BOOST_NOEXCEPT {
						return ((ku << 8) | ten) + 0x2020;
					}

					// "禁止漢字" of ISO-2022-JP-3 (from JIS X 0213:2000 附属書 2 表 1)
					const std::uint16_t PROHIBITED_IDEOGRAPHS_2000[] = {
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
					const std::uint16_t PROHIBITED_IDEOGRAPHS_2004[] = {
						jk(14, 1), jk(15,94), jk(17,19), jk(22,70), jk(23,50), jk(28,24),
						jk(33,73), jk(38,61), jk(39,77), jk(47,52), jk(47,94), jk(53,11),
						jk(54, 2), jk(54,58), jk(84, 7), jk(94,90), jk(94,91), jk(94,92),
						jk(94,93), jk(94,94)
					};

					// returns true if is "禁止漢字" of ISO-2022-JP-3.
					inline bool isISO2022JP3ProhibitedIdeograph(std::uint16_t jis) {
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
							|| boost::binary_search(PROHIBITED_IDEOGRAPHS_2000, jis);
					}

					// returns true if is "禁止漢字" added by JIS X 0213:2004.
					inline bool isISO2022JP2004ProhibitedIdeograph(std::uint16_t jis) {
						return boost::binary_search(PROHIBITED_IDEOGRAPHS_2004, jis);
					}

					// converts from ISO-2022-JP-X into UTF-16.
					Encoder::Result convertISO2022JPXtoUTF16(char x,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext,
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
						std::unique_ptr<Encoder> gb2312Encoder, ksc5601Encoder, iso88591Encoder, iso88597Encoder;
						bool checkedGB2312 = false, checkedKSC5601 = false;

#define ASCENSION_HANDLE_UNMAPPABLE()	{									\
	if(substitutionPolicy == Encoder::IGNORE_UNMAPPABLE_CHARACTERS)			\
		--toNext;															\
	else if(substitutionPolicy != Encoder::REPLACE_UNMAPPABLE_CHARACTERS)	\
		return Encoder::UNMAPPABLE_CHARACTER;								\
}

						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext == ESC) {	// expect esc. seq.
								if(fromNext + 2 < boost::const_end(from)) {
									switch(fromNext[1]) {
										case 'N':
											state.invokedG2 = true; ++fromNext; continue;	// SS2
										case '(':
											switch(fromNext[2]) {
												case 'B': state.g0 = EncodingState::ASCII; fromNext += 2; continue;				// "(B" => ASCII
//												case 'I': state.g0 = EncodingState::JIS_X_0201_KANA; fromNext += 2; continue;	// "(I" => JIS X 0201 Kana
												case 'J': state.g0 = EncodingState::JIS_X_0201_ROMAN; fromNext += 2; continue;	// "(J" => JIS X 0201 Roman
											}
											break;
										case '$':
											switch(fromNext[2]) {
												case '@': state.g0 = EncodingState::JIS_X_0208; fromNext += 2; continue;	// "$@" => JIS X 0208
												case 'A':	// "$A" => GB2312
													if(x != '2') break;
													if(!checkedGB2312) {
														if(nullptr != (gb2312Encoder = EncoderRegistry::instance().forMIB(standard::GB2312)).get())
															gb2312Encoder->setSubstitutionPolicy(substitutionPolicy);
													}
													if(gb2312Encoder.get() == nullptr) break;
													state.g0 = EncodingState::GB2312; fromNext += 2; continue;
												case 'B': state.g0 = EncodingState::JIS_X_0208; fromNext += 2; continue;	// "$B" => JIS X 0208
												case '(':
													if(fromNext + 3 < boost::const_end(from)) {
														switch(fromNext[3]) {
															case 'C':	// "$(C" => KSC5601
																if(x != '2') break;
																if(!checkedKSC5601) {
																	if(nullptr != (ksc5601Encoder = EncoderRegistry::instance().forMIB(36)).get())
																		ksc5601Encoder->setSubstitutionPolicy(substitutionPolicy);
																}
																if(ksc5601Encoder.get() == nullptr) break;
																state.g0 = EncodingState::KS_C_5601; fromNext += 3; continue;
															case 'D':	// "$(D" => JIS X 0212
																if(x != '2'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
																	&& x != '1'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
																	) break;
																state.g0 = EncodingState::JIS_X_0212; fromNext += 3; continue;
															case 'O':	// "$(O" => JIS X 0213 plane 1
																if(!jis2004) break;
																state.g0 = EncodingState::JIS_X_0213_PLANE_1; fromNext += 3; continue;
															case 'P':	// "$(P" => JIS X 0213 plane 2
																if(!jis2004) break;
																state.g0 = EncodingState::JIS_X_0213_PLANE_2; fromNext += 3; continue;
															case 'Q':	// "$(Q" => JIS X 0213 plane 1
																if(!jis2004) break;
																state.g0 = EncodingState::JIS_X_0213_PLANE_1; fromNext += 3; continue;
														}
													}
											}
											break;
										case '.':
											if(x == '2') {
												switch(from[1]) {
												case 'A': state.g2 = EncodingState::ISO_8859_1; fromNext += 2; continue;	// ".A" => ISO-8859-1 (G2)
												case 'F': state.g2 = EncodingState::ISO_8859_7; fromNext += 2; continue;	// ".F" => ISO-8859-7 (G2)
												}
											}
											break;
									}
								}

								// illegal or unsupported esc. seq.
								return Encoder::MALFORMED_INPUT;
							}

							if(*fromNext <= 0x20 || (*fromNext >= 0x80 && *fromNext < 0xa0)) {	// C0 or C1
								if(*fromNext == 0x0a || *fromNext == 0x0d) {
									state.g0 = EncodingState::ASCII;
									state.g2 = EncodingState::UNDESIGNATED;
								}
								*toNext = *fromNext;	// SI, SO, SS2 (1 byte) and SS3 (1 byte) are ignored
							} else if(state.invokedG2) {	// G2
								const Byte c = *fromNext | 0x80;
								if(state.g2 == EncodingState::ISO_8859_1) {	// ISO-8859-1
									if(iso88591Encoder.get() == nullptr)
										(iso88591Encoder = EncoderRegistry::instance().forMIB(fundamental::ISO_8859_1))->setSubstitutionPolicy(substitutionPolicy);
									const Byte* next;
									Char* temp;
									Encoder::State state;
									const auto r = iso88591Encoder->toUnicode(state, boost::make_iterator_range(toNext, boost::end(to)), temp, boost::make_iterator_range_n(&c, 1), next);
									if(r != Encoder::COMPLETED)
										return (toNext = temp), r;
								} else if(state.g2 == EncodingState::ISO_8859_7) {	// ISO-8859-7
									if(iso88597Encoder.get() == nullptr)
										(iso88597Encoder = EncoderRegistry::instance().forMIB(standard::ISO_8859_7))->setSubstitutionPolicy(substitutionPolicy);
									const Byte* next;
									Char* temp;
									Encoder::State state;
									const auto r = iso88597Encoder->toUnicode(state, boost::make_iterator_range(toNext, boost::end(to)), temp, boost::make_iterator_range_n(&c, 1), next);
									if(r != Encoder::COMPLETED)
										return (toNext = temp), r;
								} else	// G2 is not designated
									return Encoder::MALFORMED_INPUT;
								state.invokedG2 = false;
							} else if(state.g0 == EncodingState::JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
								*toNext = convertROMANtoUCS(*fromNext);
								if(*toNext == text::REPLACEMENT_CHARACTER)
									ASCENSION_HANDLE_UNMAPPABLE()
/*							} else if(state.g0 == EncodingState::JIS_X_0201_KANA) {	// JIS X 0201-Kana
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
*/							} else if(state.g0 == EncodingState::ASCII) {	// ASCII
								if(*fromNext >= 0x80)
									ASCENSION_HANDLE_UNMAPPABLE()
								*toNext = *fromNext;
							} else if(fromNext + 1 >= boost::const_end(from))	// the trail byte was not found
								return Encoder::MALFORMED_INPUT;
							else if(state.g0 == EncodingState::JIS_X_0208) {	// JIS X 0208:1978 or :1983
								const std::uint16_t jis = (*fromNext << 8) | fromNext[1];
								const Char ucs = convertX0208toUCS(jis);
								if(ucs == text::REPLACEMENT_CHARACTER)
									ASCENSION_HANDLE_UNMAPPABLE()
								++fromNext;
							} else if(state.g0 == EncodingState::JIS_X_0212) {	// JIS X 0212:1990
								const std::uint16_t jis = (*fromNext << 8) | fromNext[1];
								const Char ucs = convertX0212toUCS(jis);
								if(ucs == text::REPLACEMENT_CHARACTER)
									ASCENSION_HANDLE_UNMAPPABLE()
								++fromNext;
							} else if(state.g0 == EncodingState::GB2312 || state.g0 == EncodingState::KS_C_5601) {	// GB2312:1980 or KSC5601:1987
								const Byte buffer[2] = {*fromNext | 0x80, fromNext[1] | 0x80};
								const Byte* next;
								Char* temp;
								Encoder::State internalState;
								const auto r = ((state.g0 == EncodingState::GB2312) ?
									gb2312Encoder : ksc5601Encoder)->toUnicode(internalState, boost::make_iterator_range(toNext, boost::end(to)), temp, boost::make_iterator_range(buffer), next);
								if(r != Encoder::COMPLETED)
									return (toNext = temp), r;
								fromNext = next - 1;
							} else if(state.g0 == EncodingState::JIS_X_0213_PLANE_1
									|| state.g0 == EncodingState::JIS_X_0213_PLANE_2) {	// JIS X 0213:2004 or :2000
								const std::uint16_t jis = (*fromNext << 8) | fromNext[1];
								CodePoint ucs = (state.g0 == EncodingState::JIS_X_0213_PLANE_1) ?
									convertX0213Plane1toUCS(jis) : convertX0213Plane2toUCS(jis);

								if(ucs == text::REPLACEMENT_CHARACTER) {
									if(substitutionPolicy == Encoder::IGNORE_UNMAPPABLE_CHARACTERS) {
										--toNext;
										++fromNext;
										continue;
									} else if(substitutionPolicy != Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
										return Encoder::UNMAPPABLE_CHARACTER;
								}
								if(ucs > 0xffffu) {
									if(toNext + 1 >= boost::end(to))
										break;	// INSUFFICIENT_BUFFER
									if(ucs > 0x0010fffful) {	// two UCS characters
										*toNext = maskUCS2(ucs >> 16);
										*++toNext = maskUCS2(ucs);
									} else {
										Char* temp = toNext++;
										text::utf::encode(ucs, temp);
									}
								} else {
									if(toNext > boost::begin(to) && ((toNext[-1] == 0x02e9u && ucs == 0x02e5u) || (toNext[-1] == 0x02e5u && ucs == 0x02e9u))) {
										if(toNext + 1 >= boost::end(to))
											break;	// INSUFFICIENT_BUFFER
										*(toNext++) = text::ZERO_WIDTH_NON_JOINER;
									}
									*toNext = maskUCS2(ucs);
								}
								++fromNext;
							}
						}
#undef ASCENSION_HANDLE_UNMAPPABLE
						return (fromNext == boost::const_end(from)) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
					}

					// converts from UTF-16 into ISO-2022-JP-X.
					Encoder::Result convertUTF16toISO2022JPX(char x,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext,
							EncodingState& state, bool eob, Encoder::SubstitutionPolicy substitutionPolicy) {
						const bool jis2004 = x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							|| x == 's' || x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
							;
						int charset = EncodingState::ASCII;
						std::unique_ptr<Encoder> iso88591Encoder, iso88597Encoder, gb2312Encoder, ksc5601Encoder;
						if(x == '2') {
							if(nullptr != (iso88591Encoder = EncoderRegistry::instance().forMIB(fundamental::ISO_8859_1)).get())
								iso88591Encoder->setSubstitutionPolicy(substitutionPolicy);
							if(nullptr != (iso88597Encoder = EncoderRegistry::instance().forMIB(standard::ISO_8859_7)).get())
								iso88597Encoder->setSubstitutionPolicy(substitutionPolicy);
							if(nullptr != (gb2312Encoder = EncoderRegistry::instance().forMIB(standard::GB2312)).get())
								gb2312Encoder->setSubstitutionPolicy(substitutionPolicy);
							if(nullptr != (ksc5601Encoder = EncoderRegistry::instance().forMIB(36)).get())
								ksc5601Encoder->setSubstitutionPolicy(substitutionPolicy);
						}

#define ASCENSION_HANDLE_UNMAPPABLE()											\
	if(substitutionPolicy == Encoder::REPLACE_UNMAPPABLE_CHARACTERS) {			\
		jis = mbcs[0] = 0x1a;													\
		mbcs[1] = 1;															\
		charset = EncodingState::ASCII;											\
	} else if(substitutionPolicy == Encoder::IGNORE_UNMAPPABLE_CHARACTERS) {	\
		--toNext;																\
		continue;																\
	} else																		\
		return Encoder::UNMAPPABLE_CHARACTER;

						std::uint16_t jis;
						Byte mbcs[2];
						Byte* dummy1;
						const Char* dummy2;
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							// first, convert '*fromNext' into 'jis' or 'mbcs' buffer
							Encoder::State dummyState;
							if(*fromNext < 0x80) {
								mbcs[0] = mask8Bit(jis = *fromNext);
								mbcs[1] = 0;
								charset = EncodingState::ASCII;
							} else if(0 != (jis = convertUCStoROMAN(*fromNext)) && jis < 0x80)
								charset = /*(jis < 0x80) ?*/ EncodingState::JIS_X_0201_ROMAN /*: EncodingState::JIS_X_0201_KANA*/;
							else if(jis2004) {
								const Char* next;
								bool plane2;
								switch(convertUCStoX0213(boost::make_iterator_range(fromNext, boost::const_end(from)), next, eob, jis, plane2)) {
								case Encoder::COMPLETED:
									if(next == fromNext)
										return Encoder::COMPLETED;	// pending...
									charset = EncodingState::UNDESIGNATED;
									if(!plane2) {
										// try JIS X 0208 compatible sequence
										if(x == 'c' && convertUCStoX0208(*fromNext) != 0x00)
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
							} else if(0 != (jis = convertUCStoX0208(*fromNext)))
								charset = EncodingState::JIS_X_0208;
							else if((x == '2'
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
									|| x == '1'
#endif // !ASCENSION_NO_EXTENDED_ENCODINGS
									) && (jis = convertUCStoX0212(*fromNext)) != 0)
								charset = EncodingState::JIS_X_0212;
							else if(/*x == '2' &&*/ gb2312Encoder.get() != nullptr
									&& gb2312Encoder->fromUnicode(dummyState, boost::make_iterator_range(mbcs), dummy1, boost::make_iterator_range_n(fromNext, 1), dummy2) == Encoder::COMPLETED)
								charset = EncodingState::GB2312;
							else if(/*x == '2' &&*/ ksc5601Encoder.get() != nullptr
									&& ksc5601Encoder->fromUnicode(dummyState, boost::make_iterator_range(mbcs), dummy1, boost::make_iterator_range_n(fromNext, 1), dummy2) == Encoder::COMPLETED)
								charset = EncodingState::KS_C_5601;
							else if(x == '2'
									&& iso88591Encoder->fromUnicode(dummyState, boost::make_iterator_range(mbcs), dummy1, boost::make_iterator_range_n(fromNext, 1), dummy2) == Encoder::COMPLETED)
								charset = EncodingState::ISO_8859_1;
							else if(x == '2'
									&& iso88597Encoder->fromUnicode(dummyState, boost::make_iterator_range(mbcs), dummy1, boost::make_iterator_range_n(fromNext, 1), dummy2) == Encoder::COMPLETED)
								charset = EncodingState::ISO_8859_7;
							else 
								ASCENSION_HANDLE_UNMAPPABLE()

#define ASCENSION_DESIGNATE_TO_G0(escapeSequence, length)	\
	if(state.g0 != charset) {								\
		if(std::next(toNext, length) > boost::end(to))		\
			break;	/* INSUFFICIENT_BUFFER */				\
		std::memcpy(toNext, escapeSequence, length);		\
		std::advance(toNext, length);						\
		state.g0 = static_cast<EncodingState::G0>(charset);	\
	}
#define ASCENSION_DESIGNATE_TO_G2(escapeSequence, length)	\
	if(state.g2 != charset) {								\
		if(std::next(toNext, length) > boost::end(to))		\
			break;	/* INSUFFICIENT_BUFFER */				\
		std::memcpy(toNext, escapeSequence, length);		\
		std::advance(toNext, length);						\
		state.g2 = static_cast<EncodingState::G2>(charset);	\
	}

							if(charset == EncodingState::ASCII) {	// ASCII
								ASCENSION_DESIGNATE_TO_G0("\x1b(B", 3);
								*toNext = mask8Bit(jis);
							} else if(charset == EncodingState::JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
								ASCENSION_DESIGNATE_TO_G0("\x1b(J", 3);
								*toNext = mask8Bit(jis);
//							} else if(charset == EncodingState::JIS_X_0201_KANA) {	// JIS X 0201-Kana
//								ASCENSION_DESIGNATE_TO_G0("\x1b(I", 3);
//								*toNext = mask8Bit(jis);
							} else if(charset == EncodingState::JIS_X_0208) {	// JIS X 0208:1997 (:1990)
								ASCENSION_DESIGNATE_TO_G0("\x1b$B", 3);
								*toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis);
							} else if(charset == EncodingState::JIS_X_0212) {	// JIS X 0212:1990
								ASCENSION_DESIGNATE_TO_G0("\x1b$(D", 4);
								*toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis);
							} else if(charset == EncodingState::JIS_X_0213_PLANE_1) {	// JIS X 0213:2004 plane-1 /* or :2000 plane-1 */
								ASCENSION_DESIGNATE_TO_G0("\x1b$(Q" /*"\x1b$(O"*/, 4);
								*toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis);
							} else if(charset == EncodingState::JIS_X_0213_PLANE_2) {	// JIS X 0213:2004 (:2000) plane-2
								ASCENSION_DESIGNATE_TO_G0("\x1b$(P", 4);
								*toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis);
							} else if(charset == EncodingState::GB2312) {	// GB2312:1980
								ASCENSION_DESIGNATE_TO_G0("\x1b$A", 3);
								*toNext = mask7Bit(mbcs[0]);
								if(mbcs[1] != 0)
									*++toNext = mask7Bit(mbcs[1]);
							} else if(charset == EncodingState::KS_C_5601) {	// KSC5601:1987
								ASCENSION_DESIGNATE_TO_G0("\x1b$(C", 4);
								*toNext = mask7Bit(mbcs[0]);
								if(mbcs[1] != 0)
									*++toNext = mask7Bit(mbcs[1]);
							} else if(charset == EncodingState::ISO_8859_1) {	// ISO-8859-1
								ASCENSION_DESIGNATE_TO_G2("\x1b.A", 3);
								if(std::next(toNext, 3) >= boost::end(to))
									break;	// INSUFFICIENT_BUFFER
								*toNext = ESC;	// SS2
								*++toNext = 'N';
								*++toNext = mask8Bit(mbcs[0]);
							} else if(charset == EncodingState::ISO_8859_7) {	// ISO-8859-7

								ASCENSION_DESIGNATE_TO_G2("\x1b.F", 3);
								if(std::next(toNext, 3) >= boost::end(to))
									break;	// INSUFFICIENT_BUFFER
								*toNext = ESC;	// SS2
								*++toNext = 'N';
								*++toNext = mask8Bit(mbcs[0]);
							}
						}

						// restore G0 into ASCII and end (if sufficient buffer is)
						if(fromNext == boost::const_end(from) && state.g0 != EncodingState::ASCII && toNext + 3 <= boost::end(to)) {
							std::memcpy(toNext, "\x1b(B", 3);
							toNext += 3;
							state.g0 = EncodingState::ASCII;
						}
						return (fromNext == boost::const_end(from)) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
#undef ASCENSION_HANDLE_UNMAPPABLE
#undef ASCENSION_DESIGNATE_TO_G0
#undef ASCENSION_DESIGNATE_TO_G2
					}

					// JIS X 0208 or JIS X 0213 <-> シフト JIS 2 バイト文字の変換
					inline void shiftCode(std::uint16_t jis, Byte* dbcs, bool plane2) {
						assert(dbcs != nullptr);
						const Byte jk = mask8Bit((jis - 0x2020) >> 8);	// ku
						const Byte jt = mask8Bit((jis - 0x2020) >> 0);	// ten

						assert(jk >= 1 && jk <= 94 && jt >= 1 && jt <= 94);
						if(!plane2)	// plane 1
	//						dbcs[0] = (jk - 1) / 2 + ((jk <= 62) ? 0x81 : 0xc1);
							dbcs[0] = (jk + ((jk <= 62) ? 0x101 : 0x181)) / 2;
						else	// plane 2
							dbcs[0] = (jk >= 78) ? ((jk + 0x19b) / 2) : ((jk + 0x1df) / 2 - jk / 8 * 3);
						if((jk & 0x1) == 0)	dbcs[1] = jt + 0x9e;
						else				dbcs[1] = jt + ((jt <= 63) ? 0x3f : 0x40);
					}
					inline std::uint16_t unshiftCodeX0208(const Byte dbcs[]) {
						assert(dbcs != nullptr);
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
					inline std::uint16_t unshiftCodeX0213(const Byte dbcs[], bool& plane2) {
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

					inline bool eob(const Encoder& encoder) BOOST_NOEXCEPT {
#if 0
						return encoder.options().test(Encoder::END_OF_BUFFER);
#else
						boost::ignore_unused(encoder);
						return true;
#endif
					}


					// Shift_JIS //////////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::Result InternalEncoder<ShiftJis>::doFromUnicode(State&,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext < 0x80)
								*toNext = mask8Bit(*fromNext);
							else {
								std::uint16_t jis = convertUCStoX0208(*fromNext);	// try JIS X 0208
								if(jis == 0x00) {
									if(const Byte kana = convertUCStoKANA(*fromNext)) {	// try JIS X 0201 kana
										*toNext = kana;
										continue;
									} else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
										*toNext = properties().substitutionCharacter();
									else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
										--toNext;
									else
										return UNMAPPABLE_CHARACTER;
								} else if(toNext + 1 >= boost::end(to))
									break;	// INSUFFICIENT_BUFFER
								shiftCode(jis, toNext, false);
								++toNext;	// DBCS			
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					template<> Encoder::Result InternalEncoder<ShiftJis>::doToUnicode(State&,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext < 0x80)	// ascii
								*toNext = *fromNext;
							else if(*fromNext >= 0xa1 && *fromNext <= 0xdf)	// 1-byte kana
								*toNext = convertKANAtoUCS(*fromNext);
							else if(*fromNext == 0xa0)
								return MALFORMED_INPUT;
							else {	// DBCS leading byte
								if(fromNext + 1 < boost::const_end(from) && fromNext[1] >= 0x40 && fromNext[1] <= 0xfc && fromNext[1] != 0x7f) {
									*toNext = convertX0208toUCS(unshiftCodeX0208(fromNext));
									if(*toNext == text::REPLACEMENT_CHARACTER) {
										if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
											--toNext;
										else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS)
											return UNMAPPABLE_CHARACTER;
									}
									++fromNext;
								} else
									return (fromNext + 1 == boost::const_end(from) && eob(*this)) ? COMPLETED : MALFORMED_INPUT;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// Shift_JIS-2004 /////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::Result InternalEncoder<ShiftJis2004>::doFromUnicode(State&,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						std::uint16_t jis;
						bool plane2;
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
							if(*fromNext < 0x0080) {
								*toNext++ = mask8Bit(*fromNext++);
								continue;
							}

							const Char* next;
							switch(convertUCStoX0213(boost::make_iterator_range(fromNext, boost::const_end(from)), next, eob(*this), jis, plane2)) {
							case COMPLETED:
								if(next == fromNext) {
									assert(!eob(*this));	// pending...
									return COMPLETED;
								}
								break;
							case UNMAPPABLE_CHARACTER:
								if(0 == (jis = convertUCStoKANA(*fromNext))) {
									if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
										*toNext = properties().substitutionCharacter();
									else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
										--toNext;
									else
										return (fromNext = next), UNMAPPABLE_CHARACTER;
								} else {
									assert(jis < 0x0100);	// kana
									*toNext = mask8Bit(jis);
								}
								++toNext;
								++fromNext;
								continue;
							case MALFORMED_INPUT:
								return (fromNext = next), MALFORMED_INPUT;
							}

							// double-byte
							if(toNext + 1 == boost::end(to))
								break;	// insufficient buffer
							shiftCode(jis, toNext, plane2);
							std::advance(toNext, 2);
							fromNext = next;
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					template<> Encoder::Result InternalEncoder<ShiftJis2004>::doToUnicode(State&,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext < 0x80)	// ASCII
								*toNext = *fromNext;
							else if(*fromNext >= 0xa1 && *fromNext <= 0xdf)	// kana
								*toNext = convertKANAtoUCS(*fromNext);
							else if(*fromNext == 0xa0) {	// illegal byte
								if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*toNext = properties().substitutionCharacter();
								else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									--toNext;
								else
									return UNMAPPABLE_CHARACTER;
							} else {
								if(fromNext + 1 < boost::const_end(from) && (fromNext[1] >= 0x40 && fromNext[1] <= 0xfc && fromNext[1] != 0x7f)) {	// double byte
									bool plane2;
									const std::uint16_t jis = unshiftCodeX0213(fromNext, plane2);
									const CodePoint ucs = !plane2 ? convertX0213Plane1toUCS(jis) : convertX0213Plane2toUCS(jis);

									if(ucs == text::REPLACEMENT_CHARACTER) {	// unmappable
										if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
											--toNext;
										else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS)
											return UNMAPPABLE_CHARACTER;
										continue;
									} else if(ucs >= 0x010000ul && toNext + 1 >= boost::end(to))
										break;	// INSUFFICIENT_BUFFER

									if(ucs > 0x0010fffful) {	// a character uses two code points
										*toNext = maskUCS2(ucs >> 16);
										*++toNext = maskUCS2(ucs);
									} else if(ucs >= 0x00010000ul) {	// out of BMP
										Char* temp = toNext++;
										text::utf::encode(ucs, temp);
									} else {
										if(toNext > boost::begin(to) && (toNext[-1] == 0x02e9u && ucs == 0x02e5u) || (toNext[-1] == 0x02e5u && ucs == 0x02e9u)) {
											if(toNext + 1 >= boost::end(to))
												break;	// INSUFFICIENT_BUFFER
											*(toNext++) = text::ZERO_WIDTH_NON_JOINER;
										}
										*toNext = maskUCS2(ucs);
									}
									++fromNext;
								} else
									return MALFORMED_INPUT;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// EUC-JP /////////////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::Result InternalEncoder<EucJp>::doFromUnicode(State&,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext < 0x0080) {	// ASCII
								*toNext = mask8Bit(*fromNext);
								continue;
							}

							bool x0212 = false;
							std::uint16_t jis = convertUCStoX0208(*fromNext);
							if(jis == 0x00) {
								if((jis = convertUCStoX0212(*fromNext)) != 0x00)
									// JIS X 0212
									x0212 = true;
								else if(const Byte kana = convertUCStoKANA(*fromNext)) {
									// JIS X 0201 Kana
									if(toNext + 1 >= boost::end(to))
										return INSUFFICIENT_BUFFER;
									*toNext = SS2_8BIT;
									*++toNext = kana;
									continue;
								} else if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
									*toNext = properties().substitutionCharacter();
								else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
									--toNext;
								else
									return UNMAPPABLE_CHARACTER;
							} else if(toNext + 1 >= boost::end(to))
								return INSUFFICIENT_BUFFER;

							jis |= 0x8080;	// jis -> euc-jp
							if(!x0212) {	// JIS X 0208
								*toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis);
							} else if(toNext + 2 >= boost::end(to))
								return INSUFFICIENT_BUFFER;
							else {	// JIS X 0212
								*toNext = SS3_8BIT;
								*++toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis);
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					template<> Encoder::Result InternalEncoder<EucJp>::doToUnicode(State&,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext < 0x80)
								*toNext = *fromNext;
							else {
								const std::size_t bytes = (*fromNext != SS3_8BIT) ? 2 : 3;
								if(std::next(fromNext, bytes) > boost::const_end(from))
									return MALFORMED_INPUT;
								else if(*fromNext == SS2_8BIT)	// SS2 -> JIS X 0201 Kana
									*toNext = convertKANAtoUCS(fromNext[1]);
								else if(*fromNext == SS3_8BIT) {	// SS3 -> JIS X 0212
									const std::uint16_t jis = ((fromNext[1] << 8) | fromNext[2]) - 0x8080;
									*toNext = convertX0212toUCS(jis);
								} else {	// JIS X 0208
									const std::uint16_t jis = ((*fromNext << 8) | fromNext[1]) - 0x8080;
									*toNext = convertX0208toUCS(jis);
								}

								if(*toNext == text::REPLACEMENT_CHARACTER) {	// unmappable
									if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
										--toNext;
									else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS)
										return UNMAPPABLE_CHARACTER;
								}
								fromNext += bytes - 1;
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


					// EUC-JIS-2004 ///////////////////////////////////////////////////////////////////////////////////

					template<> Encoder::Result InternalEncoder<EucJis2004>::doFromUnicode(State&,
							const boost::iterator_range<Byte*>& to, Byte*& toNext, const boost::iterator_range<const Char*>& from, const Char*& fromNext) {
						std::uint16_t jis;
						bool plane2 = false;
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						while(toNext < boost::end(to) && fromNext < boost::const_end(from)) {
							if(*fromNext < 0x0080) {	// ASCII
								*toNext++ = mask8Bit(*fromNext++);
								continue;
							} else if(toNext + 1 == boost::end(to))	// insufficient buffer
								break;

							// UCS -> JIS
							const Char* next;
							switch(convertUCStoX0213(boost::make_iterator_range(fromNext, boost::const_end(from)), next, eob(*this), jis, plane2)) {
								case COMPLETED:
									if(next == fromNext) {
										assert(!eob(*this));	// pending...
										return COMPLETED;
									}
								case UNMAPPABLE_CHARACTER:
									if(0 == (jis = convertUCStoKANA(*fromNext))) {
										if(substitutionPolicy() == REPLACE_UNMAPPABLE_CHARACTERS)
											*toNext = properties().substitutionCharacter();
										else if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
											--toNext;
										else
											return (fromNext = next), UNMAPPABLE_CHARACTER;
									} else {
										assert(jis < 0x0100);	// kana
										*toNext = SS2_8BIT;
										*++toNext = mask8Bit(jis);
									}
									++toNext;
									++fromNext;
									continue;
								case MALFORMED_INPUT:
									return (fromNext = next), MALFORMED_INPUT;
							}

							// JIS -> EUC-JIS
							jis += 0x8080;
							if(!plane2) {	// plane 1
								*toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis >> 0);
							} else {	// plane 2
								if(toNext + 2 == boost::end(to))
									break;	// insufficient buffer
								*toNext = SS3_8BIT;
								*++toNext = mask8Bit(jis >> 8);
								*++toNext = mask8Bit(jis >> 0);
							}
							++toNext;
							fromNext = next;
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}

					template<> Encoder::Result InternalEncoder<EucJis2004>::doToUnicode(State&,
							const boost::iterator_range<Char*>& to, Char*& toNext, const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {
						toNext = boost::begin(to);
						fromNext = boost::const_begin(from);
						for(; toNext < boost::end(to) && fromNext < boost::const_end(from); ++toNext, ++fromNext) {
							if(*fromNext < 0x80)
								*toNext = *fromNext;
							else {
								const std::ptrdiff_t bytes = (*fromNext != SS3_8BIT) ? 2 : 3;
								if(std::next(fromNext, bytes) > boost::const_end(from))
									return MALFORMED_INPUT;
								else if(*fromNext == SS2_8BIT)	// SS2 -> JIS X 0201 Kana
									*toNext = convertKANAtoUCS(from[1]);
								else if(*fromNext == SS3_8BIT) {	// SS3 -> plane-2
									const std::uint16_t jis = ((fromNext[1] << 8) | fromNext[2]) - 0x8080;
									const CodePoint ucs = convertX0213Plane2toUCS(jis);
									if(ucs != text::REPLACEMENT_CHARACTER) {
										if(ucs > 0x010000ul && toNext + 1 >= boost::end(to))
											break;	// INSUFFICIENT_BUFFER
										if(ucs > 0x0010fffful) {	// a character uses two code points
											*toNext = maskUCS2(ucs >> 16);
											*++toNext = maskUCS2(ucs >> 0);
										} else if(ucs >= 0x00010000ul) {	// out of BMP
											Char* temp = toNext++;
											text::utf::encode(ucs, temp);
										} else
											*toNext = maskUCS2(ucs);
									}
								} else {	// plane-1
									const std::uint16_t jis = ((*fromNext << 8) | fromNext[1]) - 0x8080;
									const CodePoint ucs = convertX0213Plane1toUCS(jis);
									if(ucs != text::REPLACEMENT_CHARACTER) {
										if(ucs > 0x0010fffful) {	// a character uses two code points
											*toNext = maskUCS2(ucs >> 16);
											*++toNext = maskUCS2(ucs >> 0);
										} else if(ucs >= 0x00010000ul) {	// out of BMP
											Char* temp = toNext++;
											text::utf::encode(ucs, temp);
										} else {
											if(toNext > boost::begin(to)
													&& ((toNext[-1] == 0x02e9u && ucs == 0x02e5u)
													|| (toNext[-1] == 0x02e5u && ucs == 0x02e9u))) {
												if(toNext + 1 >= boost::end(to))
													break;	// INSUFFICIENT_BUFFER
												*(toNext++) = text::ZERO_WIDTH_NON_JOINER;
											}
											*toNext = maskUCS2(ucs);
										}
									}
								}
								if(*toNext == text::REPLACEMENT_CHARACTER) {	// unmappable
									if(substitutionPolicy() == IGNORE_UNMAPPABLE_CHARACTERS)
										--toNext;
									else if(substitutionPolicy() != REPLACE_UNMAPPABLE_CHARACTERS)
										return UNMAPPABLE_CHARACTER;
								}
								std::advance(fromNext, bytes - 1);
							}
						}
						return (fromNext == boost::const_end(from)) ? COMPLETED : INSUFFICIENT_BUFFER;
					}


#define ASCENSION_IMPLEMENT_ISO_2022_JP_X(x, suffix)											\
	template<> Encoder::Result InternalEncoder<Iso2022##suffix>::doFromUnicode(State& state,	\
			const boost::iterator_range<Byte*>& to, Byte*& toNext,								\
			const boost::iterator_range<const Char*>& from, const Char*& fromNext) {			\
		if(state.empty())																		\
			state = EncodingState();															\
		if(EncodingState* const st = boost::any_cast<EncodingState>(&state))					\
			return convertUTF16toISO2022JPX(x,													\
				to, toNext, from, fromNext, *st, eob(*this), substitutionPolicy());				\
		else																					\
			throw Encoder::BadStateException();													\
	}																							\
	template<> Encoder::Result InternalEncoder<Iso2022##suffix>::doToUnicode(State& state,		\
			const boost::iterator_range<Char*>& to, Char*& toNext,								\
			const boost::iterator_range<const Byte*>& from, const Byte*& fromNext) {			\
		if(state.empty())																		\
			state = EncodingState();															\
		if(EncodingState* const st = boost::any_cast<EncodingState>(&state))					\
			return convertISO2022JPXtoUTF16(x,													\
				to, toNext, from, fromNext, *st, eob(*this), substitutionPolicy());				\
		else																					\
			throw Encoder::BadStateException();													\
	}


					// ISO-2022-JP ////////////////////////////////////////////////////////////////////////////////////

					ASCENSION_IMPLEMENT_ISO_2022_JP_X('0', Jp)


					// ISO-2022-JP-2 //////////////////////////////////////////////////////////////////////////////////

					ASCENSION_IMPLEMENT_ISO_2022_JP_X('2', Jp2)


					// ISO-2022-JP-2004 ///////////////////////////////////////////////////////////////////////////////

					ASCENSION_IMPLEMENT_ISO_2022_JP_X('4', Jp2004)


#ifndef ASCENSION_NO_MINORITY_ENCODINGS

					// ISO-2022-JP-1 //////////////////////////////////////////////////////////////////////////////////

					ASCENSION_IMPLEMENT_ISO_2022_JP_X('1', Jp1)


					// ISO-2022-JP-2004-Strict ////////////////////////////////////////////////////////////////////////

					ASCENSION_IMPLEMENT_ISO_2022_JP_X('s', Jp2004Strict)


					// ISO-2022-JP-2004-Compatible ////////////////////////////////////////////////////////////////////

					ASCENSION_IMPLEMENT_ISO_2022_JP_X('c', Jp2004Compatible)

#endif // !ASCENSION_NO_MINORITY_ENCODINGS


					// JisAutoDetector ////////////////////////////////////////////////////////////////////////////////

					inline std::shared_ptr<const EncodingProperties> detectShiftJis(const boost::iterator_range<const Byte*>& bytes, std::size_t& convertibleBytes, bool& foundKana) {
						bool jis2004 = false;
						foundKana = false;
						auto p(boost::const_begin(bytes));
						for(; p < boost::const_end(bytes); ++p) {
							if(*p == ESC)	// Shift_JIS can't have an ESC
								break;
							else if(*p < 0x80)	// ASCII is ok
								continue;
							else if(*p >= 0xa1 && *p <= 0xdf)	// JIS X 0201 kana
								foundKana = true;
							else if(p < boost::const_end(bytes) - 1) {	// 2-byte character?
								if(*p < 0x81 || *p > 0xfc || (*p > 0x9f && *p < 0xe0))
									break;	// illegal lead byte
								else if(p[1] < 0x40 || p[1] > 0xfc || p[1] == 0x7f)
									break;	// illegal trail byte

								bool plane2;
								if(!jis2004) {
									if(convertX0208toUCS(unshiftCodeX0208(p)) == text::REPLACEMENT_CHARACTER) {
										const std::uint16_t jis = unshiftCodeX0213(p, plane2);
										if(!plane2 && convertX0213Plane1toUCS(jis) == text::REPLACEMENT_CHARACTER)
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
						convertibleBytes = std::distance(boost::const_begin(bytes), p);
						if(jis2004)
							return installer.SHIFT_JIS_2004;
						else
							return installer.SHIFT_JIS;
					}

					inline std::shared_ptr<const EncodingProperties> detectEucJp(const boost::iterator_range<const Byte*>& bytes, std::size_t& convertibleBytes, bool& foundKana) {
						bool jis2004 = false;
						foundKana = false;
						auto p(boost::const_begin(bytes));
						for(; p < boost::const_end(bytes); ++p) {
							if(*p == ESC)	// EUC-JP can't have an ESC
								break;
							else if(*p < 0x80)	// ASCII is ok
								continue;
							else if(*p == SS2_8BIT) {	// SS2 introduces JIS X 0201 kana
								if(p + 1 >= boost::const_end(bytes) || p[1] < 0xa0 || p[1] > 0xe0)
									break;
								foundKana = true;
								++p;
							} else if(*p == SS3_8BIT) {	// SS3 introduces JIS X 0212 or JIS X 0213 plane2
								if(p + 2 >= boost::const_end(bytes))
									break;
								std::uint16_t jis = p[1] << 8 | p[2];
								if(jis < 0x8080)
									break;	// unmappable
								jis -= 0x8080;
								if(convertX0212toUCS(jis) != text::REPLACEMENT_CHARACTER) {
									if(jis2004)
										break;
//									cp = CPEX_JAPANESE_EUC;
								} else if(convertX0213Plane2toUCS(jis) != text::REPLACEMENT_CHARACTER) {
									if(!jis2004)
										break;
									jis2004 = true;
								} else
									break;
								p += 2;
							} else if(p < boost::const_end(bytes) - 1) {	// 2-byte character
								std::uint16_t jis = *p << 8 | p[1];
								if(jis <= 0x8080)
									break;
								jis -= 0x8080;
								if(convertX0208toUCS(jis) == text::REPLACEMENT_CHARACTER) {
									if(convertX0213Plane1toUCS(jis) != text::REPLACEMENT_CHARACTER) {
//										if(cp == CPEX_JAPANESE_EUC)
//											break;
										jis2004 = true;
									} else
										break;
								}
								++p;
							} else
								break;
						}
						convertibleBytes = std::distance(boost::const_begin(bytes), p);
						if(jis2004)
							return installer.EUC_JIS_2004;
						else
							return installer.EUC_JP;
					}

					inline std::shared_ptr<const EncodingProperties> detectIso2022Jp(const boost::iterator_range<const Byte*>& bytes, std::size_t& convertibleBytes, bool& foundKana) {
						char x = '0';	// ISO-2022-JP-X
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
						bool x0208 = false;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						foundKana = false;
						auto p(boost::const_begin(bytes));
						for(; p < boost::const_end(bytes); ++p) {
							if(*p >= 0x80)	// 8-bit
								break;
							else if(*p == ESC) {
								if(p + 2 >= boost::const_end(bytes))
									break;
								if(std::memcmp(p + 1, "(J", 2) == 0 || std::memcmp(p + 1, "(I", 2) == 0) {	// JIS X 0201
									p += 2;
									foundKana = true;
								} else if(std::memcmp(p + 1, "$@", 2) == 0 || std::memcmp(p + 1, "$B", 2) == 0) {	// JIS X 0208
									p += 2;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
									x0208 = true;
									if(x == '4')
										x = 'c';
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
								} else if(std::memcmp(p + 1, "$A", 2) == 0		// GB2312
										|| std::memcmp(p + 1, ".A", 2) == 0		// ISO-8859-1
										|| std::memcmp(p + 1, ".F", 2) == 0) {	// ISO-8859-7
									if(x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
											|| x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
										)
										break;
									x = '2';
									p += 2;
								} else if(p + 3 < boost::const_end(bytes)) {
									if(std::memcmp(p + 1, "$(D", 3) == 0) {	// JIS X 0212
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
									} else if(std::memcmp(p + 1, "$(C", 3) == 0) {	// KS C 5601
										if(x == '4'
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
												|| x == 'c'
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
											)
											break;
										x = '2';
										p += 3;
									} else if(std::memcmp(p + 1, "$(", 2) == 0
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

						convertibleBytes = std::distance(boost::const_begin(bytes), p);
						std::shared_ptr<const EncodingProperties> result;
						switch(x) {
							case '0':
								result = installer.ISO_2022_JP;
								break;
							case '2':
								result = installer.ISO_2022_JP_2;
								break;
							case '4':
								result = installer.ISO_2022_JP_2004;
								break;
#ifndef ASCENSION_NO_MINORITY_ENCODINGS
							case '1':
								result = installer.ISO_2022_JP_1;
								break;
							case 'c':
								result = installer.ISO_2022_JP_2004_COMPATIBLE;
								break;
#endif // !ASCENSION_NO_MINORITY_ENCODINGS
						}
						assert(result.get() != nullptr);
						return result;
					}

					/// @see EncodingDetector#doDetector
					std::tuple<MIBenum, std::string, std::size_t> JisAutoDetector::doDetect(const boost::iterator_range<const Byte*>& bytes) const BOOST_NOEXCEPT {
						MIBenum mib = fundamental::UTF_8;
						std::string name("UTF-8");
						std::size_t score = 0;

						// first, test Unicode
						if(const std::shared_ptr<const EncodingDetector> unicodeDetector = EncodingDetector::forName("UnicodeAutoDetect")) {
							std::tie(mib, name, score) = unicodeDetector->detect(bytes);
							if(score == boost::size(bytes))
								return std::make_tuple(mib, name, score);
						}

						bool foundKana;
						std::size_t score2;
						std::shared_ptr<const EncodingProperties> result2(detectShiftJis(bytes, score2, foundKana));
						if(score2 > score) {
							mib = result2->mibEnum();
							name = result2->name();
							score = score2;
						}
						if(score < boost::size(bytes) || foundKana) {
							result2 = detectEucJp(bytes, score2, foundKana);
							if(score2 > score) {
								mib = result2->mibEnum();
								name = result2->name();
								score = score2;
							}
							if(score < boost::size(bytes) || foundKana) {
								result2 = detectIso2022Jp(bytes, score2, foundKana);
								if(score2 > score) {
									mib = result2->mibEnum();
									name = result2->name();
									score = score2;
								}
							}
						}

						return std::make_tuple(mib, name, score);
					}
				}
			}
		}
	}
}

#endif // !ASCENSION_NO_STANDARD_ENCODINGS
