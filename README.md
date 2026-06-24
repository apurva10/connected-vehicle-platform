# Connected Vehicle Platform

Modular automotive connectivity service for Linux-based vehicle ECUs. The repository is self-contained: clone, configure with CMake, and build without machine-specific paths or pre-installed third-party libraries.

## Architecture

- **ConnectivityService** — coordinates startup, telemetry publishing, health monitoring, and graceful shutdown
- **AuthClient** — JWT-based authentication with injectable cloud backends
- **MqttClient** — MQTT connectivity with connection monitoring and reconnect behavior
- **TelemetryManager** — vehicle telemetry payloads with offline queuing
- **Logger** — timestamped, thread-safe logging with source locations
- **SomeipClient** — lightweight SOME/IP messaging for ECU-to-cloud demos

FIDL definitions and pre-generated CommonAPI/SOME/IP bindings live under `fidl/` and `generated/`.

## Quick start

Requirements: CMake 3.21+, a C++20 compiler, Git, and Ninja (recommended).

```bash
git clone <repository-url>
cd connected-vehicle-platform

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
./build/cvp_app
```

Dependencies (GoogleTest, nlohmann/json, Eclipse Paho MQTT) are fetched automatically by CMake FetchContent on first configure.

## Configuration

Sample runtime settings:

| File | Purpose |
|------|---------|
| `config/mqtt.json` | MQTT broker, client ID, topics |
| `config/app_settings.json` | Project metadata |
| `config/vsomeip.json` | SOME/IP stack settings |
| `config/commonapi-someip.ini` | CommonAPI SOME/IP binding |

## Docker

```bash
docker build -f docker/Dockerfile -t cvp .
docker run --rm cvp
```

## Optional: regenerate CommonAPI bindings

The committed `generated/` tree is used by default. To regenerate from FIDL, install the CommonAPI generators and enable codegen:

```bash
export COMMONAPI_HOME=/opt/commonapi   # directory containing the generator binaries
cmake -S . -B build -DCVP_ENABLE_COMMONAPI_CODEGEN=ON
cmake --build build --target commonapi_codegen
```

## Optional: use system Paho MQTT packages

If Paho is already installed on the host (e.g. `libpaho-mqtt-dev` and `libpaho-mqttpp-dev` on Debian/Ubuntu):

```bash
cmake -S . -B build -DCVP_USE_SYSTEM_PAHO=ON
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
