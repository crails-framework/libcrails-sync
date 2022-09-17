#include "channel.hpp"
#include <crails/logger.hpp>

using namespace Crails::Sync;
using namespace std;

void Channel::add_listener(Listener& listener)
{
  Crails::logger << Crails::Logger::Debug << "Channel `" << name << "` has a new listener." << Crails::Logger::endl;
  listeners.push_back(listener.shared_from_this());
}

void Channel::remove_listener(Listener& listener)
{
  auto it = find(listeners.begin(), listeners.end(), listener.shared_from_this());

  if (it != listeners.end()) listeners.erase(it);
}

void Channel::broadcast(const string& message)
{
  Crails::logger << Crails::Logger::Debug << "Channel `" << name << "` broadcasting to " << listeners.size() << " listeners." << Crails::Logger::endl;
  for (auto it = listeners.begin() ; it != listeners.end() ; ++it)
    (*it)->send(message);
}

void Channel::set_password(const string& password, ClientMode mode)
{
  if (mode == ReadMode || mode == ReadWriteMode)
    read_password = password;
  if (mode == WriteMode || mode == ReadWriteMode)
    write_password = password;
}

size_t Channel::count() const
{
  return listeners.size();
}
