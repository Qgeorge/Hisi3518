#ifndef HKCMDPACKET_HPP__
#define HKCMDPACKET_HPP__

#include <stdlib.h>
#include "utils/HKCmdDefine.h"

#if !defined(AFX_EXT_CLASS)
#if defined(_WIN32)
#  ifdef EXPORTDECL_EXPORTS
#    define AFX_EXT_CLASS __declspec(dllexport)
#  else
#    define AFX_EXT_CLASS __declspec(dllimport)
#  endif
#else
#  define AFX_EXT_CLASS
#endif
#endif

enum { Packet_Min_BufSize=3 };

#ifdef __cplusplus
extern "C" {
#endif

    struct Pair;
    struct Dict
    {
        unsigned int length;
        struct Pair* buf;
    };
    typedef struct Dict Dict;
    typedef int IntType;
    typedef enum { bFalse=0, bTrue=1 } BoolType;

    AFX_EXT_CLASS Dict* DictCreate (const char* buf, size_t len);
    AFX_EXT_CLASS void  DictDestroy(Dict* d);

    AFX_EXT_CLASS char* DictSetStr(Dict* d, const char* key, const char* val);
    AFX_EXT_CLASS char* DictGetStr(Dict* d, const char* key);

    AFX_EXT_CLASS BoolType DictSetInt(Dict* d, const char* key, IntType val);
    AFX_EXT_CLASS IntType  DictGetInt(Dict* d, const char* key);

    //AFX_EXT_CLASS Dict* DictGetDict(Dict* d, const char* key);
    //AFX_EXT_CLASS Dict* DictSetDict(Dict* d, const char* key, Dict* sub);
    //AFX_EXT_CLASS Dict* DictNewDict(Dict* d, const char* key);

    AFX_EXT_CLASS void* DictFindKey(Dict* d, const char* key);
    AFX_EXT_CLASS void  DictDeleteKey(Dict* d, const char* key);

    AFX_EXT_CLASS int      DictEncode(char buf[], size_t len, Dict* d);
    AFX_EXT_CLASS int      DictEncodeAlloc(char** ptr, struct Dict* d);
    AFX_EXT_CLASS void     DictEncodeFree(char* ptr);
    AFX_EXT_CLASS BoolType DictIsEmpty(Dict* d);

    AFX_EXT_CLASS Dict* DictAssign(Dict* d, const char* buf, size_t len);
    AFX_EXT_CLASS Dict* DictUpdate(Dict* d, const char* buf, size_t len);

#define DictCreateEmpty() DictCreate(0,0)
#define CreateFrameB() DictCreate(0,0)
#define CreateFrameA DictCreate
#define DestroyFrame DictDestroy

#define GetFrameSize DictEncodedSize
#define GetFramePacketBuf(d, buf, size) DictEncode(buf, size, d)
#define SetParamStr DictSetStr
#define GetParamStr DictGetStr
#define SetParamUN DictSetInt
#define GetParamUN DictGetInt
#define DeleteFrameKey DictDeleteKey
#define StrDecode(in, out) untroubled(out, in, (in)+strlen(in))

    // deprecated, compact only
    typedef struct Dict HKFrameHead;
    AFX_EXT_CLASS int   DictEncodeOld(char buf[], size_t len, struct Dict* d);
    AFX_EXT_CLASS char* DictEncoded(Dict* d);
    AFX_EXT_CLASS int   DictEncodedSize(Dict* d);
    AFX_EXT_CLASS int untroubled(char buf[], const char* beg, const char* end);

#ifdef __cplusplus
}

#include <string>
#include "utils/HKChameleon.h"

class AFX_EXT_CLASS HKCmdPacket : private Dict
{
public:
    HKCmdPacket( void );
    HKCmdPacket( const std::string &strData );
    HKCmdPacket( HKCmdPacket& rCmdPacket );
    HKCmdPacket & operator = ( HKCmdPacket& rCmdPacket );
    ~HKCmdPacket( void );

    // void SetParam( const std::string& strKEY, const HKCmdPacket& pkg);

    void SetParam( const std::string& strKEY,const std::string& value );
    void SetParam( const std::string& strKEY,int value );
    void SetParam( const std::string& strKEY,unsigned int value );
    void SetParam( const std::string& strKEY,long value );
    void SetParam( const std::string& strKEY,unsigned long value );
    // void SetParam( const std::string& strKEY,unsigned long long value );

    HKChameleon GetParam( const std::string & );
    std::string PacketToString( void );
    std::string PacketToCompactString( void );

    bool Empty() const { return !!DictIsEmpty((HKCmdPacket*)this); }

private:
    void AnalysePacketData( const std::string& data );
    void PacketDataEncode( const std::string& in,std::string& out );
    void PacketDataDecode( const std::string& in,std::string& out );
};

#endif

#endif // HKCMDPACKET_HPP__

