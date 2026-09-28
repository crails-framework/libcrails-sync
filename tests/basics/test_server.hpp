#pragma once
#include <crails/server.hpp>
#include <crails/context.hpp>
#include <crails/websocket.hpp>
#include <memory>

struct TestServer : public Crails::Server
{
  SINGLETON_IMPLEMENTATION(TestServer, Crails::Server)
public:
  TestServer()
  {
    set_environment(Crails::Test);
  }
};

struct TestContext : public Crails::Context
{
  TestContext(const TestServer& server, Crails::Connection& connection)
    : Crails::Context(server, connection)
  {
  }
};

struct TestListener : public Crails::WebSocket
{
  TestListener(Crails::Context& context) : Crails::WebSocket(context) {}
};

// Each listener needs a connection of its own.
inline std::shared_ptr<TestListener> make_test_listener(const TestServer& server)
{
  Crails::HttpRequest request;

  request.method(Crails::HttpVerb::get);
  request.target("/channel");
  auto connection = std::make_shared<Crails::Connection>(server, request);
  auto context    = std::make_shared<TestContext>(server, *connection);
  return std::make_shared<TestListener>(*context);
}

inline std::shared_ptr<TestContext> make_test_context(const TestServer& server, const std::string& target)
{
  Crails::HttpRequest request;

  request.method(Crails::HttpVerb::get);
  request.target(target);
  auto connection = std::make_shared<Crails::Connection>(server, request);
  return std::make_shared<TestContext>(server, *connection);
}
