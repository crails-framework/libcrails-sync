#include "task.hpp"
#include "channels.hpp"
#include <crails/utils/random_string.hpp>
#include <crails/client.hpp>
#include <crails/logger.hpp>

using namespace Crails;
using namespace Crails::Sync;
using namespace std;

static std::string random_task_id() { return Crails::generate_random_string("abcdefghijklmnopqrstuvwxyz0123456789", 8); }

template<typename CLIENT_TYPE>
static void broadcast(const Crails::Client::Request& request)
{
  try
  {
    auto http = make_shared<CLIENT_TYPE>(Task::Settings::hostname, Task::Settings::port);

    http->connect();
    http->async_query(request, [http](const Crails::Client::Response&, boost::beast::error_code ec)
    {
      if (ec)
        logger << Logger::Error << "Crails::Sync::Task: an error occured during a broadcast: " << ec.message() << Logger::endl;
      else
        logger << Logger::Debug << "Crails::Sync::Task: broadcasted progress update" << Logger::endl;
    });
  }
  catch (const std::exception& error)
  {
    logger << Logger::Error << "Crails::Sync::Task: an error occured during a broadcast to " << Task::Settings::hostname << ':' << Task::Settings::port << ": " << error.what() << Logger::endl;
  }
}

static void broadcast(Task& task)
{
  if (!task.is_mute())
  {
    Crails::Client::Request request{Crails::HttpVerb::post, '/' + task.uri(), 11};
    const string body = task.metadata.to_json();

    logger << Logger::Debug << "Crails::Sync::Task: notify on " << task.get_id() << ":\n" << body << Logger::endl;
    request.set(Crails::HttpHeader::content_type, "application/json");
    request.set(Crails::HttpHeader::connection, "close");
    request.body() = body;
    request.content_length(body.length());
    if (Task::Settings::ssl)
      broadcast<Crails::Ssl::Client>(request);
    else
      broadcast<Crails::Client>(request);
  }
}

Task::Task(unsigned int task_count) : task_count(task_count)
{
  scope = "tasks";
  id = random_task_id();
  metadata["id"] = id;
}

Task::Task(const std::string& scope, unsigned int task_count) : scope(scope), task_count(task_count)
{
  id = random_task_id();
  metadata["id"] = id;
}

Task::Task(const std::string& scope, const std::string& id, unsigned int task_count) : scope(scope), id(id), task_count(task_count)
{
  metadata["id"] = id;
}

Task::~Task()
{
  if (task_count > task_progress)
  {
    metadata["status"] = "abort";
    broadcast(*this);
  }
}

unsigned int Task::notification_step() const
{
  return std::ceil(notification_step_perc / 100.f * task_count);
}

void Task::increment(unsigned int progress)
{
  task_progress += progress;
  if (task_progress - last_notification >= notification_step()
   || task_progress >= task_count)
    notify();
}

void Task::notify(const std::string& message)
{
  metadata["message"] = message;
  notify();
  metadata["message"].destroy();
}

void Task::notify()
{
  float progress = task_progress;
  progress /= task_count;
  if (progress > 1)
    progress = 1;
  metadata["progress"]      = progress;
  metadata["item_count"]    = task_count;
  metadata["item_progress"] = task_progress;
  last_notification = task_progress;
  broadcast(*this);
}

void Task::set_task_count(unsigned int count)
{
  if (count != task_count)
  {
    task_count = count;
    notify();
  }
}
