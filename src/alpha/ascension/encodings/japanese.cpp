/**
 * @file japanese.cpp
 * @author exeal
 * @date 2004-2007
 *
 * Implementation of Japanese encoders.
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
#include "../encoder.hpp"
#include <memory>		// std.auto_ptr
#include <map>
#include <algorithm>	// std.binary_search
using namespace ascension;
using namespace ascension::encoding;
using namespace ascension::unicode;
using namespace std;

// registry
namespace {
	ASCENSION_BEGIN_ENCODER_CLASS(ShiftJISEncoder, standard::SHIFT_JIS, "Shift_JIS")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(2)
		ASCENSION_ENCODER_ALIASES("MS_Kanji\0" "csShiftJIS\0")
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(EUCJPEncoder, standard::EUC_JP, "EUC-JP")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(3)
		ASCENSION_ENCODER_ALIASES("Extended_UNIX_Code_Packed_Format_for_Japanese\0" "csEUCPkdFmtJapanese\0")
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JPEncoder, standard::ISO_2022_JP, "ISO-2022-JP")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(8)
		ASCENSION_ENCODER_ALIASES("csISO2022JP\0")
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP2Encoder, standard::ISO_2022_JP_2, "ISO-2022-JP-2")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
		ASCENSION_ENCODER_ALIASES("csISO2022JP2\0")
	ASCENSION_END_ENCODER_CLASS()
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP1Encoder, extended::ISO_2022_JP_1, "ISO-2022-JP-1")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP2004Encoder, extended::ISO_2022_JP_2004, "ISO-2022-JP-2004")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP2004StrictEncoder, extended::ISO_2022_JP_2004_STRICT, "ISO-2022-JP-2004-Strict")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP2004CompatibleEncoder, extended::ISO_2022_JP_2004_COMPATIBLE, "ISO-2022-JP-2004-Compatible")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP3Encoder, extended::ISO_2022_JP_3, "ISO-2022-JP-3")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP3StrictEncoder, extended::ISO_2022_JP_3_STRICT, "ISO-2022-JP-3-Strict")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ISO2022JP3CompatibleEncoder, extended::ISO_2022_JP_3_COMPATIBLE, "ISO-2022-JP-3-Compatible")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(9)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(ShiftJIS2004Encoder, extended::SHIFT_JIS_2004, "Shift_JIS-2004")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(2)
	ASCENSION_END_ENCODER_CLASS()
	ASCENSION_BEGIN_ENCODER_CLASS(EUCJIS2004Encoder, extended::EUC_JIS_2004, "EUC-JIS-2004")
		ASCENSION_ENCODER_MAXIMUM_NATIVE_BYTES(3)
	ASCENSION_END_ENCODER_CLASS()
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
	ASCENSION_DEFINE_ENCODING_DETECTOR(JISAutoDetector, EncodingDetector::JIS_DETECTOR, "JISAutoDetect");

	struct Installer {
		Installer() {
			Encoder::registerEncoder(auto_ptr<Encoder>(new ShiftJISEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new EUCJPEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JPEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP2Encoder));
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP1Encoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP2004Encoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP2004StrictEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP2004CompatibleEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP3Encoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP3StrictEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ISO2022JP3CompatibleEncoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new ShiftJIS2004Encoder));
			Encoder::registerEncoder(auto_ptr<Encoder>(new EUCJIS2004Encoder));
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
			EncodingDetector::registerDetector(auto_ptr<EncodingDetector>(new JISAutoDetector));
		}
	} installer;
} // namespace @0

namespace {
	inline ushort jk(byte ku, byte ten) throw() {return ((ku << 8) | ten) + 0x2020;}
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
	inline bool isISO2022JP3(MIBenum mib) throw() {
		return mib >= extended::ISO_2022_JP_3 && mib <= extended::ISO_2022_JP_3_COMPATIBLE;
	}
	inline bool isISO2022JP2004(MIBenum mib) throw() {
		return mib >= extended::ISO_2022_JP_2004 && mib <= extended::ISO_2022_JP_2004_COMPATIBLE;
	}
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */

	// JIS <-> UCS 変換テーブル (make_table.pl より作成したファイルを切り刻んだもの)。
	// JIS X 0213 については両方向とも 32 ビット数値でテーブルを作成した。
	// UCS 側で 0x0000-0xFFFF はそのまま UCS-2、0x10000-0x10FFFF は UCS-4、0xFFFFFFF 以上は UCS-2 2 文字を表す
	// JIS 側では

	const uchar ESC = '\x1B';
	const uchar SS2 = static_cast<uchar>(0x8EU);
	const uchar SS3 = static_cast<uchar>(0x8FU);
	const Char RP__CH = REPLACEMENT_CHARACTER;
	const uchar N__A = UNMAPPABLE_NATIVE_CHARACTER;
	const Char JISX0208toUCS_2121[] = {	// 0x2121-0x2840
		#include "Jis\JISX0208toUCS_2121"
	};
	const Char JISX0208toUCS_3021[] = {	// 0x3021-0x4F53
		#include "Jis\JISX0208toUCS_3021"
	};
	const Char JISX0208toUCS_5021[] = {	// 0x5021-0x7426
		#include "Jis\JISX0208toUCS_5021"
	};
	const ushort UCStoJISX0208_00A2[] = {	// U+00A2-U+00F7
		#include "Jis\UCStoJISX0208_00A2"
	};
	const ushort UCStoJISX0208_0391[] = {	// U+0391-U+0451
		#include "Jis\UCStoJISX0208_0391"
	};
	const ushort UCStoJISX0208_2010[] = {	// U+2010-U+2312
		#include "Jis\UCStoJISX0208_2010"
	};
	const ushort UCStoJISX0208_2500[] = {	// U+2500-U+266F
		#include "Jis\UCStoJISX0208_2500"
	};
	const ushort UCStoJISX0208_3000[] = {	// U+3000-U+30FE
		#include "Jis\UCStoJISX0208_3000"
	};
	const ushort UCStoJISX0208_4E00[] = {	// U+4E00-U+9FA0
		#include "Jis\UCStoJISX0208_4E00"
	};
	const ushort UCStoJISX0208_FF01[] = {	// U+FF01-U+FFE5
		#include "Jis\UCStoJISX0208_FF01"
	};
	const Char JISX0212toUCS_222F[] = {	// 0x222F-0x2271
		#include "Jis\JISX0212toUCS_222F"
	};
	const Char JISX0212toUCS_2661[] = {	// 0x2661-0x2B77
		#include "Jis\JISX0212toUCS_2661"
	};
	const Char JISX0212toUCS_3021[] = {	// 0x3021-0x6D63
		#include "Jis\JISX0212toUCS_3021"
	};
	const ushort UCStoJISX0212_007E[] = {	// U+007E-U+045F
		#include "Jis\UCStoJISX0212_007E"
	};
	const ushort UCStoJISX0212_2116[] = {	// U+2116-U+2122
		#include "Jis\UCStoJISX0212_2116"
	};
	const ushort UCStoJISX0212_4E02[] = {	// U+4E02-U+9FA5
		#include "Jis\UCStoJISX0212_4E02"
	};
	const ulong JISX0213P1toUCS_2121[] = {	// 0x2121-0x2F7E
		#include "Jis\JISX0213P1toUCS_2121"
	};
	const ulong JISX0213P1toUCS_4F54[] = {	// 0x4F54-0x4F7E
		#include "Jis\JISX0213P1toUCS_4F54"
	};
	const ulong JISX0213P1toUCS_7427[] = {	// 0x7427-0x7E7E
		#include "Jis\JISX0213P1toUCS_7427"
	};
	const ulong JISX0213P2toUCS_2121[] = {	// 0x2121-0x217E
		#include "Jis\JISX0213P2toUCS_2121"
	};
	const ulong JISX0213P2toUCS_2321[] = {	// 0x2321-0x257E
		#include "Jis\JISX0213P2toUCS_2321"
	};
	const ulong JISX0213P2toUCS_2821[] = {	// 0x2821-0x287E
		#include "Jis\JISX0213P2toUCS_2821"
	};
	const ulong JISX0213P2toUCS_2C21[] = {	// 0x2C21-0x2F7E
		#include "Jis\JISX0213P2toUCS_2C21"
	};
	const ulong JISX0213P2toUCS_6E21[] = {	// 0x6E21-0x7E76
		#include "Jis\JISX0213P2toUCS_6E21"
	};
	const ulong UCStoJISX0213_00A0[] = {	// U+00A0-U+0451
		#include "Jis\UCStoJISX0213_00A0"
	};
	const ulong UCStoJISX0213_1E3E[] = {	// U+1E3E-U+29FB
		#include "Jis\UCStoJISX0213_1E3E"
	};
	const ulong UCStoJISX0213_3000[] = {	// U+3000-U+6568
		#include "Jis\UCStoJISX0213_3000"
	};
	const ulong UCStoJISX0213_F91D[] = {	// U+F91D-U+FA6A
		#include "Jis\UCStoJISX0213_F91D"
	};
	const ulong UCStoJISX0213_FE45[] = {	// U+FE45-U+FFE5
		#include "Jis\UCStoJISX0213_FE45"
	};

	// ISO-2022-JP-3 の禁止漢字 (JIS X 0213:2000 附属書 2 表 1)
	const ushort ProhibitedIdeographs_2000[] = {	// 不連続部分
		jk( 3,26),jk( 3,27),jk( 3,28),jk( 3,29),jk( 3,30),jk( 3,31),	// <記号>
		jk( 3,32),
		jk( 3,59),jk( 3,60),jk( 3,61),jk( 3,62),jk( 3,63),jk( 3,64),	// <記号>
		jk( 3,91),jk( 3,92),jk( 3,93),jk( 3,94),
		jk( 4,84),jk( 4,85),jk( 4,86),jk( 8,87),jk( 4,88),jk( 4,89),	// <平仮名>
		jk( 4,90),jk( 4,91),
		jk( 5,87),jk( 5,88),jk( 5,89),jk( 5,90),jk( 5,91),jk( 5,92),	// <片仮名>
		jk( 5,93),jk( 5,94),
		jk( 6,25),jk( 6,26),jk( 6,27),jk( 6,28),jk( 6,29),jk( 6,30),	// <トランプ>
		jk( 6,31),jk( 6,32),
												jk(13,83),jk(13,88),	// 　　　　∮∟
		jk(13,89),jk(13,93),jk(13,94),									// ⊿・・
														  jk(16, 2),	// 唖
		jk(16,19),jk(16,79),jk(17,58),jk(17,75),jk(17,79),jk(18, 3),	// 鯵逸謁焔縁横
		jk(18, 9),jk(18,10),jk(18,11),jk(18,25),jk(18,50),jk(18,89),	// 鴬鴎黄温禍悔
		jk(19, 4),jk(19,20),jk(19,21),jk(19,34),jk(19,41),jk(19,69),	// 海慨概蛎撹喝
		jk(19,73),jk(19,76),jk(19,86),jk(19,90),jk(20,18),jk(20,33),	// 渇褐竃噛寛漢
		jk(20,35),jk(20,50),jk(20,79),jk(20,91),jk(21, 7),jk(21,85),	// 潅諌器既祈虚
		jk(22, 2),jk(22,31),jk(22,33),jk(22,38),jk(22,48),jk(22,64),	// 侠郷響尭勤謹
		jk(22,77),jk(23,16),jk(23,39),jk(23,59),jk(23,66),jk(24, 6),	// 躯薫掲頚撃研
		jk(24,20),jk(25,60),jk(25,77),jk(25,82),jk(25,85),jk(27, 6),	// 鹸砿麹穀黒殺
		jk(27,67),jk(27,75),jk(28,40),jk(28,41),jk(28,49),jk(28,50),	// 祉視屡蕊煮社
		jk(28,52),jk(29,11),jk(29,13),jk(29,43),jk(29,75),jk(29,77),	// 者繍臭祝暑渚
		jk(29,79),jk(29,80),jk(29,84),jk(30,36),jk(30,45),jk(30,53),	// 緒署諸渉祥蒋
		jk(30,63),jk(30,85),jk(31,32),jk(31,57),jk(32, 5),jk(32,65),	// 醤状神靭瀬節
		jk(32,70),jk(33, 8),jk(33,36),jk(33,46),jk(33,56),jk(33,63),	// 蝉賎祖僧層掻
		jk(33,67),jk(33,93),jk(33,94),jk(34, 3),jk(34, 8),jk(34,45),	// 巣増憎贈即騨
		jk(34,86),jk(35,18),jk(35,29),jk(35,86),jk(35,88),jk(36, 7),	// 琢嘆箪猪著徴
		jk(36, 8),jk(36,45),jk(36,47),jk(36,59),jk(36,87),jk(37,22),	// 懲塚掴壷禎填
		jk(37,31),jk(37,52),jk(37,55),jk(37,78),jk(37,83),jk(37,88),	// 顛都砺梼涛祷
		jk(38,33),jk(38,34),jk(38,45),jk(38,81),jk(38,86),jk(39,25),	// 徳涜突難迩嚢
		jk(39,63),jk(39,72),jk(40,14),jk(40,16),jk(40,43),jk(40,53),	// 梅蝿溌醗繁晩
		jk(40,60),jk(40,74),jk(41,16),jk(41,48),jk(41,49),jk(41,50),	// 卑碑桧賓頻敏
		jk(41,51),jk(41,78),jk(42, 1),jk(42,27),jk(42,29),jk(42,57),	// 瓶侮福併塀勉
		jk(42,66),jk(43,43),jk(43,47),jk(43,72),jk(43,74),jk(43,89),	// 歩頬墨毎槙侭
		jk(44,40),jk(44,45),jk(44,65),jk(44,89),jk(45,20),jk(45,58),	// 免麺戻薮祐遥
		jk(45,73),jk(45,74),jk(45,83),jk(46,20),jk(46,26),jk(46,48),	// 莱頼欄隆虜緑
		jk(46,62),jk(46,64),jk(46,81),jk(46,82),jk(46,93),jk(47, 3),	// 涙類暦歴練錬
		jk(47,13),jk(47,15),jk(47,22),jk(47,25),jk(47,26),jk(47,31),	// 廊朗篭蝋郎録
							jk(48,54),jk(52,68),jk(57,88),jk(58,25),	// 　　儘壺攪攅
		jk(59,56),jk(59,77),jk(62,25),jk(62,85),jk(63,70),jk(64,86),	// 檜檮濤灌煕瑶
		jk(66,72),jk(66,74),jk(67,62),jk(68,38),jk(73, 2),jk(73,14),	// 礦礪竈籠蘂藪
		jk(73,58),jk(74, 4),jk(75,61),jk(76,45),jk(77,78),jk(80,55),	// 蠣蠅諫賤邇靱
		jk(80,84),jk(82,45),jk(82,84),jk(84, 1),jk(84, 2),jk(84, 3),	// 頸鰺鶯堯槇遙
		jk(84, 4),jk(84, 5),jk(84, 6),									// 瑤凜熙
	};
	const ushort ProhibitedIdeographs_2004[] = {	// ISO-2022-JP-2004 の禁止漢字 (JIS X0213:2004 附属書 2 表 2)
		jk(14, 1),jk(15,94),jk(17,19),jk(22,70),jk(23,50),jk(28,24),	// ・・嘘倶繋叱
		jk(33,73),jk(38,61),jk(39,77),jk(47,52),jk(47,94),jk(53,11),	// 痩呑剥・・妍
		jk(54, 2),jk(54,58),jk(84, 7),jk(94,90),jk(94,91),jk(94,92),	// 屏并・・・・
		jk(94,93),jk(94,94)												// ・・
	};
	const ushort CJK_EXT_B_UCS[] = {
		0x000B,0x0089,0x00A2,0x00A4,0x01A2,0x0213,0x032B,0x0371,0x0381,0x03F9,0x044A,0x0509,0x05D6,0x0628,0x074F,0x0807,
		0x083A,0x08B9,0x097C,0x099D,0x0AD3,0x0B1D,0x0B9F,0x0D45,0x0DE1,0x0E64,0x0E6D,0x0E95,0x0F5F,0x1201,0x123D,0x1255,
		0x1274,0x127B,0x12D7,0x12E4,0x12FD,0x131B,0x1336,0x1344,0x13C4,0x146D,0x146E,0x15D7,0x1647,0x16B4,0x1706,0x1742,
		0x18BD,0x19C3,0x1C56,0x1D2D,0x1D45,0x1D62,0x1D78,0x1D92,0x1D9C,0x1DA1,0x1DB7,0x1DE0,0x1E33,0x1E34,0x1F1E,0x1F76,
		0x1FFA,0x217B,0x2218,0x231E,0x23AD,0x26F3,0x285B,0x28AB,0x298F,0x2AB8,0x2B46,0x2B4F,0x2B50,0x2BA6,0x2C1D,0x2C24,
		0x2DE1,0x31B6,0x31C3,0x31C4,0x31F5,0x3372,0x33D0,0x33D2,0x33D3,0x33D5,0x33DA,0x33DF,0x33E4,0x344A,0x344B,0x3451,
		0x3465,0x34E4,0x355A,0x3594,0x35C4,0x3638,0x3639,0x363A,0x3647,0x370C,0x371C,0x373F,0x3763,0x3764,0x37E7,0x37FF,
		0x3824,0x383D,0x3A98,0x3C7F,0x3CFE,0x3D00,0x3D0E,0x3D40,0x3DD3,0x3DF9,0x3DFA,0x3F7E,0x4096,0x4103,0x41C6,0x41FE,
		0x43BC,0x4629,0x46A5,0x47F1,0x4896,0x4A4D,0x4B56,0x4B6F,0x4C16,0x4D14,0x4E0E,0x4E37,0x4E6A,0x4E8B,0x504A,0x5055,
		0x5122,0x51A9,0x51CD,0x51E5,0x521E,0x524C,0x542E,0x548E,0x54D9,0x550E,0x55A7,0x5771,0x57A9,0x57B4,0x59C4,0x59D4,
		0x5AE3,0x5AE4,0x5AF1,0x5BB2,0x5C4B,0x5C64,0x5DA1,0x5E2E,0x5E56,0x5E62,0x5E65,0x5EC2,0x5ED8,0x5EE8,0x5F23,0x5F5C,
		0x5FD4,0x5FE0,0x5FFB,0x600C,0x6017,0x6060,0x60ED,0x6270,0x6286,0x634C,0x6402,0x667E,0x66B0,0x671D,0x68DD,0x68EA,
		0x6951,0x696F,0x69DD,0x6A1E,0x6A58,0x6A8C,0x6AB7,0x6AFF,0x6C29,0x6C73,0x6CDD,0x6E40,0x6E65,0x6F94,0x6FF6,0x6FF7,
		0x6FF8,0x70F4,0x710D,0x7139,0x73DA,0x73DB,0x73FE,0x7410,0x7449,0x7614,0x7615,0x7631,0x7684,0x7693,0x770E,0x7723,
		0x7752,0x7985,0x7A84,0x7BB3,0x7BBE,0x7BC7,0x7CB8,0x7DA0,0x7E10,0x7FB7,0x808A,0x80BB,0x8277,0x8282,0x82F3,0x83CD,
		0x840C,0x8455,0x856B,0x85C8,0x85C9,0x86D7,0x86FA,0x8946,0x8949,0x896B,0x8987,0x8988,0x89BA,0x89BB,0x8A1E,0x8A29,
		0x8A43,0x8A71,0x8A99,0x8ACD,0x8ADD,0x8AE4,0x8BC1,0x8BEF,0x8D10,0x8D71,0x8DFB,0x8E1F,0x8E36,0x8E89,0x8EEB,0x8F32,
		0x8FF8,0x92A0,0x92B1,0x9490,0x95CF,0x967F,0x96F0,0x9719,0x9750,0x98C6,0x9A72,0x9DDB,0x9E15,0x9E3D,0x9E49,0x9E8A,
		0x9EC4,0x9EDB,0x9EE9,0x9FCE,0xA01A,0xA02F,0xA082,0xA0F9,0xA190,0xA38C,0xA437,0xA5F1,0xA602,0xA61A,0xA6B2,
	};
	const ulong CJK_EXT_B_JIS[] = {
		0x2E22,0x12121,0x1212B,0x1212E,0x12136,0x12146,0x12170,0x12179,0x12177,0x12322,0x12325,0x12327,0x12331,0x12332,0x12338,0x1233F,
		0x12341,0x1234A,0x12352,0x12353,0x12359,0x1235C,0x4F54,0x12377,0x1242A,0x1243A,0x12432,0x12431,0x1243D,0x12459,0x2F42,0x1245C,
		0x12463,0x1245E,0x1246B,0x1246A,0x12472,0x2F4C,0x12474,0x12475,0x12525,0x12532,0x2F60,0x1253E,0x12547,0x4F63,0x12555,0x12556,
		0x2F7B,0x1257E,0x12830,0x12837,0x12838,0x1283B,0x1283A,0x12845,0x12840,0x1283F,0x12848,0x1284A,0x1284B,0x4F6E,0x1285B,0x12866,
		0x1286C,0x12C22,0x17E53,0x12C2B,0x12C30,0x12C50,0x12C65,0x12C6D,0x12C72,0x12D24,0x12D32,0x12D29,0x12D2A,0x12D35,0x12D34,0x12D39,
		0x12D56,0x12E24,0x12D7D,0x753A,0x12E23,0x12E3A,0x12E42,0x12E3D,0x12E3C,0x12E44,0x12E47,0x12E49,0x12E43,0x12E55,0x12E57,0x12E56,
		0x12E5B,0x12E77,0x12E78,0x12F2A,0x7572,0x12F42,0x12F3F,0x12F43,0x12F40,0x12F59,0x12F4E,0x7629,0x7632,0x12F61,0x12F6A,0x12F69,
		0x12F70,0x12F75,0x16E23,0x16E34,0x7660,0x16E49,0x17475,0x16E5C,0x16E60,0x16E5F,0x16E5E,0x16F32,0x16F47,0x16F4D,0x16F61,0x16F64,
		0x17022,0x17033,0x17039,0x776C,0x17053,0x1707B,0x1712E,0x17130,0x17135,0x17144,0x1715D,0x17161,0x17166,0x17169,0x17175,0x17177,
		0x1717A,0x17221,0x17224,0x17223,0x17228,0x1722C,0x1723D,0x787E,0x17248,0x7929,0x1725B,0x7947,0x17275,0x17276,0x7954,0x17332,
		0x1733E,0x1733D,0x17340,0x17352,0x1735D,0x1735E,0x796E,0x17373,0x17374,0x17377,0x17375,0x1737D,0x1737B,0x17422,0x17424,0x17427,
		0x1742F,0x1742E,0x17435,0x17434,0x1743D,0x17442,0x1744F,0x17469,0x1746B,0x17472,0x17479,0x17535,0x1753A,0x17546,0x17556,0x17558,
		0x1755A,0x1755D,0x1755F,0x17563,0x1756A,0x17570,0x17573,0x7A5D,0x12544,0x17644,0x1764E,0x7B33,0x1765D,0x17675,0x17721,0x17722,
		0x1767E,0x7B49,0x17733,0x17736,0x17765,0x17764,0x1776B,0x1776E,0x17773,0x1782A,0x17829,0x1782C,0x7B6C,0x17834,0x1783C,0x1783E,
		0x17842,0x17856,0x17863,0x17877,0x17879,0x1787A,0x17925,0x1792F,0x17932,0x17939,0x17942,0x17948,0x7C49,0x17959,0x1795E,0x7C51,
		0x17966,0x1796B,0x1797A,0x1797E,0x17A21,0x17A2C,0x17A2F,0x17A50,0x17A4F,0x17A57,0x17A65,0x17A66,0x17A71,0x17A72,0x17A7E,0x17B21,
		0x17B2D,0x17B2C,0x17B36,0x17B37,0x17B3E,0x17B3D,0x17B4E,0x17B4F,0x17B57,0x17B5A,0x17B5C,0x17B5D,0x17B61,0x17B65,0x17B67,0x17B69,
		0x17B71,0x17C22,0x17C23,0x17C38,0x17C42,0x17C4C,0x17C56,0x17C59,0x17C5D,0x17C76,0x17D2C,0x17D4B,0x17D59,0x17D4C,0x17D5D,0x17D5B,
		0x17D67,0x17D70,0x17D6D,0x17E25,0x17E2B,0x17E29,0x17E35,0x17E32,0x7E66,0x17E58,0x17E5A,0x17E6E,0x17E70,0x17E72,0x17E76,
	};
	static map<ushort, ulong> cjkExtBtoJis;
	enum ISO2022JPCharset_G0 {
		ASCII, JIS_X_0201_ROMAN, /*JIS_X_0201_KANA,*/ JIS_X_0208, JIS_X_0212, JIS_X_0213_P1, JIS_X_0213_P2, GB2312, KSC5601
	};
	enum ISO2022JPCharset_G2 {
		UNDESIGNATED = KSC5601 + 1 , ISO_8859_1, ISO_8859_7
	};

	// ISO-2022-JP-3 の禁止漢字か
	inline bool isISO2022JP3ProhibitedIdeograph(ushort jis) {
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
			|| binary_search(ProhibitedIdeographs_2000, endof(ProhibitedIdeographs_2000), jis);
	}
	// JIS X 0213:2004 で追加された禁止漢字か
	inline bool isISO2022JP2004ProhibitedIdeograph(ushort jis) {
		return binary_search(ProhibitedIdeographs_2004, endof(ProhibitedIdeographs_2004), jis);
	}

	// JIS X 0201 Roman -> UCS 変換
	inline Char jisX0201RomanToUCS(uchar c) {
		if(c == 0x5C)					return 0x00A5;					// Yen Sign
		else if(c == 0x7E)				return 0x203E;					// Overline
		else if(c >= 0x20 && c <= 0x7D)	return c;						// 7-bit
		else							return REPLACEMENT_CHARACTER;	// invalid
	}

	// UCS -> JIS X 0201 Roman 変換
	inline uchar ucsToJISX0201Roman(Char c) {
		if(c >= 0x0020 && c <= 0x005B)		return mask8Bit(c);	// 7-bit
		else if(c >= 0x005D && c <= 0x007D)	return mask8Bit(c);	// 7-bit
		else if(c == 0x00A5)				return 0x5C;		// Yen Sign
		else if(c == 0x203E)				return 0x7E;		// Overline
		else								return 0x00;		// invalid
	}

	// JIS X 0201 Kana -> UCS 変換
	inline Char jisX0201KanaToUCS(uchar c) {
		return (c >= 0xA1 && c <= 0xDF) ? c + 0xFEC0 : REPLACEMENT_CHARACTER;
	}

	// UCS -> JIS X 0201 Kana 変換
	inline uchar ucsToJISX0201Kana(Char c) {
		return (c >= 0xFF61 && c <= 0xFF9F) ? mask8Bit(c - 0xFEC0) : 0x00;
	}

	// JIS X 0208 -> UCS 変換
	inline Char jisX0208ToUCS(ushort jis) {
		if(jis >= 0x2121 && jis < 0x2121 + countof(JISX0208toUCS_2121))			return JISX0208toUCS_2121[jis - 0x2121];
		else if(jis >= 0x3021 && jis < 0x3021 + countof(JISX0208toUCS_3021))	return JISX0208toUCS_3021[jis - 0x3021];
		else if(jis >= 0x5021 && jis < 0x5021 + countof(JISX0208toUCS_5021))	return JISX0208toUCS_5021[jis - 0x5021];
		else																	return REPLACEMENT_CHARACTER;
	}

	// UCS -> JIS X 0208 変換
	inline ushort ucsToJISX0208(Char c) {
		if(c >= 0x00A2 && c < 0x00A2 + countof(UCStoJISX0208_00A2))			return UCStoJISX0208_00A2[c - 0x00A2];
		else if(c >= 0x0391 && c < 0x0391 + countof(UCStoJISX0208_0391))	return UCStoJISX0208_0391[c - 0x0391];
		else if(c >= 0x2010 && c < 0x2010 + countof(UCStoJISX0208_2010))	return UCStoJISX0208_2010[c - 0x2010];
		else if(c >= 0x2500 && c < 0x2500 + countof(UCStoJISX0208_2500))	return UCStoJISX0208_2500[c - 0x2500];
		else if(c >= 0x3000 && c < 0x3000 + countof(UCStoJISX0208_3000))	return UCStoJISX0208_3000[c - 0x3000];
		else if(c >= 0x4E00 && c < 0x4E00 + countof(UCStoJISX0208_4E00))	return UCStoJISX0208_4E00[c - 0x4E00];
		else if(c >= 0xFF01 && c < 0xFF01 + countof(UCStoJISX0208_FF01))	return UCStoJISX0208_FF01[c - 0xFF01];
		else																return 0x0000;
	}

	// JIS X 0212 -> UCS 変換
	inline Char jisX0212ToUCS(ushort jis) {
		if(jis >= 0x222F && jis < 0x222F + countof(JISX0212toUCS_222F))			return JISX0212toUCS_222F[jis - 0x222F];
		else if(jis >= 0x2661 && jis < 0x2661 + countof(JISX0212toUCS_2661))	return JISX0212toUCS_2661[jis - 0x2661];
		else if(jis >= 0x3021 && jis < 0x3021 + countof(JISX0212toUCS_3021))	return JISX0212toUCS_3021[jis - 0x3021];
		else																	return REPLACEMENT_CHARACTER;
	}

	// UCS -> JIS X 0212 変換
	inline ushort ucsToJISX0212(Char c) {
		if(c >= 0x007E && c < 0x007E + countof(UCStoJISX0212_007E))			return UCStoJISX0212_007E[c - 0x007E];
		else if(c >= 0x2116 && c < 0x2116 + countof(UCStoJISX0212_2116))	return UCStoJISX0212_2116[c - 0x2116];
		else if(c >= 0x4E02 && c < 0x4E02 + countof(UCStoJISX0212_4E02))	return UCStoJISX0212_4E02[c - 0x4E02];
		else																return 0x0000;
	}

	// JIS X 0213 第 1 面 -> UCS 変換
	inline ulong jisX0213P1ToUCS(ushort jis) {
		if(jis >= 0x2121 && jis < 0x2121 + countof(JISX0213P1toUCS_2121))		return JISX0213P1toUCS_2121[jis - 0x2121];
		else if(jis >= 0x4F54 && jis < 0x4F54 + countof(JISX0213P1toUCS_4F54))	return JISX0213P1toUCS_4F54[jis - 0x4F54];
		else if(jis >= 0x7427 && jis < 0x7427 + countof(JISX0213P1toUCS_7427))	return JISX0213P1toUCS_7427[jis - 0x7427];
		else																	return jisX0208ToUCS(jis);
	}

	// JIS X 0213 第 2 面 -> UCS 変換
	inline ulong jisX0213P2ToUCS(ushort jis) {
		if(jis >= 0x2121 && jis < 0x2121 + countof(JISX0213P2toUCS_2121))		return JISX0213P2toUCS_2121[jis - 0x2121];
		else if(jis >= 0x2321 && jis < 0x2321 + countof(JISX0213P2toUCS_2321))	return JISX0213P2toUCS_2321[jis - 0x2321];
		else if(jis >= 0x2821 && jis < 0x2821 + countof(JISX0213P2toUCS_2821))	return JISX0213P2toUCS_2821[jis - 0x2821];
		else if(jis >= 0x2C21 && jis < 0x2C21 + countof(JISX0213P2toUCS_2C21))	return JISX0213P2toUCS_2C21[jis - 0x2C21];
		else if(jis >= 0x6E21 && jis < 0x6E21 + countof(JISX0213P2toUCS_6E21))	return JISX0213P2toUCS_6E21[jis - 0x6E21];
		else																	return 0x00000000;
	}

	// UCS -> JIS X 0213 変換 (eaten は入力時は ucs の文字数、出力時は変換に使用した文字数)
	inline ushort ucsToJISX0213(const Char* ucs, ptrdiff_t& eaten, bool& plane2) {
		assert(ucs != 0 && eaten != 0);
		if(cjkExtBtoJis.empty()) {
			for(size_t i = 0; i < countof(CJK_EXT_B_UCS); ++i)
				cjkExtBtoJis[CJK_EXT_B_UCS[i]] = CJK_EXT_B_JIS[i];
		}

		ulong jis = 0x0000;

		plane2 = false;
		if(eaten > 1) {
			if(ucs[1] == 0x309A) {	// <kana> + Combining Katakana-Hiragana Semi-Voiced Sound Mark
				switch(ucs[0]) {
				case 0x304B:	jis = 0x2477;	break;	// ka -> bidakuon nga
				case 0x304D:	jis = 0x2478;	break;	// ki -> bidakuon ngi
				case 0x304F:	jis = 0x2479;	break;	// ku -> bidakuon ngu
				case 0x3051:	jis = 0x247A;	break;	// ke -> bidakuon nge
				case 0x3053:	jis = 0x247B;	break;	// ko -> bidakuon ngo
				case 0x30AB:	jis = 0x2577;	break;	// ka -> bidakuon nga
				case 0x30AD:	jis = 0x2578;	break;	// ki -> bidakuon ngi
				case 0x30AF:	jis = 0x2579;	break;	// ku -> bidakuon ngu
				case 0x30B1:	jis = 0x257A;	break;	// ke -> bidakuon nge
				case 0x30B3:	jis = 0x257B;	break;	// ko -> bidakuon ngo
				case 0x30BB:	jis = 0x257C;	break;	// se -> ainu ce
				case 0x30C4:	jis = 0x257D;	break;	// tu -> ainu tu (tu)
				case 0x30C8:	jis = 0x257E;	break;	// to -> ainu to (tu)
				case 0x31F7:	jis = 0x2678;	break;	// small fu -> ainu p
				}
			} else if(ucs[1] == 0x0300) {	// X + Combining Grave Accent
				switch(ucs[0]) {
				case 0x00E6:	jis = 0x2B44;	break;	// ae
				case 0x0254:	jis = 0x2B48;	break;	// open o
				case 0x0259:	jis = 0x2B4C;	break;	// schwa
				case 0x025A:	jis = 0x2B4E;	break;	// schwa with hook
				case 0x028C:	jis = 0x2B4A;	break;	// turned v
				}
			} else if(ucs[1] == 0x0301) {	// X + Combining Acute Accent
				switch(ucs[0]) {
				case 0x0254:	jis = 0x2B49;	break;	// open o
				case 0x0259:	jis = 0x2B4D;	break;	// schwa
				case 0x025A:	jis = 0x2B4F;	break;	// schwa with hook
				case 0x028C:	jis = 0x2B4B;	break;	// turned v
				}
			} else if(ucs[0] == 0x02E9) {
				if(ucs[1] == 0x02E5)
					jis = 0x2B65;	// extra-low tone bar + extra-high tone bar -> rising symbol
				else if(ucs[1] == ZERO_WIDTH_NON_JOINER && eaten > 2 && ucs[2] == 0x02E5)
					jis = 0x2B64;	// just dependent extra-low tone bar
			} else if(ucs[0] == 0x02E5) {
				if(ucs[1] == 0x02E9)
					jis = 0x2B66;	// extra-high tone bar + extra-low tone bar -> falling symbol
				else if(ucs[1] == ZERO_WIDTH_NON_JOINER && eaten > 2 && ucs[2] == 0x02E9)
					jis = 0x2B60;	// just dependent extra-high tone bar
			}
			if(jis != 0x0000)
				eaten = 2;
		}
		if(jis == 0x0000) {
			if(ucs[0] >= 0x00A0 && ucs[0] < 0x00A0 + countof(UCStoJISX0213_00A0)) {
				jis = UCStoJISX0213_00A0[ucs[0] - 0x00A0]; eaten = 1;
			} else if(ucs[0] >= 0x1E3E && ucs[0] < 0x1E3E + countof(UCStoJISX0213_1E3E)) {
				jis = UCStoJISX0213_1E3E[ucs[0] - 0x1E3E]; eaten = 1;
			} else if(ucs[0] >= 0x3000 && ucs[0] < 0x3000 + countof(UCStoJISX0213_3000)) {
				jis = UCStoJISX0213_3000[ucs[0] - 0x3000]; eaten = 1;
			} else if(ucs[0] >= 0xF91D && ucs[0] < 0xF91D + countof(UCStoJISX0213_F91D)) {
				jis = UCStoJISX0213_F91D[ucs[0] - 0xF91D]; eaten = 1;
			} else if(ucs[0] >= 0xF91D && ucs[0] < 0xFE45 + countof(UCStoJISX0213_FE45)) {
				jis = UCStoJISX0213_FE45[ucs[0] - 0xFE45]; eaten = 1;
			} else if(eaten > 1 && surrogates::isHighSurrogate(ucs[0]) && surrogates::isLowSurrogate(ucs[1])) {
				const CodePoint cp = surrogates::decodeFirst(ucs, ucs + eaten);
				eaten = 1;
				if(cp >= 0x020000) {
					const map<ushort, ulong>::const_iterator it = cjkExtBtoJis.find(mask16Bit(cp - 0x020000));
					if(it != cjkExtBtoJis.end()) {
						eaten = 2;
						jis = it->second;
					}
				}
			} else
				eaten = 1;
		}

		plane2 = jis > 0xFFFF;
		return mask16Bit(jis);
	}

	// ISO-2022-JP-X -> UTF-16 変換ヘルパ
	Encoder::Result convertISO2022JPXToUTF16(MIBenum mib, Char* to, Char* toEnd, Char*& toNext,
			const uchar* from, const uchar* fromEnd, const uchar*& fromNext, Encoder::State* state, Encoder::Policy policy) {
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
		//
		// Encoder.State value allocation:
		// state := 0b  0..0 000S BBBB AAAA
		//  - AAAA : G0 as ISO2022JPCharset_G0
		//  - BBBB : G2 as ISO2022JPCharset_G2
		//  - S : 1 if G2 invoked by a SS2

		const Char* const beginning = to;
		ISO2022JPCharset_G0	g0 = (state != 0) ? static_cast<ISO2022JPCharset_G0>(*state & 0x000F) : ASCII;
		ISO2022JPCharset_G2	g2 = (state != 0) ? static_cast<ISO2022JPCharset_G2>((*state & 0x00F0) >> 4) : UNDESIGNATED;
		bool shiftedInG2 = (state != 0) ? ((*state & 0x0100) != 0) : false;
		Encoder* gb2312Encoder = 0;
		Encoder* ksc5601Encoder = 0;
		Encoder* iso88591Encoder = 0;
		Encoder* iso88597Encoder = 0;
		bool checkedGB2312 = false, checkedKSC5601 = false;

#define ASCENSION_HANDLE_UNMAPPABLE()	{							\
	if(policy == Encoder::IGNORE_UNMAPPABLE_CHARACTER)				\
		--to;														\
	else if(policy != Encoder::REPLACE_UNMAPPABLE_CHARACTER) {		\
		toNext = to;												\
		fromNext = from;											\
		if(state != 0)												\
			*state = g0 | (g2 << 4) | ((shiftedInG2 ? 1 : 0) << 8);	\
		return Encoder::UNMAPPABLE_CHARACTER;						\
	}																\
}

		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			if(*from == ESC) {	// expect esc. seq.
				if(from + 2 < fromEnd) {
					switch(from[1]) {
					case 'N':	shiftedInG2 = true; ++from; continue;	// SS2
					case '(':
						switch(from[2]) {
						case 'B':	g0 = ASCII; from += 2; continue;			// "(B" => ASCII
//						case 'I':	g0 = JIS_X_0201_KANA; from += 2; continue;	// "(I" => JIS X 0201 Kana
						case 'J':	g0 = JIS_X_0201_ROMAN; from += 2; continue;	// "(J" => JIS X 0201 Roman
						}
						break;
					case '$':
						switch(from[2]) {
						case '@':	g0 = JIS_X_0208; from += 2; continue;	// "$@" => JIS X 0208
						case 'A':	// "$A" => GB2312
							if(mib != standard::ISO_2022_JP_2) break;
							if(!checkedGB2312) {
								if(0 != (gb2312Encoder = Encoder::forMIB(standard::GB2312)))
									gb2312Encoder->setPolicy(policy);
							}
							if(gb2312Encoder == 0) break;
							g0 = GB2312; from += 2; continue;
						case 'B':	g0 = JIS_X_0208; from += 2; continue;	// "$B" => JIS X 0208
						case '(':
							if(from + 3 < fromEnd) {
								switch(from[3]) {
								case 'C':	// "$(C" => KSC5601
									if(mib != standard::ISO_2022_JP_2) break;
									if(!checkedKSC5601) {
										if(0 != (ksc5601Encoder = Encoder::forMIB(36)))
											ksc5601Encoder->setPolicy(policy);
									}
									if(ksc5601Encoder == 0) break;
									g0 = KSC5601; from += 3; continue;
								case 'D':	// "$(D" => JIS X 0212
									if(mib != standard::ISO_2022_JP_2
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
										&& mib != extended::ISO_2022_JP_1
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
										) break;
									g0 = JIS_X_0212; from += 3; continue;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
								case 'O':	// "$(O" => JIS X 0213 plane 1
									if(!isISO2022JP3(mib) && !isISO2022JP2004(mib)) break;
									g0 = JIS_X_0213_P1; from += 3; continue;
								case 'P':	// "$(P" => JIS X 0213 plane 2
									if(!isISO2022JP3(mib) && !isISO2022JP2004(mib)) break;
									g0 = JIS_X_0213_P2; from += 3; continue;
								case 'Q':	// "$(Q" => JIS X 0213 plane 1
									if(!isISO2022JP3(mib) && !isISO2022JP2004(mib)) break;
									g0 = JIS_X_0213_P1; from += 3; continue;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
								}
							}
						}
						break;
					case '.':
						if(mib == standard::ISO_2022_JP_2) {
							switch(from[1]) {
							case 'A':	g2 = ISO_8859_1; from += 2; continue;	// ".A" => ISO-8859-1 (G2)
							case 'F':	g2 = ISO_8859_7; from += 2; continue;	// ".F" => ISO-8859-7 (G2)
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

			if(*from <= 0x20 || (*from >= 0x80 && *from < 0xA0)) {	// C0 or C1
				if(*from == 0x0A || *from == 0x0D) {
					g0 = ASCII;
					g2 = UNDESIGNATED;
				}
				*to = *from;	// SI, SO, SS2 (1 byte) and SS3 (1 byte) are ignored
			} else if(shiftedInG2) {	// G2
				const uchar c = *from | 0x80;
				if(g2 == ISO_8859_1) {	// ISO-8859-1
					if(iso88591Encoder == 0)
						(iso88591Encoder = Encoder::forMIB(fundamental::ISO_8859_1))->setPolicy(policy);
					uchar* next;
					const Encoder::Result r = iso88591Encoder->toUnicode(to, toEnd, toNext, &c, &c + 1, next);
					if(r != Encoder::COMPLETED) {
						fromNext = from;
						if(state != 0)
							*state = g0 | (g2 << 4) | (1 << 8);
						return r;
					}
				} else if(g2 == ISO_8859_7) {	// ISO-8859-7
					if(iso88597Encoder == 0)
						(iso88597Encoder = Encoder::forMIB(standard::ISO_8859_7))->setPolicy(policy);
					uchar* next;
					const Encoder::Result r = iso88597Encoder->toUnicode(to, toEnd, toNext, &c, &c + 1, next);
					if(r != Encoder::COMPLETED) {
						fromNext = from;
						if(state != 0)
							*state = g0 | (g2 << 4) | (1 << 8);
						return r;
					}
				} else {	// G2 is not designated
					toNext = to;
					fromNext = from;
					if(state != 0)
						*state = g0 | (g2 << 4) | (1 << 8);
					return Encoder::MALFORMED_INPUT;
				}
				shiftedInG2 = false;
			} else if(g0 == JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
				*to = jisX0201RomanToUCS(*from);
				if(*to == REPLACEMENT_CHARACTER)
					ASCENSION_HANDLE_UNMAPPABLE()
/*			} else if(g0 == JIS_X_0201_KANA) {	// JIS X 0201-Kana
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
*/			} else if(g0 == ASCII) {	// ASCII
				if(*from >= 0x80)
					ASCENSION_HANDLE_UNMAPPABLE()
				*to = *from;
			} else if(from + 1 >= fromEnd) {	// the trail byte was not found
				toNext = to;
				fromNext = from;
				if(state != 0)
					*state = g0 | (g2 << 4) | (1 << 8);
				return Encoder::MALFORMED_INPUT;
			} else if(g0 == JIS_X_0208) {	// JIS X 0208:1978 or :1983
				const ushort jis = (*from << 8) | from[1];
				const Char ucs = jisX0208ToUCS(jis);
				if(ucs == REPLACEMENT_CHARACTER)
					ASCENSION_HANDLE_UNMAPPABLE()
				++from;
			} else if(g0 == JIS_X_0212) {	// JIS X 0212:1990
				const ushort jis = (*from << 8) | from[1];
				const Char ucs = jisX0212ToUCS(jis);
				if(ucs == REPLACEMENT_CHARACTER)
					ASCENSION_HANDLE_UNMAPPABLE()
				++from;
			} else if(g0 == GB2312 || g0 == KSC5601) {	// GB2312:1980 or KSC5601:1987
				const uchar buffer[2] = {*from | 0x80, from[1] | 0x80};
				const uchar* const bufferEnd = endof(buffer);
				const uchar* next;
				const Encoder::Result r = ((g0 == GB2312) ?
					gb2312Encoder : ksc5601Encoder)->toUnicode(to, toEnd, toNext, buffer, bufferEnd, next);
				if(r != Encoder::COMPLETED) {
					fromNext = from;
					if(state != 0)
						*state = g0 | (g2 << 4) | (1 << 8);
					return r;
				}
				from = next - 1;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
			} else if(g0 == JIS_X_0213_P1 || g0 == JIS_X_0213_P2) {	// JIS X 0213:2004 or :2000
				const ushort jis = (*from << 8) | from[1];
				CodePoint ucs = (g0 == JIS_X_0213_P1) ? jisX0213P1ToUCS(jis) : jisX0213P2ToUCS(jis);

				if(ucs == REPLACEMENT_CHARACTER) {
					if(policy == Encoder::IGNORE_UNMAPPABLE_CHARACTER) {
						--to;
						++from;
						continue;
					} else if(policy != Encoder::REPLACE_UNMAPPABLE_CHARACTER) {
						toNext = to;
						fromNext = from;
						return Encoder::UNMAPPABLE_CHARACTER;
					}
				}
				if(ucs > 0xFFFFU) {
					if(to + 1 >= toEnd)
						break;	// INSUFFICIENT_BUFFER
					if(ucs > 0x0010FFFF) {	// two UCS characters
						*to = maskUCS2(ucs >> 16);
						*++to = maskUCS2(ucs);
					} else
						surrogates::encode(ucs, to++);
				} else {
					if(to > beginning && ((to[-1] == L'\x02E9' && ucs == 0x02E5U) || (to[-1] == L'\x02E5' && ucs == 0x02E9U))) {
						if(to + 1 >= toEnd)
							break;	// INSUFFICIENT_BUFFER
						*(to++) = ZERO_WIDTH_NON_JOINER;
					}
					*to = maskUCS2(ucs);
				}
				++from;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
			}
		}
#undef ASCENSION_HANDLE_UNMAPPABLE
		toNext = to;
		fromNext = from;
		if(state != 0)
			*state = g0 | (g2 << 4) | (1 << 8);
		return (fromNext == fromEnd) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
	}

	// UTF-16 -> ISO-2022-JP-X 変換ヘルパ
	Encoder::Result convertUTF16ToISO2022JPX(MIBenum mib, uchar* to, uchar* toEnd, uchar*& toNext,
			const Char* from, const Char* fromEnd, const Char*& fromNext, Encoder::State* state, Encoder::Policy policy) {
		ISO2022JPCharset_G0 g0 = (state != 0) ? static_cast<ISO2022JPCharset_G0>(*state & 0x000F) : ASCII;
		ISO2022JPCharset_G2 g2 = (state != 0) ? static_cast<ISO2022JPCharset_G2>((*state & 0x00F0) >> 4) : UNDESIGNATED;
		int charset = ASCII;

		Encoder* iso88591Encoder = 0;
		Encoder* iso88597Encoder = 0;
		Encoder* gb2312Encoder = 0;
		Encoder* ksc5601Encoder = 0;
		if(mib == standard::ISO_2022_JP_2) {
			if(0 != (iso88591Encoder = Encoder::forMIB(fundamental::ISO_8859_1)))
				iso88591Encoder->setPolicy(policy);
			if(0 != (iso88597Encoder = Encoder::forMIB(standard::ISO_8859_7)))
				iso88597Encoder->setPolicy(policy);
			if(0 != (gb2312Encoder = Encoder::forMIB(standard::GB2312)))
				gb2312Encoder->setPolicy(policy);
			if(0 != (ksc5601Encoder = Encoder::forMIB(36)))
				ksc5601Encoder->setPolicy(policy);
		}

#define ASCENSION_HANDLE_UNMAPPABLE()							\
	if(policy == Encoder::REPLACE_UNMAPPABLE_CHARACTER) {		\
		jis = mbcs[0] = NATIVE_REPLACEMENT_CHARACTER;			\
		mbcs[1] = 1;											\
		charset = ASCII;										\
	} else if(policy == Encoder::IGNORE_UNMAPPABLE_CHARACTER) {	\
		--to;													\
		continue;												\
	} else {													\
		toNext = to;											\
		fromNext = from;										\
		if(state != 0)											\
			*state = g0 | (g2 << 4);							\
		return Encoder::UNMAPPABLE_CHARACTER;					\
	}

		ushort jis;
		uchar mbcs[2];
		uchar* dummy1;
		const Char* dummy2;
		for(; to < toEnd && from < fromEnd; ++to, ++from) {
			// first, convert '*from' into 'jis' or 'mbcs' buffer
			if(*from < 0x80) {
				mbcs[0] = mask8Bit(jis = *from);
				mbcs[1] = 0;
				charset = ASCII;
			} else if(0 != (jis = ucsToJISX0201Roman(*from)) && jis < 0x80)
				charset = /*(jis < 0x80) ?*/ JIS_X_0201_ROMAN /*: JIS_X_0201_KANA*/;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
			else if(isISO2022JP3(mib) || isISO2022JP2004(mib)) {
				ptrdiff_t eaten = fromEnd - from;
				bool plane2;
				jis = ucsToJISX0213(from, eaten, plane2);

				if(jis != UNMAPPABLE_NATIVE_CHARACTER) {
					charset = UNDESIGNATED;
					if(!plane2) {
						// try JIS X 0208 compatible sequence
						if((mib == extended::ISO_2022_JP_3_COMPATIBLE
								|| mib == extended::ISO_2022_JP_2004_COMPATIBLE)
								&& ucsToJISX0208(*from) != UNMAPPABLE_NATIVE_CHARACTER)
							charset = JIS_X_0208;
						else if((mib == extended::ISO_2022_JP_3_STRICT
								|| mib == extended::ISO_2022_JP_2004_STRICT)
								&& !isISO2022JP3ProhibitedIdeograph(jis)
								&& !isISO2022JP2004ProhibitedIdeograph(jis))
							charset = JIS_X_0208;
					}
					if(charset == UNDESIGNATED)
						charset = plane2 ? JIS_X_0213_P2 : JIS_X_0213_P1;
				} else
					ASCENSION_HANDLE_UNMAPPABLE()
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
			} else if(0 != (jis = ucsToJISX0208(*from)))
				charset = JIS_X_0208;
			else if((mib == standard::ISO_2022_JP_2
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
					|| mib == extended::ISO_2022_JP_1
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
					) && toBoolean(jis = ucsToJISX0212(*from)))
				charset = JIS_X_0212;
			else if(mib == standard::ISO_2022_JP_2
					&& gb2312Encoder != 0
					&& gb2312Encoder->fromUnicode(mbcs, endof(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = GB2312;
			else if(mib == standard::ISO_2022_JP_2
					&& ksc5601Encoder != 0
					&& ksc5601Encoder->fromUnicode(mbcs, endof(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = KSC5601;
			else if(mib == standard::ISO_2022_JP_2
					&& iso88591Encoder->fromUnicode(mbcs, endof(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = ISO_8859_1;
			else if(mib == standard::ISO_2022_JP_2
					&& iso88597Encoder->fromUnicode(mbcs, endof(mbcs), dummy1, from, from + 1, dummy2) == Encoder::COMPLETED)
				charset = ISO_8859_7;
			else 
				ASCENSION_HANDLE_UNMAPPABLE()

#define ASCENSION_DESIGNATE_TO_G0(escapeSequence, length)	\
	if(g0 != charset) {										\
		if(to + length > toEnd)								\
			break;	/* INSUFFICIENT_BUFFER */				\
		memcpy(to, escapeSequence, length);					\
		to += length;										\
		g0 = static_cast<ISO2022JPCharset_G0>(charset);		\
	}
#define ASCENSION_DESIGNATE_TO_G2(escapeSequence, length)	\
	if(g2 != charset) {										\
		if(to + length > toEnd)								\
			break;	/* INSUFFICIENT_BUFFER */				\
		memcpy(to, escapeSequence, length);					\
		to += length;										\
		g2 = static_cast<ISO2022JPCharset_G2>(charset);		\
	}

			if(charset == ASCII) {	// ASCII
				ASCENSION_DESIGNATE_TO_G0("\x1B(B", 3);
				*to = mask8Bit(jis);
			} else if(charset == JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
				ASCENSION_DESIGNATE_TO_G0("\x1B(J", 3);
				*to = mask8Bit(jis);
//			} else if(charset == JIS_X_0201_KANA) {	// JIS X 0201-Kana
//				ASCENSION_DESIGNATE_TO_G0("\x1B(I", 3);
//				*to = mask8Bit(jis);
			} else if(charset == JIS_X_0208) {	// JIS X 0208:1997 (:1990)
				ASCENSION_DESIGNATE_TO_G0("\x1B$B", 3);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == JIS_X_0212) {	// JIS X 0212:1990
				ASCENSION_DESIGNATE_TO_G0("\x1B$(D", 4);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == JIS_X_0213_P1) {	// JIS X 0213:2004 plane-1 or :2000 plane-1
				ASCENSION_DESIGNATE_TO_G0(isISO2022JP2004(mib) ? "\x1B$(Q" : "\x1B$(O", 4);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == JIS_X_0213_P2) {	// JIS X 0213:2004 (:2000) plane-2
				ASCENSION_DESIGNATE_TO_G0("\x1B$(P", 4);
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis);
			} else if(charset == GB2312) {	// GB2312:1980
				ASCENSION_DESIGNATE_TO_G0("\x1B$A", 3);
				*to = mask7Bit(mbcs[0]);
				if(mbcs[1] != 0)
					*++to = mask7Bit(mbcs[1]);
			} else if(charset == KSC5601) {	// KSC5601:1987
				ASCENSION_DESIGNATE_TO_G0("\x1B$(C", 4);
				*to = mask7Bit(mbcs[0]);
				if(mbcs[1] != 0)
					*++to = mask7Bit(mbcs[1]);
			} else if(charset == ISO_8859_1) {	// ISO-8859-1
				ASCENSION_DESIGNATE_TO_G2("\x1B.A", 3);
				if(to + 3 >= toEnd)
					break;	// INSUFFICIENT_BUFFER
				*to = ESC;	// SS2
				*++to = 'N';
				*++to = mask8Bit(mbcs[0]);
			} else if(charset == ISO_8859_7) {	// ISO-8859-7
				ASCENSION_DESIGNATE_TO_G2("\x1B.F", 3);
				if(to + 3 >= toEnd)
					break;	// INSUFFICIENT_BUFFER
				*to = ESC;	// SS2
				*++to = 'N';
				*++to = mask8Bit(mbcs[0]);
			}
		}

		// restore G0 into ASCII and end (if sufficient buffer is)
		if(from == fromEnd && g0 != ASCII && to + 3 <= toEnd) {
			memcpy(to, "\x1B(B", 3);
			to += 3;
			g0 = ASCII;
		}
		toNext = to;
		fromNext = from;
		if(state != 0)
			*state = g0 | (g2 << 4);
		return (fromNext == fromEnd) ? Encoder::COMPLETED : Encoder::INSUFFICIENT_BUFFER;
#undef ASCENSION_HANDLE_UNMAPPABLE
#undef ASCENSION_DESIGNATE_TO_G0
#undef ASCENSION_DESIGNATE_TO_G2
	}

	// JIS X 0208 or JIS X 0213 <-> シフト JIS 2 バイト文字の変換
	inline void convertX0208ToShiftJISDBCS(ushort jis, uchar* dbcs) {
		assert(dbcs != 0);
		const uchar jk = static_cast<uchar>((jis - 0x2020) >> 8);		// ku
		const uchar jt = static_cast<uchar>((jis - 0x2020) & 0x00FF);	// ten

		assert(jk >= 1 && jk <= 94 && jt >= 1 && jt <= 94);
		dbcs[0] = (jk - 1) / 2 + ((jk <= 62) ? 0x81 : 0xC1);
		if(jk % 2 == 0)	dbcs[1] = jt + 0x9E;
		else			dbcs[1] = jt + ((jt <= 63) ? 0x3F : 0x40);
	}
	inline ushort convertShiftJISDBCSToX0208(const uchar* dbcs) {
		assert(dbcs != 0);
		uchar jk, jt;

		if(dbcs[0] >= 0x81 && dbcs[0] <= 0x9F)	// ku: 01..62
			jk = (dbcs[0] - 0x81) * 2 + ((dbcs[1] > 0x9E) ? 2 : 1);	// < leadbyte = (jk - 1) / 2 + 0x81
		else	// ku: 63..94
			jk = (dbcs[0] - 0xC1) * 2 + ((dbcs[1] > 0x9E) ? 2 : 1);	// < leadbyte = (jk - 1) / 2 + 0xC1
		if(jk % 2 == 0)
			jt = dbcs[1] - 0x9E;	// < trailbyte = jt + 0x9E
		else if(dbcs[1] <= 0x3F + 63)	// ten: 01..63
			jt = dbcs[1] - 0x3F;	// < trailbyte = jt + 0x3F
		else	// ten: 64-94
			jt = dbcs[1] - 0x40;	// < trailbyte = jt + 0x40
		return ((jk << 8) | jt) + 0x2020;
	}
	inline ushort convertShiftJISDBCSToX0213(const uchar* dbcs, bool& plane2) {
		uchar jk, jt;
		const bool kuIsEven = dbcs[1] > 0x9E;

		plane2 = dbcs[0] >= 0xF0;
		if(dbcs[0] >= 0x81 && dbcs[0] <= 0x9F)
			jk = dbcs[0] * 2 - 0x101 + (kuIsEven ? 1 : 0);
		else if(dbcs[0] >= 0xE0 && dbcs[0] <= 0xEF)
			jk = dbcs[0] * 2 - 0x181 + (kuIsEven ? 1 : 0);
		else if((dbcs[0] == 0xF4 && kuIsEven) || (dbcs[0] >= 0xF5 && dbcs[0] <= 0xFC))
			jk = dbcs[0] * 2 - 0x19B + (kuIsEven ? 1 : 0);
		else if((dbcs[0] >= 0xF0 && dbcs[0] <= 0xF3) || (dbcs[0] == 0xF4 && !kuIsEven)) {
			switch(dbcs[0]) {
			case 0xF0:	jk = kuIsEven ? 8 : 1; break;
			case 0xF1:	jk = kuIsEven ? 4 : 3; break;
			case 0xF2:	jk = kuIsEven ? 12 : 5; break;
			case 0xF3:	jk = kuIsEven ? 14 : 13; break;
			case 0xF4:	jk = 15; break;
			}
		}
		if(jk % 2 == 0)
			jt = dbcs[1] - 0x9E;	// < trailbyte = jt + 0x9E
		else if(dbcs[1] <= 0x3F + 63)	// ten: 01..63
			jt = dbcs[1] - 0x3F;	// < trailbyte = jt + 0x3F
		else	// ten: 64..94
			jt = dbcs[1] - 0x40;	// < trailbyte = jt + 0x40
		return ((jk << 8) | jt) + 0x2020;
	}

} // namespace @0


// Shift_JIS ////////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ShiftJISEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
	for(; to < toEnd && from < from; ++to, ++from) {
		if(*from < 0x80)
			*to = mask8Bit(*from);
		else {
			ushort jis = ucsToJISX0208(*from);	// try JIS X 0208
			if(jis == UNMAPPABLE_NATIVE_CHARACTER) {
				if(const uchar kana = ucsToJISX0201Kana(*from)) {	// try JIS X 0201 kana
					*to = kana;
					continue;
				} else if(getPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
					*to = NATIVE_REPLACEMENT_CHARACTER;
				else if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else {
					toNext = to;
					fromNext = from;
					return UNMAPPABLE_CHARACTER;
				}
			} else if(to + 1 >= toEnd)
				break;	// INSUFFICIENT_BUFFER
			convertX0208ToShiftJISDBCS(jis, to);
			++to;	// DBCS			
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

/// @see Encoder#doToUnicode
Encoder::Result ShiftJISEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State*) const {
	for(to < toEnd && from < fromEnd; ++to; ++from) {
		if(*from < 0x80)	// ascii
			*to = *from;
		else if(*from >= 0xA1 && *from <= 0xDF)	// 1-byte kana
			*to = jisX0201KanaToUCS(*from);
		else {
			if(*from != 0xA0) {	// DBCS leading byte
				if(from + 1 < fromEnd && from[1] >= 0x40 && from[1] <= 0xFC && from[1] != 0x7F) {
					*to = jisX0208ToUCS(convertShiftJISDBCSToX0208(from));
					if(*to == REPLACEMENT_CHARACTER) {
						if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
							--to;
						else if(getPolicy() != REPLACE_UNMAPPABLE_CHARACTER) {
							toNext = to;
							fromNext = from;
							return UNMAPPABLE_CHARACTER;
						}
					}
				} else {
					toNext = to;
					fromNext = from;
					return MALFORMED_INPUT;
				}
			}
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}


// EUC-JP ///////////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result EUCJPEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x0080) {	// ASCII
			*to = mask8Bit(*from);
			continue;
		}

		bool x0212 = false;
		ushort jis = ucsToJISX0208(*from);
		if(jis == UNMAPPABLE_NATIVE_CHARACTER) {
			if((jis = ucsToJISX0212(*from)) != UNMAPPABLE_NATIVE_CHARACTER)
				// JIS X 0212
				x0212 = true;
			else if(const uchar kana = ucsToJISX0201Kana(*from)) {
				// JIS X 0201 Kana
				if(to + 1 >= toEnd) {
					toNext = to;
					fromNext = from;
					return INSUFFICIENT_BUFFER;
				}
				*to = SS2;
				*++to = kana;
				continue;
			} else if(getPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*to = NATIVE_REPLACEMENT_CHARACTER;
			else if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
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
			*to = SS3;
			*++to = mask8Bit(jis >> 8);
			*++to = mask8Bit(jis);
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

/// @see Encoder#doToUnicode
Encoder::Result EUCJPEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State*) const {
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x80)
			*to = *from;
		else {
			const size_t bytes = (*from != SS3) ? 2 : 3;
			if(from + bytes > fromEnd) {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			} else if(*from == SS2)	// SS2 -> JIS X 0201 Kana
				*to = jisX0201KanaToUCS(from[1]);
			else if(*from == SS3) {	// SS3 -> JIS X 0212
				const ushort jis = ((from[1] << 8) | from[2]) - 0x8080;
				*to = jisX0212ToUCS(jis);
			} else {	// JIS X 0208
				const ushort jis = ((*from << 8) | from[1]) - 0x8080;
				*to = jisX0208ToUCS(jis);
			}

			if(*to == REPLACEMENT_CHARACTER) {	// unmappable
				if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(getPolicy() != REPLACE_UNMAPPABLE_CHARACTER) {
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


// ISO-2022-JP //////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JPEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	// TODO: to~toEnd が足りないとエスケープシーケンスが正しく書き込まれない可能性がある
	return convertUTF16ToISO2022JPX(standard::ISO_2022_JP, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JPEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(standard::ISO_2022_JP, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-2 ////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP2Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(standard::ISO_2022_JP_2, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP2Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(standard::ISO_2022_JP_2, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS

// ISO-2022-JP-1 ////////////////////////////////////////////////////////////
		
/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP1Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_1, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP1Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_1, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-2004 /////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP2004Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_2004, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP2004Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_2004, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-2004-Strict //////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP2004StrictEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_2004_STRICT, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP2004StrictEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_2004_STRICT, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-2004-Compatible //////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP2004CompatibleEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_2004_COMPATIBLE, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP2004CompatibleEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_2004_COMPATIBLE, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-3 ////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP3Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_3, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP3Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_3, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-3-Strict /////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP3StrictEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_3_STRICT, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP3StrictEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_3_STRICT, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}


// ISO-2022-JP-3-Compatible /////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ISO2022JP3CompatibleEncoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State* state) const {
	return convertUTF16ToISO2022JPX(extended::ISO_2022_JP_3_COMPATIBLE, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}

/// @see Encoder#doToUnicode
Encoder::Result ISO2022JP3CompatibleEncoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State* state) const {
	return convertISO2022JPXToUTF16(extended::ISO_2022_JP_3_COMPATIBLE, to, toEnd, toNext, from, fromEnd, fromNext, state, getPolicy());
}
// Shift_JIS-2004 ///////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result ShiftJIS2004Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
	bool plane2;
	for(ptrdiff_t utf16Length; to < toEnd && from < fromEnd; from += utf16Length) {
		utf16Length = (*from > 0x007F) ? (fromEnd - from) : 1;
		ushort jis = (*from > 0x007F) ? ucsToJISX0213(from, utf16Length, plane2) : *from;
		if(jis == UNMAPPABLE_NATIVE_CHARACTER && (jis = ucsToJISX0201Kana(*from)) == UNMAPPABLE_NATIVE_CHARACTER) {
			if(getPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*to = NATIVE_REPLACEMENT_CHARACTER;
			else if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				--to;
			else {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
			assert(utf16Length == 1);
			continue;
		}
		if(jis < 0x0100)	// ASCII or kana
			*to = mask8Bit(jis);
		else if(to + 1 < toEnd) {
			const uchar jk = mask8Bit((jis - 0x2020) >> 8);	// ku
			const uchar jt = mask8Bit(jis - 0x2020);		// ten
			assert(jk >= 1 && jk <= 94 && jt >= 1 && jt <= 94);
			if(!plane2)	// plane 1
				*to = (jk + ((jk <= 62) ? 0x101 : 0x181)) / 2;
			else	// plane 2
				*to = (jk >= 78) ? ((jk + 0x19B) / 2) : ((jk + 0x1DF) / 2 - jk / 8 * 3);
			if(jk % 2 == 0)	*++to = jt + 0x9E;
			else			*++to = jt + ((jt <= 63) ? 0x3F : 0x40);
		} else
			break;	// INSUFFICIENT_BUFFER
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

/// @see Encoder#doToUnicode
Encoder::Result ShiftJIS2004Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State*) const {
	const Char* const beginning = to;
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x80)	// ASCII
			*to = *from;
		else if(*from >= 0xA1 && *from <= 0xDF)	// kana
			*to = jisX0201KanaToUCS(*from);
		else if(*from == 0xA0) {	// illegal byte
			if(getPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
				*to = NATIVE_REPLACEMENT_CHARACTER;
			else if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
				--to;
			else {
				toNext = to;
				fromNext = from;
				return UNMAPPABLE_CHARACTER;
			}
		} else {
			if(from + 1 < fromEnd && (from[1] >= 0x40 && from[1] <= 0xFC && from[1] != 0x7F)) {	// double byte
				bool plane2;
				const ushort jis = convertShiftJISDBCSToX0213(from, plane2);
				const CodePoint ucs = !plane2 ? jisX0213P1ToUCS(jis) : jisX0213P2ToUCS(jis);

				if(ucs == REPLACEMENT_CHARACTER) {	// unmappable
					if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
						--to;
					else if(getPolicy() != REPLACE_UNMAPPABLE_CHARACTER) {
						toNext = to;
						fromNext = from;
						return UNMAPPABLE_CHARACTER;
					}
					continue;
				} else if(ucs >= 0x010000 && to + 1 >= toEnd)
					break;	// INSUFFICIENT_BUFFER

				if(ucs > 0x0010FFFF) {	// a character uses two code points
					*to = maskUCS2(ucs >> 16);
					*++to = maskUCS2(ucs);
				} else if(ucs >= 0x00010000)	// out of BMP
					surrogates::encode(ucs, to++);
				else {
					if(to > beginning && (to[-1] == L'\x02E9' && ucs == 0x02E5UL) || (to[-1] == L'\x02E5' && ucs == 0x02E9UL)) {
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


// EUC-JIS-2004 /////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result EUCJIS2004Encoder::doFromUnicode(uchar* to, uchar* toEnd, uchar*& toNext,
		const Char* from, const Char* fromEnd, const Char*& fromNext, State*) const {
	ushort jis;
	bool plane2 = false;
	for(ptrdiff_t utf16Length; to < toEnd && from < fromEnd; ++to, from += utf16Length) {
		// UCS -> JIS
		utf16Length = (*from >= 0x80) ? (fromEnd - from) : 1;
		if((jis = (*from >= 0x80) ? ucsToJISX0213(from, utf16Length, plane2) : *from) == UNMAPPABLE_NATIVE_CHARACTER) {
			if((jis = ucsToJISX0201Kana(*from)) != UNMAPPABLE_NATIVE_CHARACTER) {
				if(to + 1 >= toEnd) {
					toNext = to;
					fromNext = from;
					return INSUFFICIENT_BUFFER;
				}
				*(to++) = SS2;
			} else {	// unmappable
				if(getPolicy() == REPLACE_UNMAPPABLE_CHARACTER)
					*to = NATIVE_REPLACEMENT_CHARACTER;
				else if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else {
					toNext = to;
					fromNext = from;
					return UNMAPPABLE_CHARACTER;
				}
				continue;
			}
		}

		// JIS -> EUC-JP
		if(jis < 0x100)
			*to = mask8Bit(jis);
		else if(to + (plane2 ? 2 : 1) >= toEnd) {
			toNext = to;
			fromNext = from;
			return INSUFFICIENT_BUFFER;
		} else {
			jis += 0x8080;
			if(!plane2) {	// plane 1
				*to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis >> 0);
			} else {	// plane 2
				*to = SS3;
				*++to = mask8Bit(jis >> 8);
				*++to = mask8Bit(jis >> 0);
			}
		}
	}
	toNext = to;
	fromNext = from;
	return (fromNext == fromEnd) ? COMPLETED : INSUFFICIENT_BUFFER;
}

/// @see Encoder#doToUnicode
Encoder::Result EUCJIS2004Encoder::doToUnicode(Char* to, Char* toEnd, Char*& toNext,
		const uchar* from, const uchar* fromEnd, const uchar*& fromNext, State*) const {
	const Char* const beginning = to;
	for(; to < toEnd && from < fromEnd; ++to, ++from) {
		if(*from < 0x80)
			*to = *from;
		else {
			const ptrdiff_t bytes = (*from != SS3) ? 2 : 3;
			if(from + bytes > fromEnd) {
				toNext = to;
				fromNext = from;
				return MALFORMED_INPUT;
			} else if(*from == SS2)	// SS2 -> JIS X 0201 Kana
				*to = jisX0201KanaToUCS(from[1]);
			else if(*from == SS3) {	// SS3 -> plane-2
				const ushort jis = ((from[1] << 8) | from[2]) - 0x8080;
				const CodePoint ucs = jisX0213P2ToUCS(jis);
				if(ucs != REPLACEMENT_CHARACTER) {
					if(ucs > 0x010000 && to + 1 >= toEnd)
						break;	// INSUFFICIENT_BUFFER
					if(ucs > 0x0010FFFF) {	// a character uses two code points
						*to = maskUCS2(ucs >> 16);
						*++to = maskUCS2(ucs >> 0);
					} else if(ucs >= 0x00010000)	// out of BMP
						surrogates::encode(ucs, to++);
					else
						*to = maskUCS2(ucs);
				}
			} else {	// plane-1
				const ushort jis = ((*from << 8) | from[1]) - 0x8080;
				const CodePoint ucs = jisX0213P1ToUCS(jis);
				if(ucs != REPLACEMENT_CHARACTER) {
					if(ucs > 0x0010FFFF) {	// a character uses two code points
						*to = maskUCS2(ucs >> 16);
						*++to = maskUCS2(ucs >> 0);
					} else if(ucs >= 0x00010000)	// out of BMP
						surrogates::encode(ucs, to++);
					else {
						if(to > beginning
								&& ((to[-1] == L'\x02E9' && ucs == 0x02E5UL)
								|| (to[-1] == L'\x02E5' && ucs == 0x02E9UL))) {
							if(to + 1 >= toEnd)
								break;	// INSUFFICIENT_BUFFER
							*(to++) = ZERO_WIDTH_NON_JOINER;
						}
						*to = maskUCS2(ucs);
					}
				}
			}
			if(*to == REPLACEMENT_CHARACTER) {	// unmappable
				if(getPolicy() == IGNORE_UNMAPPABLE_CHARACTER)
					--to;
				else if(getPolicy() != REPLACE_UNMAPPABLE_CHARACTER) {
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

#if 0
#ifdef _WIN32

// Windows-51932 ////////////////////////////////////////////////////////////

/// @see Encoder#doFromUnicode
Encoder::Result CP51932Encoder::doFromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	BOOL usedDefaultChar;
	size_t j = 0;
	uchar dbcs[2];
	for(size_t i = 0; i < srcLength && j < destLength; ++i) {
		// UTF-16 -> cp932
		const int convertedBytes = ::WideCharToMultiByte(932, 0,
			src + i, 1, reinterpret_cast<char*>(dbcs), 2, 0, &usedDefaultChar);
		if(usedDefaultChar) {
			if(callback != 0 && callback->unconvertableCharacterFound())
				return 0;
			callback = 0;
		}

		// cp932 -> cp51932
		if(convertedBytes == 1 && dbcs[0] >= 0xA1 && dbcs[0] <= 0xDF) {	// 半角カナ
			dest[j++] = SS2;
			dest[j++] = dbcs[0];
		} else if(convertedBytes == 2
				&& ((dbcs[0] >= 0x81 && dbcs[0] <= 0x9F) || (dbcs[0] >= 0xE0 && dbcs[0] <= 0xFC))
				&& (dbcs[1] >= 0x40 && dbcs[1] <= 0xFC) && dbcs[1] != 0x7F) {	// 2 バイト文字
			const ushort jis = convertShiftJISDBCSToX0208(dbcs);
			dest[j++] = BIT8_MASK((jis | 0x8000) >> 8);
			dest[j++] = BIT8_MASK((jis | 0x0080) >> 0);
		} else	// その他
			dest[j++] = dbcs[0];
	}
	return j;
}

/// @see Encoder#doToUnicode
Encoder::Result CP51932Encoder::doToUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0;
	uchar dbcs[2];
	for(size_t i = 0; i < srcLength && j < destLength; ++j) {
		// cp51932 -> cp932
		if(src[i] == SS2) {	// 半角カナ
			if(i + 1 >= srcLength)
				break;
			dbcs[0] = src[i + 1];
			dbcs[1] = 0;
			i += 2;
		} else if(src[i] >= 0xA1 && src[i] <= 0xFE
				&& src[i + 1] >= 0xA1 && src[i + 1] <= 0xFE) {	// 2バイト文字
			convertX0208ToShiftJISDBCS(((src[i] << 8) | src[i + 1]) - 0x8080, dbcs);
			i += 2;
		} else {
			dbcs[0] = src[i++];
			dbcs[1] = 0;
		}

		// cp932 -> UTF-16
		::MultiByteToWideChar(932, 0, reinterpret_cast<const char*>(dbcs), (dbcs[1] == 0) ? 1 : 2, dest + j, 1);
	}
	return j;
}

#endif /* _WIN32 */
#endif /* 0 */

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */


// JISAutoDetector //////////////////////////////////////////////////////////

namespace {
	inline MIBenum detectShiftJIS(const uchar* from, const uchar* last, ptrdiff_t& convertibleBytes, bool& foundKana) {
		bool jis2004 = false;
		foundKana = false;
		const uchar* p;
		for(p = from; p < last; ++p) {
			if(*p == ESC)	// Shift_JIS can't have an ESC
				break;
			else if(*p < 0x80)	// ASCII is ok
				continue;
			else if(*p >= 0xA1 && *p <= 0xDF)	// JIS X 0201 kana
				foundKana = true;
			else if(p < last - 1) {	// 2-byte character?
				if(*p < 0x81 || *p > 0xFC || (*p > 0x9F && *p < 0xE0))
					break;	// illegal lead byte
				else if(p[1] < 0x40 || p[1] > 0xFC || p[1] == 0x7F)
					break;	// illegal trail byte

#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
				bool plane2;
				if(!jis2004) {
					if(jisX0208ToUCS(convertShiftJISDBCSToX0208(p)) == REPLACEMENT_CHARACTER) {
						const ushort jis = convertShiftJISDBCSToX0213(p, plane2);
						if(!plane2 && jisX0213P1ToUCS(jis) == REPLACEMENT_CHARACTER)
							break;	// unmappable
						jis2004 = true;
					}
				} else {	// Shift_JIS-2004
					if(convertShiftJISDBCSToX0213(p, plane2) == UNMAPPABLE_NATIVE_CHARACTER)
						break;
				}
				++p;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
			} else
				break;
		}
		convertibleBytes = p - from;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		if(jis2004)
			return extended::SHIFT_JIS_2004;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		return standard::SHIFT_JIS;
	}
	inline MIBenum detectEUCJP(const uchar* from, const uchar* last, ptrdiff_t& convertibleBytes, bool& foundKana) {
		bool jis2004 = false;
		foundKana = false;
		const uchar* p;
		for(p = from; p < last; ++p) {
			if(*p == ESC)	// EUC-JP can't have an ESC
				break;
			else if(*p < 0x80)	// ASCII is ok
				continue;
			else if(*p == SS2) {	// SS2 introduces JIS X 0201 kana
				if(p + 1 >= last || p[1] < 0xA0 || p[1] > 0xE0)
					break;
				foundKana = true;
				++p;
			} else if(*p == SS3) {	// SS3 introduces JIS X 0212 or JIS X 0213 plane2
				if(p + 2 >= last)
					break;
				ushort jis = p[1] << 8 | p[2];
				if(jis < 0x8080)
					break;	// unmappable
				jis -= 0x8080;
				if(jisX0212ToUCS(jis) != REPLACEMENT_CHARACTER) {
					if(jis2004)
						break;
//					cp = CPEX_JAPANESE_EUC;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
				} else if(jisX0213P2ToUCS(jis) != REPLACEMENT_CHARACTER) {
					if(!jis2004)
						break;
					jis2004 = true;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
				} else
					break;
				from += 2;
			} else if(from < last - 1) {	// 2-byte character
				ushort jis = *p << 8 | p[1];
				if(jis <= 0x8080)
					break;
				jis -= 0x8080;
				if(jisX0208ToUCS(jis) == REPLACEMENT_CHARACTER) {
					if(jisX0213P1ToUCS(jis) != REPLACEMENT_CHARACTER) {
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
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		if(jis2004)
			return extended::EUC_JIS_2004;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		return standard::EUC_JP;
	}
	inline MIBenum detectISO2022JP(const uchar* from, const uchar* last, ptrdiff_t& convertibleBytes, bool& foundKana) {
		const uchar* foundEsc[7] = {0, 0, 0, 0
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
			, 0, 0, 0
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		};	// X0201, X0208, X0212, JP2, JP3-plane1, JP2004-plane1, X0213-plane2
		foundKana = false;
		const uchar* p;
		for(p = from; p < last; ++p) {
			if(*p >= 0x80)	// 8-bit
				break;
			else if(*p == ESC) {
				if(p + 2 >= last)
					break;
				if(memcmp(p + 1, "(J", 2) == 0 || memcmp(p + 1, "(I", 2) == 0) {	// JIS X 0201
					foundEsc[0] = p + 3; p += 2; foundKana = true;
				} else if(memcmp(p + 1, "$@", 2) == 0 || memcmp(p + 1, "$B", 2) == 0) {	// JIS X 0208
					foundEsc[1] = p + 3; p += 2;
				} else if(memcmp(p + 1, "$A", 2) == 0		// GB2312
						|| memcmp(p + 1, ".A", 2) == 0		// ISO-8859-1
						|| memcmp(p + 1, ".F", 2) == 0) {	// ISO-8859-7
					foundEsc[3] = p + 3; p += 2;
				} else if(p + 3 < last) {
					if(memcmp(p + 1, "$(D", 3) == 0) {	// JIS X 0212
						foundEsc[2] = p + 4; p += 3;
					} else if(memcmp(p + 1, "$(C", 3) == 0) {	// KSC5601
						foundEsc[3] = p + 4; p += 3;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
					} else if(memcmp(p + 1, "$(O", 3) == 0) {	// JIS X 0213:2000 plane1
						foundEsc[4] = p + 4; p += 3;
					} else if(memcmp(p + 1, "$(Q", 3) == 0) {	// JIS X 0213:2004 plane1
						foundEsc[5] = p + 4; p += 3;
					} else if(memcmp(p + 1, "$(P", 3) == 0) {	// JIS X 0213 plane2
						foundEsc[6] = p + 4; p += 3;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
					}
				} else
					break;
			}
		}
		convertibleBytes = p - from;

		if(foundEsc[2] > 0 || foundEsc[3] > 0)
			return /*(foundEsc[4] > 0 || foundEsc[5] > 0 || foundEsc[6] > 0) ? extended::ISO_2022_7BIT :*/ standard::ISO_2022_JP_2;
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
		else if(foundEsc[5] > 0) {
//			if(foundEsc[2] > 0 || foundEsc[3] > 0)
//				return extended::ISO_2022_7BIT;
//			else
				return (foundEsc[1] > 0) ? extended::ISO_2022_JP_2004_STRICT : extended::ISO_2022_JP_2004;
		} else if(foundEsc[4] > 0) {
//			if(foundEsc[2] > 0 || foundEsc[3] > 0)
//				return extended::ISO_2022_7BIT;
//			else
				return (foundEsc[1] > 0) ? extended::ISO_2022_JP_3_STRICT : extended::ISO_2022_JP_3;
		} else if(foundEsc[6] > 0)
			return /*(foundEsc[2] > 0 || foundEsc[3] > 0) ? extended::ISO_2022_7BIT :*/ extended::ISO_2022_JP_2004;
#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
		return standard::ISO_2022_JP;
	}

}

/// @see EncodingDetector#doDetector
MIBenum JISAutoDetector::doDetect(const uchar* first, const uchar* last, ptrdiff_t* convertibleBytes) const throw() {
	MIBenum result = fundamental::UTF_8;
	ptrdiff_t cb = 0;

	// first, test Unicode
	if(const EncodingDetector* unicodeDetector = EncodingDetector::forID(UNICODE_DETECTOR)) {
		result = unicodeDetector->detect(first, last, &cb);
		if(cb == last - first) {
			if(convertibleBytes != 0)
				*convertibleBytes = cb;
			return result;
		}
	}

	bool foundKana;
	ptrdiff_t cb2;
	MIBenum result2 = detectShiftJIS(first, last, cb2, foundKana);
	if(cb2 > cb) {
		result = result2;
		cb = cb2;
	}
	if(cb < last - first || foundKana) {
		result2 = detectEUCJP(first, last, cb2, foundKana);
		if(cb2 > cb) {
			result = result2;
			cb = cb2;
		}
		if(cb < last - first || foundKana) {
			result2 = detectISO2022JP(first, last, cb2, foundKana);
			if(cb2 > cb) {
				result = result2;
				cb = cb2;
			}
		}
	}

	if(convertibleBytes != 0)
		*convertibleBytes = cb;
	return result;
}

#endif /* !ASCENSION_NO_STANDARD_ENCODINGS */
