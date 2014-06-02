#ifndef _TESTLIB_SKEL_H
#define _TESTLIB_SKEL_H
#include "testlib.h"
#include "remote.h"
#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdlib.h>
#include "AEEStdDef.h"

typedef struct _heap _heap;
struct _heap {
   _heap* pPrev;
   const char* loc;
   uint64 buf;
};

typedef struct allocator {
   _heap* pheap;
   byte* stack;
   byte* stackEnd;
   int nSize;
} allocator;

static __inline int _heap_alloc(_heap** ppa, const char* loc, int size, void** ppbuf) {
   _heap* pn = 0;
   pn = malloc(size + sizeof(_heap) - sizeof(uint64));
   if(pn != 0) {
      pn->pPrev = *ppa;
      pn->loc = loc;
      *ppa = pn;
      *ppbuf = (void*)&(pn->buf);
      return 0;
   } else {
      return -1;
   }
}
#define _ALIGN_SIZE(x, y) (((x) + (y-1)) & ~(y-1))


static __inline int allocator_alloc(allocator* me,
                                    const char* loc,
                                    int size, 
                                    unsigned int al,
                                    void** ppbuf) {
   if(size < 0) {                   
      return -1;
   } else if (size == 0) {
      *ppbuf = 0;
      return 0;
   }  
   if((_ALIGN_SIZE((unsigned int)me->stackEnd, al) + size) < (unsigned int)me->stack + me->nSize) {
      *ppbuf = (byte*)_ALIGN_SIZE((unsigned int)me->stackEnd, al);
      me->stackEnd = (byte*)_ALIGN_SIZE((unsigned int)me->stackEnd, al) + size;
      return 0;
   } else {
      return _heap_alloc(&me->pheap, loc, size, ppbuf);
   }
}


static __inline void allocator_deinit(allocator* me) {
   _heap* pa = me->pheap;
   while(pa != 0) {
      _heap* pn = pa;
      const char* loc = pn->loc;
      (void)loc;
      pa = pn->pPrev;
      free(pn); 
   }
}

static __inline void allocator_init(allocator* me, byte* stack, int stackSize) {
   me->stack =  stack;
   me->stackEnd =  stack + stackSize;
   me->nSize = stackSize;
   me->pheap = 0;
}


#endif // ALLOCATOR_H

#ifndef SLIM_H
#define SLIM_H

#include "AEEStdDef.h"

//a C data structure for the idl types that can be used to implement
//static and dynamic language bindings fairly efficiently.
//
//the goal is to have a minimal ROM and RAM footprint and without
//doing too many allocations.  A good way to package these things seemed
//like the module boundary, so all the idls within  one module can share
//all the type references.


#define PARAMETER_IN       0x0
#define PARAMETER_OUT      0x1
#define PARAMETER_INOUT    0x2
#define PARAMETER_ROUT     0x3
#define PARAMETER_INROUT   0x4

//the types that we get from idl
#define TYPE_OBJECT             0x0
#define TYPE_INTERFACE          0x1
#define TYPE_PRIMITIVE          0x2
#define TYPE_ENUM               0x3
#define TYPE_STRING             0x4
#define TYPE_WSTRING            0x5
#define TYPE_STRUCTURE          0x6
#define TYPE_UNION              0x7
#define TYPE_ARRAY              0x8
#define TYPE_SEQUENCE           0x9

//these require the pack/unpack to recurse
//so it's a hint to those languages that can optimize in cases where
//recursion isn't necessary. 
#define TYPE_COMPLEX_STRUCTURE  (0x10 | TYPE_STRUCTURE)
#define TYPE_COMPLEX_UNION      (0x10 | TYPE_UNION)
#define TYPE_COMPLEX_ARRAY      (0x10 | TYPE_ARRAY)
#define TYPE_COMPLEX_SEQUENCE   (0x10 | TYPE_SEQUENCE)


typedef struct Type Type;

#define INHERIT_TYPE\
   int32 nativeSize;                /*in the simple case its the same as wire size and alignment*/\
   union {\
      struct {\
         const uint32      p1;\
         const uint32      p2;\
      } _cast;\
      struct {\
         AEEIID  iid;\
         uint32  bNotNil;\
      } object;\
      struct {\
         const Type  *arrayType;\
         int32       nItems;\
      } array;\
      struct {\
         const Type *seqType;\
         int32       nMaxLen;\
      } seqSimple; \
      struct {\
         uint32 bFloating;\
         uint32 bSigned;\
      } prim; \
      const SequenceType* seqComplex;\
      const UnionType  *unionType;\
      const StructType *structType;\
      int32          stringMaxLen;\
      boolean        bInterfaceNotNil;\
   } param;\
   uint8    type;\
   uint8    nativeAlignment\

typedef struct UnionType UnionType;
typedef struct StructType StructType;
typedef struct SequenceType SequenceType;
struct Type {
   INHERIT_TYPE;
};

struct SequenceType {
   const Type *         seqType;
   uint32               nMaxLen;
   uint32               inSize;
   uint32               routSizePrimIn;
   uint32               routSizePrimROut;
};

//byte offset from the start of the case values for 
//this unions case value array.  it MUST be aligned
//at the alignment requrements for the descriptor
//
//if negative it means that the unions cases are
//simple enumerators, so the value read from the descriptor
//can be used directly to find the correct case
typedef union CaseValuePtr CaseValuePtr;
union CaseValuePtr {
   const uint8*   value8s;
   const uint16*  value16s;
   const uint32*  value32s;
   const uint64*  value64s;
};

//these are only used in complex cases
//so I pulled them out of the type definition as references to make
//the type smaller
struct UnionType {
   const Type           *descriptor; 
   uint32               nCases;
   const CaseValuePtr   caseValues;
   const Type * const   *cases;
   int32                inSize;
   int32                routSizePrimIn;
   int32                routSizePrimROut;
   uint8                inAlignment;
   uint8                routAlignmentPrimIn;
   uint8                routAlignmentPrimROut;
   uint8                inCaseAlignment;
   uint8                routCaseAlignmentPrimIn;
   uint8                routCaseAlignmentPrimROut;
   uint8                nativeCaseAlignment;
   boolean              bDefaultCase;
};

struct StructType {
   uint32               nMembers;
   const Type * const   *members;
   int32                inSize;
   int32                routSizePrimIn;
   int32                routSizePrimROut;
   uint8                inAlignment;
   uint8                routAlignmentPrimIn;
   uint8                routAlignmentPrimROut;
};

typedef struct Parameter Parameter;
struct Parameter {
   INHERIT_TYPE;
   uint8    mode;
   boolean  bNotNil;
};

#define SLIM_SCALARS_IS_DYNAMIC(u) (((u) & 0x00ffffff) == 0x00ffffff)

typedef struct Method Method;
struct Method {
   uint32                      uScalars;            //no method index
   int32                       primInSize;
   int32                       primROutSize;
   int                         maxArgs;
   int                         numParams;
   const Parameter * const     *params;
   uint8                       primInAlignment;
   uint8                       primROutAlignment;
};

typedef struct Interface Interface;

struct Interface {
   int                            nMethods;
   const Method  * const          *methodArray;
   int                            nIIds;
   const AEEIID                   *iids;
   const uint16*                  methodStringArray;
   const uint16*                  methodStrings;
   const char*                    strings;
};


#endif //SLIM_H


#ifndef _TESTLIB_SLIM_H
#define _TESTLIB_SLIM_H
#include "remote.h"

#ifndef __QAIC_SLIM
#define __QAIC_SLIM(ff) ff
#endif
#ifndef __QAIC_SLIM_EXPORT
#define __QAIC_SLIM_EXPORT
#endif

static const Type types[1];
static const Type types[1] = {{0x4,{{0,1}}, 2,0x4}};
static const Parameter parameters[2] = {{0x8,{{(const uint32)&(types[0]),(const uint32)0x0}}, 9,0x4,0,0},{0x8,{{0,1}}, 2,0x8,3,0}};
static const Parameter* const parameterArrays[2] = {(&(parameters[0])),(&(parameters[1]))};
static const Method methods[1] = {{REMOTE_SCALARS_MAKEX(0,0,0x2,0x1,0x0,0x0),0x4,0x8,3,2,(&(parameterArrays[0])),0x4,0x8}};
static const Method* const methodArrays[1] = {&(methods[0])};
static const char strings[22] = "function\0result\0array\0";
static const uint16 methodStrings[3] = {0,16,9};
static const uint16 methodStringsArrays[1] = {0};
__QAIC_SLIM_EXPORT const Interface __QAIC_SLIM(testlib_slim) = {1,&(methodArrays[0]),0,0,&(methodStringsArrays [0]),methodStrings,strings};
#endif //_TESTLIB_SLIM_H
#ifdef __qdsp6__
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
#endif

#ifndef __QAIC_REMOTE
#define __QAIC_REMOTE(ff) ff 
#endif //__QAIC_REMOTE

#ifndef __QAIC_HEADER
#define __QAIC_HEADER(ff) ff 
#endif //__QAIC_HEADER

#ifndef __QAIC_HEADER_EXPORT
#define __QAIC_HEADER_EXPORT
#endif // __QAIC_HEADER_EXPORT

#ifndef __QAIC_HEADER_ATTRIBUTE
#define __QAIC_HEADER_ATTRIBUTE
#endif // __QAIC_HEADER_ATTRIBUTE

#ifndef __QAIC_IMPL
#define __QAIC_IMPL(ff) ff 
#endif //__QAIC_IMPL

#ifndef __QAIC_IMPL_EXPORT
#define __QAIC_IMPL_EXPORT
#endif // __QAIC_IMPL_EXPORT

#ifndef __QAIC_IMPL_ATTRIBUTE
#define __QAIC_IMPL_ATTRIBUTE
#endif // __QAIC_IMPL_ATTRIBUTE

#ifndef __QAIC_STUB
#define __QAIC_STUB(ff) ff 
#endif //__QAIC_STUB

#ifndef __QAIC_STUB_EXPORT
#define __QAIC_STUB_EXPORT
#endif // __QAIC_STUB_EXPORT

#ifndef __QAIC_STUB_ATTRIBUTE
#define __QAIC_STUB_ATTRIBUTE
#endif // __QAIC_STUB_ATTRIBUTE

#ifndef __QAIC_SKEL
#define __QAIC_SKEL(ff) ff 
#endif //__QAIC_SKEL__

#ifndef __QAIC_SKEL_EXPORT
#define __QAIC_SKEL_EXPORT
#endif // __QAIC_SKEL_EXPORT

#ifndef __QAIC_SKEL_ATTRIBUTE
#define __QAIC_SKEL_ATTRIBUTE
#endif // __QAIC_SKEL_ATTRIBUTE

#ifdef __QAIC_DEBUG__
   #ifndef __QAIC_DBG_PRINTF__
   #define __QAIC_DBG_PRINTF__( ee ) do { printf ee ; } while(0)
   #endif
#else
   #define __QAIC_DBG_PRINTF__( ee ) (void)0
#endif


#define _OFFSET(src, sof)  ((void*)(((char*)(src)) + (sof)))

#define _COPY(dst, dof, src, sof, sz)  \
   do {\
         struct __copy { \
            char ar[sz]; \
         };\
         *(struct __copy*)_OFFSET(dst, dof) = *(struct __copy*)_OFFSET(src, sof);\
   } while (0)

#define _ASSIGN(dst, src, sof)  \
   do {\
      dst = OFFSET(src, sof); \
   } while (0)

#define _STD_STRLEN_IF(str) (str == 0 ? 0 : strlen(str))

#include "AEEStdErr.h"

#define _TRY(ee, func) \
   do { \
      if (AEE_SUCCESS != ((ee) = func)) {\
         __QAIC_DBG_PRINTF__((__FILE_LINE__  ": error: %d\n", (int)(ee)));\
         goto ee##bail;\
      } \
   } while (0)

#define _CATCH(exception) exception##bail: if (exception != AEE_SUCCESS)

#define _ASSERT(nErr, ff) _TRY(nErr, 0 == (ff) ? AEE_EBADPARM : AEE_SUCCESS)

#ifdef __QAIC_DEBUG__
#define _ALLOCATE(nErr, pal, size, alignment, pv) _TRY(nErr, allocator_alloc(pal, __FILE_LINE__, size, alignment, (void**)&pv))
#else
#define _ALLOCATE(nErr, pal, size, alignment, pv) _TRY(nErr, allocator_alloc(pal, 0, size, alignment, (void**)&pv))
#endif


#ifdef __cplusplus
extern "C" {
#endif
static __inline int _skel_method(int (*_pfn)(char*, uint32, uint64*), uint32 _sc, remote_arg* _pra) {
   remote_arg* _praEnd;
   char* _in0[1];
   uint32 _in0Len[1];
   uint64 _rout1[1];
   uint32* _primIn;
   int _numIn[1];
   uint64* _primROut;
   remote_arg* _praIn;
   int _nErr = 0;
   _praEnd = ((_pra + REMOTE_SCALARS_INBUFS(_sc)) + REMOTE_SCALARS_OUTBUFS(_sc));
   _ASSERT(_nErr, (_pra + 3) <= _praEnd);
   _numIn[0] = (REMOTE_SCALARS_INBUFS(_sc) - 1);
   _ASSERT(_nErr, _pra[0].buf.nLen >= 4);
   _primIn = _pra[0].buf.pv;
   _ASSERT(_nErr, _pra[(_numIn[0] + 1)].buf.nLen >= 8);
   _primROut = _pra[(_numIn[0] + 1)].buf.pv;
   _COPY(_in0Len, 0, _primIn, 0, 4);
   _praIn = (_pra + 1);
   _ASSERT(_nErr, (_praIn[0].buf.nLen / 4) >= (int)_in0Len[0]);
   _in0[0] = _praIn[0].buf.pv;
   _TRY(_nErr, _pfn(*_in0, *_in0Len, _rout1));
   _COPY(_primROut, 0, _rout1, 0, 8);
   _CATCH(_nErr) {}
   return _nErr;
}
__QAIC_SKEL_EXPORT int __QAIC_SKEL(testlib_skel_invoke)(uint32 _sc, remote_arg* _pra) __QAIC_SKEL_ATTRIBUTE {
   switch(REMOTE_SCALARS_METHOD(_sc))
   {
      case 0:
      return _skel_method((void*)__QAIC_IMPL(testlib_function), _sc, _pra);
   }
   return AEE_EUNSUPPORTED;
}
#ifdef __cplusplus
}
#endif
#endif //_TESTLIB_SKEL_H
