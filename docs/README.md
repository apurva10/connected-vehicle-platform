# Connected Vehicle Platform

This project implements a modular automotive connectivity service designed for Linux-based vehicle ECUs.

## Architecture

- ConnectivityService: coordinates startup, telemetry publishing, health monitoring, and graceful shutdown.
- AuthClient: handles JWT-based authentication with dependency injection for cloud backends.
- MqttClient: wraps MQTT connectivity with connection state monitoring and reconnect behavior.
- TelemetryManager: generates vehicle telemetry payloads and queues messages when offline.
- Logger: provides timestamped, thread-safe logging with source locations.

## Build

See the [project README](../README.md) for clone-and-build instructions. The project uses CMake FetchContent and does not rely on local machine paths.

## Configuration

The sample runtime reads MQTT settings from [config/mqtt.json](../config/mqtt.json).
