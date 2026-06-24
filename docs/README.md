# Connected Vehicle Platform

This project implements a modular automotive connectivity service designed for Linux-based vehicle ECUs.

## Architecture

- ConnectivityService: coordinates startup, telemetry publishing, health monitoring, and graceful shutdown.
- AuthClient: handles JWT-based authentication with dependency injection for cloud backends.
- MqttClient: wraps MQTT connectivity with connection state monitoring and reconnect behavior.
- TelemetryManager: generates vehicle telemetry payloads and queues messages when offline.
- Logger: provides timestamped, thread-safe logging with source locations.

## Build

```bash
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

## Configuration

The sample runtime reads MQTT settings from [config/mqtt.json](../config/mqtt.json).
