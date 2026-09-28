#include "channels.hpp"
#include <crails/logger.hpp>

using namespace Crails::Sync;
using namespace std;

typedef std::map<std::string, std::shared_ptr<Channel>> ChannelMap;

/*
 * Channels
 */
Channels::~Channels()
{
}

shared_ptr<Channel> Channels::require_unlocked_channel(const string& key)
{
  lock_guard<mutex> mutex_lock(channels_mutex);
  auto it = channels.find(key);

  if (it == channels.end())
    it = channels.emplace(key, make_shared<Channel>(key)).first;
  return it->second;
}

void Channels::broadcast(const string& key, const string& message)
{
  require_channel(key)->broadcast(message);
}

// Must be called with channels_mutex held.
static ChannelMap::iterator cleanup_channel_if_empty(ChannelMap& channels, ChannelMap::iterator it)
{
  if (it->second.use_count() == 1)
  {
    bool empty;

    {
      // Locked to synchronize with the last user's writes.
      lock_guard<mutex> channel_lock(it->second->mutex());
      empty = it->second->count() == 0;
    }
    if (empty)
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
