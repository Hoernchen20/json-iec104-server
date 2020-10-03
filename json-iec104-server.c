#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#include "iec60870_slave.h"
#include "cs104_slave.h"

#include "hal_thread.h"
#include "hal_time.h"

#include "cJSON/cJSON.h"

#define ASDU 193
#define IP_ADDRESS_FER_1 "10.1.21.154"
#define IP_ADDRESS_FER_2 "172.16.3.12"

#define BASE_ADDRESS_M_SP_TB_1 4097
#define BASE_ADDRESS_M_DP_TB_1 6144
#define BASE_ADDRESS_M_ME_TD_1 8193
#define BASE_ADDRESS_M_IT_NA_1 12289
#define BASE_ADDRESS_C_SC_NA_1 24577
#define BASE_ADDRESS_C_DC_NA_1 26624
#define BASE_ADDRESS_C_SE_NA_1 20481

#define NUMBERS_OF_M_SP_TB_1 10
#define NUMBERS_OF_M_DP_TB_1 5
#define NUMBERS_OF_M_ME_TD_1 20
#define NUMBERS_OF_M_IT_TB_1 2

struct sM_SP_TB_1 {
    bool value;
    QualityDescriptor qualifier;
} M_SP_TB_1_data[NUMBERS_OF_M_SP_TB_1];

struct sM_DP_TB_1 {
    DoublePointValue value;
    QualityDescriptor qualifier;
} M_DP_TB_1_data[NUMBERS_OF_M_DP_TB_1];

struct sM_ME_TD_1 {
    float value;
    QualityDescriptor qualifier;
} M_ME_TD_1_data[NUMBERS_OF_M_ME_TD_1];

static CS101_AppLayerParameters appLayerParameters;
struct sBinaryCounterReading M_IT_TB_1_data[NUMBERS_OF_M_IT_TB_1];
bool running = true;

pthread_mutex_t M_IT_TB_1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t M_SP_TB_1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t M_DP_TB_1_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t M_ME_TD_1_mutex = PTHREAD_MUTEX_INITIALIZER;

bool IsClientConnected(CS104_Slave self)
{
    if(CS104_Slave_getOpenConnections(self) > 0) {
        return true;
    } else {
        return false;
    }
}

CP56Time2a GetCP56Time2a(void)
{
    CP56Time2a iec_time = CP56Time2a_createFromMsTimestamp(NULL, Hal_getTimeInMs()+2*60*60*1000);;
    time_t rawtime;
    struct tm * timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    CP56Time2a_setSummerTime(iec_time, (bool)(timeinfo->tm_isdst));

    return iec_time;
}

int GetDataTypeNumber(const char *str)
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

int GetQualifierNumber(const char *str) {
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

static void *
SendIntegratedTotalsPeriodic(void *arg)
{
    CS104_Slave slave = arg;
    uint32_t sequenc_number = 0;
    time_t rawtime;
    struct tm * timeinfo;

    while(1) {
        time (&rawtime);
        timeinfo = localtime (&rawtime);

        if (IsClientConnected(slave) == true && timeinfo->tm_sec == 1) {
            CS101_ASDU newAsdu = CS101_ASDU_create(
                appLayerParameters, false, CS101_COT_PERIODIC, ASDU, ASDU, false, false);

            InformationObject io = (InformationObject) MeasuredValueNormalized_create(NULL, 0, 0.0, 0);

            pthread_mutex_lock(&M_IT_TB_1_mutex);

            for (int i = 0; i < NUMBERS_OF_M_IT_TB_1; i++) {
                BinaryCounterReading_setSequenceNumber(&M_IT_TB_1_data[i], sequenc_number);
                CS101_ASDU_addInformationObject(newAsdu, (InformationObject)
                IntegratedTotals_create((IntegratedTotals) io, BASE_ADDRESS_M_IT_NA_1 + i, &M_IT_TB_1_data[i]));
            }

            pthread_mutex_unlock(&M_IT_TB_1_mutex);

            InformationObject_destroy(io);

            CS104_Slave_enqueueASDU(slave, newAsdu);

            CS101_ASDU_destroy(newAsdu);

            sequenc_number++;
        }
        Thread_sleep(1000);
    }

    return NULL;
}

void
sigint_handler(int signalId)
{
    running = false;
}

static bool
clockSyncHandler (void* parameter, IMasterConnection connection, CS101_ASDU asdu, CP56Time2a newTime)
{
    struct tm tmTime;
    char buffer[100];
    char cmd_buf[200];

    tmTime.tm_sec = CP56Time2a_getSecond(newTime);
    tmTime.tm_min = CP56Time2a_getMinute(newTime);
    tmTime.tm_hour = CP56Time2a_getHour(newTime);
    tmTime.tm_mday = CP56Time2a_getDayOfMonth(newTime);
    tmTime.tm_mon = CP56Time2a_getMonth(newTime) - 1;
    tmTime.tm_year = CP56Time2a_getYear(newTime) + 100;

    mktime(&tmTime);
    strftime(buffer, sizeof(buffer), "%FT%X%z", &tmTime);
    printf ("{\"type\":\"C_CS_NA_1\",\"value\":\"%s\"}\n", buffer);

    snprintf(cmd_buf, sizeof(cmd_buf), "sudo /bin/date --set=\"%s\" >/dev/null", buffer);
    if (system(cmd_buf)) {
         printf("{\"warn\":\"set system time failed\"}\n");
    } else {
         printf("{\"info\":\"set system time success\"}\n");
    }

    return true;
}

static bool
interrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
    printf("{\"info\":\"received interrogation for group %i\"}\n", qoi);

    if (qoi == 20) { /* only handle station interrogation */

        IMasterConnection_sendACT_CON(connection, asdu, false);

        /* The CS101 specification only allows information objects without timestamp in GI responses */

        //M_SP_TB_1
        CS101_ASDU newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_INTERROGATED_BY_STATION,
            ASDU, ASDU, false, false);

        InformationObject io = (InformationObject) MeasuredValueNormalized_create(NULL, 0, 0.0, 0);

        pthread_mutex_lock(&M_SP_TB_1_mutex);

        for (int i = 0; i < NUMBERS_OF_M_SP_TB_1; i++) {
            CS101_ASDU_addInformationObject(
                newAsdu, (InformationObject) SinglePointInformation_create(
                    NULL, BASE_ADDRESS_M_SP_TB_1 + i, M_SP_TB_1_data[i].value, M_SP_TB_1_data[i].qualifier));
        }

        pthread_mutex_unlock(&M_SP_TB_1_mutex);

        InformationObject_destroy(io);

        IMasterConnection_sendASDU(connection, newAsdu);

        CS101_ASDU_destroy(newAsdu);

        //M_DP_TB_1
        newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_INTERROGATED_BY_STATION,
            ASDU, ASDU, false, false);

        io = (InformationObject) MeasuredValueNormalized_create(NULL, 0, 0.0, 0);

        pthread_mutex_lock(&M_DP_TB_1_mutex);

        for (int i = 0; i < NUMBERS_OF_M_DP_TB_1; i++) {
            CS101_ASDU_addInformationObject(
                newAsdu, (InformationObject) DoublePointInformation_create(
                    NULL, BASE_ADDRESS_M_DP_TB_1 + i, M_DP_TB_1_data[i].value, M_DP_TB_1_data[i].qualifier));
        }

        pthread_mutex_unlock(&M_DP_TB_1_mutex);

        InformationObject_destroy(io);

        IMasterConnection_sendASDU(connection, newAsdu);

        CS101_ASDU_destroy(newAsdu);

        //M_ME_TD_1
        newAsdu = CS101_ASDU_create(appLayerParameters, false, CS101_COT_INTERROGATED_BY_STATION,
            ASDU, ASDU, false, false);

        io = (InformationObject) MeasuredValueNormalized_create(NULL, 0, 0.0, 0);

        pthread_mutex_lock(&M_ME_TD_1_mutex);

        for (int i = 0; i < NUMBERS_OF_M_ME_TD_1; i++) {
            CS101_ASDU_addInformationObject(
                newAsdu, (InformationObject) MeasuredValueNormalized_create(
                    NULL, BASE_ADDRESS_M_ME_TD_1 + i, M_ME_TD_1_data[i].value, M_ME_TD_1_data[i].qualifier));
        }

        pthread_mutex_unlock(&M_ME_TD_1_mutex);

        InformationObject_destroy(io);

        IMasterConnection_sendASDU(connection, newAsdu);

        CS101_ASDU_destroy(newAsdu);
        IMasterConnection_sendACT_TERM(connection, asdu);
    }
    else {
        IMasterConnection_sendACT_CON(connection, asdu, true);
    }

    return true;
}

static bool asduHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu)
{
    bool rc = false;
    if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {
        InformationObject io = CS101_ASDU_getElement(asdu, 0);

        if(io != NULL) {
            int object_address = InformationObject_getObjectAddress(io);
            switch (CS101_ASDU_getTypeID(asdu)){
                case C_SC_NA_1: {
                    if (object_address >= BASE_ADDRESS_C_SC_NA_1 && object_address <= BASE_ADDRESS_C_SC_NA_1 + 2048 ) {
                        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);
                        IMasterConnection_sendASDU(connection, asdu);
                        SingleCommand sc = (SingleCommand) io;
                        printf("{\"type\":\"C_SC_NA_1\",\"address\":%d,\"value\":%d,\"qualifier\":%d}\n",
                            object_address - BASE_ADDRESS_C_SC_NA_1, SingleCommand_getState(sc), SingleCommand_getQU(sc));
                        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_TERMINATION);
                        IMasterConnection_sendASDU(connection, asdu);
                    } else {
                        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
                        IMasterConnection_sendASDU(connection, asdu);
                        printf("{\"warn\":\"received address not in range: %d\"}\n", object_address);
                    }
                    rc = true;
                    break;
                }
                case C_DC_NA_1: {
                    if (object_address >= BASE_ADDRESS_C_DC_NA_1 && object_address <= BASE_ADDRESS_C_DC_NA_1 + 2048 ) {
                        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);
                        IMasterConnection_sendASDU(connection, asdu);
                        DoubleCommand dc = (DoubleCommand) io;
                        printf("{\"type\":\"C_DC_NA_1\",\"address\":%d,\"value\":%d,\"qualifier\":%d}\n",
                            object_address - BASE_ADDRESS_C_DC_NA_1, DoubleCommand_getState(dc), DoubleCommand_getQU(dc));
                        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_TERMINATION);
                        IMasterConnection_sendASDU(connection, asdu);
                    } else {
                        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
                        IMasterConnection_sendASDU(connection, asdu);
                        printf("{\"warn\":\"received address not in range: %d\"}\n", object_address);
                    }
                    rc = true;
                    break;
                }
                case C_SE_NA_1: {
                    if (object_address >= BASE_ADDRESS_C_SE_NA_1 && object_address <= BASE_ADDRESS_C_SE_NA_1 + 2048 ) {
                        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);
                        IMasterConnection_sendASDU(connection, asdu);
                        SetpointCommandNormalized sp = (SetpointCommandNormalized) io;
                        printf("{\"type\":\"C_SE_NA_1\",\"address\":%d,\"value\":%f,\"qualifier\":%d}\n",
                            object_address - BASE_ADDRESS_C_SE_NA_1, SetpointCommandNormalized_getValue(sp), SetpointCommandNormalized_getQL(sp));
                        CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_TERMINATION);
                        IMasterConnection_sendASDU(connection, asdu);
                    } else {
                        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
                        IMasterConnection_sendASDU(connection, asdu);
                        printf("{\"warn\":\"received address not in range: %d\"}\n", object_address);
                    }
                    rc = true;
                    break;
                }
                default: {
                    CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_TYPE_ID);
                    IMasterConnection_sendASDU(connection, asdu);
                    printf("{\"warn\":\"received unknown type id: %d\"}\n", CS101_ASDU_getTypeID(asdu));
                    break;
                }
            }
        }

        InformationObject_destroy(io);

    } else {
        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
        IMasterConnection_sendASDU(connection, asdu);
    }

    rc = true;

    return rc;
}

static bool
connectionRequestHandler(void* parameter, const char* ipAddress)
{
    if (strcmp(ipAddress, IP_ADDRESS_FER_1) == 0) {
        printf("{\"info\":\"accept connection from %s\"}\n", ipAddress);
        return true;
    } else if (strcmp(ipAddress, IP_ADDRESS_FER_2) == 0) {
        printf("{\"info\":\"accept connection from %s\"}\n", ipAddress);
        return true;
    } else if (strcmp(ipAddress, "192.168.45.5") == 0) {
        printf("{\"info\":\"accept connection from %s\"}\n", ipAddress);
        return true;
    } else {
        printf("{\"warn\":\"deny connection from %s\"}\n", ipAddress);
        return false;
    }
}

static void
ConnectionEventHandler(void* parameter, IMasterConnection connection, CS104_PeerConnectionEvent event)
{
    switch (event) {
    case CS104_CON_EVENT_ACTIVATED:
        printf("{\"info\":\"received STARTDT_CON\"}\n");
        break;
    case CS104_CON_EVENT_DEACTIVATED:
        printf("{\"info\":\"received STOPDT_CON\"}\n");
        break;
    case CS104_CON_EVENT_CONNECTION_OPENED:
    case CS104_CON_EVENT_CONNECTION_CLOSED:
        break;
    }
}


int
main(int argc, char** argv)
{
    /* Add Ctrl-C handler */
    signal(SIGINT, sigint_handler);

    /* create a new slave/server instance with default connection parameters and
     * default message queue size */
    CS104_Slave slave = CS104_Slave_create(100, 100);

    CS104_Slave_setLocalAddress(slave, "0.0.0.0");

    CS104_Slave_setServerMode(slave, CS104_MODE_MULTIPLE_REDUNDANCY_GROUPS);

    CS104_RedundancyGroup redGroup1 = CS104_RedundancyGroup_create("red-group-1");
    CS104_RedundancyGroup_addAllowedClient(redGroup1, IP_ADDRESS_FER_1);
    CS104_RedundancyGroup_addAllowedClient(redGroup1, IP_ADDRESS_FER_2);

    CS104_Slave_addRedundancyGroup(slave, redGroup1);

    /* get the connection parameters - we need them to create correct ASDUs */
    appLayerParameters = CS104_Slave_getAppLayerParameters(slave);

    /* set the callback handler for the clock synchronization command */
    CS104_Slave_setClockSyncHandler(slave, clockSyncHandler, NULL);

    /* set the callback handler for the interrogation command */
    CS104_Slave_setInterrogationHandler(slave, interrogationHandler, NULL);

    /* set handler for other message types */
    CS104_Slave_setASDUHandler(slave, asduHandler, NULL);

    CS104_Slave_setConnectionRequestHandler(slave, connectionRequestHandler, NULL);
    CS104_Slave_setConnectionEventHandler(slave, ConnectionEventHandler, NULL);

    /* Set server mode to allow multiple clients using the application layer
     * NOTE: library has to be compiled with CONFIG_CS104_SUPPORT_SERVER_MODE_CONNECTION_IS_REDUNDANCY_GROUP enabled (=1)
     */
//    CS104_Slave_setServerMode(slave, CS104_MODE_CONNECTION_IS_REDUNDANCY_GROUP);

    /* init data structures */
    for (int i = 0; i < NUMBERS_OF_M_IT_TB_1; i++) {
        BinaryCounterReading_create(&M_IT_TB_1_data[i], 0, 0, false, false, false);
    }

    for (int i = 0; i < NUMBERS_OF_M_DP_TB_1; i++) {
        M_DP_TB_1_data[i].value = IEC60870_DOUBLE_POINT_OFF;
    }

    CS104_Slave_start(slave);

    if (CS104_Slave_isRunning(slave) == false) {
        printf("{\"error\":\"Starting server failed!\"}\n");
        goto exit_program;
    }

    pthread_t IntegratedTotals_thread;
    int ret;

    ret = pthread_create(&IntegratedTotals_thread, NULL, SendIntegratedTotalsPeriodic, slave);
    if (ret == 0) {
        pthread_detach(IntegratedTotals_thread);
    }

    char input[1000];
    setvbuf(stdout, NULL, _IOLBF, 1024);
    int status_json_parse = 0;
    TypeID type;
    double value;
    int address;
    int qualifier;

    while (running) {
        fgets(input, 1000, stdin);

        //parse input
        cJSON *json = cJSON_Parse(input);
        if (json != NULL) {
            //parse type
            cJSON *type_json = cJSON_GetObjectItemCaseSensitive(json, "type");
            if (cJSON_IsString(type_json) && (type_json->valuestring != NULL)) {
                type = GetDataTypeNumber(type_json->valuestring);
                status_json_parse++;
            } else {
                printf("{\"error\":\"invalid data type\"}\n");
            }

            //parse value
            cJSON *value_json = cJSON_GetObjectItemCaseSensitive(json, "value");
            if (cJSON_IsNumber(value_json)) {
                value = value_json->valuedouble;
                status_json_parse++;
            } else {
                printf("{\"error\":\"invalid data value\"}\n");
            }

            //parse address
            cJSON *address_json = cJSON_GetObjectItemCaseSensitive(json, "address");
            if (cJSON_IsNumber(address_json)) {
                address = (int)(address_json->valuedouble);
                status_json_parse++;
            } else {
                printf("{\"error\":\"invalid data address\"}\n");
            }

            //parse qualifier
            cJSON *qualifier_json = cJSON_GetObjectItemCaseSensitive(json, "qualifier");
            if (cJSON_IsString(qualifier_json) && (qualifier_json->valuestring != NULL)) {
                qualifier = GetQualifierNumber(qualifier_json->valuestring);
                status_json_parse++;                
            } else {
                printf("{\"error\":\"invalid data qualifier\"}\n");
            }
        } else {
            printf("{\"error\":\"invalid json\"}\n");
        }
        cJSON_Delete(json);

        //send asdu
        if (status_json_parse >=4) {
            switch(type) {
                case M_SP_TB_1: {
                    if(0 <= address && address < NUMBERS_OF_M_SP_TB_1) {
                        if (IsClientConnected(slave) == true) {
                            CS101_ASDU newAsdu = CS101_ASDU_create(
                                appLayerParameters, false, CS101_COT_SPONTANEOUS, ASDU, ASDU, false, false);
                            InformationObject io = (InformationObject) SinglePointWithCP56Time2a_create(
                                    NULL, BASE_ADDRESS_M_SP_TB_1 + address, (bool)(value), qualifier, GetCP56Time2a());
                            CS101_ASDU_addInformationObject(newAsdu, io);
                            InformationObject_destroy(io);
                            CS104_Slave_enqueueASDU(slave, newAsdu);
                            CS101_ASDU_destroy(newAsdu);
                        }

                        pthread_mutex_lock(&M_SP_TB_1_mutex);
                        M_SP_TB_1_data[address].value = (bool)(value);
                        M_SP_TB_1_data[address].qualifier = qualifier;
                        pthread_mutex_unlock(&M_SP_TB_1_mutex);
                    } else {
                        printf("{\"error\":\"address not in range\"}\n");
                    }
                    break;
                }
                case M_DP_TB_1: {
                    if(0 <= address && address < NUMBERS_OF_M_ME_TD_1) {
                        if (IsClientConnected(slave) == true) {
                            CS101_ASDU newAsdu = CS101_ASDU_create(
                                appLayerParameters, false, CS101_COT_SPONTANEOUS, ASDU, ASDU, false, false);
                            InformationObject io = (InformationObject) DoublePointWithCP56Time2a_create(
                                    NULL, BASE_ADDRESS_M_DP_TB_1 + address, (DoublePointValue)(value), qualifier, GetCP56Time2a());
                            CS101_ASDU_addInformationObject(newAsdu, io);
                            InformationObject_destroy(io);
                            CS104_Slave_enqueueASDU(slave, newAsdu);
                            CS101_ASDU_destroy(newAsdu);
                        }

                        pthread_mutex_lock(&M_DP_TB_1_mutex);
                        M_DP_TB_1_data[address].value = (DoublePointValue)(value);
                        M_DP_TB_1_data[address].qualifier = qualifier;
                        pthread_mutex_unlock(&M_DP_TB_1_mutex);
                    } else {
                        printf("{\"error\":\"address not in range\"}\n");
                    }
                    break;
                }
                case M_ME_TD_1: {
                    if(0 <= address && address < NUMBERS_OF_M_ME_TD_1) {
                            if (IsClientConnected(slave) == true) {
                            CS101_ASDU newAsdu = CS101_ASDU_create(
                                appLayerParameters, false, CS101_COT_SPONTANEOUS, ASDU, ASDU, false, false);
                            InformationObject io = (InformationObject) MeasuredValueNormalizedWithCP56Time2a_create(
                                    NULL, BASE_ADDRESS_M_ME_TD_1 + address, (float)(value), qualifier, GetCP56Time2a());
                            CS101_ASDU_addInformationObject(newAsdu, io);
                            InformationObject_destroy(io);
                            CS104_Slave_enqueueASDU(slave, newAsdu);
                            CS101_ASDU_destroy(newAsdu);
                        }

                        pthread_mutex_lock(&M_ME_TD_1_mutex);
                        M_ME_TD_1_data[address].value = (float)(value);
                        M_ME_TD_1_data[address].qualifier = qualifier;
                        pthread_mutex_unlock(&M_ME_TD_1_mutex);
                    } else {
                        printf("{\"error\":\"address not in range\"}\n");
                    }
                    break;
                }
                case M_IT_TB_1: {
                    if(0 <= address && address < NUMBERS_OF_M_IT_TB_1) {
                        pthread_mutex_lock(&M_IT_TB_1_mutex);
                        BinaryCounterReading_setValue(&M_IT_TB_1_data[address], (uint64_t)(value) % UINT32_MAX);
                        pthread_mutex_unlock(&M_IT_TB_1_mutex);
                    } else {
                        printf("{\"error\":\"address not in range\"}\n");
                    }
                    break;
                }
                default:
                    break;
            }
        } else if(status_json_parse == 2) {
            if(type == 0 && value == 0) {
                running = false;
                printf("{\"info\":\"shutdown server\"}\n");
            }
        } else {
            printf("{\"error\":\"json incomplete\"}\n");
        }
        memset(input, 0, sizeof(input));
        status_json_parse = 0;
    }

exit_program:
    CS104_Slave_stop(slave);
    /* destroy data structures */
/*    for (int i = 0; i < NUMBERS_OF_M_IT_TB_1; i++) {
        BinaryCounterReading_destroy(&M_IT_TB_1_data[i]);
    }*/

    CS104_Slave_destroy(slave);
}
