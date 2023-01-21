#include <string.h>
#include "convert.h"

IEC60870_5_TypeID getTypeIdFromString( const char * str)
{
    if(strcmp(str, "M_SP_TB_1") == 0) {
        return M_SP_TB_1;
    }

    if(strcmp(str, "M_DP_TB_1") == 0) {
        return M_DP_TB_1;
    }

    if(strcmp(str, "M_ME_TD_1") == 0) {
        return M_ME_TD_1;
    }

    if(strcmp(str, "M_IT_TB_1") == 0) {
        return M_IT_TB_1;
    }

    if(strcmp(str, "command") == 0) {
        return 0;
    }

    return 0;
}

QualityDescriptor getQualityDescriptorFromString(const char * str)
{
    if(strcmp(str, "IEC60870_QUALITY_GOOD") == 0) {
        return IEC60870_QUALITY_GOOD;
    }

    if(strcmp(str, "IEC60870_QUALITY_INVALID") == 0) {
        return IEC60870_QUALITY_INVALID;
    }

    if(strcmp(str, "IEC60870_QUALITY_OVERFLOW") == 0) {
        return IEC60870_QUALITY_OVERFLOW;
    }

    if(strcmp(str, "IEC60870_QUALITY_RESERVED") == 0) {
        return IEC60870_QUALITY_RESERVED;
    }

    if(strcmp(str, "IEC60870_QUALITY_ELAPSED_TIME_INVALID") == 0) {
        return IEC60870_QUALITY_ELAPSED_TIME_INVALID;
    }

    if(strcmp(str, "IEC60870_QUALITY_BLOCKED") == 0) {
        return IEC60870_QUALITY_BLOCKED;
    }

    if(strcmp(str, "IEC60870_QUALITY_SUBSTITUTED") == 0) {
        return IEC60870_QUALITY_SUBSTITUTED;
    }

    if(strcmp(str, "IEC60870_QUALITY_NON_TOPICAL") == 0) {
        return IEC60870_QUALITY_NON_TOPICAL;
    }

    return IEC60870_QUALITY_GOOD;
}