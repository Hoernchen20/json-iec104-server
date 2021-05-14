# json-iec104-server

This software acts as an gateway between json styled messages and a scada system via [IEC60870-5-104](https://de.wikipedia.org/wiki/IEC_60870). It is designed to use from a daemon-node in Node-Red.

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

## Credits
* MZ Automation ([lib60870](https://github.com/mz-automation/lib60870))
* Dave Gamble ([cJSON](https://github.com/DaveGamble/cJSON))

## License
This Software is license under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html).