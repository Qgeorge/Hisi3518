#ifndef HK_PARAM_H__
#define HK_PARAM_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(AFX_EXT_CLASS)
#if defined(_WIN32)
#define snprintf sprintf_s
#  ifdef EXPORTDECL_EXPORTS
#    define AFX_EXT_CLASS __declspec(dllexport)
#  else
#    define AFX_EXT_CLASS __declspec(dllimport)
#  endif
#else
#  define AFX_EXT_CLASS
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

	AFX_EXT_CLASS int GB2312_2_UTF8( char * buf, int buf_len, const char * src, int src_len );	//×Ö·û×ª»»..

	//---------------------md5------------------------------------------------------------
#ifndef PROTOTYPES
#define PROTOTYPES 1
#endif

	/* POINTER defines a generic pointer type */
	typedef unsigned char *POINTER;

	/* UINT2 defines a two byte word */
	typedef unsigned short int UINT2;

	/* UINT4 defines a four byte word */
	typedef unsigned long int UINT4;

	/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
	If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
	returns an empty list.
	*/
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) 
#endif

	/* MD5 context. */
	typedef struct {
		UINT4 state[4];                                   /* state (ABCD) */
		UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
		unsigned char buffer[64];                         /* input buffer */
	} MD5_CTX;


	AFX_EXT_CLASS void MD5Init PROTO_LIST ((MD5_CTX *));
	AFX_EXT_CLASS void MD5Update PROTO_LIST ((MD5_CTX *, unsigned char *, unsigned int));
	AFX_EXT_CLASS void MD5Final PROTO_LIST ((unsigned char [16], MD5_CTX *));
	AFX_EXT_CLASS void MakeMD5(unsigned char *input, int inputLen,char responseHex[32]);
	AFX_EXT_CLASS void MakeMD5Int64( const char *input, size_t inputLen, char outBuf[] );

	//-----------------------------d3des---------------------------------------------
	AFX_EXT_CLASS int dec_convert( char *key, const char* in, unsigned int* ilen, char* out, unsigned int* olen );		//½âÃÜ
	
	AFX_EXT_CLASS int enc_convert( char *key, const char* in, unsigned int* ilen, char* endate, unsigned int* olen );	 //¼ÓÃÜ

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
