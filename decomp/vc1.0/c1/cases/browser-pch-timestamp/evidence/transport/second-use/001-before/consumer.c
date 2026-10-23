#include "api.h"
static const char consumer_stamp[] = API_STAMP;
static const char consumer_label[] = API_LABEL;
static int equal_text(const char *left, const char *right)
{
    while (*left && *left == *right) { ++left; ++right; }
    return *left == *right;
}
int main(void)
{
    ApiRecord record = { 11, 1, 5, 4 };
    ApiCallback callback = API_NAME(checksum);
    unsigned expected = (((4u * 3u + 7u) ^ 0x35u) + 13u) + 11u + 5u + 1u;
    if (API_NAME(record)(&record,callback) != expected) return 1;
    if (API_APPLY(callback,2u) != ((13u ^ 0x35u) + 13u)) return 2;
    if (!equal_text(producer_stamp,"Thu Jan 15 04:34:56 2026")) return 3;
    if (!equal_text(consumer_stamp,producer_stamp)) return 4;
    if (!equal_text(producer_label,"api_checksum")) return 5;
    if (!equal_text(consumer_label,producer_label)) return 6;
    return 0;
}
