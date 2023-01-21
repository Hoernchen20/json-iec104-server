#ifndef CONVERT_H_
#define CONVERT_H_

#include "iec60870_common.h"

IEC60870_5_TypeID getTypeIdFromString(const char * str);
QualityDescriptor getQualityDescriptorFromString(const char * str);


#endif // CONVERT_H_