#ifndef BROWSER_API_H
#define BROWSER_API_H
#define API_JOIN_RAW(a,b) a##b
#define API_JOIN(a,b) API_JOIN_RAW(a,b)
#define API_SCALE(value) ((value) * 3u)
#define API_BIAS(value) (API_SCALE(value) + 7u)
#define API_APPLY(callback,value) ((callback)(API_BIAS(value)))
#define API_NAME(name) API_JOIN(api_,name)
#define API_STAMP __TIMESTAMP__
#define API_TEXT_RAW(value) #value
#define API_TEXT(value) API_TEXT_RAW(value)
#define API_LABEL API_TEXT(API_NAME(checksum))
#pragma pack(2)
typedef enum ApiKind { API_FIRST = -3, API_SECOND = 9 } ApiKind;
typedef struct ApiRecord {
    unsigned short tag;
    unsigned enabled : 1;
    unsigned count : 7;
    unsigned value;
} ApiRecord;
#pragma pack()
typedef unsigned (__cdecl *ApiCallback)(unsigned);
extern const char producer_stamp[];
extern const char producer_label[];
unsigned __cdecl API_NAME(checksum)(unsigned value);
unsigned __cdecl API_NAME(record)(const ApiRecord *record, ApiCallback callback);
#endif
