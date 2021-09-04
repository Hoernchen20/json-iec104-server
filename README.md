# json-iec104-server

This software converts json styled messages to/from stdin/stdout and a scada system via [IEC60870-5-104](https://de.wikipedia.org/wiki/IEC_60870). It is designed to use from a daemon-node in Node-Red.

Supported ASDU types in monitoring direction:
* M_SP_TB_1 (Single-point information with time tag CP56Time2a)
* M_DP_TB_1 (Double-point information with time tag CP56Time2a)
* M_ME_TD_1 (Measured value, normalized value with time tag CP56Time2a)
* M_IT_NA_1 (Integrated totals)

Supported ASDU types in control direction:
* C_SC_NA_1 (Single command)
* C_DC_NA_1 (Double command)
* C_SE_NA_1 (Set-point command, normalized value)
* C_CS_NA_1 (Clock synchronization command)
* C_IC_NA_1 (Interrogation command group 20)

The server holds a local copy of all ASDUs in monitirong direction, to answer the interrogation command.
The integrated totals are send every minute and not in the interrogation command.
All ASDUs in monitoring direction are only send, if the value has changed.

## Installation
```
git clone https://github.com/Hoernchen20/json-iec104-server.git
cd json-iec104-server
cp server_config.h.example server_config.h
```

Change server settings like ip addresses, data model size and object addresses in `server_config.h`.
```
nano server_config.h
make
```

## Usage
There are some example flows in `nodered-flows`.

If you like to test without Node-Red, simple starts the server with `./json-iec104-server`.

To send a singel point information with CP56 timetag, use:
```
{"type":"M_SP_TB_1", "value":1, "address":0, "qualifier":"IEC60870_QUALITY_GOOD"}
```

To send a normalizied measurment value with CP56 timetag, use:
```
{"type":"M_ME_TD_1", "value":0.12345, "address":0, "qualifier":"IEC60870_QUALITY_GOOD"}
```

To send a normalizied measurment value with CP56 timetag and invalid bit set, use:
```
{"type":"M_ME_TD_1", "value":0.12345, "address":0, "qualifier":"IEC60870_QUALITY_INVALID"}
```

Messages receive from the scada system has the same style.


## Credits
* MZ Automation ([lib60870](https://github.com/mz-automation/lib60870))
* Dave Gamble ([cJSON](https://github.com/DaveGamble/cJSON))

## License
This Software is license under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html).