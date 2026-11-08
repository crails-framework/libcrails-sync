#include "remote.hpp"
#include "task.hpp"
#include <crails/client.hpp>
#include <crails/logger.hpp>

using namespace Crails;
using namespace std;

bool Sync::post_to_server(const string& path, const string& json_body)
{
  try
  {
    ClientInterface::Request request{HttpVerb::post, path, 11};
    shared_ptr<ClientInterface> http;

    request.set(HttpHeader::content_type, "application/json");
    request.set(HttpHeader::connection, "close");
    request.set(password_header, Task::Settings::password);
    request.body() = json_body;
    request.content_length(json_body.length());
    if (Task::Settings::ssl)
      http = Ssl::Client::create(Task::Settings::hostname, Task::Settings::port);
    else
      http = Client::create(Task::Settings::hostname, Task::Settings::port);
    http->connect();
    auto response = http->query(request);
    http->disconnect();
    if (response.result() != HttpStatus::ok)
    {
      logger << Logger::Error << "Crails::Sync: server rejected POST " << path
             << " (status " << response.result_int() << ')' << Logger::endl;
      return false;
    }
    return true;
  }
  catch (const std::exception& error)
  {
    logger << Logger::Error << "Crails::Sync: failed to POST " << path << " to "
           << Task::Settings::hostname << ':' << Task::Settings::port << ": " << error.what() << Logger::endl;
  }
  return false;
}
