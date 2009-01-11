

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __ambient2Didl_hpp__
#define __ambient2Didl_hpp__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IPosition_FWD_DEFINED__
#define __IPosition_FWD_DEFINED__
typedef interface IPosition IPosition;
#endif 	/* __IPosition_FWD_DEFINED__ */


#ifndef __IRegion_FWD_DEFINED__
#define __IRegion_FWD_DEFINED__
typedef interface IRegion IRegion;
#endif 	/* __IRegion_FWD_DEFINED__ */


#ifndef __IBuffer_FWD_DEFINED__
#define __IBuffer_FWD_DEFINED__
typedef interface IBuffer IBuffer;
#endif 	/* __IBuffer_FWD_DEFINED__ */


#ifndef __IPoint_FWD_DEFINED__
#define __IPoint_FWD_DEFINED__
typedef interface IPoint IPoint;
#endif 	/* __IPoint_FWD_DEFINED__ */


#ifndef __IEditPoint_FWD_DEFINED__
#define __IEditPoint_FWD_DEFINED__
typedef interface IEditPoint IEditPoint;
#endif 	/* __IEditPoint_FWD_DEFINED__ */


#ifndef __IBookmarker_FWD_DEFINED__
#define __IBookmarker_FWD_DEFINED__
typedef interface IBookmarker IBookmarker;
#endif 	/* __IBookmarker_FWD_DEFINED__ */


#ifndef __IBufferList_FWD_DEFINED__
#define __IBufferList_FWD_DEFINED__
typedef interface IBufferList IBufferList;
#endif 	/* __IBufferList_FWD_DEFINED__ */


#ifndef __IVisualPoint_FWD_DEFINED__
#define __IVisualPoint_FWD_DEFINED__
typedef interface IVisualPoint IVisualPoint;
#endif 	/* __IVisualPoint_FWD_DEFINED__ */


#ifndef __ICaret_FWD_DEFINED__
#define __ICaret_FWD_DEFINED__
typedef interface ICaret ICaret;
#endif 	/* __ICaret_FWD_DEFINED__ */


#ifndef __ITextEditor_FWD_DEFINED__
#define __ITextEditor_FWD_DEFINED__
typedef interface ITextEditor ITextEditor;
#endif 	/* __ITextEditor_FWD_DEFINED__ */


#ifndef __IWindow_FWD_DEFINED__
#define __IWindow_FWD_DEFINED__
typedef interface IWindow IWindow;
#endif 	/* __IWindow_FWD_DEFINED__ */


#ifndef __IWindowList_FWD_DEFINED__
#define __IWindowList_FWD_DEFINED__
typedef interface IWindowList IWindowList;
#endif 	/* __IWindowList_FWD_DEFINED__ */


#ifndef __IMenu_FWD_DEFINED__
#define __IMenu_FWD_DEFINED__
typedef interface IMenu IMenu;
#endif 	/* __IMenu_FWD_DEFINED__ */


#ifndef __IPopupMenu_FWD_DEFINED__
#define __IPopupMenu_FWD_DEFINED__
typedef interface IPopupMenu IPopupMenu;
#endif 	/* __IPopupMenu_FWD_DEFINED__ */


#ifndef __IPopupMenuConstructor_FWD_DEFINED__
#define __IPopupMenuConstructor_FWD_DEFINED__
typedef interface IPopupMenuConstructor IPopupMenuConstructor;
#endif 	/* __IPopupMenuConstructor_FWD_DEFINED__ */


#ifndef __IMenuBar_FWD_DEFINED__
#define __IMenuBar_FWD_DEFINED__
typedef interface IMenuBar IMenuBar;
#endif 	/* __IMenuBar_FWD_DEFINED__ */


#ifndef __IMenuBarConstructor_FWD_DEFINED__
#define __IMenuBarConstructor_FWD_DEFINED__
typedef interface IMenuBarConstructor IMenuBarConstructor;
#endif 	/* __IMenuBarConstructor_FWD_DEFINED__ */


#ifndef __IServiceObjectProvider_FWD_DEFINED__
#define __IServiceObjectProvider_FWD_DEFINED__
typedef interface IServiceObjectProvider IServiceObjectProvider;
#endif 	/* __IServiceObjectProvider_FWD_DEFINED__ */


#ifndef __IScriptSystem_FWD_DEFINED__
#define __IScriptSystem_FWD_DEFINED__
typedef interface IScriptSystem IScriptSystem;
#endif 	/* __IScriptSystem_FWD_DEFINED__ */


#ifndef __INamedArguments_FWD_DEFINED__
#define __INamedArguments_FWD_DEFINED__
typedef interface INamedArguments INamedArguments;
#endif 	/* __INamedArguments_FWD_DEFINED__ */


#ifndef __IUnnamedArguments_FWD_DEFINED__
#define __IUnnamedArguments_FWD_DEFINED__
typedef interface IUnnamedArguments IUnnamedArguments;
#endif 	/* __IUnnamedArguments_FWD_DEFINED__ */


#ifndef __IArguments_FWD_DEFINED__
#define __IArguments_FWD_DEFINED__
typedef interface IArguments IArguments;
#endif 	/* __IArguments_FWD_DEFINED__ */


#ifndef __IScriptHost_FWD_DEFINED__
#define __IScriptHost_FWD_DEFINED__
typedef interface IScriptHost IScriptHost;
#endif 	/* __IScriptHost_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dispex.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __ambient_LIBRARY_DEFINED__
#define __ambient_LIBRARY_DEFINED__

/* library ambient */
/* [helpstring][version][lcid][uuid] */ 

typedef /* [public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_ambient_0000_0000_0001
    {	LineFeed	= 0,
	CarriageReturn	= ( LineFeed + 1 ) ,
	CrLf	= ( CarriageReturn + 1 ) ,
	NextLine	= ( CrLf + 1 ) ,
	LineSeparator	= ( NextLine + 1 ) ,
	ParagraphSeparator	= ( LineSeparator + 1 ) ,
	RawValue	= 0x1000,
	DocumentInput	= 0x1001
    } 	Newline;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_ambient_0000_0000_0002
    {	DontLock	= 0,
	SharedLock	= ( DontLock + 1 ) ,
	ExclusiveLock	= ( SharedLock + 1 ) ,
	LockOnlyAsEditing	= ( ExclusiveLock + 1 ) 
    } 	FileLockMode;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ambient_0000_0000_0003
    {	Utf16CodeUnit	= 0,
	Utf32CodeUnit	= ( Utf16CodeUnit + 1 ) ,
	GraphemeCluster	= ( Utf32CodeUnit + 1 ) ,
	GlyphCluster	= ( GraphemeCluster + 1 ) ,
	DefaultUnit	= ( GlyphCluster + 1 ) 
    } 	CharacterUnit;

typedef /* [public][public][public][public] */ 
enum __MIDL___MIDL_itf_ambient_0000_0000_0004
    {	Forward	= 0,
	Backward	= ( Forward + 1 ) 
    } 	Direction;



EXTERN_C const IID LIBID_ambient;

#ifndef __IPosition_INTERFACE_DEFINED__
#define __IPosition_INTERFACE_DEFINED__

/* interface IPosition */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IPosition;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A685BE8A-DCA7-4817-8A20-0C628D0B0B32")
    IPosition : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Column( 
            /* [retval][out] */ long *column) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Column( 
            /* [in] */ long column) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Line( 
            /* [retval][out] */ long *line) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Line( 
            /* [in] */ long line) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPositionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPosition * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPosition * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPosition * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPosition * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPosition * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPosition * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPosition * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Column )( 
            IPosition * This,
            /* [retval][out] */ long *column);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Column )( 
            IPosition * This,
            /* [in] */ long column);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Line )( 
            IPosition * This,
            /* [retval][out] */ long *line);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Line )( 
            IPosition * This,
            /* [in] */ long line);
        
        END_INTERFACE
    } IPositionVtbl;

    interface IPosition
    {
        CONST_VTBL struct IPositionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPosition_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPosition_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPosition_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPosition_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPosition_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPosition_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPosition_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPosition_get_Column(This,column)	\
    ( (This)->lpVtbl -> get_Column(This,column) ) 

#define IPosition_put_Column(This,column)	\
    ( (This)->lpVtbl -> put_Column(This,column) ) 

#define IPosition_get_Line(This,line)	\
    ( (This)->lpVtbl -> get_Line(This,line) ) 

#define IPosition_put_Line(This,line)	\
    ( (This)->lpVtbl -> put_Line(This,line) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPosition_INTERFACE_DEFINED__ */


#ifndef __IRegion_INTERFACE_DEFINED__
#define __IRegion_INTERFACE_DEFINED__

/* interface IRegion */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IRegion;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A25036BA-F43D-4270-AD6E-433989012583")
    IRegion : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Beginning( 
            /* [retval][out] */ IPosition **beginning) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_End( 
            /* [retval][out] */ IPosition **end) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Encompasses( 
            /* [in] */ IRegion *other,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetIntersection( 
            /* [in] */ IRegion *other,
            /* [retval][out] */ IRegion **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetUnion( 
            /* [in] */ IRegion *other,
            /* [retval][out] */ IRegion **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Includes( 
            /* [in] */ IPosition *p,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IntersectsWith( 
            /* [in] */ IRegion *other,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsEmpty( 
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRegion * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRegion * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRegion * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IRegion * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IRegion * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IRegion * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IRegion * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Beginning )( 
            IRegion * This,
            /* [retval][out] */ IPosition **beginning);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_End )( 
            IRegion * This,
            /* [retval][out] */ IPosition **end);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Encompasses )( 
            IRegion * This,
            /* [in] */ IRegion *other,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetIntersection )( 
            IRegion * This,
            /* [in] */ IRegion *other,
            /* [retval][out] */ IRegion **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetUnion )( 
            IRegion * This,
            /* [in] */ IRegion *other,
            /* [retval][out] */ IRegion **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Includes )( 
            IRegion * This,
            /* [in] */ IPosition *p,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IntersectsWith )( 
            IRegion * This,
            /* [in] */ IRegion *other,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsEmpty )( 
            IRegion * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        END_INTERFACE
    } IRegionVtbl;

    interface IRegion
    {
        CONST_VTBL struct IRegionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegion_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IRegion_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IRegion_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IRegion_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IRegion_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IRegion_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IRegion_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IRegion_get_Beginning(This,beginning)	\
    ( (This)->lpVtbl -> get_Beginning(This,beginning) ) 

#define IRegion_get_End(This,end)	\
    ( (This)->lpVtbl -> get_End(This,end) ) 

#define IRegion_Encompasses(This,other,result)	\
    ( (This)->lpVtbl -> Encompasses(This,other,result) ) 

#define IRegion_GetIntersection(This,other,result)	\
    ( (This)->lpVtbl -> GetIntersection(This,other,result) ) 

#define IRegion_GetUnion(This,other,result)	\
    ( (This)->lpVtbl -> GetUnion(This,other,result) ) 

#define IRegion_Includes(This,p,result)	\
    ( (This)->lpVtbl -> Includes(This,p,result) ) 

#define IRegion_IntersectsWith(This,other,result)	\
    ( (This)->lpVtbl -> IntersectsWith(This,other,result) ) 

#define IRegion_IsEmpty(This,result)	\
    ( (This)->lpVtbl -> IsEmpty(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IRegion_INTERFACE_DEFINED__ */


#ifndef __IBuffer_INTERFACE_DEFINED__
#define __IBuffer_INTERFACE_DEFINED__

/* interface IBuffer */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IBuffer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AFF3034C-4B74-40b1-8820-B2AA0D179CFF")
    IBuffer : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AccessibleRegion( 
            /* [retval][out] */ IRegion **region) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Bookmarker( 
            /* [retval][out] */ IBookmarker **bookmarker) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BoundToFile( 
            /* [retval][out] */ VARIANT_BOOL *bound) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Encoding( 
            /* [retval][out] */ BSTR *encoding) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Encoding( 
            /* [in] */ BSTR encoding) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_InCompoundChanging( 
            /* [retval][out] */ VARIANT_BOOL *compound) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [defaultvalue][in] */ Newline __MIDL__IBuffer0000,
            /* [retval][out] */ long *length) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Line( 
            /* [in] */ long line,
            /* [retval][out] */ BSTR *s) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Modified( 
            /* [retval][out] */ VARIANT_BOOL *modified) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Narrowed( 
            /* [retval][out] */ VARIANT_BOOL *narrowed) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Newline( 
            /* [retval][out] */ Newline *newline) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Newline( 
            /* [in] */ Newline newline) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReadOnly( 
            /* [retval][out] */ VARIANT_BOOL *readOnly) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ReadOnly( 
            /* [in] */ VARIANT_BOOL readOnly) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RecordsChanges( 
            /* [retval][out] */ VARIANT_BOOL *records) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_RecordsChanges( 
            /* [in] */ VARIANT_BOOL record) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Region( 
            /* [retval][out] */ IRegion **region) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_RevisionNumber( 
            /* [retval][out] */ long *revisionNumber) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_UnicodeByteOrderMark( 
            /* [retval][out] */ VARIANT_BOOL *p) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BeginCompoundChange( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ClearUndoBuffer( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EndCompoundChange( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Erase( 
            /* [in] */ IRegion *region,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Insert( 
            /* [in] */ IPosition *position,
            /* [in] */ BSTR text,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InsertUndoBoundary( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MarkUnmodified( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE NarrowToRegion( 
            /* [in] */ IRegion *region) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Redo( 
            /* [defaultvalue][in] */ long n,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ResetContent( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Undo( 
            /* [defaultvalue][in] */ long n,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Widen( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBufferVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBuffer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBuffer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBuffer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBuffer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBuffer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBuffer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBuffer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AccessibleRegion )( 
            IBuffer * This,
            /* [retval][out] */ IRegion **region);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Bookmarker )( 
            IBuffer * This,
            /* [retval][out] */ IBookmarker **bookmarker);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BoundToFile )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *bound);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Encoding )( 
            IBuffer * This,
            /* [retval][out] */ BSTR *encoding);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Encoding )( 
            IBuffer * This,
            /* [in] */ BSTR encoding);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_InCompoundChanging )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *compound);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Length )( 
            IBuffer * This,
            /* [defaultvalue][in] */ Newline __MIDL__IBuffer0000,
            /* [retval][out] */ long *length);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Line )( 
            IBuffer * This,
            /* [in] */ long line,
            /* [retval][out] */ BSTR *s);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Modified )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *modified);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IBuffer * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Narrowed )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *narrowed);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Newline )( 
            IBuffer * This,
            /* [retval][out] */ Newline *newline);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Newline )( 
            IBuffer * This,
            /* [in] */ Newline newline);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ReadOnly )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *readOnly);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ReadOnly )( 
            IBuffer * This,
            /* [in] */ VARIANT_BOOL readOnly);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RecordsChanges )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *records);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_RecordsChanges )( 
            IBuffer * This,
            /* [in] */ VARIANT_BOOL record);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Region )( 
            IBuffer * This,
            /* [retval][out] */ IRegion **region);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_RevisionNumber )( 
            IBuffer * This,
            /* [retval][out] */ long *revisionNumber);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UnicodeByteOrderMark )( 
            IBuffer * This,
            /* [retval][out] */ VARIANT_BOOL *p);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BeginCompoundChange )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ClearUndoBuffer )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EndCompoundChange )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Erase )( 
            IBuffer * This,
            /* [in] */ IRegion *region,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Insert )( 
            IBuffer * This,
            /* [in] */ IPosition *position,
            /* [in] */ BSTR text,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InsertUndoBoundary )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MarkUnmodified )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *NarrowToRegion )( 
            IBuffer * This,
            /* [in] */ IRegion *region);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Redo )( 
            IBuffer * This,
            /* [defaultvalue][in] */ long n,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ResetContent )( 
            IBuffer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Undo )( 
            IBuffer * This,
            /* [defaultvalue][in] */ long n,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Widen )( 
            IBuffer * This);
        
        END_INTERFACE
    } IBufferVtbl;

    interface IBuffer
    {
        CONST_VTBL struct IBufferVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBuffer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBuffer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBuffer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBuffer_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IBuffer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IBuffer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IBuffer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IBuffer_get_AccessibleRegion(This,region)	\
    ( (This)->lpVtbl -> get_AccessibleRegion(This,region) ) 

#define IBuffer_get_Bookmarker(This,bookmarker)	\
    ( (This)->lpVtbl -> get_Bookmarker(This,bookmarker) ) 

#define IBuffer_get_BoundToFile(This,bound)	\
    ( (This)->lpVtbl -> get_BoundToFile(This,bound) ) 

#define IBuffer_get_Encoding(This,encoding)	\
    ( (This)->lpVtbl -> get_Encoding(This,encoding) ) 

#define IBuffer_put_Encoding(This,encoding)	\
    ( (This)->lpVtbl -> put_Encoding(This,encoding) ) 

#define IBuffer_get_InCompoundChanging(This,compound)	\
    ( (This)->lpVtbl -> get_InCompoundChanging(This,compound) ) 

#define IBuffer_get_Length(This,__MIDL__IBuffer0000,length)	\
    ( (This)->lpVtbl -> get_Length(This,__MIDL__IBuffer0000,length) ) 

#define IBuffer_get_Line(This,line,s)	\
    ( (This)->lpVtbl -> get_Line(This,line,s) ) 

#define IBuffer_get_Modified(This,modified)	\
    ( (This)->lpVtbl -> get_Modified(This,modified) ) 

#define IBuffer_get_Name(This,name)	\
    ( (This)->lpVtbl -> get_Name(This,name) ) 

#define IBuffer_get_Narrowed(This,narrowed)	\
    ( (This)->lpVtbl -> get_Narrowed(This,narrowed) ) 

#define IBuffer_get_Newline(This,newline)	\
    ( (This)->lpVtbl -> get_Newline(This,newline) ) 

#define IBuffer_put_Newline(This,newline)	\
    ( (This)->lpVtbl -> put_Newline(This,newline) ) 

#define IBuffer_get_ReadOnly(This,readOnly)	\
    ( (This)->lpVtbl -> get_ReadOnly(This,readOnly) ) 

#define IBuffer_put_ReadOnly(This,readOnly)	\
    ( (This)->lpVtbl -> put_ReadOnly(This,readOnly) ) 

#define IBuffer_get_RecordsChanges(This,records)	\
    ( (This)->lpVtbl -> get_RecordsChanges(This,records) ) 

#define IBuffer_put_RecordsChanges(This,record)	\
    ( (This)->lpVtbl -> put_RecordsChanges(This,record) ) 

#define IBuffer_get_Region(This,region)	\
    ( (This)->lpVtbl -> get_Region(This,region) ) 

#define IBuffer_get_RevisionNumber(This,revisionNumber)	\
    ( (This)->lpVtbl -> get_RevisionNumber(This,revisionNumber) ) 

#define IBuffer_get_UnicodeByteOrderMark(This,p)	\
    ( (This)->lpVtbl -> get_UnicodeByteOrderMark(This,p) ) 

#define IBuffer_BeginCompoundChange(This)	\
    ( (This)->lpVtbl -> BeginCompoundChange(This) ) 

#define IBuffer_ClearUndoBuffer(This)	\
    ( (This)->lpVtbl -> ClearUndoBuffer(This) ) 

#define IBuffer_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define IBuffer_EndCompoundChange(This)	\
    ( (This)->lpVtbl -> EndCompoundChange(This) ) 

#define IBuffer_Erase(This,region,result)	\
    ( (This)->lpVtbl -> Erase(This,region,result) ) 

#define IBuffer_Insert(This,position,text,result)	\
    ( (This)->lpVtbl -> Insert(This,position,text,result) ) 

#define IBuffer_InsertUndoBoundary(This)	\
    ( (This)->lpVtbl -> InsertUndoBoundary(This) ) 

#define IBuffer_MarkUnmodified(This)	\
    ( (This)->lpVtbl -> MarkUnmodified(This) ) 

#define IBuffer_NarrowToRegion(This,region)	\
    ( (This)->lpVtbl -> NarrowToRegion(This,region) ) 

#define IBuffer_Redo(This,n,result)	\
    ( (This)->lpVtbl -> Redo(This,n,result) ) 

#define IBuffer_ResetContent(This)	\
    ( (This)->lpVtbl -> ResetContent(This) ) 

#define IBuffer_Undo(This,n,result)	\
    ( (This)->lpVtbl -> Undo(This,n,result) ) 

#define IBuffer_Widen(This)	\
    ( (This)->lpVtbl -> Widen(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBuffer_INTERFACE_DEFINED__ */


#ifndef __IPoint_INTERFACE_DEFINED__
#define __IPoint_INTERFACE_DEFINED__

/* interface IPoint */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IPoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A92A18B8-6A66-4b89-968C-9F1F5E92FCD3")
    IPoint : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AdaptsToBuffer( 
            /* [retval][out] */ VARIANT_BOOL *adapts) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AdaptsToBuffer( 
            /* [in] */ VARIANT_BOOL adapt) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Buffer( 
            /* [retval][out] */ IBuffer **buffer) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Column( 
            /* [retval][out] */ long *column) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ExcludedFromRestriction( 
            /* [retval][out] */ VARIANT_BOOL *excluded) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ExcludedFromRestriction( 
            /* [in] */ VARIANT_BOOL excluded) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Gravity( 
            /* [retval][out] */ Direction *gravity) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Gravity( 
            /* [in] */ Direction gravity) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Line( 
            /* [retval][out] */ long *line) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Position( 
            /* [retval][out] */ IPosition **position) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsBufferDeleted( 
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MoveTo( 
            /* [in] */ IPosition *to) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPoint * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPoint * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPoint * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPoint * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdaptsToBuffer )( 
            IPoint * This,
            /* [retval][out] */ VARIANT_BOOL *adapts);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AdaptsToBuffer )( 
            IPoint * This,
            /* [in] */ VARIANT_BOOL adapt);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Buffer )( 
            IPoint * This,
            /* [retval][out] */ IBuffer **buffer);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Column )( 
            IPoint * This,
            /* [retval][out] */ long *column);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExcludedFromRestriction )( 
            IPoint * This,
            /* [retval][out] */ VARIANT_BOOL *excluded);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ExcludedFromRestriction )( 
            IPoint * This,
            /* [in] */ VARIANT_BOOL excluded);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Gravity )( 
            IPoint * This,
            /* [retval][out] */ Direction *gravity);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Gravity )( 
            IPoint * This,
            /* [in] */ Direction gravity);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Line )( 
            IPoint * This,
            /* [retval][out] */ long *line);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            IPoint * This,
            /* [retval][out] */ IPosition **position);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsBufferDeleted )( 
            IPoint * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            IPoint * This,
            /* [in] */ IPosition *to);
        
        END_INTERFACE
    } IPointVtbl;

    interface IPoint
    {
        CONST_VTBL struct IPointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPoint_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPoint_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPoint_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPoint_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPoint_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPoint_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPoint_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPoint_get_AdaptsToBuffer(This,adapts)	\
    ( (This)->lpVtbl -> get_AdaptsToBuffer(This,adapts) ) 

#define IPoint_put_AdaptsToBuffer(This,adapt)	\
    ( (This)->lpVtbl -> put_AdaptsToBuffer(This,adapt) ) 

#define IPoint_get_Buffer(This,buffer)	\
    ( (This)->lpVtbl -> get_Buffer(This,buffer) ) 

#define IPoint_get_Column(This,column)	\
    ( (This)->lpVtbl -> get_Column(This,column) ) 

#define IPoint_get_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> get_ExcludedFromRestriction(This,excluded) ) 

#define IPoint_put_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> put_ExcludedFromRestriction(This,excluded) ) 

#define IPoint_get_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> get_Gravity(This,gravity) ) 

#define IPoint_put_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> put_Gravity(This,gravity) ) 

#define IPoint_get_Line(This,line)	\
    ( (This)->lpVtbl -> get_Line(This,line) ) 

#define IPoint_get_Position(This,position)	\
    ( (This)->lpVtbl -> get_Position(This,position) ) 

#define IPoint_IsBufferDeleted(This,result)	\
    ( (This)->lpVtbl -> IsBufferDeleted(This,result) ) 

#define IPoint_MoveTo(This,to)	\
    ( (This)->lpVtbl -> MoveTo(This,to) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPoint_INTERFACE_DEFINED__ */


#ifndef __IEditPoint_INTERFACE_DEFINED__
#define __IEditPoint_INTERFACE_DEFINED__

/* interface IEditPoint */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IEditPoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A178F060-C5A6-4e56-8283-CEE0FAC58A35")
    IEditPoint : public IPoint
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IEditPointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IEditPoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IEditPoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IEditPoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IEditPoint * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IEditPoint * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IEditPoint * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IEditPoint * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdaptsToBuffer )( 
            IEditPoint * This,
            /* [retval][out] */ VARIANT_BOOL *adapts);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AdaptsToBuffer )( 
            IEditPoint * This,
            /* [in] */ VARIANT_BOOL adapt);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Buffer )( 
            IEditPoint * This,
            /* [retval][out] */ IBuffer **buffer);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Column )( 
            IEditPoint * This,
            /* [retval][out] */ long *column);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExcludedFromRestriction )( 
            IEditPoint * This,
            /* [retval][out] */ VARIANT_BOOL *excluded);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ExcludedFromRestriction )( 
            IEditPoint * This,
            /* [in] */ VARIANT_BOOL excluded);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Gravity )( 
            IEditPoint * This,
            /* [retval][out] */ Direction *gravity);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Gravity )( 
            IEditPoint * This,
            /* [in] */ Direction gravity);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Line )( 
            IEditPoint * This,
            /* [retval][out] */ long *line);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            IEditPoint * This,
            /* [retval][out] */ IPosition **position);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsBufferDeleted )( 
            IEditPoint * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            IEditPoint * This,
            /* [in] */ IPosition *to);
        
        END_INTERFACE
    } IEditPointVtbl;

    interface IEditPoint
    {
        CONST_VTBL struct IEditPointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEditPoint_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IEditPoint_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IEditPoint_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IEditPoint_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IEditPoint_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IEditPoint_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IEditPoint_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IEditPoint_get_AdaptsToBuffer(This,adapts)	\
    ( (This)->lpVtbl -> get_AdaptsToBuffer(This,adapts) ) 

#define IEditPoint_put_AdaptsToBuffer(This,adapt)	\
    ( (This)->lpVtbl -> put_AdaptsToBuffer(This,adapt) ) 

#define IEditPoint_get_Buffer(This,buffer)	\
    ( (This)->lpVtbl -> get_Buffer(This,buffer) ) 

#define IEditPoint_get_Column(This,column)	\
    ( (This)->lpVtbl -> get_Column(This,column) ) 

#define IEditPoint_get_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> get_ExcludedFromRestriction(This,excluded) ) 

#define IEditPoint_put_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> put_ExcludedFromRestriction(This,excluded) ) 

#define IEditPoint_get_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> get_Gravity(This,gravity) ) 

#define IEditPoint_put_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> put_Gravity(This,gravity) ) 

#define IEditPoint_get_Line(This,line)	\
    ( (This)->lpVtbl -> get_Line(This,line) ) 

#define IEditPoint_get_Position(This,position)	\
    ( (This)->lpVtbl -> get_Position(This,position) ) 

#define IEditPoint_IsBufferDeleted(This,result)	\
    ( (This)->lpVtbl -> IsBufferDeleted(This,result) ) 

#define IEditPoint_MoveTo(This,to)	\
    ( (This)->lpVtbl -> MoveTo(This,to) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IEditPoint_INTERFACE_DEFINED__ */


#ifndef __IBookmarker_INTERFACE_DEFINED__
#define __IBookmarker_INTERFACE_DEFINED__

/* interface IBookmarker */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IBookmarker;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AFACAF4D-69B8-4917-95D3-5170C67BF7E2")
    IBookmarker : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsMarked( 
            /* [in] */ long line,
            /* [retval][out] */ VARIANT_BOOL *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Mark( 
            /* [in] */ long line,
            /* [defaultvalue][in] */ VARIANT_BOOL set = ( VARIANT_BOOL  )-1) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long from,
            /* [in] */ Direction direction,
            /* [defaultvalue][in] */ VARIANT_BOOL wrapAround,
            /* [defaultvalue][in] */ long marks,
            /* [retval][out] */ long *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Toggle( 
            /* [in] */ long line) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBookmarkerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBookmarker * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBookmarker * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBookmarker * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBookmarker * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBookmarker * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBookmarker * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBookmarker * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IBookmarker * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsMarked )( 
            IBookmarker * This,
            /* [in] */ long line,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Mark )( 
            IBookmarker * This,
            /* [in] */ long line,
            /* [defaultvalue][in] */ VARIANT_BOOL set);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IBookmarker * This,
            /* [in] */ long from,
            /* [in] */ Direction direction,
            /* [defaultvalue][in] */ VARIANT_BOOL wrapAround,
            /* [defaultvalue][in] */ long marks,
            /* [retval][out] */ long *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Toggle )( 
            IBookmarker * This,
            /* [in] */ long line);
        
        END_INTERFACE
    } IBookmarkerVtbl;

    interface IBookmarker
    {
        CONST_VTBL struct IBookmarkerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBookmarker_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBookmarker_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBookmarker_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBookmarker_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IBookmarker_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IBookmarker_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IBookmarker_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IBookmarker_Clear(This)	\
    ( (This)->lpVtbl -> Clear(This) ) 

#define IBookmarker_IsMarked(This,line,result)	\
    ( (This)->lpVtbl -> IsMarked(This,line,result) ) 

#define IBookmarker_Mark(This,line,set)	\
    ( (This)->lpVtbl -> Mark(This,line,set) ) 

#define IBookmarker_Next(This,from,direction,wrapAround,marks,result)	\
    ( (This)->lpVtbl -> Next(This,from,direction,wrapAround,marks,result) ) 

#define IBookmarker_Toggle(This,line)	\
    ( (This)->lpVtbl -> Toggle(This,line) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBookmarker_INTERFACE_DEFINED__ */


#ifndef __IBufferList_INTERFACE_DEFINED__
#define __IBufferList_INTERFACE_DEFINED__

/* interface IBufferList */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IBufferList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A4F51429-782F-49fe-8840-E59B800CC393")
    IBufferList : public IDispatch
    {
    public:
        virtual /* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **enumerator) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IBuffer **buffer) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [retval][out] */ long *length) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddNew( 
            /* [defaultvalue][in] */ BSTR name,
            /* [defaultvalue][in] */ BSTR encoding,
            /* [defaultvalue][in] */ Newline newline,
            /* [retval][out] */ IBuffer **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddNewDialog( 
            /* [defaultvalue][in] */ BSTR name,
            /* [retval][out] */ IBuffer **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ BSTR fileName,
            /* [defaultvalue][in] */ BSTR encoding,
            /* [defaultvalue][in] */ FileLockMode lockMode,
            /* [defaultvalue][in] */ VARIANT_BOOL asReadOnly,
            /* [retval][out] */ IBuffer **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE OpenDialog( 
            /* [defaultvalue][in] */ BSTR initialDirectory,
            /* [retval][out] */ VARIANT_BOOL *succeeded) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveSomeDialog( 
            /* [retval][out] */ VARIANT_BOOL *ok) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBufferListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBufferList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBufferList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBufferList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IBufferList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IBufferList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IBufferList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IBufferList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [restricted][hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IBufferList * This,
            /* [retval][out] */ IUnknown **enumerator);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IBufferList * This,
            /* [in] */ long index,
            /* [retval][out] */ IBuffer **buffer);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Length )( 
            IBufferList * This,
            /* [retval][out] */ long *length);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddNew )( 
            IBufferList * This,
            /* [defaultvalue][in] */ BSTR name,
            /* [defaultvalue][in] */ BSTR encoding,
            /* [defaultvalue][in] */ Newline newline,
            /* [retval][out] */ IBuffer **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddNewDialog )( 
            IBufferList * This,
            /* [defaultvalue][in] */ BSTR name,
            /* [retval][out] */ IBuffer **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Open )( 
            IBufferList * This,
            /* [in] */ BSTR fileName,
            /* [defaultvalue][in] */ BSTR encoding,
            /* [defaultvalue][in] */ FileLockMode lockMode,
            /* [defaultvalue][in] */ VARIANT_BOOL asReadOnly,
            /* [retval][out] */ IBuffer **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *OpenDialog )( 
            IBufferList * This,
            /* [defaultvalue][in] */ BSTR initialDirectory,
            /* [retval][out] */ VARIANT_BOOL *succeeded);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SaveSomeDialog )( 
            IBufferList * This,
            /* [retval][out] */ VARIANT_BOOL *ok);
        
        END_INTERFACE
    } IBufferListVtbl;

    interface IBufferList
    {
        CONST_VTBL struct IBufferListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBufferList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBufferList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBufferList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBufferList_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IBufferList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IBufferList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IBufferList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IBufferList_get__NewEnum(This,enumerator)	\
    ( (This)->lpVtbl -> get__NewEnum(This,enumerator) ) 

#define IBufferList_get_Item(This,index,buffer)	\
    ( (This)->lpVtbl -> get_Item(This,index,buffer) ) 

#define IBufferList_get_Length(This,length)	\
    ( (This)->lpVtbl -> get_Length(This,length) ) 

#define IBufferList_AddNew(This,name,encoding,newline,result)	\
    ( (This)->lpVtbl -> AddNew(This,name,encoding,newline,result) ) 

#define IBufferList_AddNewDialog(This,name,result)	\
    ( (This)->lpVtbl -> AddNewDialog(This,name,result) ) 

#define IBufferList_Open(This,fileName,encoding,lockMode,asReadOnly,result)	\
    ( (This)->lpVtbl -> Open(This,fileName,encoding,lockMode,asReadOnly,result) ) 

#define IBufferList_OpenDialog(This,initialDirectory,succeeded)	\
    ( (This)->lpVtbl -> OpenDialog(This,initialDirectory,succeeded) ) 

#define IBufferList_SaveSomeDialog(This,ok)	\
    ( (This)->lpVtbl -> SaveSomeDialog(This,ok) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBufferList_INTERFACE_DEFINED__ */


#ifndef __IVisualPoint_INTERFACE_DEFINED__
#define __IVisualPoint_INTERFACE_DEFINED__

/* interface IVisualPoint */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IVisualPoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE850138-5112-41ba-B180-4CC399FA4D6D")
    IVisualPoint : public IEditPoint
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BeginningOfVisualLine( 
            /* [retval][out] */ IPosition **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CanPaste( 
            /* [retval][out] */ boolean *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EndOfVisualLine( 
            /* [retval][out] */ IPosition **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FirstPrintableCharacterOfLine( 
            /* [retval][out] */ IPosition **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FirstPrintableCharacterOfVisualLine( 
            /* [retval][out] */ IPosition **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ForwardPage( 
            /* [defaultvalue][in] */ long pages,
            /* [retval][out] */ IPosition **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ForwardVisualLine( 
            /* [defaultvalue][in] */ long lines,
            /* [retval][out] */ IPosition **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InsertRectangle( 
            /* [in] */ BSTR text,
            /* [retval][out] */ boolean *result) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_VisualColumn( 
            /* [retval][out] */ long **result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVisualPointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IVisualPoint * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IVisualPoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IVisualPoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IVisualPoint * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IVisualPoint * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IVisualPoint * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IVisualPoint * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdaptsToBuffer )( 
            IVisualPoint * This,
            /* [retval][out] */ VARIANT_BOOL *adapts);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AdaptsToBuffer )( 
            IVisualPoint * This,
            /* [in] */ VARIANT_BOOL adapt);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Buffer )( 
            IVisualPoint * This,
            /* [retval][out] */ IBuffer **buffer);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Column )( 
            IVisualPoint * This,
            /* [retval][out] */ long *column);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExcludedFromRestriction )( 
            IVisualPoint * This,
            /* [retval][out] */ VARIANT_BOOL *excluded);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ExcludedFromRestriction )( 
            IVisualPoint * This,
            /* [in] */ VARIANT_BOOL excluded);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Gravity )( 
            IVisualPoint * This,
            /* [retval][out] */ Direction *gravity);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Gravity )( 
            IVisualPoint * This,
            /* [in] */ Direction gravity);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Line )( 
            IVisualPoint * This,
            /* [retval][out] */ long *line);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            IVisualPoint * This,
            /* [retval][out] */ IPosition **position);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsBufferDeleted )( 
            IVisualPoint * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            IVisualPoint * This,
            /* [in] */ IPosition *to);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BeginningOfVisualLine )( 
            IVisualPoint * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CanPaste )( 
            IVisualPoint * This,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EndOfVisualLine )( 
            IVisualPoint * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FirstPrintableCharacterOfLine )( 
            IVisualPoint * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FirstPrintableCharacterOfVisualLine )( 
            IVisualPoint * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ForwardPage )( 
            IVisualPoint * This,
            /* [defaultvalue][in] */ long pages,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ForwardVisualLine )( 
            IVisualPoint * This,
            /* [defaultvalue][in] */ long lines,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InsertRectangle )( 
            IVisualPoint * This,
            /* [in] */ BSTR text,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VisualColumn )( 
            IVisualPoint * This,
            /* [retval][out] */ long **result);
        
        END_INTERFACE
    } IVisualPointVtbl;

    interface IVisualPoint
    {
        CONST_VTBL struct IVisualPointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVisualPoint_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IVisualPoint_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IVisualPoint_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IVisualPoint_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IVisualPoint_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IVisualPoint_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IVisualPoint_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IVisualPoint_get_AdaptsToBuffer(This,adapts)	\
    ( (This)->lpVtbl -> get_AdaptsToBuffer(This,adapts) ) 

#define IVisualPoint_put_AdaptsToBuffer(This,adapt)	\
    ( (This)->lpVtbl -> put_AdaptsToBuffer(This,adapt) ) 

#define IVisualPoint_get_Buffer(This,buffer)	\
    ( (This)->lpVtbl -> get_Buffer(This,buffer) ) 

#define IVisualPoint_get_Column(This,column)	\
    ( (This)->lpVtbl -> get_Column(This,column) ) 

#define IVisualPoint_get_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> get_ExcludedFromRestriction(This,excluded) ) 

#define IVisualPoint_put_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> put_ExcludedFromRestriction(This,excluded) ) 

#define IVisualPoint_get_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> get_Gravity(This,gravity) ) 

#define IVisualPoint_put_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> put_Gravity(This,gravity) ) 

#define IVisualPoint_get_Line(This,line)	\
    ( (This)->lpVtbl -> get_Line(This,line) ) 

#define IVisualPoint_get_Position(This,position)	\
    ( (This)->lpVtbl -> get_Position(This,position) ) 

#define IVisualPoint_IsBufferDeleted(This,result)	\
    ( (This)->lpVtbl -> IsBufferDeleted(This,result) ) 

#define IVisualPoint_MoveTo(This,to)	\
    ( (This)->lpVtbl -> MoveTo(This,to) ) 



#define IVisualPoint_BeginningOfVisualLine(This,result)	\
    ( (This)->lpVtbl -> BeginningOfVisualLine(This,result) ) 

#define IVisualPoint_CanPaste(This,result)	\
    ( (This)->lpVtbl -> CanPaste(This,result) ) 

#define IVisualPoint_EndOfVisualLine(This,result)	\
    ( (This)->lpVtbl -> EndOfVisualLine(This,result) ) 

#define IVisualPoint_FirstPrintableCharacterOfLine(This,result)	\
    ( (This)->lpVtbl -> FirstPrintableCharacterOfLine(This,result) ) 

#define IVisualPoint_FirstPrintableCharacterOfVisualLine(This,result)	\
    ( (This)->lpVtbl -> FirstPrintableCharacterOfVisualLine(This,result) ) 

#define IVisualPoint_ForwardPage(This,pages,result)	\
    ( (This)->lpVtbl -> ForwardPage(This,pages,result) ) 

#define IVisualPoint_ForwardVisualLine(This,lines,result)	\
    ( (This)->lpVtbl -> ForwardVisualLine(This,lines,result) ) 

#define IVisualPoint_InsertRectangle(This,text,result)	\
    ( (This)->lpVtbl -> InsertRectangle(This,text,result) ) 

#define IVisualPoint_get_VisualColumn(This,result)	\
    ( (This)->lpVtbl -> get_VisualColumn(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IVisualPoint_INTERFACE_DEFINED__ */


#ifndef __ICaret_INTERFACE_DEFINED__
#define __ICaret_INTERFACE_DEFINED__

/* interface ICaret */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_ICaret;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA709221-38BC-4581-AF37-F012E97A1327")
    ICaret : public IVisualPoint
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Anchor( 
            /* [retval][out] */ IPosition **anchor) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Beginning( 
            /* [retval][out] */ IPosition **beginning) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BeginRectangleSelection( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ClearSelection( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopySelection( 
            /* [defaultvalue][in] */ boolean useKillRing = ( VARIANT_BOOL  )0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CutSelection( 
            /* [defaultvalue][in] */ boolean useKillRing = ( VARIANT_BOOL  )0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE End( 
            /* [retval][out] */ IPosition **end) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EndRectangleSelection( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EraseSelection( 
            /* [retval][out] */ boolean **succeeded) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExtendSelection( 
            /* [in] */ IDispatch *to) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InputCharacter( 
            /* [in] */ long character,
            /* [defaultvalue][in] */ boolean validateSequence,
            /* [defaultvalue][in] */ boolean blockControls,
            /* [retval][out] */ boolean *succeeded) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsOvertypeMode( 
            /* [retval][out] */ boolean *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsSelectionEmpty( 
            /* [retval][out] */ boolean *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsSelectionRectangle( 
            /* [retval][out] */ boolean *result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PasteToSelection( 
            /* [defaultvalue][in] */ boolean useKillRing = ( VARIANT_BOOL  )0) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReplaceSelection( 
            /* [in] */ BSTR text,
            /* [defaultvalue][in] */ boolean rectangleInsertion,
            /* [retval][out] */ boolean *succeeded) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Select( 
            /* [in] */ IRegion *region) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SelectionRegion( 
            /* [retval][out] */ IRegion **region) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SelectionText( 
            /* [defaultvalue][in] */ Newline newline,
            /* [retval][out] */ BSTR *text) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SelectWord( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetOvertypeMode( 
            /* [defaultvalue][in] */ boolean enable = ( VARIANT_BOOL  )-1) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowAutomatically( 
            /* [defaultvalue][in] */ boolean enable = ( VARIANT_BOOL  )-1) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowsAutomatically( 
            /* [retval][out] */ boolean *enabled) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICaretVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICaret * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICaret * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICaret * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICaret * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICaret * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICaret * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICaret * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_AdaptsToBuffer )( 
            ICaret * This,
            /* [retval][out] */ VARIANT_BOOL *adapts);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_AdaptsToBuffer )( 
            ICaret * This,
            /* [in] */ VARIANT_BOOL adapt);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Buffer )( 
            ICaret * This,
            /* [retval][out] */ IBuffer **buffer);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Column )( 
            ICaret * This,
            /* [retval][out] */ long *column);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExcludedFromRestriction )( 
            ICaret * This,
            /* [retval][out] */ VARIANT_BOOL *excluded);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ExcludedFromRestriction )( 
            ICaret * This,
            /* [in] */ VARIANT_BOOL excluded);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Gravity )( 
            ICaret * This,
            /* [retval][out] */ Direction *gravity);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Gravity )( 
            ICaret * This,
            /* [in] */ Direction gravity);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Line )( 
            ICaret * This,
            /* [retval][out] */ long *line);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            ICaret * This,
            /* [retval][out] */ IPosition **position);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsBufferDeleted )( 
            ICaret * This,
            /* [retval][out] */ VARIANT_BOOL *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveTo )( 
            ICaret * This,
            /* [in] */ IPosition *to);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BeginningOfVisualLine )( 
            ICaret * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CanPaste )( 
            ICaret * This,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EndOfVisualLine )( 
            ICaret * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FirstPrintableCharacterOfLine )( 
            ICaret * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FirstPrintableCharacterOfVisualLine )( 
            ICaret * This,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ForwardPage )( 
            ICaret * This,
            /* [defaultvalue][in] */ long pages,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ForwardVisualLine )( 
            ICaret * This,
            /* [defaultvalue][in] */ long lines,
            /* [retval][out] */ IPosition **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InsertRectangle )( 
            ICaret * This,
            /* [in] */ BSTR text,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_VisualColumn )( 
            ICaret * This,
            /* [retval][out] */ long **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Anchor )( 
            ICaret * This,
            /* [retval][out] */ IPosition **anchor);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Beginning )( 
            ICaret * This,
            /* [retval][out] */ IPosition **beginning);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BeginRectangleSelection )( 
            ICaret * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ClearSelection )( 
            ICaret * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CopySelection )( 
            ICaret * This,
            /* [defaultvalue][in] */ boolean useKillRing);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CutSelection )( 
            ICaret * This,
            /* [defaultvalue][in] */ boolean useKillRing);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *End )( 
            ICaret * This,
            /* [retval][out] */ IPosition **end);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EndRectangleSelection )( 
            ICaret * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *EraseSelection )( 
            ICaret * This,
            /* [retval][out] */ boolean **succeeded);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ExtendSelection )( 
            ICaret * This,
            /* [in] */ IDispatch *to);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InputCharacter )( 
            ICaret * This,
            /* [in] */ long character,
            /* [defaultvalue][in] */ boolean validateSequence,
            /* [defaultvalue][in] */ boolean blockControls,
            /* [retval][out] */ boolean *succeeded);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsOvertypeMode )( 
            ICaret * This,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsSelectionEmpty )( 
            ICaret * This,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsSelectionRectangle )( 
            ICaret * This,
            /* [retval][out] */ boolean *result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *PasteToSelection )( 
            ICaret * This,
            /* [defaultvalue][in] */ boolean useKillRing);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ReplaceSelection )( 
            ICaret * This,
            /* [in] */ BSTR text,
            /* [defaultvalue][in] */ boolean rectangleInsertion,
            /* [retval][out] */ boolean *succeeded);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Select )( 
            ICaret * This,
            /* [in] */ IRegion *region);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SelectionRegion )( 
            ICaret * This,
            /* [retval][out] */ IRegion **region);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SelectionText )( 
            ICaret * This,
            /* [defaultvalue][in] */ Newline newline,
            /* [retval][out] */ BSTR *text);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SelectWord )( 
            ICaret * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetOvertypeMode )( 
            ICaret * This,
            /* [defaultvalue][in] */ boolean enable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ShowAutomatically )( 
            ICaret * This,
            /* [defaultvalue][in] */ boolean enable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ShowsAutomatically )( 
            ICaret * This,
            /* [retval][out] */ boolean *enabled);
        
        END_INTERFACE
    } ICaretVtbl;

    interface ICaret
    {
        CONST_VTBL struct ICaretVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICaret_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICaret_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICaret_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICaret_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ICaret_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ICaret_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ICaret_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ICaret_get_AdaptsToBuffer(This,adapts)	\
    ( (This)->lpVtbl -> get_AdaptsToBuffer(This,adapts) ) 

#define ICaret_put_AdaptsToBuffer(This,adapt)	\
    ( (This)->lpVtbl -> put_AdaptsToBuffer(This,adapt) ) 

#define ICaret_get_Buffer(This,buffer)	\
    ( (This)->lpVtbl -> get_Buffer(This,buffer) ) 

#define ICaret_get_Column(This,column)	\
    ( (This)->lpVtbl -> get_Column(This,column) ) 

#define ICaret_get_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> get_ExcludedFromRestriction(This,excluded) ) 

#define ICaret_put_ExcludedFromRestriction(This,excluded)	\
    ( (This)->lpVtbl -> put_ExcludedFromRestriction(This,excluded) ) 

#define ICaret_get_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> get_Gravity(This,gravity) ) 

#define ICaret_put_Gravity(This,gravity)	\
    ( (This)->lpVtbl -> put_Gravity(This,gravity) ) 

#define ICaret_get_Line(This,line)	\
    ( (This)->lpVtbl -> get_Line(This,line) ) 

#define ICaret_get_Position(This,position)	\
    ( (This)->lpVtbl -> get_Position(This,position) ) 

#define ICaret_IsBufferDeleted(This,result)	\
    ( (This)->lpVtbl -> IsBufferDeleted(This,result) ) 

#define ICaret_MoveTo(This,to)	\
    ( (This)->lpVtbl -> MoveTo(This,to) ) 



#define ICaret_BeginningOfVisualLine(This,result)	\
    ( (This)->lpVtbl -> BeginningOfVisualLine(This,result) ) 

#define ICaret_CanPaste(This,result)	\
    ( (This)->lpVtbl -> CanPaste(This,result) ) 

#define ICaret_EndOfVisualLine(This,result)	\
    ( (This)->lpVtbl -> EndOfVisualLine(This,result) ) 

#define ICaret_FirstPrintableCharacterOfLine(This,result)	\
    ( (This)->lpVtbl -> FirstPrintableCharacterOfLine(This,result) ) 

#define ICaret_FirstPrintableCharacterOfVisualLine(This,result)	\
    ( (This)->lpVtbl -> FirstPrintableCharacterOfVisualLine(This,result) ) 

#define ICaret_ForwardPage(This,pages,result)	\
    ( (This)->lpVtbl -> ForwardPage(This,pages,result) ) 

#define ICaret_ForwardVisualLine(This,lines,result)	\
    ( (This)->lpVtbl -> ForwardVisualLine(This,lines,result) ) 

#define ICaret_InsertRectangle(This,text,result)	\
    ( (This)->lpVtbl -> InsertRectangle(This,text,result) ) 

#define ICaret_get_VisualColumn(This,result)	\
    ( (This)->lpVtbl -> get_VisualColumn(This,result) ) 


#define ICaret_Anchor(This,anchor)	\
    ( (This)->lpVtbl -> Anchor(This,anchor) ) 

#define ICaret_Beginning(This,beginning)	\
    ( (This)->lpVtbl -> Beginning(This,beginning) ) 

#define ICaret_BeginRectangleSelection(This)	\
    ( (This)->lpVtbl -> BeginRectangleSelection(This) ) 

#define ICaret_ClearSelection(This)	\
    ( (This)->lpVtbl -> ClearSelection(This) ) 

#define ICaret_CopySelection(This,useKillRing)	\
    ( (This)->lpVtbl -> CopySelection(This,useKillRing) ) 

#define ICaret_CutSelection(This,useKillRing)	\
    ( (This)->lpVtbl -> CutSelection(This,useKillRing) ) 

#define ICaret_End(This,end)	\
    ( (This)->lpVtbl -> End(This,end) ) 

#define ICaret_EndRectangleSelection(This)	\
    ( (This)->lpVtbl -> EndRectangleSelection(This) ) 

#define ICaret_EraseSelection(This,succeeded)	\
    ( (This)->lpVtbl -> EraseSelection(This,succeeded) ) 

#define ICaret_ExtendSelection(This,to)	\
    ( (This)->lpVtbl -> ExtendSelection(This,to) ) 

#define ICaret_InputCharacter(This,character,validateSequence,blockControls,succeeded)	\
    ( (This)->lpVtbl -> InputCharacter(This,character,validateSequence,blockControls,succeeded) ) 

#define ICaret_IsOvertypeMode(This,result)	\
    ( (This)->lpVtbl -> IsOvertypeMode(This,result) ) 

#define ICaret_IsSelectionEmpty(This,result)	\
    ( (This)->lpVtbl -> IsSelectionEmpty(This,result) ) 

#define ICaret_IsSelectionRectangle(This,result)	\
    ( (This)->lpVtbl -> IsSelectionRectangle(This,result) ) 

#define ICaret_PasteToSelection(This,useKillRing)	\
    ( (This)->lpVtbl -> PasteToSelection(This,useKillRing) ) 

#define ICaret_ReplaceSelection(This,text,rectangleInsertion,succeeded)	\
    ( (This)->lpVtbl -> ReplaceSelection(This,text,rectangleInsertion,succeeded) ) 

#define ICaret_Select(This,region)	\
    ( (This)->lpVtbl -> Select(This,region) ) 

#define ICaret_SelectionRegion(This,region)	\
    ( (This)->lpVtbl -> SelectionRegion(This,region) ) 

#define ICaret_SelectionText(This,newline,text)	\
    ( (This)->lpVtbl -> SelectionText(This,newline,text) ) 

#define ICaret_SelectWord(This)	\
    ( (This)->lpVtbl -> SelectWord(This) ) 

#define ICaret_SetOvertypeMode(This,enable)	\
    ( (This)->lpVtbl -> SetOvertypeMode(This,enable) ) 

#define ICaret_ShowAutomatically(This,enable)	\
    ( (This)->lpVtbl -> ShowAutomatically(This,enable) ) 

#define ICaret_ShowsAutomatically(This,enabled)	\
    ( (This)->lpVtbl -> ShowsAutomatically(This,enabled) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICaret_INTERFACE_DEFINED__ */


#ifndef __ITextEditor_INTERFACE_DEFINED__
#define __ITextEditor_INTERFACE_DEFINED__

/* interface ITextEditor */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_ITextEditor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A80D020F-9576-4fae-B9DE-A000B7F9EDEB")
    ITextEditor : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetBuffer( 
            /* [retval][out] */ IBuffer **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCaret( 
            /* [retval][out] */ ICaret **result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITextEditorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITextEditor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITextEditor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITextEditor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITextEditor * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITextEditor * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITextEditor * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITextEditor * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
            ITextEditor * This,
            /* [retval][out] */ IBuffer **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetCaret )( 
            ITextEditor * This,
            /* [retval][out] */ ICaret **result);
        
        END_INTERFACE
    } ITextEditorVtbl;

    interface ITextEditor
    {
        CONST_VTBL struct ITextEditorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITextEditor_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ITextEditor_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ITextEditor_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ITextEditor_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define ITextEditor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define ITextEditor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define ITextEditor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define ITextEditor_GetBuffer(This,result)	\
    ( (This)->lpVtbl -> GetBuffer(This,result) ) 

#define ITextEditor_GetCaret(This,result)	\
    ( (This)->lpVtbl -> GetCaret(This,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITextEditor_INTERFACE_DEFINED__ */


#ifndef __IWindow_INTERFACE_DEFINED__
#define __IWindow_INTERFACE_DEFINED__

/* interface IWindow */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IWindow;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A5B43144-7520-41ba-A5D9-021AC23B2BA6")
    IWindow : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Activate( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Close( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Select( 
            /* [in] */ VARIANT *o) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SelectedBuffer( 
            /* [retval][out] */ IBuffer **result) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SelectedEditor( 
            /* [retval][out] */ ITextEditor **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Split( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SplitSideBySide( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWindowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWindow * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWindow * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWindow * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWindow * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWindow * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWindow * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWindow * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Activate )( 
            IWindow * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Close )( 
            IWindow * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Select )( 
            IWindow * This,
            /* [in] */ VARIANT *o);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedBuffer )( 
            IWindow * This,
            /* [retval][out] */ IBuffer **result);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SelectedEditor )( 
            IWindow * This,
            /* [retval][out] */ ITextEditor **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Split )( 
            IWindow * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SplitSideBySide )( 
            IWindow * This);
        
        END_INTERFACE
    } IWindowVtbl;

    interface IWindow
    {
        CONST_VTBL struct IWindowVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWindow_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IWindow_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IWindow_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IWindow_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IWindow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IWindow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IWindow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IWindow_Activate(This)	\
    ( (This)->lpVtbl -> Activate(This) ) 

#define IWindow_Close(This)	\
    ( (This)->lpVtbl -> Close(This) ) 

#define IWindow_Select(This,o)	\
    ( (This)->lpVtbl -> Select(This,o) ) 

#define IWindow_get_SelectedBuffer(This,result)	\
    ( (This)->lpVtbl -> get_SelectedBuffer(This,result) ) 

#define IWindow_get_SelectedEditor(This,result)	\
    ( (This)->lpVtbl -> get_SelectedEditor(This,result) ) 

#define IWindow_Split(This)	\
    ( (This)->lpVtbl -> Split(This) ) 

#define IWindow_SplitSideBySide(This)	\
    ( (This)->lpVtbl -> SplitSideBySide(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IWindow_INTERFACE_DEFINED__ */


#ifndef __IWindowList_INTERFACE_DEFINED__
#define __IWindowList_INTERFACE_DEFINED__

/* interface IWindowList */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IWindowList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD5BE76A-0203-4a0f-AE7A-4152B36EF15A")
    IWindowList : public IDispatch
    {
    public:
        virtual /* [restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE _NewEnum( 
            /* [retval][out] */ IUnknown **enumerator) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ VARIANT **value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [retval][out] */ long *length) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ActivateNext( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ActivatePrevious( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnsplitAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWindowListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IWindowList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IWindowList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IWindowList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IWindowList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IWindowList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IWindowList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IWindowList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *_NewEnum )( 
            IWindowList * This,
            /* [retval][out] */ IUnknown **enumerator);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IWindowList * This,
            /* [in] */ long index,
            /* [retval][out] */ VARIANT **value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Length )( 
            IWindowList * This,
            /* [retval][out] */ long *length);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ActivateNext )( 
            IWindowList * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ActivatePrevious )( 
            IWindowList * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnsplitAll )( 
            IWindowList * This);
        
        END_INTERFACE
    } IWindowListVtbl;

    interface IWindowList
    {
        CONST_VTBL struct IWindowListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWindowList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IWindowList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IWindowList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IWindowList_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IWindowList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IWindowList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IWindowList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IWindowList__NewEnum(This,enumerator)	\
    ( (This)->lpVtbl -> _NewEnum(This,enumerator) ) 

#define IWindowList_get_Item(This,index,value)	\
    ( (This)->lpVtbl -> get_Item(This,index,value) ) 

#define IWindowList_get_Length(This,length)	\
    ( (This)->lpVtbl -> get_Length(This,length) ) 

#define IWindowList_ActivateNext(This)	\
    ( (This)->lpVtbl -> ActivateNext(This) ) 

#define IWindowList_ActivatePrevious(This)	\
    ( (This)->lpVtbl -> ActivatePrevious(This) ) 

#define IWindowList_UnsplitAll(This)	\
    ( (This)->lpVtbl -> UnsplitAll(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IWindowList_INTERFACE_DEFINED__ */


#ifndef __IMenu_INTERFACE_DEFINED__
#define __IMenu_INTERFACE_DEFINED__

/* interface IMenu */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A373ED31-7A38-45f2-A7A8-29F31554FC85")
    IMenu : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Append( 
            /* [in] */ short identifier,
            /* [in] */ BSTR caption,
            /* [in] */ VARIANT *command,
            /* [defaultvalue][in] */ VARIANT_BOOL alternative,
            /* [retval][out] */ IMenu **self) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AppendSeparator( 
            /* [retval][out] */ IMenu **self) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Check( 
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL check,
            /* [retval][out] */ IMenu **self) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Enable( 
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL enable,
            /* [retval][out] */ IMenu **self) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Erase( 
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self) = 0;
        
        virtual /* [restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE GetHandle( 
            /* [out] */ LONG_PTR *handle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetChild( 
            /* [in] */ short identifier,
            /* [in] */ IMenu *child,
            /* [retval][out] */ IMenu **self) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetDefault( 
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMenu * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMenu * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMenu * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMenu * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            IMenu * This,
            /* [in] */ short identifier,
            /* [in] */ BSTR caption,
            /* [in] */ VARIANT *command,
            /* [defaultvalue][in] */ VARIANT_BOOL alternative,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AppendSeparator )( 
            IMenu * This,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Check )( 
            IMenu * This,
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL check,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Enable )( 
            IMenu * This,
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL enable,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Erase )( 
            IMenu * This,
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self);
        
        /* [restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *GetHandle )( 
            IMenu * This,
            /* [out] */ LONG_PTR *handle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetChild )( 
            IMenu * This,
            /* [in] */ short identifier,
            /* [in] */ IMenu *child,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetDefault )( 
            IMenu * This,
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self);
        
        END_INTERFACE
    } IMenuVtbl;

    interface IMenu
    {
        CONST_VTBL struct IMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMenu_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMenu_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMenu_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMenu_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMenu_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMenu_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMenu_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMenu_Append(This,identifier,caption,command,alternative,self)	\
    ( (This)->lpVtbl -> Append(This,identifier,caption,command,alternative,self) ) 

#define IMenu_AppendSeparator(This,self)	\
    ( (This)->lpVtbl -> AppendSeparator(This,self) ) 

#define IMenu_Check(This,identifier,check,self)	\
    ( (This)->lpVtbl -> Check(This,identifier,check,self) ) 

#define IMenu_Enable(This,identifier,enable,self)	\
    ( (This)->lpVtbl -> Enable(This,identifier,enable,self) ) 

#define IMenu_Erase(This,identifier,self)	\
    ( (This)->lpVtbl -> Erase(This,identifier,self) ) 

#define IMenu_GetHandle(This,handle)	\
    ( (This)->lpVtbl -> GetHandle(This,handle) ) 

#define IMenu_SetChild(This,identifier,child,self)	\
    ( (This)->lpVtbl -> SetChild(This,identifier,child,self) ) 

#define IMenu_SetDefault(This,identifier,self)	\
    ( (This)->lpVtbl -> SetDefault(This,identifier,self) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMenu_INTERFACE_DEFINED__ */


#ifndef __IPopupMenu_INTERFACE_DEFINED__
#define __IPopupMenu_INTERFACE_DEFINED__

/* interface IPopupMenu */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IPopupMenu;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A88658D8-EF74-4c5a-B7D3-7953BC90F367")
    IPopupMenu : public IMenu
    {
    public:
        virtual /* [restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ short identifier) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPopupMenuVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPopupMenu * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPopupMenu * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPopupMenu * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPopupMenu * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPopupMenu * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPopupMenu * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPopupMenu * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            IPopupMenu * This,
            /* [in] */ short identifier,
            /* [in] */ BSTR caption,
            /* [in] */ VARIANT *command,
            /* [defaultvalue][in] */ VARIANT_BOOL alternative,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AppendSeparator )( 
            IPopupMenu * This,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Check )( 
            IPopupMenu * This,
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL check,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Enable )( 
            IPopupMenu * This,
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL enable,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Erase )( 
            IPopupMenu * This,
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self);
        
        /* [restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *GetHandle )( 
            IPopupMenu * This,
            /* [out] */ LONG_PTR *handle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetChild )( 
            IPopupMenu * This,
            /* [in] */ short identifier,
            /* [in] */ IMenu *child,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetDefault )( 
            IPopupMenu * This,
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self);
        
        /* [restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *Update )( 
            IPopupMenu * This,
            /* [in] */ short identifier);
        
        END_INTERFACE
    } IPopupMenuVtbl;

    interface IPopupMenu
    {
        CONST_VTBL struct IPopupMenuVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPopupMenu_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPopupMenu_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPopupMenu_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPopupMenu_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPopupMenu_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPopupMenu_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPopupMenu_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPopupMenu_Append(This,identifier,caption,command,alternative,self)	\
    ( (This)->lpVtbl -> Append(This,identifier,caption,command,alternative,self) ) 

#define IPopupMenu_AppendSeparator(This,self)	\
    ( (This)->lpVtbl -> AppendSeparator(This,self) ) 

#define IPopupMenu_Check(This,identifier,check,self)	\
    ( (This)->lpVtbl -> Check(This,identifier,check,self) ) 

#define IPopupMenu_Enable(This,identifier,enable,self)	\
    ( (This)->lpVtbl -> Enable(This,identifier,enable,self) ) 

#define IPopupMenu_Erase(This,identifier,self)	\
    ( (This)->lpVtbl -> Erase(This,identifier,self) ) 

#define IPopupMenu_GetHandle(This,handle)	\
    ( (This)->lpVtbl -> GetHandle(This,handle) ) 

#define IPopupMenu_SetChild(This,identifier,child,self)	\
    ( (This)->lpVtbl -> SetChild(This,identifier,child,self) ) 

#define IPopupMenu_SetDefault(This,identifier,self)	\
    ( (This)->lpVtbl -> SetDefault(This,identifier,self) ) 


#define IPopupMenu_Update(This,identifier)	\
    ( (This)->lpVtbl -> Update(This,identifier) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPopupMenu_INTERFACE_DEFINED__ */


#ifndef __IPopupMenuConstructor_INTERFACE_DEFINED__
#define __IPopupMenuConstructor_INTERFACE_DEFINED__

/* interface IPopupMenuConstructor */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IPopupMenuConstructor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A53B0C04-F00D-4a85-9A83-272CF7034570")
    IPopupMenuConstructor : public IDispatchEx
    {
    public:
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE Construct( 
            /* [optional][in] */ VARIANT popupHandler,
            /* [retval][out] */ IPopupMenu **instance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPopupMenuConstructorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPopupMenuConstructor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPopupMenuConstructor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPopupMenuConstructor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IPopupMenuConstructor * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IPopupMenuConstructor * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IPopupMenuConstructor * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IPopupMenuConstructor * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *GetDispID )( 
            IPopupMenuConstructor * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD grfdex,
            /* [out] */ DISPID *pid);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *InvokeEx )( 
            IPopupMenuConstructor * This,
            /* [in] */ DISPID id,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS *pdp,
            /* [out] */ VARIANT *pvarRes,
            /* [out] */ EXCEPINFO *pei,
            /* [unique][in] */ IServiceProvider *pspCaller);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMemberByName )( 
            IPopupMenuConstructor * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD grfdex);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMemberByDispID )( 
            IPopupMenuConstructor * This,
            /* [in] */ DISPID id);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemberProperties )( 
            IPopupMenuConstructor * This,
            /* [in] */ DISPID id,
            /* [in] */ DWORD grfdexFetch,
            /* [out] */ DWORD *pgrfdex);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemberName )( 
            IPopupMenuConstructor * This,
            /* [in] */ DISPID id,
            /* [out] */ BSTR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextDispID )( 
            IPopupMenuConstructor * This,
            /* [in] */ DWORD grfdex,
            /* [in] */ DISPID id,
            /* [out] */ DISPID *pid);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameSpaceParent )( 
            IPopupMenuConstructor * This,
            /* [out] */ IUnknown **ppunk);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *Construct )( 
            IPopupMenuConstructor * This,
            /* [optional][in] */ VARIANT popupHandler,
            /* [retval][out] */ IPopupMenu **instance);
        
        END_INTERFACE
    } IPopupMenuConstructorVtbl;

    interface IPopupMenuConstructor
    {
        CONST_VTBL struct IPopupMenuConstructorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPopupMenuConstructor_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IPopupMenuConstructor_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IPopupMenuConstructor_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IPopupMenuConstructor_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IPopupMenuConstructor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IPopupMenuConstructor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IPopupMenuConstructor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IPopupMenuConstructor_GetDispID(This,bstrName,grfdex,pid)	\
    ( (This)->lpVtbl -> GetDispID(This,bstrName,grfdex,pid) ) 

#define IPopupMenuConstructor_InvokeEx(This,id,lcid,wFlags,pdp,pvarRes,pei,pspCaller)	\
    ( (This)->lpVtbl -> InvokeEx(This,id,lcid,wFlags,pdp,pvarRes,pei,pspCaller) ) 

#define IPopupMenuConstructor_DeleteMemberByName(This,bstrName,grfdex)	\
    ( (This)->lpVtbl -> DeleteMemberByName(This,bstrName,grfdex) ) 

#define IPopupMenuConstructor_DeleteMemberByDispID(This,id)	\
    ( (This)->lpVtbl -> DeleteMemberByDispID(This,id) ) 

#define IPopupMenuConstructor_GetMemberProperties(This,id,grfdexFetch,pgrfdex)	\
    ( (This)->lpVtbl -> GetMemberProperties(This,id,grfdexFetch,pgrfdex) ) 

#define IPopupMenuConstructor_GetMemberName(This,id,pbstrName)	\
    ( (This)->lpVtbl -> GetMemberName(This,id,pbstrName) ) 

#define IPopupMenuConstructor_GetNextDispID(This,grfdex,id,pid)	\
    ( (This)->lpVtbl -> GetNextDispID(This,grfdex,id,pid) ) 

#define IPopupMenuConstructor_GetNameSpaceParent(This,ppunk)	\
    ( (This)->lpVtbl -> GetNameSpaceParent(This,ppunk) ) 


#define IPopupMenuConstructor_Construct(This,popupHandler,instance)	\
    ( (This)->lpVtbl -> Construct(This,popupHandler,instance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPopupMenuConstructor_INTERFACE_DEFINED__ */


#ifndef __IMenuBar_INTERFACE_DEFINED__
#define __IMenuBar_INTERFACE_DEFINED__

/* interface IMenuBar */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IMenuBar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AF898B12-1F02-4517-8FD1-6C810F2262B8")
    IMenuBar : public IMenu
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetAsMenuBar( 
            /* [retval][out] */ IMenuBar **oldMenuBar) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMenuBarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMenuBar * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMenuBar * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMenuBar * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMenuBar * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMenuBar * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMenuBar * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMenuBar * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Append )( 
            IMenuBar * This,
            /* [in] */ short identifier,
            /* [in] */ BSTR caption,
            /* [in] */ VARIANT *command,
            /* [defaultvalue][in] */ VARIANT_BOOL alternative,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AppendSeparator )( 
            IMenuBar * This,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Check )( 
            IMenuBar * This,
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL check,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Enable )( 
            IMenuBar * This,
            /* [in] */ short identifier,
            /* [in] */ VARIANT_BOOL enable,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Erase )( 
            IMenuBar * This,
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self);
        
        /* [restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *GetHandle )( 
            IMenuBar * This,
            /* [out] */ LONG_PTR *handle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetChild )( 
            IMenuBar * This,
            /* [in] */ short identifier,
            /* [in] */ IMenu *child,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetDefault )( 
            IMenuBar * This,
            /* [in] */ short identifier,
            /* [retval][out] */ IMenu **self);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetAsMenuBar )( 
            IMenuBar * This,
            /* [retval][out] */ IMenuBar **oldMenuBar);
        
        END_INTERFACE
    } IMenuBarVtbl;

    interface IMenuBar
    {
        CONST_VTBL struct IMenuBarVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMenuBar_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMenuBar_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMenuBar_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMenuBar_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMenuBar_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMenuBar_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMenuBar_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMenuBar_Append(This,identifier,caption,command,alternative,self)	\
    ( (This)->lpVtbl -> Append(This,identifier,caption,command,alternative,self) ) 

#define IMenuBar_AppendSeparator(This,self)	\
    ( (This)->lpVtbl -> AppendSeparator(This,self) ) 

#define IMenuBar_Check(This,identifier,check,self)	\
    ( (This)->lpVtbl -> Check(This,identifier,check,self) ) 

#define IMenuBar_Enable(This,identifier,enable,self)	\
    ( (This)->lpVtbl -> Enable(This,identifier,enable,self) ) 

#define IMenuBar_Erase(This,identifier,self)	\
    ( (This)->lpVtbl -> Erase(This,identifier,self) ) 

#define IMenuBar_GetHandle(This,handle)	\
    ( (This)->lpVtbl -> GetHandle(This,handle) ) 

#define IMenuBar_SetChild(This,identifier,child,self)	\
    ( (This)->lpVtbl -> SetChild(This,identifier,child,self) ) 

#define IMenuBar_SetDefault(This,identifier,self)	\
    ( (This)->lpVtbl -> SetDefault(This,identifier,self) ) 


#define IMenuBar_SetAsMenuBar(This,oldMenuBar)	\
    ( (This)->lpVtbl -> SetAsMenuBar(This,oldMenuBar) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMenuBar_INTERFACE_DEFINED__ */


#ifndef __IMenuBarConstructor_INTERFACE_DEFINED__
#define __IMenuBarConstructor_INTERFACE_DEFINED__

/* interface IMenuBarConstructor */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IMenuBarConstructor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A581CCF8-C1DB-4a04-8BDB-680AC19F0EC7")
    IMenuBarConstructor : public IDispatchEx
    {
    public:
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE Construct( 
            /* [retval][out] */ IMenuBar **instance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMenuBarConstructorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMenuBarConstructor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMenuBarConstructor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMenuBarConstructor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMenuBarConstructor * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMenuBarConstructor * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMenuBarConstructor * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMenuBarConstructor * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *GetDispID )( 
            IMenuBarConstructor * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD grfdex,
            /* [out] */ DISPID *pid);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *InvokeEx )( 
            IMenuBarConstructor * This,
            /* [in] */ DISPID id,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS *pdp,
            /* [out] */ VARIANT *pvarRes,
            /* [out] */ EXCEPINFO *pei,
            /* [unique][in] */ IServiceProvider *pspCaller);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMemberByName )( 
            IMenuBarConstructor * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ DWORD grfdex);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteMemberByDispID )( 
            IMenuBarConstructor * This,
            /* [in] */ DISPID id);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemberProperties )( 
            IMenuBarConstructor * This,
            /* [in] */ DISPID id,
            /* [in] */ DWORD grfdexFetch,
            /* [out] */ DWORD *pgrfdex);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemberName )( 
            IMenuBarConstructor * This,
            /* [in] */ DISPID id,
            /* [out] */ BSTR *pbstrName);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextDispID )( 
            IMenuBarConstructor * This,
            /* [in] */ DWORD grfdex,
            /* [in] */ DISPID id,
            /* [out] */ DISPID *pid);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameSpaceParent )( 
            IMenuBarConstructor * This,
            /* [out] */ IUnknown **ppunk);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE *Construct )( 
            IMenuBarConstructor * This,
            /* [retval][out] */ IMenuBar **instance);
        
        END_INTERFACE
    } IMenuBarConstructorVtbl;

    interface IMenuBarConstructor
    {
        CONST_VTBL struct IMenuBarConstructorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMenuBarConstructor_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IMenuBarConstructor_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IMenuBarConstructor_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IMenuBarConstructor_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IMenuBarConstructor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IMenuBarConstructor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IMenuBarConstructor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IMenuBarConstructor_GetDispID(This,bstrName,grfdex,pid)	\
    ( (This)->lpVtbl -> GetDispID(This,bstrName,grfdex,pid) ) 

#define IMenuBarConstructor_InvokeEx(This,id,lcid,wFlags,pdp,pvarRes,pei,pspCaller)	\
    ( (This)->lpVtbl -> InvokeEx(This,id,lcid,wFlags,pdp,pvarRes,pei,pspCaller) ) 

#define IMenuBarConstructor_DeleteMemberByName(This,bstrName,grfdex)	\
    ( (This)->lpVtbl -> DeleteMemberByName(This,bstrName,grfdex) ) 

#define IMenuBarConstructor_DeleteMemberByDispID(This,id)	\
    ( (This)->lpVtbl -> DeleteMemberByDispID(This,id) ) 

#define IMenuBarConstructor_GetMemberProperties(This,id,grfdexFetch,pgrfdex)	\
    ( (This)->lpVtbl -> GetMemberProperties(This,id,grfdexFetch,pgrfdex) ) 

#define IMenuBarConstructor_GetMemberName(This,id,pbstrName)	\
    ( (This)->lpVtbl -> GetMemberName(This,id,pbstrName) ) 

#define IMenuBarConstructor_GetNextDispID(This,grfdex,id,pid)	\
    ( (This)->lpVtbl -> GetNextDispID(This,grfdex,id,pid) ) 

#define IMenuBarConstructor_GetNameSpaceParent(This,ppunk)	\
    ( (This)->lpVtbl -> GetNameSpaceParent(This,ppunk) ) 


#define IMenuBarConstructor_Construct(This,instance)	\
    ( (This)->lpVtbl -> Construct(This,instance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IMenuBarConstructor_INTERFACE_DEFINED__ */


#ifndef __IServiceObjectProvider_INTERFACE_DEFINED__
#define __IServiceObjectProvider_INTERFACE_DEFINED__

/* interface IServiceObjectProvider */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IServiceObjectProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AB4CC2F7-4873-43f4-AA5E-536D8653FE2C")
    IServiceObjectProvider : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryService( 
            /* [in] */ BSTR serviceName,
            /* [retval][out] */ IDispatch **serviceObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServiceObjectProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IServiceObjectProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IServiceObjectProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IServiceObjectProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IServiceObjectProvider * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IServiceObjectProvider * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IServiceObjectProvider * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IServiceObjectProvider * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *QueryService )( 
            IServiceObjectProvider * This,
            /* [in] */ BSTR serviceName,
            /* [retval][out] */ IDispatch **serviceObject);
        
        END_INTERFACE
    } IServiceObjectProviderVtbl;

    interface IServiceObjectProvider
    {
        CONST_VTBL struct IServiceObjectProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IServiceObjectProvider_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IServiceObjectProvider_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IServiceObjectProvider_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IServiceObjectProvider_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IServiceObjectProvider_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IServiceObjectProvider_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IServiceObjectProvider_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IServiceObjectProvider_QueryService(This,serviceName,serviceObject)	\
    ( (This)->lpVtbl -> QueryService(This,serviceName,serviceObject) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IServiceObjectProvider_INTERFACE_DEFINED__ */


#ifndef __IScriptSystem_INTERFACE_DEFINED__
#define __IScriptSystem_INTERFACE_DEFINED__

/* interface IScriptSystem */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IScriptSystem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A8B776AA-560E-4262-9CFA-5C0DFA33CEF8")
    IScriptSystem : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Buffers( 
            /* [retval][out] */ IBufferList **buffers) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Windows( 
            /* [retval][out] */ IWindowList **windows) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExecuteFile( 
            /* [in] */ BSTR fileName,
            /* [retval][out] */ VARIANT **result) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetServiceProvider( 
            /* [retval][out] */ IServiceObjectProvider **serviceProvider) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadConstants( 
            /* [in] */ VARIANT *libraryNameOrObject,
            /* [in] */ VARIANT *parent) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadScript( 
            /* [in] */ BSTR fileName,
            /* [retval][out] */ VARIANT *result) = 0;
        
        virtual /* [helpstring][vararg][id] */ HRESULT STDMETHODCALLTYPE Position( 
            /* [in] */ SAFEARRAY * parameters,
            /* [retval][out] */ IPosition **newInstance) = 0;
        
        virtual /* [helpstring][vararg][id] */ HRESULT STDMETHODCALLTYPE Region( 
            /* [in] */ SAFEARRAY * parameters,
            /* [retval][out] */ IRegion **newInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptSystemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptSystem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptSystem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptSystem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IScriptSystem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IScriptSystem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IScriptSystem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IScriptSystem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Buffers )( 
            IScriptSystem * This,
            /* [retval][out] */ IBufferList **buffers);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Windows )( 
            IScriptSystem * This,
            /* [retval][out] */ IWindowList **windows);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteFile )( 
            IScriptSystem * This,
            /* [in] */ BSTR fileName,
            /* [retval][out] */ VARIANT **result);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetServiceProvider )( 
            IScriptSystem * This,
            /* [retval][out] */ IServiceObjectProvider **serviceProvider);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadConstants )( 
            IScriptSystem * This,
            /* [in] */ VARIANT *libraryNameOrObject,
            /* [in] */ VARIANT *parent);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadScript )( 
            IScriptSystem * This,
            /* [in] */ BSTR fileName,
            /* [retval][out] */ VARIANT *result);
        
        /* [helpstring][vararg][id] */ HRESULT ( STDMETHODCALLTYPE *Position )( 
            IScriptSystem * This,
            /* [in] */ SAFEARRAY * parameters,
            /* [retval][out] */ IPosition **newInstance);
        
        /* [helpstring][vararg][id] */ HRESULT ( STDMETHODCALLTYPE *Region )( 
            IScriptSystem * This,
            /* [in] */ SAFEARRAY * parameters,
            /* [retval][out] */ IRegion **newInstance);
        
        END_INTERFACE
    } IScriptSystemVtbl;

    interface IScriptSystem
    {
        CONST_VTBL struct IScriptSystemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptSystem_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IScriptSystem_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IScriptSystem_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IScriptSystem_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IScriptSystem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IScriptSystem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IScriptSystem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IScriptSystem_get_Buffers(This,buffers)	\
    ( (This)->lpVtbl -> get_Buffers(This,buffers) ) 

#define IScriptSystem_get_Windows(This,windows)	\
    ( (This)->lpVtbl -> get_Windows(This,windows) ) 

#define IScriptSystem_ExecuteFile(This,fileName,result)	\
    ( (This)->lpVtbl -> ExecuteFile(This,fileName,result) ) 

#define IScriptSystem_GetServiceProvider(This,serviceProvider)	\
    ( (This)->lpVtbl -> GetServiceProvider(This,serviceProvider) ) 

#define IScriptSystem_LoadConstants(This,libraryNameOrObject,parent)	\
    ( (This)->lpVtbl -> LoadConstants(This,libraryNameOrObject,parent) ) 

#define IScriptSystem_LoadScript(This,fileName,result)	\
    ( (This)->lpVtbl -> LoadScript(This,fileName,result) ) 

#define IScriptSystem_Position(This,parameters,newInstance)	\
    ( (This)->lpVtbl -> Position(This,parameters,newInstance) ) 

#define IScriptSystem_Region(This,parameters,newInstance)	\
    ( (This)->lpVtbl -> Region(This,parameters,newInstance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IScriptSystem_INTERFACE_DEFINED__ */


#ifndef __INamedArguments_INTERFACE_DEFINED__
#define __INamedArguments_INTERFACE_DEFINED__

/* interface INamedArguments */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_INamedArguments;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AFF456A8-8042-46aa-ADCC-E3A32D64690C")
    INamedArguments : public IDispatch
    {
    public:
        virtual /* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **enumerator) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ BSTR switchString,
            /* [retval][out] */ VARIANT **value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Exists( 
            /* [in] */ BSTR switchString,
            /* [retval][out] */ VARIANT_BOOL *exists) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INamedArgumentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INamedArguments * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INamedArguments * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INamedArguments * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            INamedArguments * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            INamedArguments * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            INamedArguments * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            INamedArguments * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [restricted][hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            INamedArguments * This,
            /* [retval][out] */ IUnknown **enumerator);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            INamedArguments * This,
            /* [in] */ BSTR switchString,
            /* [retval][out] */ VARIANT **value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            INamedArguments * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Count )( 
            INamedArguments * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Exists )( 
            INamedArguments * This,
            /* [in] */ BSTR switchString,
            /* [retval][out] */ VARIANT_BOOL *exists);
        
        END_INTERFACE
    } INamedArgumentsVtbl;

    interface INamedArguments
    {
        CONST_VTBL struct INamedArgumentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INamedArguments_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define INamedArguments_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define INamedArguments_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define INamedArguments_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define INamedArguments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define INamedArguments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define INamedArguments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define INamedArguments_get__NewEnum(This,enumerator)	\
    ( (This)->lpVtbl -> get__NewEnum(This,enumerator) ) 

#define INamedArguments_get_Item(This,switchString,value)	\
    ( (This)->lpVtbl -> get_Item(This,switchString,value) ) 

#define INamedArguments_get_length(This,count)	\
    ( (This)->lpVtbl -> get_length(This,count) ) 

#define INamedArguments_Count(This,count)	\
    ( (This)->lpVtbl -> Count(This,count) ) 

#define INamedArguments_Exists(This,switchString,exists)	\
    ( (This)->lpVtbl -> Exists(This,switchString,exists) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INamedArguments_INTERFACE_DEFINED__ */


#ifndef __IUnnamedArguments_INTERFACE_DEFINED__
#define __IUnnamedArguments_INTERFACE_DEFINED__

/* interface IUnnamedArguments */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IUnnamedArguments;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A8AEF8E8-35EF-49da-82A3-B57DCDE1A097")
    IUnnamedArguments : public IDispatch
    {
    public:
        virtual /* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **enumerator) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ VARIANT **value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ long *count) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUnnamedArgumentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUnnamedArguments * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUnnamedArguments * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUnnamedArguments * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IUnnamedArguments * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IUnnamedArguments * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IUnnamedArguments * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IUnnamedArguments * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [restricted][hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IUnnamedArguments * This,
            /* [retval][out] */ IUnknown **enumerator);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IUnnamedArguments * This,
            /* [in] */ long index,
            /* [retval][out] */ VARIANT **value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IUnnamedArguments * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Count )( 
            IUnnamedArguments * This,
            /* [retval][out] */ long *count);
        
        END_INTERFACE
    } IUnnamedArgumentsVtbl;

    interface IUnnamedArguments
    {
        CONST_VTBL struct IUnnamedArgumentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUnnamedArguments_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IUnnamedArguments_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IUnnamedArguments_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IUnnamedArguments_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IUnnamedArguments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IUnnamedArguments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IUnnamedArguments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IUnnamedArguments_get__NewEnum(This,enumerator)	\
    ( (This)->lpVtbl -> get__NewEnum(This,enumerator) ) 

#define IUnnamedArguments_get_Item(This,index,value)	\
    ( (This)->lpVtbl -> get_Item(This,index,value) ) 

#define IUnnamedArguments_get_length(This,count)	\
    ( (This)->lpVtbl -> get_length(This,count) ) 

#define IUnnamedArguments_Count(This,count)	\
    ( (This)->lpVtbl -> Count(This,count) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IUnnamedArguments_INTERFACE_DEFINED__ */


#ifndef __IArguments_INTERFACE_DEFINED__
#define __IArguments_INTERFACE_DEFINED__

/* interface IArguments */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IArguments;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A843FB1A-8E28-4d37-805F-9FCFB98A6F05")
    IArguments : public IDispatch
    {
    public:
        virtual /* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **enumerator) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ VARIANT **value) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Named( 
            /* [retval][out] */ INamedArguments **named) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Unnamed( 
            /* [retval][out] */ IUnnamedArguments **unnamed) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ long *count) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ShowUsage( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IArgumentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IArguments * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IArguments * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IArguments * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IArguments * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IArguments * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IArguments * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IArguments * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [restricted][hidden][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IArguments * This,
            /* [retval][out] */ IUnknown **enumerator);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IArguments * This,
            /* [in] */ long index,
            /* [retval][out] */ VARIANT **value);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            IArguments * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Named )( 
            IArguments * This,
            /* [retval][out] */ INamedArguments **named);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Unnamed )( 
            IArguments * This,
            /* [retval][out] */ IUnnamedArguments **unnamed);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Count )( 
            IArguments * This,
            /* [retval][out] */ long *count);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ShowUsage )( 
            IArguments * This);
        
        END_INTERFACE
    } IArgumentsVtbl;

    interface IArguments
    {
        CONST_VTBL struct IArgumentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IArguments_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IArguments_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IArguments_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IArguments_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IArguments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IArguments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IArguments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IArguments_get__NewEnum(This,enumerator)	\
    ( (This)->lpVtbl -> get__NewEnum(This,enumerator) ) 

#define IArguments_get_Item(This,index,value)	\
    ( (This)->lpVtbl -> get_Item(This,index,value) ) 

#define IArguments_get_length(This,count)	\
    ( (This)->lpVtbl -> get_length(This,count) ) 

#define IArguments_get_Named(This,named)	\
    ( (This)->lpVtbl -> get_Named(This,named) ) 

#define IArguments_get_Unnamed(This,unnamed)	\
    ( (This)->lpVtbl -> get_Unnamed(This,unnamed) ) 

#define IArguments_Count(This,count)	\
    ( (This)->lpVtbl -> Count(This,count) ) 

#define IArguments_ShowUsage(This)	\
    ( (This)->lpVtbl -> ShowUsage(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IArguments_INTERFACE_DEFINED__ */


#ifndef __IScriptHost_INTERFACE_DEFINED__
#define __IScriptHost_INTERFACE_DEFINED__

/* interface IScriptHost */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IScriptHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A34BB582-A2DA-4197-8A81-3E3FB2E3FD16")
    IScriptHost : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Application( 
            /* [retval][out] */ IDispatch **application) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Arguments( 
            /* [retval][out] */ IArguments **arguments) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BuildVersion( 
            /* [retval][out] */ int *version) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FullName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Interactive( 
            /* [retval][out] */ VARIANT_BOOL *interactive) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Interactive( 
            /* [in] */ VARIANT_BOOL interactive) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *path) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ScriptFullName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ScriptName( 
            /* [retval][out] */ BSTR *name) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_StdErr( 
            /* [retval][out] */ IDispatch **stdErr) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_StdIn( 
            /* [retval][out] */ IDispatch **stdIn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_StdOut( 
            /* [retval][out] */ IDispatch **stdOut) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Timeout( 
            /* [retval][out] */ long *timeout) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Timeout( 
            /* [in] */ long timeout) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *version) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectObject( 
            /* [in] */ IDispatch *eventSource,
            /* [in] */ BSTR prefix) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectObjectEx( 
            /* [in] */ IDispatch *eventSource,
            /* [in] */ IDispatch *eventSink) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateObject( 
            /* [in] */ BSTR progID,
            /* [defaultvalue][in] */ BSTR prefix,
            /* [retval][out] */ IDispatch **object) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisconnectObject( 
            /* [in] */ IDispatch *eventSource) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisconnectObjectEx( 
            /* [in] */ IDispatch *eventSource,
            /* [in] */ IDispatch *eventSink) = 0;
        
        virtual /* [helpstring][vararg][id] */ HRESULT STDMETHODCALLTYPE Echo( 
            /* [in] */ SAFEARRAY * arguments) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR pathName,
            /* [defaultvalue][in] */ BSTR progID,
            /* [defaultvalue][in] */ BSTR prefix,
            /* [retval][out] */ IDispatch **object) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Quit( 
            /* [in] */ int exitCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Sleep( 
            /* [in] */ long time) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptHost * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IScriptHost * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IScriptHost * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IScriptHost * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IScriptHost * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Application )( 
            IScriptHost * This,
            /* [retval][out] */ IDispatch **application);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Arguments )( 
            IScriptHost * This,
            /* [retval][out] */ IArguments **arguments);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BuildVersion )( 
            IScriptHost * This,
            /* [retval][out] */ int *version);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FullName )( 
            IScriptHost * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Interactive )( 
            IScriptHost * This,
            /* [retval][out] */ VARIANT_BOOL *interactive);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Interactive )( 
            IScriptHost * This,
            /* [in] */ VARIANT_BOOL interactive);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IScriptHost * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            IScriptHost * This,
            /* [retval][out] */ BSTR *path);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScriptFullName )( 
            IScriptHost * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScriptName )( 
            IScriptHost * This,
            /* [retval][out] */ BSTR *name);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StdErr )( 
            IScriptHost * This,
            /* [retval][out] */ IDispatch **stdErr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StdIn )( 
            IScriptHost * This,
            /* [retval][out] */ IDispatch **stdIn);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_StdOut )( 
            IScriptHost * This,
            /* [retval][out] */ IDispatch **stdOut);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Timeout )( 
            IScriptHost * This,
            /* [retval][out] */ long *timeout);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Timeout )( 
            IScriptHost * This,
            /* [in] */ long timeout);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IScriptHost * This,
            /* [retval][out] */ BSTR *version);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectObject )( 
            IScriptHost * This,
            /* [in] */ IDispatch *eventSource,
            /* [in] */ BSTR prefix);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectObjectEx )( 
            IScriptHost * This,
            /* [in] */ IDispatch *eventSource,
            /* [in] */ IDispatch *eventSink);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateObject )( 
            IScriptHost * This,
            /* [in] */ BSTR progID,
            /* [defaultvalue][in] */ BSTR prefix,
            /* [retval][out] */ IDispatch **object);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisconnectObject )( 
            IScriptHost * This,
            /* [in] */ IDispatch *eventSource);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisconnectObjectEx )( 
            IScriptHost * This,
            /* [in] */ IDispatch *eventSource,
            /* [in] */ IDispatch *eventSink);
        
        /* [helpstring][vararg][id] */ HRESULT ( STDMETHODCALLTYPE *Echo )( 
            IScriptHost * This,
            /* [in] */ SAFEARRAY * arguments);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IScriptHost * This,
            /* [in] */ BSTR pathName,
            /* [defaultvalue][in] */ BSTR progID,
            /* [defaultvalue][in] */ BSTR prefix,
            /* [retval][out] */ IDispatch **object);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Quit )( 
            IScriptHost * This,
            /* [in] */ int exitCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Sleep )( 
            IScriptHost * This,
            /* [in] */ long time);
        
        END_INTERFACE
    } IScriptHostVtbl;

    interface IScriptHost
    {
        CONST_VTBL struct IScriptHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IScriptHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IScriptHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IScriptHost_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IScriptHost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IScriptHost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IScriptHost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IScriptHost_get_Application(This,application)	\
    ( (This)->lpVtbl -> get_Application(This,application) ) 

#define IScriptHost_get_Arguments(This,arguments)	\
    ( (This)->lpVtbl -> get_Arguments(This,arguments) ) 

#define IScriptHost_get_BuildVersion(This,version)	\
    ( (This)->lpVtbl -> get_BuildVersion(This,version) ) 

#define IScriptHost_get_FullName(This,name)	\
    ( (This)->lpVtbl -> get_FullName(This,name) ) 

#define IScriptHost_get_Interactive(This,interactive)	\
    ( (This)->lpVtbl -> get_Interactive(This,interactive) ) 

#define IScriptHost_put_Interactive(This,interactive)	\
    ( (This)->lpVtbl -> put_Interactive(This,interactive) ) 

#define IScriptHost_get_Name(This,name)	\
    ( (This)->lpVtbl -> get_Name(This,name) ) 

#define IScriptHost_get_Path(This,path)	\
    ( (This)->lpVtbl -> get_Path(This,path) ) 

#define IScriptHost_get_ScriptFullName(This,name)	\
    ( (This)->lpVtbl -> get_ScriptFullName(This,name) ) 

#define IScriptHost_get_ScriptName(This,name)	\
    ( (This)->lpVtbl -> get_ScriptName(This,name) ) 

#define IScriptHost_get_StdErr(This,stdErr)	\
    ( (This)->lpVtbl -> get_StdErr(This,stdErr) ) 

#define IScriptHost_get_StdIn(This,stdIn)	\
    ( (This)->lpVtbl -> get_StdIn(This,stdIn) ) 

#define IScriptHost_get_StdOut(This,stdOut)	\
    ( (This)->lpVtbl -> get_StdOut(This,stdOut) ) 

#define IScriptHost_get_Timeout(This,timeout)	\
    ( (This)->lpVtbl -> get_Timeout(This,timeout) ) 

#define IScriptHost_put_Timeout(This,timeout)	\
    ( (This)->lpVtbl -> put_Timeout(This,timeout) ) 

#define IScriptHost_get_Version(This,version)	\
    ( (This)->lpVtbl -> get_Version(This,version) ) 

#define IScriptHost_ConnectObject(This,eventSource,prefix)	\
    ( (This)->lpVtbl -> ConnectObject(This,eventSource,prefix) ) 

#define IScriptHost_ConnectObjectEx(This,eventSource,eventSink)	\
    ( (This)->lpVtbl -> ConnectObjectEx(This,eventSource,eventSink) ) 

#define IScriptHost_CreateObject(This,progID,prefix,object)	\
    ( (This)->lpVtbl -> CreateObject(This,progID,prefix,object) ) 

#define IScriptHost_DisconnectObject(This,eventSource)	\
    ( (This)->lpVtbl -> DisconnectObject(This,eventSource) ) 

#define IScriptHost_DisconnectObjectEx(This,eventSource,eventSink)	\
    ( (This)->lpVtbl -> DisconnectObjectEx(This,eventSource,eventSink) ) 

#define IScriptHost_Echo(This,arguments)	\
    ( (This)->lpVtbl -> Echo(This,arguments) ) 

#define IScriptHost_GetObject(This,pathName,progID,prefix,object)	\
    ( (This)->lpVtbl -> GetObject(This,pathName,progID,prefix,object) ) 

#define IScriptHost_Quit(This,exitCode)	\
    ( (This)->lpVtbl -> Quit(This,exitCode) ) 

#define IScriptHost_Sleep(This,time)	\
    ( (This)->lpVtbl -> Sleep(This,time) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IScriptHost_INTERFACE_DEFINED__ */

#endif /* __ambient_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


