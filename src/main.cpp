#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

#include <nlohmann/json.hpp>

#include "connected_vehicle_platform/auth_client.hpp"
#include "connected_vehicle_platform/connectivity_service.hpp"
#include "connected_vehicle_platform/logger.hpp"
#include "connected_vehicle_platform/mqtt_client.hpp"
#include "connected_vehicle_platform/someip_client.hpp"
#include "connected_vehicle_platform/telemetry_manager.hpp"

namespace {

class DemoAuthBackend : public cvp::IAuthBackend {
 public:
  cvp::AuthResponse authenticate(const cvp::AuthRequest& request) override {
    if (request.vehicle_id.empty() || request.vehicle_secret.empty()) {
      return {};
    }

    return {"jwt-demo-token", std::chrono::seconds{3600}};
  }
};

void runCloudListener(unsigned short port) {
  const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    return;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);

  if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
      listen(server_fd, 1) != 0) {
    close(server_fd);
    return;
  }

  std::cout << "[Cloud] listening for SOME/IP traffic on localhost:" << port << std::endl;
  const int client_fd = accept(server_fd, nullptr, nullptr);
  if (client_fd >= 0) {
    std::array<char, 2048> buffer{};
    const auto received = recv(client_fd, buffer.data(), buffer.size(), 0);
    if (received > 0) {
      const auto payload_size = std::max<int>(0, received - 8);
      const auto payload_offset = std::min<int>(8, received);
      std::string payload;
      if (payload_size > 0) {
        payload.assign(buffer.data() + payload_offset, static_cast<std::size_t>(payload_size));
      }
      std::cout << "[Cloud] received SOME/IP payload: " << payload << std::endl;
    }
    close(client_fd);
  }
  close(server_fd);
}

}  // namespace

int main() {
  cvp::SomeipConfig someip_config{"127.0.0.1", 41002, 0x1234, 0x0001, "ECU_001"};
  auto logger = std::make_shared<cvp::Logger>("logs/connected_vehicle.log");
  auto auth_backend = std::make_shared<DemoAuthBackend>();
  auto auth_client = std::make_shared<cvp::AuthClient>(auth_backend, someip_config.client_id,
                                                       "secret123");
  auto someip_client = std::make_shared<cvp::SomeipClient>(someip_config);
  auto telemetry_manager = std::make_shared<cvp::TelemetryManager>(someip_config.client_id,
                                                                     someip_client);
  cvp::ConnectivityService service(auth_client, someip_client, telemetry_manager, logger);

  std::thread cloud_listener([&] { runCloudListener(someip_config.port); });

  if (!service.start()) {
    std::cerr << "Failed to initialize the connectivity service." << std::endl;
    return 1;
  }

  std::this_thread::sleep_for(std::chrono::seconds(6));
  service.stop();
  cloud_listener.join();
  return 0;
}
