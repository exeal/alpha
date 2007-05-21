/**
 * @file japanese.cpp
 * @author exeal
 * @date 2004-2007
 *
 * Implementation of Japanese encoders.
 *
 * Following documentation is written in Japanese... (HeHe).
 *
 *	<h3>実装する文字集合とエンコード</h3>
 *
 *	このファイルでは以下のエンコードを実装する:
 *	<ul>
 *		<li>JIS X 0208 -- シフト JIS 、ISO-2022-JP</li>
 *		<li>JIS X 0208 と JIS X 0212 -- EUC 、ISO-2022-JP-1 、ISO-2022-JP-2</li>
 *		<li>JIS X 0213 -- Shift_JIS-2004 、EUC-JIS-2004 、ISO-2022-JP-3 (3つ)、ISO-2022-JP-2004 (3つ)</li>
 *		<li>CP932 -- シフト JIS 、EUC 、ISO-2022-JP</li>
 *	</ul>
 *	CP932 ベースのエンコードは Windows の変換表に基づくもので、
 *	EUC でも JIS X 0212 文字集合を含んでいない。またシステムに
 *	CP932 がインストールされていなければ無効
 *
 *	各エンコードの簡単な解説と Ascension での取り扱いについてコード中にコメントした
 *
 *	ISO-2022-JP* から UCS への変換において、不正なエスケープシーケンスとその後続データは1バイトずつ等価なコード値の
 *	UCS に変換する。これはユーザが誤った変換を発見し易くなる効果があるが、そのまま同じエンコードで非
 *	UCS に変換しても元のデータには戻らないので注意
 *
 *	<h3>JIS X 0208 と UCS の対応で複数の解釈がある文字</h3>
 *
 *	KUBOTA 氏の調査 (http://www.debian.or.jp/~kubota/unicode-symbols-map2.html) によれば
 *	JIS X 0208 の12個の文字は UCS との対応について複数の変換表で解釈に違いがある。
 *	Ascension では JISX0213 InfoCenter の表を JIS X 0208 、JIS X 0213 のテーブル作成に用いており、
 *	これらの表も上記調査の比較対象に含まれている。私はこれらの表の中で解釈に揺れのある12文字を
 *	libiconv EUC-JP のものに変更し、自分のテーブルを作成した (Jis ディレクトリ)
 *
 *	<h3>ISO-2022-JP-2004 の3つのエンコード</h3>
 *
 *	Emacs は ISO-2022-JP との互換性のために ISO-2022-JP-3 とその変種を合わせて3つ実装している。
 *	これは JIS X 0208 と JIS X 0213 で漢字の包摂基準が異なるためである。
 *	詳細や各エンコードについては以下のページを参照:
 *	<ul>
 *		<li>JIS X 0213の特徴と、Emacs上での実装
 *		(http://www.m17n.org/m17n2000_all_but_registration/proceedings/kawabata/jisx0213.html)</li>
 *		<li>Becky! JIS X 0213 プラグイン
 *		(http://members.at.infoseek.co.jp/jisx0213/bk0213.html)</li>
 *	</ul>
 *
 *	<h3>制限</h3>
 *
 *	JIS X 0213 には合成可能な発音記号が含まれており、UCS からの変換で
 *	JIS 側に無い合成済み文字については基礎文字と発音記号に分解することで理論上は表現できる。
 *	しかし Ascension ではこの分解を行わなず、現時点では変換は不可能である。
 *	JIS X 0213 に現れる合成済み仮名については対応している
 *
 *	<h3>声調記号の合字</h3>
 *
 *	JIS X 0213 の2つの声調記号、上昇調 (1-11-69) と下降調 (1-11-70) に直接対応する UCS 文字は無く、
 *	2つのコードポイントの合字が対応すると考えられる。すなわち JIS から UCS への変換において、上昇調は
 *	U+02E9+02E5、下降調は U+02E5+02E9 となる。しかしこのような単純な変換を行うと JIS と UCS
 *	のコード交換性が失われてしまうようだ (http://wakaba-web.hp.infoseek.co.jp/table/jis-note.ja.html)。
 *	Ascension では ZWNJ を使って合字と意図したコードポイントの組み合わせとそうでないものを区別する。
 *	つまり、JIS 側で超高 (1-11-64) と超低 (1-11-68) が並んでいる場合は、それぞれの文字を UCS
 *	に変換して間に ZWNJ を挟む。逆に UCS 側で U+02E5 と U+02E9 が並んでいる場合は JIS
 *	の対応する1つの声調記号に変換し、間に ZWNJ がある場合は2つの声調記号に変換する
 */

#include "stdafx.h"
#ifndef ASCENSION_NO_EXTENDED_ENCODINGS
#include "../encoder.hpp"
#include <memory>		// std::auto_ptr
#include <map>
#include <algorithm>	// std::binary_search
using namespace ascension;
using namespace ascension::encodings;
using namespace ascension::unicode;
using namespace std;


BEGIN_ENCODER_DEFINITION()
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_SHIFTJIS, Japanese_ShiftJIS, 2, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_SHIFTJIS2004, Japanese_ShiftJIS2004, 2, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_EUC, Japanese_EUCJP, 3, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_EUCJIS2004, Japanese_EUCJIS2004, 3, 1)
	DEFINE_ENCODER_CLASS_(51932, Japanese_EUCJPWindows)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP, Japanese_ISO2022JP, 8, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP1, Japanese_ISO2022JP1, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP2, Japanese_ISO2022JP2, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP2004, Japanese_ISO2022JP2004, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP2004_STRICT, Japanese_ISO2022JP2004_Strict, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP2004_COMPATIBLE, Japanese_ISO2022JP2004_Compatible, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP3, Japanese_ISO2022JP3, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP3_STRICT, Japanese_ISO2022JP3_Strict, 9, 1)
	DEFINE_ENCODER_CLASS(CPEX_JAPANESE_ISO2022JP3_COMPATIBLE, Japanese_ISO2022JP3_Compatible, 9, 1)
//	DEFINE_ENCODER_CLASS(50221, Japanese_ISO2022JPWindows)
END_ENCODER_DEFINITION()
DEFINE_DETECTOR(CPEX_JAPANESE_AUTODETECT, Japanese);


#define IS_ISO2022JP3(cp)	\
	(cp >= CPEX_JAPANESE_ISO2022JP3 && cp <= CPEX_JAPANESE_ISO2022JP3_COMPATIBLE)

#define IS_ISO2022JP2004(cp)	\
	(cp >= CPEX_JAPANESE_ISO2022JP2004 && cp <= CPEX_JAPANESE_ISO2022JP2004_COMPATIBLE)

#define JK(ku, ten)	((ku << 8) | ten) + 0x2020


namespace {
	// JIS <-> UCS 変換テーブル (make_table.pl より作成したファイルを切り刻んだもの)。
	// JIS X 0213 については両方向とも 32 ビット数値でテーブルを作成した。
	// UCS 側で 0x0000-0xFFFF はそのまま UCS-2、0x10000-0x10FFFF は UCS-4、0xFFFFFFF 以上は UCS-2 2文字を表す
	// JIS 側では

	const uchar ESC = '\x1B';
	const uchar SS2 = static_cast<uchar>(0x8E);
	const uchar SS3 = static_cast<uchar>(0x8F);
	const wchar_t ZWNJ = L'\x200C';	// Zero Width Non-Joiner (U+200C)
	const wchar_t JISX0208toUCS_2121[] = {	// 0x2121-0x2840
		#include "Jis\JISX0208toUCS_2121"
	};
	const wchar_t JISX0208toUCS_3021[] = {	// 0x3021-0x4F53
		#include "Jis\JISX0208toUCS_3021"
	};
	const wchar_t JISX0208toUCS_5021[] = {	// 0x5021-0x7426
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
	const wchar_t JISX0212toUCS_222F[] = {	// 0x222F-0x2271
		#include "Jis\JISX0212toUCS_222F"
	};
	const wchar_t JISX0212toUCS_2661[] = {	// 0x2661-0x2B77
		#include "Jis\JISX0212toUCS_2661"
	};
	const wchar_t JISX0212toUCS_3021[] = {	// 0x3021-0x6D63
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

	// ISO-2022-JP-3 の禁止漢字 (JIS X 0213:2000 附属書2表1)
	const ushort ProhibitedIdeographs_2000[] = {	// 不連続部分
		JK( 3,26),JK( 3,27),JK( 3,28),JK( 3,29),JK( 3,30),JK( 3,31),	// <記号>
		JK( 3,32),
		JK( 3,59),JK( 3,60),JK( 3,61),JK( 3,62),JK( 3,63),JK( 3,64),	// <記号>
		JK( 3,91),JK( 3,92),JK( 3,93),JK( 3,94),
		JK( 4,84),JK( 4,85),JK( 4,86),JK( 8,87),JK( 4,88),JK( 4,89),	// <平仮名>
		JK( 4,90),JK( 4,91),
		JK( 5,87),JK( 5,88),JK( 5,89),JK( 5,90),JK( 5,91),JK( 5,92),	// <片仮名>
		JK( 5,93),JK( 5,94),
		JK( 6,25),JK( 6,26),JK( 6,27),JK( 6,28),JK( 6,29),JK( 6,30),	// <トランプ>
		JK( 6,31),JK( 6,32),
												JK(13,83),JK(13,88),	// 　　　　∮∟
		JK(13,89),JK(13,93),JK(13,94),									// ⊿・・
														  JK(16, 2),	// 唖
		JK(16,19),JK(16,79),JK(17,58),JK(17,75),JK(17,79),JK(18, 3),	// 鯵逸謁焔縁横
		JK(18, 9),JK(18,10),JK(18,11),JK(18,25),JK(18,50),JK(18,89),	// 鴬鴎黄温禍悔
		JK(19, 4),JK(19,20),JK(19,21),JK(19,34),JK(19,41),JK(19,69),	// 海慨概蛎撹喝
		JK(19,73),JK(19,76),JK(19,86),JK(19,90),JK(20,18),JK(20,33),	// 渇褐竃噛寛漢
		JK(20,35),JK(20,50),JK(20,79),JK(20,91),JK(21, 7),JK(21,85),	// 潅諌器既祈虚
		JK(22, 2),JK(22,31),JK(22,33),JK(22,38),JK(22,48),JK(22,64),	// 侠郷響尭勤謹
		JK(22,77),JK(23,16),JK(23,39),JK(23,59),JK(23,66),JK(24, 6),	// 躯薫掲頚撃研
		JK(24,20),JK(25,60),JK(25,77),JK(25,82),JK(25,85),JK(27, 6),	// 鹸砿麹穀黒殺
		JK(27,67),JK(27,75),JK(28,40),JK(28,41),JK(28,49),JK(28,50),	// 祉視屡蕊煮社
		JK(28,52),JK(29,11),JK(29,13),JK(29,43),JK(29,75),JK(29,77),	// 者繍臭祝暑渚
		JK(29,79),JK(29,80),JK(29,84),JK(30,36),JK(30,45),JK(30,53),	// 緒署諸渉祥蒋
		JK(30,63),JK(30,85),JK(31,32),JK(31,57),JK(32, 5),JK(32,65),	// 醤状神靭瀬節
		JK(32,70),JK(33, 8),JK(33,36),JK(33,46),JK(33,56),JK(33,63),	// 蝉賎祖僧層掻
		JK(33,67),JK(33,93),JK(33,94),JK(34, 3),JK(34, 8),JK(34,45),	// 巣増憎贈即騨
		JK(34,86),JK(35,18),JK(35,29),JK(35,86),JK(35,88),JK(36, 7),	// 琢嘆箪猪著徴
		JK(36, 8),JK(36,45),JK(36,47),JK(36,59),JK(36,87),JK(37,22),	// 懲塚掴壷禎填
		JK(37,31),JK(37,52),JK(37,55),JK(37,78),JK(37,83),JK(37,88),	// 顛都砺梼涛祷
		JK(38,33),JK(38,34),JK(38,45),JK(38,81),JK(38,86),JK(39,25),	// 徳涜突難迩嚢
		JK(39,63),JK(39,72),JK(40,14),JK(40,16),JK(40,43),JK(40,53),	// 梅蝿溌醗繁晩
		JK(40,60),JK(40,74),JK(41,16),JK(41,48),JK(41,49),JK(41,50),	// 卑碑桧賓頻敏
		JK(41,51),JK(41,78),JK(42, 1),JK(42,27),JK(42,29),JK(42,57),	// 瓶侮福併塀勉
		JK(42,66),JK(43,43),JK(43,47),JK(43,72),JK(43,74),JK(43,89),	// 歩頬墨毎槙侭
		JK(44,40),JK(44,45),JK(44,65),JK(44,89),JK(45,20),JK(45,58),	// 免麺戻薮祐遥
		JK(45,73),JK(45,74),JK(45,83),JK(46,20),JK(46,26),JK(46,48),	// 莱頼欄隆虜緑
		JK(46,62),JK(46,64),JK(46,81),JK(46,82),JK(46,93),JK(47, 3),	// 涙類暦歴練錬
		JK(47,13),JK(47,15),JK(47,22),JK(47,25),JK(47,26),JK(47,31),	// 廊朗篭蝋郎録
							JK(48,54),JK(52,68),JK(57,88),JK(58,25),	// 　　儘壺攪攅
		JK(59,56),JK(59,77),JK(62,25),JK(62,85),JK(63,70),JK(64,86),	// 檜檮濤灌煕瑶
		JK(66,72),JK(66,74),JK(67,62),JK(68,38),JK(73, 2),JK(73,14),	// 礦礪竈籠蘂藪
		JK(73,58),JK(74, 4),JK(75,61),JK(76,45),JK(77,78),JK(80,55),	// 蠣蠅諫賤邇靱
		JK(80,84),JK(82,45),JK(82,84),JK(84, 1),JK(84, 2),JK(84, 3),	// 頸鰺鶯堯槇遙
		JK(84, 4),JK(84, 5),JK(84, 6),									// 瑤凜熙
	};
	const ushort ProhibitedIdeographs_2004[] = {	// ISO-2022-JP-2004 の禁止漢字 (JIS X0213:2004 附属書2表2)
		JK(14, 1),JK(15,94),JK(17,19),JK(22,70),JK(23,50),JK(28,24),	// ・・嘘倶繋叱
		JK(33,73),JK(38,61),JK(39,77),JK(47,52),JK(47,94),JK(53,11),	// 痩呑剥・・妍
		JK(54, 2),JK(54,58),JK(84, 7),JK(94,90),JK(94,91),JK(94,92),	// 屏并・・・・
		JK(94,93),JK(94,94)												// ・・
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
		return (jis >= JK(6, 57) && jis <= JK(6, 94))
			|| (jis >= JK(7, 34) && jis <= JK(7, 48))
			|| (jis >= JK(7, 82) && jis <= JK(8, 62))
			|| (jis >= JK(8, 71) && jis <= JK(8, 92))
			|| (jis >= JK(9, 1) && jis <= JK(12, 83))
			|| (jis >= JK(12, 93) && jis <= JK(13, 55))
			|| (jis >= JK(13, 63) && jis <= JK(13, 79))
			|| (jis >= JK(14, 2) && jis <= JK(15, 93))
			|| (jis >= JK(47, 53) && jis <= JK(47, 93))
			|| (jis >= JK(84, 8) && jis <= JK(94, 89))
			|| binary_search(ProhibitedIdeographs_2000, endof(ProhibitedIdeographs_2000), jis);
	}
	// JIS X 0213:2004 で追加された禁止漢字か
	inline bool isISO2022JP2004ProhibitedIdeograph(ushort jis) {
		return binary_search(ProhibitedIdeographs_2004, endof(ProhibitedIdeographs_2004), jis);
	}

	// JIS X 0201 Roman -> UCS 変換
	inline wchar_t jisX0201RomanToUCS(uchar ch) {
		if(ch == 0x5C)						return 0x00A5;					// Yen Sign
		else if(ch == 0x7E)					return 0x203E;					// Overline
		else if(ch >= 0x20 && ch <= 0x7D)	return ch;						// 7-bit
		else								return REPLACEMENT_CHARACTER;	// invalid
	}

	// UCS -> JIS X 0201 Roman 変換
	inline uchar ucsToJISX0201Roman(wchar_t ch) {
		if(ch >= 0x0020 && ch <= 0x005B)		return BIT8_MASK(ch);	// 7-bit
		else if(ch >= 0x005D && ch <= 0x007D)	return BIT8_MASK(ch);	// 7-bit
		else if(ch == 0x00A5)					return 0x5C;			// Yen Sign
		else if(ch == 0x203E)					return 0x7E;			// Overline
		else									return 0x00;			// invalid
	}

	// JIS X 0201 Kana -> UCS 変換
	inline wchar_t jisX0201KanaToUCS(uchar ch) {
		return (ch >= 0xA1 && ch <= 0xDF) ? ch + 0xFEC0 : REPLACEMENT_CHARACTER;
	}

	// UCS -> JIS X 0201 Kana 変換
	inline uchar ucsToJISX0201Kana(wchar_t ch) {
		return (ch >= 0xFF61 && ch <= 0xFF9F) ? BIT8_MASK(ch - 0xFEC0) : 0x00;
	}

	// JIS X 0208 -> UCS 変換
	inline wchar_t jisX0208ToUCS(ushort jis) {
		if(jis >= 0x2121 && jis < 0x2121 + countof(JISX0208toUCS_2121))			return JISX0208toUCS_2121[jis - 0x2121];
		else if(jis >= 0x3021 && jis < 0x3021 + countof(JISX0208toUCS_3021))	return JISX0208toUCS_3021[jis - 0x3021];
		else if(jis >= 0x5021 && jis < 0x5021 + countof(JISX0208toUCS_5021))	return JISX0208toUCS_5021[jis - 0x5021];
		else																	return REPLACEMENT_CHARACTER;
	}

	// UCS -> JIS X 0208 変換
	inline ushort ucsToJISX0208(wchar_t ch) {
		if(ch >= 0x00A2 && ch < 0x00A2 + countof(UCStoJISX0208_00A2))		return UCStoJISX0208_00A2[ch - 0x00A2];
		else if(ch >= 0x0391 && ch < 0x0391 + countof(UCStoJISX0208_0391))	return UCStoJISX0208_0391[ch - 0x0391];
		else if(ch >= 0x2010 && ch < 0x2010 + countof(UCStoJISX0208_2010))	return UCStoJISX0208_2010[ch - 0x2010];
		else if(ch >= 0x2500 && ch < 0x2500 + countof(UCStoJISX0208_2500))	return UCStoJISX0208_2500[ch - 0x2500];
		else if(ch >= 0x3000 && ch < 0x3000 + countof(UCStoJISX0208_3000))	return UCStoJISX0208_3000[ch - 0x3000];
		else if(ch >= 0x4E00 && ch < 0x4E00 + countof(UCStoJISX0208_4E00))	return UCStoJISX0208_4E00[ch - 0x4E00];
		else if(ch >= 0xFF01 && ch < 0xFF01 + countof(UCStoJISX0208_FF01))	return UCStoJISX0208_FF01[ch - 0xFF01];
		else																return 0x0000;
	}

	// JIS X 0212 -> UCS 変換
	inline wchar_t jisX0212ToUCS(ushort jis) {
		if(jis >= 0x222F && jis < 0x222F + countof(JISX0212toUCS_222F))			return JISX0212toUCS_222F[jis - 0x222F];
		else if(jis >= 0x2661 && jis < 0x2661 + countof(JISX0212toUCS_2661))	return JISX0212toUCS_2661[jis - 0x2661];
		else if(jis >= 0x3021 && jis < 0x3021 + countof(JISX0212toUCS_3021))	return JISX0212toUCS_3021[jis - 0x3021];
		else																	return REPLACEMENT_CHARACTER;
	}

	// UCS -> JIS X 0212 変換
	inline ushort ucsToJISX0212(wchar_t ch) {
		if(ch >= 0x007E && ch < 0x007E + countof(UCStoJISX0212_007E))		return UCStoJISX0212_007E[ch - 0x007E];
		else if(ch >= 0x2116 && ch < 0x2116 + countof(UCStoJISX0212_2116))	return UCStoJISX0212_2116[ch - 0x2116];
		else if(ch >= 0x4E02 && ch < 0x4E02 + countof(UCStoJISX0212_4E02))	return UCStoJISX0212_4E02[ch - 0x4E02];
		else																return 0x0000;
	}

	// JIS X 0213 第1面 -> UCS 変換
	inline ulong jisX0213P1ToUCS(ushort jis) {
		if(jis >= 0x2121 && jis < 0x2121 + countof(JISX0213P1toUCS_2121))		return JISX0213P1toUCS_2121[jis - 0x2121];
		else if(jis >= 0x4F54 && jis < 0x4F54 + countof(JISX0213P1toUCS_4F54))	return JISX0213P1toUCS_4F54[jis - 0x4F54];
		else if(jis >= 0x7427 && jis < 0x7427 + countof(JISX0213P1toUCS_7427))	return JISX0213P1toUCS_7427[jis - 0x7427];
		else																	return jisX0208ToUCS(jis);
	}

	// JIS X 0213 第2面 -> UCS 変換
	inline ulong jisX0213P2ToUCS(ushort jis) {
		if(jis >= 0x2121 && jis < 0x2121 + countof(JISX0213P2toUCS_2121))		return JISX0213P2toUCS_2121[jis - 0x2121];
		else if(jis >= 0x2321 && jis < 0x2321 + countof(JISX0213P2toUCS_2321))	return JISX0213P2toUCS_2321[jis - 0x2321];
		else if(jis >= 0x2821 && jis < 0x2821 + countof(JISX0213P2toUCS_2821))	return JISX0213P2toUCS_2821[jis - 0x2821];
		else if(jis >= 0x2C21 && jis < 0x2C21 + countof(JISX0213P2toUCS_2C21))	return JISX0213P2toUCS_2C21[jis - 0x2C21];
		else if(jis >= 0x6E21 && jis < 0x6E21 + countof(JISX0213P2toUCS_6E21))	return JISX0213P2toUCS_6E21[jis - 0x6E21];
		else																	return 0x00000000;
	}

	// UCS -> JIS X 0213 変換 (eaten は入力時は ucs の文字数、出力時は変換に使用した文字数)
	inline ushort ucsToJISX0213(const wchar_t* ucs, ushort& eaten, bool& plane2) {
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
				else if(ucs[1] == ZWNJ && eaten > 2 && ucs[2] == 0x02E5)
					jis = 0x2B64;	// just dependent extra-low tone bar
			} else if(ucs[0] == 0x02E5) {
				if(ucs[1] == 0x02E9)
					jis = 0x2B66;	// extra-high tone bar + extra-low tone bar -> falling symbol
				else if(ucs[1] == ZWNJ && eaten > 2 && ucs[2] == 0x02E9)
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
					const map<ushort, ulong>::const_iterator it = cjkExtBtoJis.find(BIT16_MASK(cp - 0x020000));
					if(it != cjkExtBtoJis.end()) {
						eaten = 2;
						jis = it->second;
					}
				}
			} else
				eaten = 1;
		}

		plane2 = jis > 0xFFFF;
		return BIT16_MASK(jis);
	}

	// ISO-2022-JP-X -> UTF-16 変換ヘルパ
	size_t convertISO2022JPXToUTF16(CodePage cp, wchar_t* dest, size_t destLength,
			const uchar* src, size_t srcLength, IUnconvertableCharCallback* callback) {
		assert(cp == CPEX_JAPANESE_ISO2022JP || cp == CPEX_JAPANESE_ISO2022JP1
			|| cp == CPEX_JAPANESE_ISO2022JP2 || IS_ISO2022JP3(cp) || IS_ISO2022JP2004(cp));

		// 含めることのできる文字集合と指示シーケンスは次の通り。特に記述が無い限り G0:
		//
		// ISO-2022-JP
		//	ASCII					ESC ( B
		//	JIS X 0201:1976-Roman	ESC ( J
		//	JIS X 0208:1978			ESC $ @
		//	JIS X 0208:1983			ESC $ B
		//
		// ISO-2022-JP-1 (ISO-2022-JP に以下を追加)
		//	JIS X 0212:1990			ESC $ ( D
		//
		// ISO-2022-JP-2 (ISO-2022-JP-1 に以下を追加)
		//	GB2312:1980				ESC $ A
		//	KSC5601:1987			ESC $ ( C
		//	ISO-8859-1				ESC . A		96文字集合につき G2
		//	ISO-8859-7				ESC . F		96文字集合につき G2
		//
		// ISO-2022-JP-3
		//	ASCII					ESC ( B
		//	JIS X 0213:2000 1面		ESC $ ( O
		//							ESC $ B		禁止漢字がある
		//	JIS X 0213:2000 2面		ESC $ ( P
		//
		// ISO-2022-JP-2004
		//	ASCII					ESC ( B
		//	JIS X 0213:2004 1面		ESC $ ( Q
		//							ESC $ B		禁止漢字がある
		//	JIS X 0213:2004 2面		ESC $ ( P
		//	JIS X 0213:2000 1面		ESC $ ( O	禁止漢字がある
		//
		// JIS X 0213 1面の指示では JIS X 0208 を指示するために使用していた ESC $ B を代用することが認められているが、
		// 包摂基準の変更により、この互換シーケンスで JIS X 0213 1面を指示した場合、JIS X 0208
		// に含まれる文字の中で使用が禁止されている文字がある
		//
		// JIS X 0213:2004 (追補1) ではこの禁止漢字が幾つか追加された。これらの文字は ISO-2022-JP-2004 において
		// ESC $ ( O で JIS X 0213 1面を指示した場合にも使用できない
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
		//	  系エンコーディングでも追補1で追加された文字を使うことができる (UCS への変換については区別が無い)
		//  - ISO-2022-JP-2004-Compatible の互換性は ISO-2022-JP に対するものであり、ISO-2022-JP-3 に対するものではない

		size_t i = 0, j = 0;
		ISO2022JPCharset_G0	g0 = ASCII;
		ISO2022JPCharset_G2	g2 = UNDESIGNATED;

		EncoderFactory& encoders = EncoderFactory::getInstance();
		auto_ptr<Encoder> iso88591Encoder, iso88597Encoder;

		while(i < srcLength && j < destLength) {
			if(src[i] == ESC && srcLength - i >= 3) {	// expect esc. seq.
				if(memcmp(src + i + 1, "(B", 2) == 0) {
					g0 = ASCII; i += 3; continue;
				} else if(memcmp(src + i + 1, "(J", 2) == 0) {
					g0 = JIS_X_0201_ROMAN; i += 3; continue;
//				} else if(memcmp(src + i + 1, "(I", 2) == 0) {
//					g0 = JIS_X_0201_KANA; i += 3; continue;
				} else if(cp != CPEX_JAPANESE_ISO2022JP2004 && cp != CPEX_JAPANESE_ISO2022JP3
						&& (memcmp(src + i + 1, "$@", 2) == 0 || memcmp(src + i + 1, "$B", 2) == 0)) {
					g0 = JIS_X_0208; i += 3; continue;
				} else if((cp == CPEX_JAPANESE_ISO2022JP1 || cp == CPEX_JAPANESE_ISO2022JP2)
						&& srcLength - i >= 4 && memcmp(src + i + 1, "$(D", 3) == 0) {
					g0 = JIS_X_0212; i += 4; continue;
				} else if(cp == CPEX_JAPANESE_ISO2022JP2) {
					if(memcmp(src + i + 1, "$A", 2) == 0 && encoders.isValidCodePage(936)) {
						g0 = GB2312; i += 3; continue;
					} else if(srcLength - i >= 4
							&& memcmp(src + i + 1, "$(C", 3) == 0 && encoders.isValidCodePage(949)) {
						g0 = KSC5601; i += 4; continue;
					} else if(memcmp(src + i + 1, ".A", 2) == 0) {
						g2 = ISO_8859_1; i += 3; continue;
					} else if(memcmp(src + i + 1, ".F", 2) == 0) {
						g2 = ISO_8859_7; i += 3; continue;
					}
				} else if((IS_ISO2022JP3(cp) || IS_ISO2022JP2004(cp)) && srcLength - i >= 4) {
					if(memcmp(src + i + 1, "$(O", 3) == 0 || memcmp(src + i + 1, "$(Q", 3) == 0) {
						g0 = JIS_X_0213_P1; i += 4; continue;
					} else if(memcmp(src + i + 1, "$(P", 3) == 0) {
						g0 = JIS_X_0213_P2; i += 4; continue;
					}
				}
			}

			if((src[i] <= 0x20 && src[i] != ESC) || (src[i] >= 0x80 && src[i] < 0xA0)) {	// C0 、C1
				if(src[i] == 0x0A || src[i] == 0x0D) {
					g0 = ASCII;
					g2 = UNDESIGNATED;
				}
				dest[j++] = src[i++];	// SI 、SO 、(1バイトの) SS2 、SS3 は無視
			} else if(srcLength - i > 1 && destLength - j > 1
					&& memcmp(src + i, "\x1BN", 2) == 0) {	// SS2
				i += 2;
				if(srcLength > i) {
					const uchar ansi = src[i] | 0x80;
					if(g2 == ISO_8859_1) {	// ISO-8859-1
						if(iso88591Encoder.get() == 0)
							iso88591Encoder.reset(encoders.createEncoder(CPEX_ISO8859_1).release());
						const size_t converted = iso88591Encoder->toUnicode(
							dest + j, destLength - j, reinterpret_cast<const uchar*>(&ansi), 1, callback);
						if(converted == 0)
							return 0;
						++i;
						j += converted;
					} else if(g2 == ISO_8859_7) {	// ISO-8859-7
						if(iso88597Encoder.get() == 0)
							iso88597Encoder.reset(encoders.createEncoder(CPEX_ISO8859_7).release());
						const size_t converted = iso88597Encoder->toUnicode(
							dest + j, destLength - j, reinterpret_cast<const uchar*>(&ansi), 1, callback);
						if(converted == 0)
							return 0;
						++i;
						j += converted;
					} else {
						wchar_t	ucs;
						CONFIRM_ILLEGAL_CHAR(ucs);
						dest[j++] = ucs;
						++i;
					}
				}
			} else if(g0 == JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
				wchar_t u = jisX0201RomanToUCS(src[i]);
				if(u == REPLACEMENT_CHARACTER)
					CONFIRM_ILLEGAL_CHAR(u);
				dest[j++] = u;
				++i;
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
*/			} else if(g0 == ASCII || srcLength - i == 1 || destLength - j == 1) {	// ASCII or illegal char
				uchar jis = src[i];
				if(jis >= 0x80)
					CONFIRM_ILLEGAL_CHAR(jis);
				dest[j++] = jis;
				++i;
			} else if(g0 == JIS_X_0208) {	// JIS X 0208:1978 or :1983
				const ushort	jis = (src[i] << 8) | src[i + 1];
				wchar_t			u = jisX0208ToUCS(jis);

				if(u == REPLACEMENT_CHARACTER)
					CONFIRM_ILLEGAL_CHAR(u);
				dest[j++] = u;
				i += 2;
			} else if(g0 == JIS_X_0212) {	// JIS X 0212:1990
				const ushort	jis = (src[i] << 8) | src[i + 1];
				wchar_t			u = jisX0212ToUCS(jis);

				if(u == REPLACEMENT_CHARACTER)
					CONFIRM_ILLEGAL_CHAR(u);
				dest[j++] = u;
				i += 2;
			} else if(g0 == GB2312 || g0 == KSC5601) {	// GB2312:1980 or KSC5601:1987
				wchar_t		u;	// for error
				char		buffer[2] = {src[i] | 0x80, src[i + 1] | 0x80};
				const int	cch = ::MultiByteToWideChar(
									(g0 == GB2312) ? 936 : 949, MB_PRECOMPOSED, buffer, 2, dest + j, 2);
				if(cch == 0) {
					CONFIRM_ILLEGAL_CHAR(u);
					dest[j++] = u;
				} else
					j += cch;
				i += 2;
			} else if(g0 == JIS_X_0213_P1 || g0 == JIS_X_0213_P2) {	// JIS X 0213:2004 or 2000
				const ushort	jis = (src[i] << 8) | src[i + 1];
				ulong			u = (g0 == JIS_X_0213_P1) ? jisX0213P1ToUCS(jis) : jisX0213P2ToUCS(jis);

				if(u == REPLACEMENT_CHARACTER)
					CONFIRM_ILLEGAL_CHAR(u);
				if(u > 0xFFFF) {
					if(destLength - j == 2)
						return j;
					if(u > 0x0010FFFF) {	// 2 UCS characters
						dest[j++] = UTF16_MASK(u >> 16);
						dest[j++] = UTF16_MASK(u);
					} else {
						surrogates::encode(u, dest + j);
						j += 2;
					}
				} else {
					if(j > 0) {
						if((dest[j - 1] == L'\x02E9' && u == 0x02E5)
								|| (dest[j - 1] == L'\x02E5' && u == 0x02E9)) {
							if(j + 1 >= destLength)
								return j;
							dest[j++] = ZWNJ;
						}
					}
					dest[j++] = UTF16_MASK(u);
				}
				i += 2;
			}
		}
		return j;
	}

	// UTF-16 -> ISO-2022-JP-X 変換ヘルパ
	size_t convertUTF16ToISO2022JPX(CodePage cp, uchar* dest, size_t destLength,
			const wchar_t* src, size_t srcLength, IUnconvertableCharCallback* callback) {
		assert(cp == CPEX_JAPANESE_ISO2022JP || cp == CPEX_JAPANESE_ISO2022JP1
			|| cp == CPEX_JAPANESE_ISO2022JP2 || IS_ISO2022JP3(cp) || IS_ISO2022JP2004(cp));

		size_t				i = 0, j = 0;
		int					charset = ASCII;
		ISO2022JPCharset_G0	g0 = ASCII;
		ISO2022JPCharset_G2	g2 = UNDESIGNATED;

		EncoderFactory&		encoders = EncoderFactory::getInstance();
		auto_ptr<Encoder>	iso88591Encoder;
		auto_ptr<Encoder>	iso88597Encoder;

		if(cp == CPEX_JAPANESE_ISO2022JP2) {
			iso88591Encoder.reset(EncoderFactory::getInstance().createEncoder(CPEX_ISO8859_1).release());
			iso88597Encoder.reset(EncoderFactory::getInstance().createEncoder(CPEX_ISO8859_7).release());
		}

		while(i < srcLength && j < destLength) {
			const wchar_t	ucs = src[i];
			ushort			jis, eatenUtf16 = 1;
			uchar			mbcs[2];

			if(ucs < 0x80) {
				jis = ucs;
				mbcs[0] = BIT8_MASK(ucs);
				mbcs[1] = 0;
				charset = ASCII;
			} else if(0 != (jis = ucsToJISX0201Roman(ucs)) && jis < 0x80)
				charset = /*(jis < 0x80) ?*/ JIS_X_0201_ROMAN /*: JIS_X_0201_KANA*/;
			else if(IS_ISO2022JP3(cp) || IS_ISO2022JP2004(cp)) {
				bool isPlane2;

				eatenUtf16 = static_cast<ushort>(srcLength - i);
				jis = ucsToJISX0213(src + i, eatenUtf16, isPlane2);

				if(jis != N__A) {
					charset = UNDESIGNATED;
					if(!isPlane2) {	// JIS X 0208 互換シーケンスを使うか
						if((cp == CPEX_JAPANESE_ISO2022JP3_COMPATIBLE
								|| cp == CPEX_JAPANESE_ISO2022JP2004_COMPATIBLE) && ucsToJISX0208(ucs) != N__A)
							charset = JIS_X_0208;
						else if((cp == CPEX_JAPANESE_ISO2022JP3_STRICT
								|| cp == CPEX_JAPANESE_ISO2022JP2004_STRICT)
								&& !isISO2022JP3ProhibitedIdeograph(jis)
								&& !isISO2022JP2004ProhibitedIdeograph(jis))
							charset = JIS_X_0208;
					}
					if(charset == UNDESIGNATED)
						charset = isPlane2 ? JIS_X_0213_P2 : JIS_X_0213_P1;
				} else {
					CONFIRM_ILLEGAL_CHAR(jis);
					charset = ASCII;
				}
			} else if(0 != (jis = ucsToJISX0208(ucs)))
				charset = JIS_X_0208;
			else if((cp == CPEX_JAPANESE_ISO2022JP1 || cp == CPEX_JAPANESE_ISO2022JP2)
					&& toBoolean(jis = ucsToJISX0212(ucs)))
				charset = JIS_X_0212;
			else if(cp == CPEX_JAPANESE_ISO2022JP2
					&& encoders.isValidCodePage(936)
					&& ::WideCharToMultiByte(936, 0, src + i, 1, reinterpret_cast<char*>(mbcs), 2, 0, 0) != 0)
				charset = GB2312;
			else if(cp == CPEX_JAPANESE_ISO2022JP2
					&& encoders.isValidCodePage(949)
					&& ::WideCharToMultiByte(949, 0, src + i, 1, reinterpret_cast<char*>(mbcs), 2, 0, 0) != 0)
				charset = KSC5601;
			else if(cp == CPEX_JAPANESE_ISO2022JP2 && iso88591Encoder->fromUnicode(mbcs, 2, src + i, 1, 0) != 0)
				charset = ISO_8859_1;
			else if(cp == CPEX_JAPANESE_ISO2022JP2 && iso88597Encoder->fromUnicode(mbcs, 2, src + i, 1, 0) != 0)
				charset = ISO_8859_7;
			else {
				CONFIRM_ILLEGAL_CHAR(jis);
				charset = ASCII;
			}

#define DESIGNATE_TO_G0(esc_sequence, esc_len)							\
	if(g0 != charset) {													\
		if(destLength < esc_len + 1 || j > destLength - esc_len - 1)	\
			break;														\
		memcpy(dest + j, esc_sequence, esc_len);						\
		j += esc_len;													\
		g0 = static_cast<ISO2022JPCharset_G0>(charset);					\
	}
#define DESIGNATE_TO_G2(esc_sequence, esc_len)						\
	if(g2 != charset) {												\
		if(destLength < esc_len + 3 || j > destLength- esc_len - 3)	\
			break;													\
		memcpy(dest + j, esc_sequence, esc_len);					\
		j += esc_len;												\
		g2 = static_cast<ISO2022JPCharset_G2>(charset);				\
	}

			if(charset == ASCII) {	// ASCII
				DESIGNATE_TO_G0("\x1B(B", 3);
				dest[j++] = BIT8_MASK(jis);
			} else if(charset == JIS_X_0201_ROMAN) {	// JIS X 0201-Roman
				DESIGNATE_TO_G0("\x1B(J", 3);
				dest[j++] = BIT8_MASK(jis);
//			} else if(charset == JIS_X_0201_KANA) {	// JIS X 0201-Kana
//				DESIGNATE_TO_G0("\x1B(I", 3);
//				dest[j++] = BIT8_MASK(jis);
			} else if(charset == JIS_X_0208) {	// JIS X 0208:1997 (:1990)
				DESIGNATE_TO_G0("\x1B$B", 3);
				dest[j++] = BIT8_MASK(jis >> 8);
				dest[j++] = BIT8_MASK(jis);
			} else if(charset == JIS_X_0212) {	// JIS X 0212:1990
				DESIGNATE_TO_G0("\x1B$(D", 4);
				dest[j++] = BIT8_MASK(jis >> 8);
				dest[j++] = BIT8_MASK(jis);
			} else if(charset == JIS_X_0213_P1) {	// JIS X 0213:2004 plane-1 or :2000 plane-1
				DESIGNATE_TO_G0(IS_ISO2022JP2004(cp) ? "\x1B$(Q" : "\x1B$(O", 4);
				dest[j++] = BIT8_MASK(jis >> 8);
				dest[j++] = BIT8_MASK(jis);
			} else if(charset == JIS_X_0213_P2) {	// JIS X 0213:2004 (:2000) plane-2
				DESIGNATE_TO_G0("\x1B$(P", 4);
				dest[j++] = BIT8_MASK(jis >> 8);
				dest[j++] = BIT8_MASK(jis);
			} else if(charset == GB2312) {	// GB2312:1980
				DESIGNATE_TO_G0("\x1B$A", 3);
				dest[j++] = BIT7_MASK(mbcs[0]);
				if(mbcs[1] != 0)
					dest[j++] = BIT7_MASK(mbcs[1]);
			} else if(charset == KSC5601) {	// KSC5601:1987
				DESIGNATE_TO_G0("\x1B$(C", 4);
				dest[j++] = BIT7_MASK(mbcs[0]);
				if(mbcs[1] != 0)
					dest[j++] = BIT7_MASK(mbcs[1]);
			} else if(charset == ISO_8859_1) {	// ISO-8859-1
				DESIGNATE_TO_G2("\x1B.A", 3);
				if(j + 3 >= destLength)
					break;
				dest[j++] = ESC;	// SS2
				dest[j++] = 'N';
				dest[j++] = BIT8_MASK(mbcs[0]);
			} else if(charset == ISO_8859_7) {	// ISO-8859-7
				DESIGNATE_TO_G2("\x1B.F", 3);
				if(j + 3 >= destLength)
					break;
				dest[j++] = 0x1B;	// SS2
				dest[j++] = 'N';
				dest[j++] = BIT8_MASK(mbcs[0]);
			}
			i += eatenUtf16;
		}

		// G0 を ASCII に戻して終わり
		if(g0 != ASCII && destLength > 3 && j <= destLength - 3) {
			memcpy(dest + j, "\x1B(B", 3);
			j += 3;
		}
		return j;

#undef DESIGNATE_TO_G0
#undef DESIGNATE_TO_G2
	}

	// JIS X 0208 or JIS X 0213 <-> シフト JIS 2バイト文字の変換
	inline void convertX0208ToShiftJISDBCS(ushort jis, uchar* dbcs) {
		assert(dbcs != 0);
		const uchar jk = static_cast<uchar>((jis - 0x2020) >> 8);		// 区
		const uchar jt = static_cast<uchar>((jis - 0x2020) & 0x00FF);	// 点

		assert(jk >= 1 && jk <= 94 && jt >= 1 && jt <= 94);
		dbcs[0] = (jk - 1) / 2 + ((jk <= 62) ? 0x81 : 0xC1);
		if(jk % 2 == 0)	dbcs[1] = jt + 0x9E;
		else			dbcs[1] = jt + ((jt <= 63) ? 0x3F : 0x40);
	}
	inline ushort convertShiftJISDBCSToX0208(const uchar* dbcs) {
		assert(dbcs != 0);
		uchar jk, jt;

		if(dbcs[0] >= 0x81 && dbcs[0] <= 0x9F)	// 区: 01-62
			jk = (dbcs[0] - 0x81) * 2 + ((dbcs[1] > 0x9E) ? 2 : 1);	// < leadbyte = (jk - 1) / 2 + 0x81
		else	// 区: 63-94
			jk = (dbcs[0] - 0xC1) * 2 + ((dbcs[1] > 0x9E) ? 2 : 1);	// < leadbyte = (jk - 1) / 2 + 0xC1
		if(jk % 2 == 0)
			jt = dbcs[1] - 0x9E;	// < trailbyte = jt + 0x9E
		else if(dbcs[1] <= 0x3F + 63)	// 点: 01-63
			jt = dbcs[1] - 0x3F;	// < trailbyte = jt + 0x3F
		else	// 点: 64-94
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
		else if(dbcs[1] <= 0x3F + 63)	// 点: 01-63
			jt = dbcs[1] - 0x3F;	// < trailbyte = jt + 0x3F
		else	// 点: 64-94
			jt = dbcs[1] - 0x40;	// < trailbyte = jt + 0x40
		return ((jk << 8) | jt) + 0x2020;
	}

	// 自動判別ヘルパ
	inline size_t isShiftJIS(const uchar* src, size_t len, CodePage& cp, bool& foundKana) {
		cp = 932;
		foundKana = false;
		for(size_t i = 0; i < len; ++i) {
			const uchar	ch = src[i];

			if(ch == ESC)	// ESC
				return i;
			else if(ch < 0x80)	// ASCII
				continue;
			else if(ch >= 0xA1 && ch <= 0xDF) {	// JIS X0201 仮名
				foundKana = true;
				continue;
			} else if(i < len - 1) {	// 2バイト文字?
				if(ch < 0x81 || ch > 0xFC || (ch > 0x9F && ch < 0xE0))
					return i;
				const uchar	trail = src[i + 1];
				if(trail < 0x40 || trail > 0xFC || trail == 0x7F)
					return i;

				bool plane2;
				if(cp == 932) {
					if(jisX0208ToUCS(convertShiftJISDBCSToX0208(src + i)) == REPLACEMENT_CHARACTER) {
						const ushort jis = convertShiftJISDBCSToX0213(src + i, plane2);
						if(!plane2 && jisX0213P1ToUCS(jis) == REPLACEMENT_CHARACTER)
							return i;
						cp = CPEX_JAPANESE_SHIFTJIS2004;
					}
				} else {	// Shift_JIS-2004
					if(convertShiftJISDBCSToX0213(src + i, plane2) == N__A)
						return i;
				}
				++i;
				continue;
			}
			return i;
		}
		return len;
	}
	inline size_t isEUCJP(const uchar* src, size_t len, CodePage& cp, bool& foundKana) {
		cp = 51932;
		foundKana = false;
		for(size_t i = 0; i < len; ++i) {
			const uchar ch = src[i];

			if(ch == ESC)	// ESC
				return i;
			else if(ch < 0x80)	// ASCII
				continue;
			else if(ch == SS2) {	// SS2 -> JIS X 0201 仮名
				if(i + 1 >= len || src[i + 1] < 0xA0 || src[i + 1] > 0xE0)
					return i;
				foundKana = true;
				++i;
			} else if(ch == SS3) {	// SS3 -> JIS X 0212 or JIS X 0213 plane2
				if(i + 2 >= len)
					return i;
				ushort jis = src[i + 1] << 8 | src[i + 2];
				if(jis < 0x8080)
					return i;
				jis -= 0x8080;
				if(jisX0212ToUCS(jis) != REPLACEMENT_CHARACTER) {
					if(cp == CPEX_JAPANESE_EUCJIS2004)
						return i;
					cp = CPEX_JAPANESE_EUC;
				} else if(jisX0213P2ToUCS(jis) != REPLACEMENT_CHARACTER) {
					if(cp == CPEX_JAPANESE_EUC)
						return i;
					cp = CPEX_JAPANESE_EUCJIS2004;
				} else
					return i;
				i += 2;
			} else if(i < len - 1) {	// 2バイト文字?
				ushort jis = ch << 8 | src[i + 1];
				if(jis > 0x8080) {
					jis -= 0x8080;
					if(jisX0208ToUCS(jis) != REPLACEMENT_CHARACTER)
						++i;
					else if(jisX0213P1ToUCS(jis) != REPLACEMENT_CHARACTER) {
						if(cp == CPEX_JAPANESE_EUC)
							return i;
						cp = CPEX_JAPANESE_EUCJIS2004;
						++i;
					} else
						return i;
					continue;
				}
				return i;
			}
		}
		return len;
	}
	inline size_t isISO2022JP(const uchar* src, size_t len, CodePage& cp, bool& foundKana) {
		// X0201, X0208, X0212, JP2, JP3-plane1, JP2004-plane1, X0213-plane2
		size_t	foundEsc[7] = {0, 0, 0, 0, 0, 0, 0};

		cp = 50221;
		foundKana = false;
		for(size_t i = 0; i < len; ++i) {
			const uchar ch = src[i];

			if(ch >= 0x80)	// 8bit
				return i;
			else if(ch == ESC) {
				if(i + 2 >= len)
					return i;
				if(memcmp(src + i + 1, "(J", 2) == 0 || memcmp(src + i + 1, "(I", 2) == 0) {	// JIS X 0201
					foundEsc[0] = i + 3; i += 2; foundKana = true;
				} else if(memcmp(src + i + 1, "$@", 2) == 0 || memcmp(src + i + 1, "$B", 2) == 0) {	// JIS X 0208
					foundEsc[1] = i + 3; i += 2;
				} else if(memcmp(src + i + 1, "$A", 2) == 0		// GB2312
						|| memcmp(src + i + 1, ".A", 2) == 0	// ISO-8859-1
						|| memcmp(src + i + 1, ".F", 2) == 0) {	// ISO-8859-7
					foundEsc[3] = i + 3; i += 2;
				} else if(i + 3 < len) {
					if(memcmp(src + i + 1, "$(D", 3) == 0) {	// JIS X 0212
						foundEsc[2] = i + 4; i += 3;
					} else if(memcmp(src + i + 1, "$(C", 3) == 0) {	// KSC5601
						foundEsc[3] = i + 4; i += 3;
					} else if(memcmp(src + i + 1, "$(O", 3) == 0) {	// JIS X 0213:2000 plane1
						foundEsc[4] = i + 4; i += 3;
					} else if(memcmp(src + i + 1, "$(Q", 3) == 0) {	// JIS X 0213:2004 plane1
						foundEsc[5] = i + 4; i += 3;
					} else if(memcmp(src + i + 1, "$(P", 3) == 0) {	// JIS X 0213 plane2
						foundEsc[6] = i + 4; i += 3;
					}
				} else
					return i;
			}
		}

		if(foundEsc[2] > 0 || foundEsc[3] > 0) {
//			if(foundEsc[4] > 0 || foundEsc[5] > 0 || foundEsc[6] > 0)
//				cp = CPEX_MULTILINGUAL_ISO2022_7BIT;
//			else
				cp = CPEX_JAPANESE_ISO2022JP2;
		} else if(foundEsc[5] > 0) {
//			if(foundEsc[2] > 0 || foundEsc[3] > 0)
//				cp = CPEX_MULTILINGUAL_ISO2022_7BIT;
//			else
			if(foundEsc[1] > 0)	cp = CPEX_JAPANESE_ISO2022JP2004_STRICT;
			else				cp = CPEX_JAPANESE_ISO2022JP2004;
		} else if(foundEsc[4] > 0) {
//			if(foundEsc[2] > 0 || foundEsc[3] > 0)
//				cp = CPEX_MULTILINGUAL_ISO2022_7BIT;
//			else
			if(foundEsc[1] > 0)	cp = CPEX_JAPANESE_ISO2022JP3_STRICT;
			else				cp = CPEX_JAPANESE_ISO2022JP3;
		} else if(foundEsc[6] > 0) {
//			if(foundEsc[2] > 0 || foundEsc[3] > 0)
//				cp = CPEX_MULTILINGUAL_ISO2022_7BIT;
//			else
				cp = CPEX_JAPANESE_ISO2022JP2004;
		} else
			cp = 50221;
		return len;
	}

	void detectCodePage_Japanese(const uchar* src, size_t len, CodePage& result, size_t& convertableLength) {
		// まず Unicode を調べる (CP932 がインストールされていない場合は Unicode のみ)
		if(EncoderFactory::CodePageDetector unicodeDetector = EncoderFactory::getInstance().getUnicodeDetector()) {
			unicodeDetector(src, len, result, convertableLength);
			if(len == convertableLength || !EncoderFactory::getInstance().isValidCodePage(932))
				return;
		} else
			convertableLength = 0;

		size_t converted;
		bool foundKana;

		CodePage sjisCodePage;
		converted = isShiftJIS(src, len, sjisCodePage, foundKana);
		if(converted > convertableLength) {
			result = sjisCodePage;
			convertableLength = converted;
		}
		if(converted == len && !foundKana) {
			convertableLength = len;
			return;
		}

		CodePage eucCodePage;
		converted = isEUCJP(src, len, eucCodePage, foundKana);
		if(converted >= convertableLength && !foundKana) {
			result = eucCodePage;
			convertableLength = converted;
		}
		if(converted == len && !foundKana)
			return;

		CodePage iso2022JpCodePage;
		converted = isISO2022JP(src, len, iso2022JpCodePage, foundKana);
		if(converted >= convertableLength && !foundKana) {
			result = iso2022JpCodePage;
			convertableLength = converted;
		}
	}
} // namespace `anonymous'


// 日本語 (シフト JIS) ///////////////////////////////////////////////////////////////

size_t Encoder_Japanese_ShiftJIS::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j < destLength; ++i) {
		if(src[i] < 0x80)
			dest[j++] = BIT8_MASK(src[i]);
		else {
			ushort jis = ucsToJISX0208(src[i]);
			if(jis == N__A) {
				if(const uchar kana = ucsToJISX0201Kana(src[i]))
					dest[j++] = kana;
				else
					CONFIRM_ILLEGAL_CHAR(dest[j++]);
			} else if(j + 1 < destLength) {
				convertX0208ToShiftJISDBCS(jis, reinterpret_cast<uchar*>(dest + j));
				j += 2;
			} else
				return j;
		}
	}
	return j;
}

size_t Encoder_Japanese_ShiftJIS::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t i = 0, j = 0;
	while(i < srcLength && j < destLength) {
		if(src[i] < 0x80)	// ascii
			dest[j++] = src[i++];
		else if(src[i] >= 0xA1 && src[i] <= 0xDF)	// kana
			dest[j++] = jisX0201KanaToUCS(src[i++]);
		else if(src[i] == 0xA0) {	// illegal byte
			CONFIRM_ILLEGAL_CHAR(dest[j]);
			++j;
		} else {	// DBCS lead byte
			const uchar trailByte = src[i + 1];
			if(i < srcLength - 1
					&& (trailByte >= 0x40 && trailByte <= 0xFC && trailByte != 0x7F)) {	// double-byte
				wchar_t ucs = jisX0208ToUCS(convertShiftJISDBCSToX0208(src + i));
				if(ucs == REPLACEMENT_CHARACTER)
					CONFIRM_ILLEGAL_CHAR(ucs);
				dest[j++] = ucs;
				i += 2;
			} else {	// illegal couple
				CONFIRM_ILLEGAL_CHAR(dest[j]);
				++j;
			}
		}
	}
	return j;
}


// 日本語 (Shift_JIS-2004) /////////////////////////////////////////////////

size_t Encoder_Japanese_ShiftJIS2004::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	ushort utf16Length;
	bool plane2;
	for(size_t i = 0; i < srcLength && j < destLength; i += utf16Length) {
		utf16Length = (src[i] > 0x007F) ? static_cast<ushort>(srcLength - i) : 1;

		ushort jis = (src[i] > 0x007F) ? ucsToJISX0213(src + i, utf16Length, plane2) : src[i];
		if(jis == N__A && (jis = ucsToJISX0201Kana(src[i])) == N__A)
			CONFIRM_ILLEGAL_CHAR(jis);
		if(jis < 0x0100)	// ascii or kana
			dest[j++] = BIT8_MASK(jis);
		else if(j + 1 < destLength) {
			const uchar jk = BIT8_MASK((jis - 0x2020) >> 8);	// 区
			const uchar jt = BIT8_MASK(jis - 0x2020);			// 点

			assert(jk >= 1 && jk <= 94 && jt >= 1 && jt <= 94);
			if(!plane2)	// 1面
				dest[j++] = (jk + ((jk <= 62) ? 0x101 : 0x181)) / 2;
			else	// 2面
				dest[j++] = (jk >= 78) ? ((jk + 0x19B) / 2) : ((jk + 0x1DF) / 2 - jk / 8 * 3);
			if(jk % 2 == 0)	dest[j++] = jt + 0x9E;
			else			dest[j++] = jt + ((jt <= 63) ? 0x3F : 0x40);
		}
	}
	return j;
}

size_t Encoder_Japanese_ShiftJIS2004::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t i = 0, j = 0;
	while(i < srcLength && j < destLength) {
		if(src[i] < 0x80)	// ascii
			dest[j++] = src[i++];
		else if(src[i] >= 0xA1 && src[i] <= 0xDF)	// kana
			dest[j++] = jisX0201KanaToUCS(src[i++]);
		else if(src[i] == 0xA0) {	// illegal byte
			CONFIRM_ILLEGAL_CHAR(dest[j]);
			++j;
		} else {
			const uchar trailByte = src[i + 1];
			if(i < srcLength - 1 && (trailByte >= 0x40 && trailByte <= 0xFC && trailByte != 0x7F)) {	// double-byte
				bool plane2;
				const ushort jis = convertShiftJISDBCSToX0213(src + i, plane2);
				ulong ucs = !plane2 ? jisX0213P1ToUCS(jis) : jisX0213P2ToUCS(jis);

				if(ucs == REPLACEMENT_CHARACTER)
					CONFIRM_ILLEGAL_CHAR(ucs);
				if(ucs > 0x0010FFFF) {	// 2コードポイント相当文字
					if(j + 1 >= destLength)
						return j;
					dest[j++] = UTF16_MASK(ucs >> 16);
					dest[j++] = UTF16_MASK(ucs);
				} else if(ucs >= 0x00010000) {	// 超 BMP
					if(j + 1 >= destLength)
						return j;
					surrogates::encode(ucs, dest + j);
					j += 2;
				} else {
					if(j > 0) {
						if((dest[j - 1] == L'\x02E9' && ucs == 0x02E5UL) || (dest[j - 1] == L'\x02E5' && ucs == 0x02E9UL)) {
							if(j + 1 >= destLength)
								return j;
							dest[j++] = ZWNJ;
						}
					}
					dest[j++] = UTF16_MASK(ucs);
				}
				i += 2;
			} else {
				CONFIRM_ILLEGAL_CHAR(dest[j]);
				++j;
			}
		}
	}
	return j;
}


// 日本語 (EUC) ////////////////////////////////////////////////////////////

size_t Encoder_Japanese_EUCJP::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j < destLength; ++i) {
		if(src[i] < 0x0080) {	// ascii
			dest[j++] = BIT8_MASK(src[i]);
			continue;
		}

		bool x0212 = false;
		ushort jis = ucsToJISX0208(src[i]);

		if(jis == N__A) {
			if((jis = ucsToJISX0212(src[i])) != N__A)
				// JIS X 0212
				x0212 = true;
			else if(uchar kana = ucsToJISX0201Kana(src[i])) {
				// JIS X 0201 Kana
				if(destLength - j < 2)
					return j;
				dest[j++] = SS2;	// SS2
				dest[j++] = kana;
				continue;
			} else {	// unconvertable
				CONFIRM_ILLEGAL_CHAR(jis);
				dest[j++] = NATIVE_DEFAULT_CHARACTER;
				continue;
			}
		} else if(destLength - j < 2)
			return j;

		jis |= 0x8080;	// jis -> euc-jp
		if(!x0212) {	// JIS X 0208
			dest[j++] = BIT8_MASK(jis >> 8);
			dest[j++] = BIT8_MASK(jis);
		} else if(destLength - j < 3)
			return j;
		else {	// JIS X 0212
			dest[j++] = static_cast<uchar>('\x8F');	// SS3
			dest[j++] = BIT8_MASK(jis >> 8);
			dest[j++] = BIT8_MASK(jis);
		}
	}
	return j;
}

size_t Encoder_Japanese_EUCJP::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t j = 0;
	for(size_t i = 0; i < srcLength && j < destLength; ++j) {
		const uchar firstByte = src[i];

		if(firstByte == SS2) {	// SS2 -> JIS X 0201 Kana
			if(i + 2 > srcLength)
				return j;
			wchar_t ucs = jisX0201KanaToUCS(src[i + 1]);

			if(ucs == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(ucs);
			dest[j] = ucs;
			i += 2;
		} else if(firstByte == SS3) {	// SS3 -> JIS X 0212
			if(i + 3 > srcLength)
				return j;
			const ushort jis = ((src[i + 1] << 8) | src[i + 2]) - 0x8080;
			wchar_t ucs = jisX0212ToUCS(jis);

			if(ucs == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(ucs);
			dest[j] = ucs;
			i += 3;
		} else if(firstByte >= 0x80) {	// JIS X 0208
			if(i + 2 > srcLength)
				return j;
			const ushort jis = ((firstByte << 8) | src[i + 1]) - 0x8080;
			wchar_t ucs = jisX0208ToUCS(jis);

			if(ucs == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(ucs);
			dest[j] = ucs;
			i += 2;
		} else
			dest[j] = src[i++];
	}
	return j;
}


// 日本語 (EUC-JIS-2004) /////////////////////////////////////////////////

size_t Encoder_Japanese_EUCJIS2004::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();

	size_t j = 0;
	ushort utf16Length;
	bool plane2 = false;
	for(size_t i = 0; i < srcLength && j < destLength; i += utf16Length) {
		utf16Length = (src[i] >= 0x80) ? static_cast<ushort>(srcLength - i) : 1;
		ushort jis = (src[i] >= 0x80) ? ucsToJISX0213(src + i, utf16Length, plane2) : src[i];

		if(jis == N__A) {
			if((jis = ucsToJISX0201Kana(src[i])) != N__A) {
				if(destLength - j < 2)
					return j;
				dest[j++] = SS2;
			} else
				CONFIRM_ILLEGAL_CHAR(jis);
		}
		if(jis < 0x100)
			dest[j++] = BIT8_MASK(jis);
		else if(destLength - j < 2)
			return j;
		else {
			jis += 0x8080;	// jis -> euc-jp
			if(!plane2) {	// 1 面
				dest[j++] = BIT8_MASK(jis >> 8);
				dest[j++] = BIT8_MASK(jis >> 0);
			} else if(destLength - j < 3)
				return j;
			else {	// 2 面
				dest[j++] = SS3;
				dest[j++] = BIT8_MASK(jis >> 8);
				dest[j++] = BIT8_MASK(jis >> 0);
			}
		}
	}
	return j;
}

size_t Encoder_Japanese_EUCJIS2004::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();

	size_t i = 0, j = 0;
	while(i < srcLength && j < destLength) {
		const uchar firstByte = src[i];

		if(firstByte == SS2) {	// SS2 -> JIS X 0201 Kana
			if(i + 2 > srcLength)
				return j;
			wchar_t ucs = jisX0201KanaToUCS(src[i + 1]);

			if(ucs == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(ucs);
			dest[j++] = ucs;
			i += 2;
		} else if(firstByte == SS3) {	// SS3 -> plane-2
			if(i + 3 > srcLength)
				return j;
			const ushort jis = ((src[i + 1] << 8) | src[i + 2]) - 0x8080;
			ulong ucs = jisX0213P2ToUCS(jis);

			if(ucs == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(ucs);
			if(ucs > 0x0010FFFF) {	// 2コードポイント相当文字 (無いと思う)
				if(j + 1 >= destLength)
					return j;
				dest[j++] = UTF16_MASK(ucs >> 16);
				dest[j++] = UTF16_MASK(ucs >> 0);
			} else if(ucs >= 0x00010000) {	// 超 BMP
				if(j + 1 >= destLength)
					return j;
				surrogates::encode(ucs, dest + j);
				j += 2;
			} else
				dest[j++] = UTF16_MASK(ucs);
			i += 3;
		} else if(firstByte >= 0x80) {	// plane-1
			if(i + 2 > srcLength)
				return j;
			const ushort jis = ((firstByte << 8) | src[i + 1]) - 0x8080;
			ulong ucs = jisX0213P1ToUCS(jis);

			if(ucs == REPLACEMENT_CHARACTER)
				CONFIRM_ILLEGAL_CHAR(ucs);
			if(ucs > 0x0010FFFF) {	// 2コードポイント相当
				dest[j++] = UTF16_MASK(ucs >> 16);
				dest[j++] = UTF16_MASK(ucs >> 0);
			} else if(ucs >= 0x00010000) {	// 超 BMP
				surrogates::encode(ucs, dest + j);
				j += 2;
			} else {
				if(j > 0) {
					if((dest[j - 1] == L'\x02E9' && ucs == 0x02E5UL)
							|| (dest[j - 1] == L'\x02E5' && ucs == 0x02E9UL)) {
						if(j + 1 >= destLength)
							return j;
						dest[j++] = ZWNJ;
					}
				}
				dest[j++] = UTF16_MASK(ucs);
			}
			i += 2;
		} else
			dest[j++] = src[i++];
	}
	return j;
}


// 日本語 (ISO-2022-JP) //////////////////////////////////////////////////////////////

// destLength が足りないとエスケープシーケンスが正しく書き込まれない可能性がある
size_t Encoder_Japanese_ISO2022JP::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-1) ///////////////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP1::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP1, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP1::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP1, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-2) ///////////////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP2::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP2, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP2::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP2, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-2004) ////////////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP2004::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP2004, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP2004::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP2004, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-2004-Strict) ////////////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP2004_Strict::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP2004_STRICT, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP2004_Strict::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP2004_STRICT, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-2004-Compatible) ////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP2004_Compatible::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP2004_COMPATIBLE, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP2004_Compatible::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP2004_COMPATIBLE, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-3) ////////////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP3::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP3, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP3::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP3, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-3-Strict) ////////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP3_Strict::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(
		CPEX_JAPANESE_ISO2022JP3_STRICT, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP3_Strict::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP3_STRICT, dest, destLength, src, srcLength, callback);
}


// 日本語 (ISO-2022-JP-3-Compatible) ////////////////////////////////////////

size_t Encoder_Japanese_ISO2022JP3_Compatible::fromUnicode(CFU_ARGLIST) {
	CFU_CHECKARGS();
	return convertUTF16ToISO2022JPX(CPEX_JAPANESE_ISO2022JP3_COMPATIBLE, dest, destLength, src, srcLength, callback);
}

size_t Encoder_Japanese_ISO2022JP3_Compatible::toUnicode(CTU_ARGLIST) {
	CTU_CHECKARGS();
	return convertISO2022JPXToUTF16(CPEX_JAPANESE_ISO2022JP3_COMPATIBLE, dest, destLength, src, srcLength, callback);
}


// 日本語 (EUC, windows-51932) //////////////////////////////////////////////

Encoder_Japanese_EUCJPWindows::Encoder_Japanese_EUCJPWindows() {
	if(!::IsValidCodePage(932))
		throw exception("This code page is not unsupported.");
}

size_t Encoder_Japanese_EUCJPWindows::fromUnicode(CFU_ARGLIST) {
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

size_t Encoder_Japanese_EUCJPWindows::toUnicode(CTU_ARGLIST) {
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

uchar Encoder_Japanese_EUCJPWindows::getMaxNativeCharLength() const {
	return 2;
}

uchar Encoder_Japanese_EUCJPWindows::getMaxUCSCharLength() const {
	return 1;
}


#undef IS_ISO2022JP3
#undef IS_ISO2022JP2004
#undef JK

#endif /* !ASCENSION_NO_EXTENDED_ENCODINGS */
