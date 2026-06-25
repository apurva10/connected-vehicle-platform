#include <chrono>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "connected_vehicle_platform/auth_client.hpp"
#include "connected_vehicle_platform/connectivity_service.hpp"
#include "connected_vehicle_platform/logger.hpp"
#include "connected_vehicle_platform/mqtt_client.hpp"
#include "connected_vehicle_platform/telemetry_manager.hpp"

namespace cvp {
namespace {

class DynamicAuthBackend : public IAuthBackend {
 public:
  explicit DynamicAuthBackend(bool success, std::chrono::seconds ttl = std::chrono::seconds{3600})
      : success_(success), ttl_(ttl) {}

  AuthResponse authenticate(const AuthRequest&) override {
    std::scoped_lock lock(mutex_);
    authenticate_calls_++;
    if (!success_) {
      return {};
    }
    return {token_, ttl_};
  }

  void setSuccess(bool success) {
    std::scoped_lock lock(mutex_);
    success_ = success;
  }

  void setToken(std::string token) {
    std::scoped_lock lock(mutex_);
    token_ = std::move(token);
  }

  int getCalls() const {
    std::scoped_lock lock(mutex_);
    return authenticate_calls_;
  }

 private:
  mutable std::mutex mutex_;
  bool success_{true};
  std::string token_{"jwt-token"};
  std::chrono::seconds ttl_;
  int authenticate_calls_{0};
};

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

TEST(ConnectivityServiceTests, TokenRetryLogic) {
  auto backend = std::make_shared<DynamicAuthBackend>(true, std::chrono::seconds{3600});
  auto auth_client = std::make_shared<AuthClient>(backend, "ECU_001", "secret123");
  auto mqtt = std::make_shared<FakeMqttClient>();
  auto logger = std::make_shared<Logger>("test_run.log");
  auto telemetry = std::make_shared<TelemetryManager>("ECU_001", mqtt);

  ConnectivityService service(auth_client, mqtt, telemetry, logger);

  // 1. Initial start should succeed
  EXPECT_TRUE(service.start());
  EXPECT_EQ(service.state(), ServiceState::Publishing);
  EXPECT_EQ(backend->getCalls(), 1);

  // 2. Clear token to make it invalid/empty
  auth_client->clearToken();
  EXPECT_TRUE(auth_client->token().empty());
  EXPECT_FALSE(auth_client->isTokenValid());

  // 3. Make authentication backend fail initially when refreshing
  backend->setSuccess(false);

  // 4. Wait for runLoop to notice invalid token and attempt refresh
  // Since retry interval is 2 seconds, we will wait 3 seconds.
  // The first refresh attempt will fail, and it should retry.
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // The state should be Disconnected or Authenticating depending on timing,
  // but most importantly it should have tried to authenticate again.
  EXPECT_GT(backend->getCalls(), 1);

  // 5. Now make authentication succeed again
  backend->setSuccess(true);
  backend->setToken("new-jwt-token");

  // 6. Wait for another retry which should now succeed
  std::this_thread::sleep_for(std::chrono::seconds(3));

  // The token should be successfully updated and service back to publishing
  EXPECT_EQ(auth_client->token(), "new-jwt-token");
  EXPECT_TRUE(auth_client->isTokenValid());
  EXPECT_EQ(service.state(), ServiceState::Publishing);

  // 7. Stop service
  EXPECT_TRUE(service.stop());
}

}  // namespace cvp
