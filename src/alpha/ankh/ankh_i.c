

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 6.00.0361 */
/* at Sun Aug 12 22:48:28 2007
 */
/* Compiler settings for .\ankh\ankh.idl:
    Oicf, W4, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)


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

MIDL_DEFINE_GUID(IID, LIBID_Ankh,0xAD251D25,0xDF1F,0x43c0,0x9D,0x16,0x08,0x89,0x58,0x18,0x3A,0x50);


MIDL_DEFINE_GUID(IID, IID_INamedArguments,0xAFF456A8,0x8042,0x46aa,0xAD,0xCC,0xE3,0xA3,0x2D,0x64,0x69,0x0C);


MIDL_DEFINE_GUID(IID, IID_IUnnamedArguments,0xA8AEF8E8,0x35EF,0x49da,0x82,0xA3,0xB5,0x7D,0xCD,0xE1,0xA0,0x97);


MIDL_DEFINE_GUID(IID, IID_IArguments,0xA843FB1A,0x8E28,0x4d37,0x80,0x5F,0x9F,0xCF,0xB9,0x8A,0x6F,0x05);


MIDL_DEFINE_GUID(IID, IID_IScriptHost,0xA34BB582,0xA2DA,0x4197,0x8A,0x81,0x3E,0x3F,0xB2,0xE3,0xFD,0x16);


MIDL_DEFINE_GUID(IID, IID_INamespace,0xA1E2E6F3,0xDEF6,0x4949,0x9D,0x4D,0xD5,0x09,0xDD,0x0E,0xFF,0x14);


MIDL_DEFINE_GUID(IID, IID_INamespaceWatcher,0xA1611576,0x63EA,0x4c7c,0xBD,0x44,0x22,0xBE,0x0D,0x46,0x08,0xC2);


MIDL_DEFINE_GUID(IID, IID_IScriptSystem,0xAD54E19E,0xC3D5,0x4220,0xB9,0x54,0x06,0x18,0x7D,0x80,0xC9,0x64);


MIDL_DEFINE_GUID(CLSID, CLSID_ObjectModel,0xA0D98D3C,0x9CA7,0x4675,0x9C,0x25,0xDF,0xA2,0xBD,0x03,0x69,0xAB);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

