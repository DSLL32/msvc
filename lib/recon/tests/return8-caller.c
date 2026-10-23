#include "return8-shape.h"
static ParserValue results[2];
static unsigned count;
static ParserValue *next_result(void){return &results[count++];}
int main(void)
{
    unsigned code=130,i;
    RECON_ASSIGN_RETURN8(*next_result(),return_value(code++));
    if(code!=131 || count!=1 || sizeof(ParserValue)!=8)return 125;
    printf("size=%u calls=%u code=%u low=%08x high=%08x bytes=",(unsigned)sizeof(ParserValue),count,code,results[0].words.low,results[0].words.high);
    for(i=0;i<8;++i)printf("%02x",results[0].bytes[i]);
    printf("\n");return 0;
}
