#ifndef LINK_CVPACK_FIXTURE_H
#define LINK_CVPACK_FIXTURE_H
#include <stdint.h>
/* Proven prepack/packed clock-field correspondence; all other bytes strict. */
static const uint32_t cvpack_clocks[][3]={
    {136,136,4}, /* PE COFF TimeDateStamp */
    {7172,7172,4}, /* Debug directory TimeDateStamp */
    {7200,7200,4}, /* Debug directory TimeDateStamp */
    {7228,7228,4}, /* Debug directory TimeDateStamp */
    {11780,11780,4}, /* Resource directory TimeDateStamp */
    {11916,11916,4}, /* Resource directory TimeDateStamp */
    {11940,11940,4}, /* Resource directory TimeDateStamp */
    {11964,11964,4}, /* Resource directory TimeDateStamp */
    {11988,11988,4}, /* Resource directory TimeDateStamp */
    {12020,12020,4}, /* Resource directory TimeDateStamp */
    {12044,12044,4}, /* Resource directory TimeDateStamp */
    {12076,12076,4}, /* Resource directory TimeDateStamp */
    {12108,12108,4}, /* Resource directory TimeDateStamp */
    {12132,12132,4}, /* Resource directory TimeDateStamp */
    {12156,12156,4}, /* Resource directory TimeDateStamp */
    {12180,12180,4}, /* Resource directory TimeDateStamp */
    {12204,12204,4}, /* Resource directory TimeDateStamp */
    {12228,12228,4}, /* Resource directory TimeDateStamp */
    {12252,12252,4}, /* Resource directory TimeDateStamp */
    {12276,12276,4}, /* Resource directory TimeDateStamp */
    {12300,12300,4}, /* Resource directory TimeDateStamp */
    {12324,12324,4}, /* Resource directory TimeDateStamp */
    {12348,12348,4}, /* Resource directory TimeDateStamp */
    {12372,12372,4}, /* Resource directory TimeDateStamp */
    {12396,12396,4}, /* Resource directory TimeDateStamp */
    {12420,12420,4}, /* Resource directory TimeDateStamp */
    {12444,12444,4}, /* Resource directory TimeDateStamp */
    {12468,12468,4}, /* Resource directory TimeDateStamp */
    {12492,12492,4}, /* Resource directory TimeDateStamp */
    {12524,12524,4}, /* Resource directory TimeDateStamp */
    {12548,12548,4}, /* Resource directory TimeDateStamp */
    {12572,12572,4}, /* Resource directory TimeDateStamp */
    {12596,12596,4}, /* Resource directory TimeDateStamp */
    {12620,12620,4}, /* Resource directory TimeDateStamp */
    {12644,12644,4}, /* Resource directory TimeDateStamp */
    {12668,12668,4}, /* Resource directory TimeDateStamp */
    {12692,12692,4}, /* Resource directory TimeDateStamp */
    {12716,12716,4}, /* Resource directory TimeDateStamp */
    {17924,17924,4}, /* Export directory TimeDateStamp */
};
#endif
