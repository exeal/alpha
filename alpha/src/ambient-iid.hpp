

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Sun Jan 11 19:47:34 2009
 */
/* Compiler settings for ..\src\ambient.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_ambient,0xA2263752,0x16CA,0x4336,0xB8,0x0C,0x5F,0xCE,0x16,0x46,0x6E,0x8D);


MIDL_DEFINE_GUID(IID, IID_IPosition,0xA685BE8A,0xDCA7,0x4817,0x8A,0x20,0x0C,0x62,0x8D,0x0B,0x0B,0x32);


MIDL_DEFINE_GUID(IID, IID_IRegion,0xA25036BA,0xF43D,0x4270,0xAD,0x6E,0x43,0x39,0x89,0x01,0x25,0x83);


MIDL_DEFINE_GUID(IID, IID_IBuffer,0xAFF3034C,0x4B74,0x40b1,0x88,0x20,0xB2,0xAA,0x0D,0x17,0x9C,0xFF);


MIDL_DEFINE_GUID(IID, IID_IPoint,0xA92A18B8,0x6A66,0x4b89,0x96,0x8C,0x9F,0x1F,0x5E,0x92,0xFC,0xD3);


MIDL_DEFINE_GUID(IID, IID_IEditPoint,0xA178F060,0xC5A6,0x4e56,0x82,0x83,0xCE,0xE0,0xFA,0xC5,0x8A,0x35);


MIDL_DEFINE_GUID(IID, IID_IBookmarker,0xAFACAF4D,0x69B8,0x4917,0x95,0xD3,0x51,0x70,0xC6,0x7B,0xF7,0xE2);


MIDL_DEFINE_GUID(IID, IID_IBufferList,0xA4F51429,0x782F,0x49fe,0x88,0x40,0xE5,0x9B,0x80,0x0C,0xC3,0x93);


MIDL_DEFINE_GUID(IID, IID_IVisualPoint,0xAE850138,0x5112,0x41ba,0xB1,0x80,0x4C,0xC3,0x99,0xFA,0x4D,0x6D);


MIDL_DEFINE_GUID(IID, IID_ICaret,0xAA709221,0x38BC,0x4581,0xAF,0x37,0xF0,0x12,0xE9,0x7A,0x13,0x27);


MIDL_DEFINE_GUID(IID, IID_ITextEditor,0xA80D020F,0x9576,0x4fae,0xB9,0xDE,0xA0,0x00,0xB7,0xF9,0xED,0xEB);


MIDL_DEFINE_GUID(IID, IID_IWindow,0xA5B43144,0x7520,0x41ba,0xA5,0xD9,0x02,0x1A,0xC2,0x3B,0x2B,0xA6);


MIDL_DEFINE_GUID(IID, IID_IWindowList,0xAD5BE76A,0x0203,0x4a0f,0xAE,0x7A,0x41,0x52,0xB3,0x6E,0xF1,0x5A);


MIDL_DEFINE_GUID(IID, IID_IMenu,0xA373ED31,0x7A38,0x45f2,0xA7,0xA8,0x29,0xF3,0x15,0x54,0xFC,0x85);


MIDL_DEFINE_GUID(IID, IID_IPopupMenu,0xA88658D8,0xEF74,0x4c5a,0xB7,0xD3,0x79,0x53,0xBC,0x90,0xF3,0x67);


MIDL_DEFINE_GUID(IID, IID_IPopupMenuConstructor,0xA53B0C04,0xF00D,0x4a85,0x9A,0x83,0x27,0x2C,0xF7,0x03,0x45,0x70);


MIDL_DEFINE_GUID(IID, IID_IMenuBar,0xAF898B12,0x1F02,0x4517,0x8F,0xD1,0x6C,0x81,0x0F,0x22,0x62,0xB8);


MIDL_DEFINE_GUID(IID, IID_IMenuBarConstructor,0xA581CCF8,0xC1DB,0x4a04,0x8B,0xDB,0x68,0x0A,0xC1,0x9F,0x0E,0xC7);


MIDL_DEFINE_GUID(IID, IID_IServiceObjectProvider,0xAB4CC2F7,0x4873,0x43f4,0xAA,0x5E,0x53,0x6D,0x86,0x53,0xFE,0x2C);


MIDL_DEFINE_GUID(IID, IID_IScriptSystem,0xA8B776AA,0x560E,0x4262,0x9C,0xFA,0x5C,0x0D,0xFA,0x33,0xCE,0xF8);


MIDL_DEFINE_GUID(IID, IID_INamedArguments,0xAFF456A8,0x8042,0x46aa,0xAD,0xCC,0xE3,0xA3,0x2D,0x64,0x69,0x0C);


MIDL_DEFINE_GUID(IID, IID_IUnnamedArguments,0xA8AEF8E8,0x35EF,0x49da,0x82,0xA3,0xB5,0x7D,0xCD,0xE1,0xA0,0x97);


MIDL_DEFINE_GUID(IID, IID_IArguments,0xA843FB1A,0x8E28,0x4d37,0x80,0x5F,0x9F,0xCF,0xB9,0x8A,0x6F,0x05);


MIDL_DEFINE_GUID(IID, IID_IScriptHost,0xA34BB582,0xA2DA,0x4197,0x8A,0x81,0x3E,0x3F,0xB2,0xE3,0xFD,0x16);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



