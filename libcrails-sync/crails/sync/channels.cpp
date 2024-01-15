#include "channels.hpp"
#include <crails/logger.hpp>

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
    it = channels.emplace(key, new Channel(key)).first;
  return *(it->second);
}

void Channels::broadcast(const string& key, const string& message)
{
  require_channel(key)->broadcast(message);
}

static std::map<std::string, Channel*>::iterator cleanup_channel_if_empty(std::map<std::string, Channel*>& channels, std::map<std::string, Channel*>::iterator it)
{
  int count;

  {
    ChannelHandle handle(*it->second);
    count = handle->count();
  }
  // As channels_mutex should be locked when this function is run,
  // it shouldn't be possible for the channel's own mutex to be locked
  // from anywhere else, hence making it safe to delete the mutex:
  if (count == 0)
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
