#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <thread>

#include <gtest/gtest.h>

#include "connected_vehicle_platform/someip_client.hpp"

namespace {

void runTestServer(unsigned short port, bool* ready, bool* stop_flag) {
  const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_NE(server_fd, -1);

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(port);

  ASSERT_EQ(bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
  ASSERT_EQ(listen(server_fd, 1), 0);

  *ready = true;
  int client_fd = accept(server_fd, nullptr, nullptr);
  ASSERT_NE(client_fd, -1);

  char buffer[256]{};
  const auto received = recv(client_fd, buffer, sizeof(buffer), 0);
  EXPECT_GT(received, 0);

  close(client_fd);
  close(server_fd);
  *stop_flag = true;
}

}  // namespace

TEST(SomeipClientTests, ConnectAndPublish) {
  constexpr unsigned short kPort = 41001;
  bool ready = false;
  bool stop_flag = false;

  std::thread server([&] { runTestServer(kPort, &ready, &stop_flag); });

  while (!ready) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  cvp::SomeipConfig config{"127.0.0.1", kPort, 0x1234, 0x0001};
  cvp::SomeipClient client(config);

  EXPECT_TRUE(client.connect());
  EXPECT_TRUE(client.publish("vehicle-payload"));
  EXPECT_TRUE(client.disconnect());

  server.join();
  EXPECT_TRUE(stop_flag);
}
