# Ready Player Pi — Test Setup
To ensure all libraries are installed and it is compiled correctly there is 2 python scripts which install/update the libraries, compiles the project and run the executable. 

To run the server use:  

run_server.sh by writing the command "bash run_server.s" in the terminal on the rpi

To run a client use:  

run_client.sh by writing the command "bash run_client.sh" in the terminal on the rpi

This installs the libraries:
- nlohmann JSON
- ALSA
- Qt6


## Kør — 3 RPi'er
Antag Server RPi har IP `192.168.1.100`.

```
RPi 1 (Server):   bash run_server.sh
RPi 2 (Spiller):  bash run_client.sh 192.168.1.100
RPi 3 (Spiller):  bash run_client.sh 192.168.1.100
```

## Kør — lokal test (1 maskine, 3 terminaler)
```
Terminal 1:  bash run_server.sh
Terminal 2:  bash run_client.sh
Terminal 3:  bash run_client.sh
```

## Flow
1. Begge spillere starter `./client` og taster et username
2. Spiller A vælger "1. Host Room" → får room-kode
3. Spiller B vælger "2. Join Room" → taster room-koden
4. Spiller A vælger spil (1. GyroPi / 2. NotePi)
5. Server fork()'er → child = game room
6. Roller tildeles tilfældigt (Performer / Communicator)
7. GyroPi kører med hardcoded tilt data
8. Ved goal → scoreboard vises → game room exits
