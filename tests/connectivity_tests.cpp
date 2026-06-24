#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "connected_vehicle_platform/auth_client.hpp"
#include "connected_vehicle_platform/mqtt_client.hpp"
#include "connected_vehicle_platform/telemetry_manager.hpp"

namespace cvp {
namespace {

class FakeAuthBackend : public IAuthBackend {
 public:
  explicit FakeAuthBackend(bool success, std::chrono::seconds ttl = std::chrono::seconds{3600})
      : success_(success), ttl_(ttl) {}

  AuthResponse authenticate(const AuthRequest&) override {
    if (!success_) {
      return {};
    }
    return {"jwt-token", ttl_};
  }

 private:
  bool success_;
  std::chrono::seconds ttl_;
};

class FakeMqttBackend : public IMqttBackend {
 public:
  bool connect(std::string_view, std::string_view) override {
    connected_ = true;
    return connect_result_;
  }

  bool disconnect() override {
    connected_ = false;
    return true;
  }

  bool publish(std::string_view, std::string_view, int) override {
    return publish_result_;
  }

  bool subscribe(std::string_view, int) override {
    return subscribe_result_;
  }

  bool isConnected() const override {
    return connected_;
  }

  bool connect_result_{true};
  bool publish_result_{true};
  bool subscribe_result_{true};
  bool connected_{false};
};

class FakeMqttClient : public IMqttClient {
 public:
  explicit FakeMqttClient(bool connected = true)
      : config_{"ssl://localhost:8883", "ECU_001", "vehicle/ECU_001/telemetry",
                "vehicle/ECU_001/commands", 1}, connected_(connected) {}

  bool connect() override {
    connected_ = true;
    return true;
  }

  bool disconnect() override {
    connected_ = false;
    return true;
  }

  bool reconnect() override {
    connected_ = true;
    return true;
  }

  bool publish(std::string_view topic, std::string_view payload, int) override {
    published_topics_.push_back(std::string(topic));
    published_payloads_.push_back(std::string(payload));
    return connected_;
  }

  bool subscribe(std::string_view, int) override {
    return connected_;
  }

  bool isConnected() const override {
    return connected_;
  }

  const MqttConfig& config() const override {
    return config_;
  }

  std::vector<std::string> published_topics_;
  std::vector<std::string> published_payloads_;

 private:
  MqttConfig config_;
  bool connected_;
};

}  // namespace

TEST(AuthClientTests, SuccessfulAuthentication) {
  auto backend = std::make_shared<FakeAuthBackend>(true);
  AuthClient client(backend, "ECU_001", "secret123");

  EXPECT_TRUE(client.authenticate());
  EXPECT_EQ(client.token(), "jwt-token");
  EXPECT_TRUE(client.isTokenValid());
}

TEST(AuthClientTests, InvalidCredentials) {
  auto backend = std::make_shared<FakeAuthBackend>(false);
  AuthClient client(backend, "ECU_001", "wrong-secret");

  EXPECT_FALSE(client.authenticate());
  EXPECT_TRUE(client.token().empty());
}

TEST(AuthClientTests, TokenExpiry) {
  auto backend = std::make_shared<FakeAuthBackend>(true, std::chrono::seconds{1});
  AuthClient client(backend, "ECU_001", "secret123");

  EXPECT_TRUE(client.authenticate());
  std::this_thread::sleep_for(std::chrono::seconds{2});
  EXPECT_FALSE(client.isTokenValid());
}

TEST(MqttClientTests, ConnectionHandling) {
  auto backend = std::make_shared<FakeMqttBackend>();
  MqttClient client({"ssl://localhost:8883", "ECU_001", "vehicle/ECU_001/telemetry",
                     "vehicle/ECU_001/commands", 1}, backend);

  EXPECT_FALSE(client.isConnected());
  EXPECT_TRUE(client.connect());
  EXPECT_TRUE(client.isConnected());
}

TEST(MqttClientTests, PublishFailureWhenOffline) {
  auto backend = std::make_shared<FakeMqttBackend>();
  backend->publish_result_ = false;
  MqttClient client({"ssl://localhost:8883", "ECU_001", "vehicle/ECU_001/telemetry",
                     "vehicle/ECU_001/commands", 1}, backend);
  EXPECT_TRUE(client.connect());
  EXPECT_FALSE(client.publish("vehicle/ECU_001/telemetry", "payload", 1));
}

TEST(MqttClientTests, ReconnectLogic) {
  auto backend = std::make_shared<FakeMqttBackend>();
  MqttClient client({"ssl://localhost:8883", "ECU_001", "vehicle/ECU_001/telemetry",
                     "vehicle/ECU_001/commands", 1}, backend);
  EXPECT_TRUE(client.connect());
  EXPECT_TRUE(client.disconnect());
  EXPECT_TRUE(client.reconnect());
}

TEST(TelemetryManagerTests, JsonGenerationAndPublishing) {
  auto mqtt = std::make_shared<FakeMqttClient>();
  TelemetryManager manager("ECU_001", mqtt);

  auto sample = manager.generateSample();
  EXPECT_NE(sample.serialize().find("\"vehicle_id\":\"ECU_001\""), std::string::npos);

  EXPECT_TRUE(manager.publishSample());
  EXPECT_EQ(mqtt->published_topics_.size(), 1u);
}

TEST(TelemetryManagerTests, QueueHandlingWhenOffline) {
  auto mqtt = std::make_shared<FakeMqttClient>(false);
  TelemetryManager manager("ECU_001", mqtt);

  EXPECT_TRUE(manager.publishSample());
  EXPECT_EQ(manager.pendingCount(), 1u);

  mqtt->connect();
  EXPECT_TRUE(manager.flushQueue());
  EXPECT_EQ(manager.pendingCount(), 0u);
}

}  // namespace cvp
