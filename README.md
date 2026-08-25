# Qiyuan-Clowire Protocol Gateway

An embedded Linux gateway that connects Qiyuan lighting and passive infrared
(PIR) modules to a Clowire smart-home network.

The application exchanges messages with the Clowire gateway over RS-485 and
communicates with Qiyuan devices over a second serial interface. It translates
device addresses and protocol frames, manages network enrollment, stores scene
configuration, and maintains lighting and sensor state in SQLite databases.

> This project targets a specific ARM Linux hardware platform. It requires
> platform-specific device nodes and is not a general-purpose desktop program.

## Features

- Translate Clowire lighting commands into Qiyuan device commands.
- Control dimming and color-temperature channels.
- Convert Clowire brightness values from `0-100` to Qiyuan values from `0-255`.
- Restore the previously stored brightness when switching a light back on.
- Configure, trigger, and delete lighting and PIR scenes.
- Enable or disable PIR sensors and associate sensors with devices or scenes.
- Coordinate multiple PIR sensors assigned to the same target.
- Support delayed and reverse-delayed device actions.
- Assign device addresses from an eight-position hardware dial.
- Enroll Qiyuan modules in the Clowire network using a hardware button.
- Persist runtime state in SQLite and configuration files under `/config`.
- Feed the platform watchdog while the application is running.

## System Architecture

```text
                           +-----------------------+
                           |   Dial / Button / LED |
                           +-----------+-----------+
                                       |
                                       v
+------------------+        +-----------------------+        +------------------+
| Clowire Gateway  | RS-485 |   Protocol Gateway    |  UART  | Qiyuan Devices   |
| /dev/ttyS1       +------->+                       +------->+ /dev/ttyS10      |
| 9600 baud        |<-------+  Parsing, translation |<-------+ 115200 baud      |
+------------------+        |  scenes, PIR handling |        +------------------+
                            +-----------+-----------+
                                        |
                         +--------------+--------------+
                         |                             |
                         v                             v
                +------------------+          +------------------+
                | /config/*.txt    |          | SQLite databases |
                +------------------+          +------------------+
```

The Clowire interface performs frame escaping and XOR validation. The Qiyuan
interface receives and transmits the device-specific serial protocol. Separate
read and write queues connect serial I/O with the application logic.

## Repository Layout

```text
.
|-- Makefile
|-- core/
|   |-- data_handle.c          # Device control, scenes, PIR events, feedback
|   |-- remote.c               # SQLite initialization and state management
|   `-- transform.c            # Clowire-to-Qiyuan command dispatch
|-- include/
|   |-- addr_req.h             # Configuration paths and address definitions
|   |-- data_handle.h          # Scene, callback, and PIR data structures
|   |-- interface_manage.h     # Hardware interface abstractions
|   |-- protocol.h             # Protocol constants and command identifiers
|   |-- queue.h                # Circular queue definitions
|   `-- transform.h            # Protocol transformation declarations
|-- interface/
|   |-- interface_manage.c     # Registration and lifecycle of hardware devices
|   |-- Serial/
|   |   |-- calculate.c        # Frame extraction, escaping, and XOR checks
|   |   |-- queue.c            # Thread-safe serial queues
|   |   `-- serial.c           # Serial configuration, I/O, and delayed actions
|   `-- watchdog/
|       `-- watchdog.c         # Hardware watchdog support
|-- main/
|   `-- main_qc.c              # Application entry point
`-- request_addr/
    `-- addr_req.c             # Dial addresses, scenes, and text configuration
```

## Hardware and Runtime Requirements

The target system must provide the following device nodes:

| Device | Purpose |
| --- | --- |
| `/dev/ttyS1` | Clowire RS-485 interface, configured at 9600 baud |
| `/dev/ttyS10` | Qiyuan serial interface, configured at 115200 baud |
| `/dev/dial0` through `/dev/dial7` | Eight-bit hardware address dial |
| `/dev/inetbut0` | Network enrollment button |
| `/dev/inetled0` | Network enrollment status LED |
| `/dev/watchdog` | Hardware watchdog |

Additional requirements:

- An ARM Linux environment compatible with the selected cross-toolchain.
- Linux serial, RS-485, and watchdog kernel interfaces.
- POSIX threads, linked with `-lpthread`.
- SQLite 3 development and runtime libraries, linked with `-lsqlite3`.
- A writable `/config` directory.
- Sufficient permissions to access the required device nodes.

The application does not emulate unavailable hardware. In particular, failure
to open `/dev/watchdog` terminates the process.

## Build

The supplied `Makefile` defaults to this cross-compiler prefix:

```text
/home/7200/arm-2014.05/bin/arm-none-linux-gnueabi-
```

Build with the default toolchain:

```sh
make
```

Use a different cross-compiler installation by overriding `COMPILE_PREX`:

```sh
make COMPILE_PREX=/opt/toolchains/bin/arm-none-linux-gnueabi-
```

`COMPILE_PREX` is the variable name used by the existing `Makefile`; its
spelling is intentional. The corresponding toolchain must also provide access
to compatible pthread and SQLite libraries for the target architecture.

The output executable is created as `./qc_code`. Object files are generated next
to their respective source files.

Remove generated build artifacts with:

```sh
make clean
```

The code depends on Linux-specific headers and legacy compiler behavior. A
native macOS build is not supported, and modern compilers may require source
changes before a clean build succeeds.

## Runtime Configuration

All configuration and persistent state use hard-coded paths beneath `/config`.

### Device Counts

Create `/config/dev_num.txt` before starting the gateway:

```text
dimmer_count 2
pir_count 1
```

The current parser reads the second whitespace-separated field from each line.
The first line must contain the number of dimmer modules, and the second line
must contain the number of PIR modules. The labels are descriptive; their exact
names are not interpreted by the application.

### Generated Address File

The gateway reads `/dev/dial0` through `/dev/dial7` to determine the starting
device address, then writes `/config/addr.txt`.

For a starting address of `16`, two dimmer modules, and one PIR module, the file
will contain:

```text
dimmer_addr 16
dimmer_addr 17
pir_addr 18
```

Dimmer addresses are allocated first, followed by PIR addresses. Dial values
above `0xF0` are rejected.

> Changing the configured starting address can delete existing brightness
> history, scene configuration, and both SQLite databases. Back up `/config`
> before changing the hardware dial on an existing installation.

### Persistent Files

| Path | Purpose |
| --- | --- |
| `/config/dev_num.txt` | Required module counts: dimmers first, PIR modules second |
| `/config/addr.txt` | Generated Clowire addresses for configured modules |
| `/config/photometric.txt` | Last nonzero brightness for individual dimmer channels |
| `/config/scene.txt` | Stored dimmer and PIR scene configuration |
| `/config/remote.db` | SQLite state and target configuration for PIR channels |
| `/config/dimmer.db` | SQLite state for dimmer channels |
| `/config/module_tmp.txt` | Temporary module list used when preparing scene feedback |
| `/config/scene_tmp.txt` | Temporary file used during scene updates |
| `/config/photemetric_tmp.txt` | Temporary file used during brightness updates |

`photemetric_tmp.txt` reflects the path currently defined in the source code.

## Device and Protocol Model

### Device Types

| Device | Enrollment type | Addressing |
| --- | --- | --- |
| Qiyuan dimmer module | `0x35` | Eight Clowire subaddresses, `0x31` through `0x38` |
| Qiyuan PIR module | `0x24` | Eight PIR subaddresses, starting at `0x21` |

Dimmer subaddresses are paired into four Qiyuan circuits. Odd subaddresses use
application code `0x3A`, while adjacent even subaddresses use application code
`0x3B` for the corresponding color-temperature channel.

### Supported Clowire Commands

| Command | Value | Behavior |
| --- | --- | --- |
| Single-device control | `0x01` | Control a dimmer channel or update PIR availability |
| Scene configuration | `0x09` | Store a dimmer or PIR scene entry |
| Scene configuration deletion | `0x0B` | Remove a previously stored scene entry |
| Scene control | `0x0D` | Apply a stored scene and send device feedback |
| Delay/input configuration | `0x11` | Associate a PIR input with a device or scene |
| Network enrollment callback | `0x1A` | Continue enrolling configured Qiyuan modules |

For single-device dimmer control:

- Values from `0x00` through `0x64` represent brightness from 0% through 100%.
- `0x9A` switches the channel off.
- `0x90` restores the previously recorded brightness, defaulting to 100% when
  no value has been stored.

### SQLite Tables

The PIR database stores one row per sensor channel:

```sql
CREATE TABLE remote (
    num INT,
    state INT,
    value INT,
    devaddr INT,
    subaddr INT,
    sceneid INT
);
```

The dimmer database stores the current value of each module subaddress:

```sql
CREATE TABLE dimmer (
    dev INT,
    sub INT,
    value INT
);
```

## Running on the Target Device

Prepare the writable configuration directory and required module-count file:

```sh
mkdir -p /config
printf 'dimmer_count 2\npir_count 1\n' > /config/dev_num.txt
```

Start the application on the supported target hardware:

```sh
./qc_code
```

At startup, the application:

1. Reads the hardware dial and generates the module address list.
2. Initializes the PIR and dimmer SQLite databases.
3. Registers both serial interfaces and the hardware watchdog.
4. Starts serial read/write workers and the enrollment-button monitor.
5. Processes Clowire commands and Qiyuan PIR events continuously.

Address discovery and database initialization currently run in separate
background threads, so these startup operations are not strictly synchronized.

To enroll devices, press the button exposed through `/dev/inetbut0`. The gateway
submits each configured module to the Clowire network and updates the enrollment
LED through `/dev/inetled0`.

## Troubleshooting

Check whether the required device nodes are present:

```sh
ls -l /dev/ttyS1 /dev/ttyS10 /dev/watchdog /dev/inetbut0 /dev/inetled0
ls -l /dev/dial0 /dev/dial1 /dev/dial2 /dev/dial3
ls -l /dev/dial4 /dev/dial5 /dev/dial6 /dev/dial7
```

Inspect the generated configuration and databases:

```sh
cat /config/dev_num.txt
cat /config/addr.txt
sqlite3 /config/remote.db '.schema'
sqlite3 /config/dimmer.db '.schema'
```

Inspect the build commands without executing them:

```sh
make -B -n
```

Common causes of startup failure include a missing `/config/dev_num.txt`,
insufficient device permissions, unavailable SQLite libraries, missing custom
device drivers, and a cross-toolchain that does not match the target platform.

## Current Limitations

- Hardware paths, serial ports, and configuration locations are hard-coded.
- The application depends on custom Linux device drivers and an ARM toolchain.
- The repository does not currently include automated tests or CI configuration.
- Several public functions lack declarations required by modern C compilers.
- Startup threads are not synchronized before serial event processing begins.
- Input validation, file error handling, and graceful shutdown are incomplete.
- Existing reverse-delay and PIR-group logic should be reviewed before relying
  on those features in a production deployment.
