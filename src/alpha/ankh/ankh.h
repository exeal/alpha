

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0361 */
/* at Sat Aug 04 18:57:22 2007
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


#ifndef __ankh_h__
#define __ankh_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

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


#ifndef __IScriptSystem_FWD_DEFINED__
#define __IScriptSystem_FWD_DEFINED__
typedef interface IScriptSystem IScriptSystem;
#endif 	/* __IScriptSystem_FWD_DEFINED__ */


#ifndef __ObjectModel_FWD_DEFINED__
#define __ObjectModel_FWD_DEFINED__

#ifdef __cplusplus
typedef class ObjectModel ObjectModel;
#else
typedef struct ObjectModel ObjectModel;
#endif /* __cplusplus */

#endif 	/* __ObjectModel_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dispex.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __Ankh_LIBRARY_DEFINED__
#define __Ankh_LIBRARY_DEFINED__

/* library Ankh */
/* [helpstring][version][lcid][uuid] */ 


EXTERN_C const IID LIBID_Ankh;

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
            /* [iid_is][out] */ void **ppvObject);
        
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
            /* [in] */ UINT cNames,
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
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INamedArguments_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INamedArguments_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INamedArguments_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INamedArguments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INamedArguments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INamedArguments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define INamedArguments_get__NewEnum(This,enumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,enumerator)

#define INamedArguments_get_Item(This,switchString,value)	\
    (This)->lpVtbl -> get_Item(This,switchString,value)

#define INamedArguments_get_length(This,count)	\
    (This)->lpVtbl -> get_length(This,count)

#define INamedArguments_Count(This,count)	\
    (This)->lpVtbl -> Count(This,count)

#define INamedArguments_Exists(This,switchString,exists)	\
    (This)->lpVtbl -> Exists(This,switchString,exists)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE INamedArguments_get__NewEnum_Proxy( 
    INamedArguments * This,
    /* [retval][out] */ IUnknown **enumerator);


void __RPC_STUB INamedArguments_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE INamedArguments_get_Item_Proxy( 
    INamedArguments * This,
    /* [in] */ BSTR switchString,
    /* [retval][out] */ VARIANT **value);


void __RPC_STUB INamedArguments_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE INamedArguments_get_length_Proxy( 
    INamedArguments * This,
    /* [retval][out] */ long *count);


void __RPC_STUB INamedArguments_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE INamedArguments_Count_Proxy( 
    INamedArguments * This,
    /* [retval][out] */ long *count);


void __RPC_STUB INamedArguments_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE INamedArguments_Exists_Proxy( 
    INamedArguments * This,
    /* [in] */ BSTR switchString,
    /* [retval][out] */ VARIANT_BOOL *exists);


void __RPC_STUB INamedArguments_Exists_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



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
            /* [iid_is][out] */ void **ppvObject);
        
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
            /* [in] */ UINT cNames,
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
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUnnamedArguments_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUnnamedArguments_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUnnamedArguments_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IUnnamedArguments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IUnnamedArguments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IUnnamedArguments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IUnnamedArguments_get__NewEnum(This,enumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,enumerator)

#define IUnnamedArguments_get_Item(This,index,value)	\
    (This)->lpVtbl -> get_Item(This,index,value)

#define IUnnamedArguments_get_length(This,count)	\
    (This)->lpVtbl -> get_length(This,count)

#define IUnnamedArguments_Count(This,count)	\
    (This)->lpVtbl -> Count(This,count)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE IUnnamedArguments_get__NewEnum_Proxy( 
    IUnnamedArguments * This,
    /* [retval][out] */ IUnknown **enumerator);


void __RPC_STUB IUnnamedArguments_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IUnnamedArguments_get_Item_Proxy( 
    IUnnamedArguments * This,
    /* [in] */ long index,
    /* [retval][out] */ VARIANT **value);


void __RPC_STUB IUnnamedArguments_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IUnnamedArguments_get_length_Proxy( 
    IUnnamedArguments * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IUnnamedArguments_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IUnnamedArguments_Count_Proxy( 
    IUnnamedArguments * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IUnnamedArguments_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



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
            /* [iid_is][out] */ void **ppvObject);
        
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
            /* [in] */ UINT cNames,
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
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IArguments_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IArguments_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IArguments_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IArguments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IArguments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IArguments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IArguments_get__NewEnum(This,enumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,enumerator)

#define IArguments_get_Item(This,index,value)	\
    (This)->lpVtbl -> get_Item(This,index,value)

#define IArguments_get_length(This,count)	\
    (This)->lpVtbl -> get_length(This,count)

#define IArguments_get_Named(This,named)	\
    (This)->lpVtbl -> get_Named(This,named)

#define IArguments_get_Unnamed(This,unnamed)	\
    (This)->lpVtbl -> get_Unnamed(This,unnamed)

#define IArguments_Count(This,count)	\
    (This)->lpVtbl -> Count(This,count)

#define IArguments_ShowUsage(This)	\
    (This)->lpVtbl -> ShowUsage(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [restricted][hidden][propget][id] */ HRESULT STDMETHODCALLTYPE IArguments_get__NewEnum_Proxy( 
    IArguments * This,
    /* [retval][out] */ IUnknown **enumerator);


void __RPC_STUB IArguments_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IArguments_get_Item_Proxy( 
    IArguments * This,
    /* [in] */ long index,
    /* [retval][out] */ VARIANT **value);


void __RPC_STUB IArguments_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IArguments_get_length_Proxy( 
    IArguments * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IArguments_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IArguments_get_Named_Proxy( 
    IArguments * This,
    /* [retval][out] */ INamedArguments **named);


void __RPC_STUB IArguments_get_Named_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IArguments_get_Unnamed_Proxy( 
    IArguments * This,
    /* [retval][out] */ IUnnamedArguments **unnamed);


void __RPC_STUB IArguments_get_Unnamed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IArguments_Count_Proxy( 
    IArguments * This,
    /* [retval][out] */ long *count);


void __RPC_STUB IArguments_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IArguments_ShowUsage_Proxy( 
    IArguments * This);


void __RPC_STUB IArguments_ShowUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



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
            /* [iid_is][out] */ void **ppvObject);
        
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
            /* [in] */ UINT cNames,
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
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptHost_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IScriptHost_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IScriptHost_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IScriptHost_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IScriptHost_get_Application(This,application)	\
    (This)->lpVtbl -> get_Application(This,application)

#define IScriptHost_get_Arguments(This,arguments)	\
    (This)->lpVtbl -> get_Arguments(This,arguments)

#define IScriptHost_get_BuildVersion(This,version)	\
    (This)->lpVtbl -> get_BuildVersion(This,version)

#define IScriptHost_get_FullName(This,name)	\
    (This)->lpVtbl -> get_FullName(This,name)

#define IScriptHost_get_Interactive(This,interactive)	\
    (This)->lpVtbl -> get_Interactive(This,interactive)

#define IScriptHost_put_Interactive(This,interactive)	\
    (This)->lpVtbl -> put_Interactive(This,interactive)

#define IScriptHost_get_Name(This,name)	\
    (This)->lpVtbl -> get_Name(This,name)

#define IScriptHost_get_Path(This,path)	\
    (This)->lpVtbl -> get_Path(This,path)

#define IScriptHost_get_ScriptFullName(This,name)	\
    (This)->lpVtbl -> get_ScriptFullName(This,name)

#define IScriptHost_get_ScriptName(This,name)	\
    (This)->lpVtbl -> get_ScriptName(This,name)

#define IScriptHost_get_StdErr(This,stdErr)	\
    (This)->lpVtbl -> get_StdErr(This,stdErr)

#define IScriptHost_get_StdIn(This,stdIn)	\
    (This)->lpVtbl -> get_StdIn(This,stdIn)

#define IScriptHost_get_StdOut(This,stdOut)	\
    (This)->lpVtbl -> get_StdOut(This,stdOut)

#define IScriptHost_get_Timeout(This,timeout)	\
    (This)->lpVtbl -> get_Timeout(This,timeout)

#define IScriptHost_put_Timeout(This,timeout)	\
    (This)->lpVtbl -> put_Timeout(This,timeout)

#define IScriptHost_get_Version(This,version)	\
    (This)->lpVtbl -> get_Version(This,version)

#define IScriptHost_ConnectObject(This,eventSource,prefix)	\
    (This)->lpVtbl -> ConnectObject(This,eventSource,prefix)

#define IScriptHost_ConnectObjectEx(This,eventSource,eventSink)	\
    (This)->lpVtbl -> ConnectObjectEx(This,eventSource,eventSink)

#define IScriptHost_CreateObject(This,progID,prefix,object)	\
    (This)->lpVtbl -> CreateObject(This,progID,prefix,object)

#define IScriptHost_DisconnectObject(This,eventSource)	\
    (This)->lpVtbl -> DisconnectObject(This,eventSource)

#define IScriptHost_DisconnectObjectEx(This,eventSource,eventSink)	\
    (This)->lpVtbl -> DisconnectObjectEx(This,eventSource,eventSink)

#define IScriptHost_Echo(This,arguments)	\
    (This)->lpVtbl -> Echo(This,arguments)

#define IScriptHost_GetObject(This,pathName,progID,prefix,object)	\
    (This)->lpVtbl -> GetObject(This,pathName,progID,prefix,object)

#define IScriptHost_Quit(This,exitCode)	\
    (This)->lpVtbl -> Quit(This,exitCode)

#define IScriptHost_Sleep(This,time)	\
    (This)->lpVtbl -> Sleep(This,time)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Application_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ IDispatch **application);


void __RPC_STUB IScriptHost_get_Application_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Arguments_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ IArguments **arguments);


void __RPC_STUB IScriptHost_get_Arguments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_BuildVersion_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ int *version);


void __RPC_STUB IScriptHost_get_BuildVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_FullName_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IScriptHost_get_FullName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Interactive_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ VARIANT_BOOL *interactive);


void __RPC_STUB IScriptHost_get_Interactive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_put_Interactive_Proxy( 
    IScriptHost * This,
    /* [in] */ VARIANT_BOOL interactive);


void __RPC_STUB IScriptHost_put_Interactive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Name_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IScriptHost_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Path_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ BSTR *path);


void __RPC_STUB IScriptHost_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_ScriptFullName_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IScriptHost_get_ScriptFullName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_ScriptName_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ BSTR *name);


void __RPC_STUB IScriptHost_get_ScriptName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_StdErr_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ IDispatch **stdErr);


void __RPC_STUB IScriptHost_get_StdErr_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_StdIn_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ IDispatch **stdIn);


void __RPC_STUB IScriptHost_get_StdIn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_StdOut_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ IDispatch **stdOut);


void __RPC_STUB IScriptHost_get_StdOut_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Timeout_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ long *timeout);


void __RPC_STUB IScriptHost_get_Timeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_put_Timeout_Proxy( 
    IScriptHost * This,
    /* [in] */ long timeout);


void __RPC_STUB IScriptHost_put_Timeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_get_Version_Proxy( 
    IScriptHost * This,
    /* [retval][out] */ BSTR *version);


void __RPC_STUB IScriptHost_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_ConnectObject_Proxy( 
    IScriptHost * This,
    /* [in] */ IDispatch *eventSource,
    /* [in] */ BSTR prefix);


void __RPC_STUB IScriptHost_ConnectObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_ConnectObjectEx_Proxy( 
    IScriptHost * This,
    /* [in] */ IDispatch *eventSource,
    /* [in] */ IDispatch *eventSink);


void __RPC_STUB IScriptHost_ConnectObjectEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_CreateObject_Proxy( 
    IScriptHost * This,
    /* [in] */ BSTR progID,
    /* [defaultvalue][in] */ BSTR prefix,
    /* [retval][out] */ IDispatch **object);


void __RPC_STUB IScriptHost_CreateObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_DisconnectObject_Proxy( 
    IScriptHost * This,
    /* [in] */ IDispatch *eventSource);


void __RPC_STUB IScriptHost_DisconnectObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_DisconnectObjectEx_Proxy( 
    IScriptHost * This,
    /* [in] */ IDispatch *eventSource,
    /* [in] */ IDispatch *eventSink);


void __RPC_STUB IScriptHost_DisconnectObjectEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][vararg][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_Echo_Proxy( 
    IScriptHost * This,
    /* [in] */ SAFEARRAY * arguments);


void __RPC_STUB IScriptHost_Echo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_GetObject_Proxy( 
    IScriptHost * This,
    /* [in] */ BSTR pathName,
    /* [defaultvalue][in] */ BSTR progID,
    /* [defaultvalue][in] */ BSTR prefix,
    /* [retval][out] */ IDispatch **object);


void __RPC_STUB IScriptHost_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_Quit_Proxy( 
    IScriptHost * This,
    /* [in] */ int exitCode);


void __RPC_STUB IScriptHost_Quit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptHost_Sleep_Proxy( 
    IScriptHost * This,
    /* [in] */ long time);


void __RPC_STUB IScriptHost_Sleep_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptHost_INTERFACE_DEFINED__ */


#ifndef __IScriptSystem_INTERFACE_DEFINED__
#define __IScriptSystem_INTERFACE_DEFINED__

/* interface IScriptSystem */
/* [helpstring][unique][dual][object][uuid] */ 


EXTERN_C const IID IID_IScriptSystem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AD54E19E-C3D5-4220-B954-06187D80C964")
    IScriptSystem : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SecurityLevel( 
            /* [retval][out] */ short *level) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_SecurityLevel( 
            /* [in] */ short level) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ExecuteScript( 
            /* [in] */ BSTR fileName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsScriptFileLoaded( 
            /* [in] */ BSTR fileName,
            /* [retval][out] */ VARIANT_BOOL *loaded) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadConstants( 
            /* [in] */ VARIANT *libraryNameOrObject,
            /* [defaultvalue][in] */ BSTR itemName = L"") = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadScript( 
            /* [in] */ BSTR fileName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptSystemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptSystem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
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
            /* [in] */ UINT cNames,
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
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SecurityLevel )( 
            IScriptSystem * This,
            /* [retval][out] */ short *level);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SecurityLevel )( 
            IScriptSystem * This,
            /* [in] */ short level);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ExecuteScript )( 
            IScriptSystem * This,
            /* [in] */ BSTR fileName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *IsScriptFileLoaded )( 
            IScriptSystem * This,
            /* [in] */ BSTR fileName,
            /* [retval][out] */ VARIANT_BOOL *loaded);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadConstants )( 
            IScriptSystem * This,
            /* [in] */ VARIANT *libraryNameOrObject,
            /* [defaultvalue][in] */ BSTR itemName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadScript )( 
            IScriptSystem * This,
            /* [in] */ BSTR fileName);
        
        END_INTERFACE
    } IScriptSystemVtbl;

    interface IScriptSystem
    {
        CONST_VTBL struct IScriptSystemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptSystem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptSystem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptSystem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptSystem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IScriptSystem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IScriptSystem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IScriptSystem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IScriptSystem_get_SecurityLevel(This,level)	\
    (This)->lpVtbl -> get_SecurityLevel(This,level)

#define IScriptSystem_put_SecurityLevel(This,level)	\
    (This)->lpVtbl -> put_SecurityLevel(This,level)

#define IScriptSystem_ExecuteScript(This,fileName)	\
    (This)->lpVtbl -> ExecuteScript(This,fileName)

#define IScriptSystem_IsScriptFileLoaded(This,fileName,loaded)	\
    (This)->lpVtbl -> IsScriptFileLoaded(This,fileName,loaded)

#define IScriptSystem_LoadConstants(This,libraryNameOrObject,itemName)	\
    (This)->lpVtbl -> LoadConstants(This,libraryNameOrObject,itemName)

#define IScriptSystem_LoadScript(This,fileName)	\
    (This)->lpVtbl -> LoadScript(This,fileName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE IScriptSystem_get_SecurityLevel_Proxy( 
    IScriptSystem * This,
    /* [retval][out] */ short *level);


void __RPC_STUB IScriptSystem_get_SecurityLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IScriptSystem_put_SecurityLevel_Proxy( 
    IScriptSystem * This,
    /* [in] */ short level);


void __RPC_STUB IScriptSystem_put_SecurityLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptSystem_ExecuteScript_Proxy( 
    IScriptSystem * This,
    /* [in] */ BSTR fileName);


void __RPC_STUB IScriptSystem_ExecuteScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptSystem_IsScriptFileLoaded_Proxy( 
    IScriptSystem * This,
    /* [in] */ BSTR fileName,
    /* [retval][out] */ VARIANT_BOOL *loaded);


void __RPC_STUB IScriptSystem_IsScriptFileLoaded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptSystem_LoadConstants_Proxy( 
    IScriptSystem * This,
    /* [in] */ VARIANT *libraryNameOrObject,
    /* [defaultvalue][in] */ BSTR itemName);


void __RPC_STUB IScriptSystem_LoadConstants_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IScriptSystem_LoadScript_Proxy( 
    IScriptSystem * This,
    /* [in] */ BSTR fileName);


void __RPC_STUB IScriptSystem_LoadScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptSystem_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ObjectModel;

#ifdef __cplusplus

class DECLSPEC_UUID("A0D98D3C-9CA7-4675-9C25-DFA2BD0369AB")
ObjectModel;
#endif
#endif /* __Ankh_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


