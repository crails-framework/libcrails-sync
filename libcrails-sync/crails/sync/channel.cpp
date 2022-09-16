#include "channel.hpp"

using namespace Crails::Sync;
using namespace std;

/*
 * Channels
 */
Channels::~Channels()
{
  for (auto entry : channels)
    delete entry.second;
}

Channel& Channels::require_unlocked_channel(const string& key)
{
  lock_guard<mutex> mutex_lock(channels_mutex);
  auto it = channels.find(key);

  if (it == channels.end())
    it = channels.emplace(key, new Channel).first;
  return *(it->second);
}

void Channels::broadcast(const string& key, const string& message)
{
  require_channel(key)->broadcast(message);
}

static std::map<std::string, Channel*>::iterator cleanup_channel_if_empty(std::map<std::string, Channel*>& channels, std::map<std::string, Channel*>::iterator it)
{
  ChannelHandle handle(*it->second);

  if (handle->count() == 0)
  {
    delete it->second;
    return channels.erase(it);
  }
  return ++it;
}

void Channels::cleanup()
{
  lock_guard<mutex> mutex_lock(channels_mutex);

  for (auto it = channels.begin() ; it != channels.end();)
    it = cleanup_channel_if_empty(channels, it);
}

void Channels::cleanup(const std::string& key)
{
  lock_guard<mutex> mutex_lock(channels_mutex);
  auto it = channels.find(key);

  if (it != channels.end())
    cleanup_channel_if_empty(channels, it);
}

/*
 * Channel
 */
void Channel::add_listener(Listener& listener)
{
  listeners.push_back(listener.shared_from_this());
}

void Channel::remove_listener(Listener& listener)
{
  auto it = find(listeners.begin(), listeners.end(), listener.shared_from_this());

  if (it != listeners.end()) listeners.erase(it);
}

void Channel::broadcast(const string& message)
{
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
