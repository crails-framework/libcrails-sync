#ifndef  CRAILS_SYNC_CHANNELS_HPP
# define CRAILS_SYNC_CHANNELS_HPP

# include <crails/utils/singleton.hpp>
# include "channel.hpp"

namespace Crails
{
  namespace Sync
  {
    class Channels
    {
      SINGLETON(Channels)
    public:
      ~Channels();

      Channel& require_unlocked_channel(const std::string& key);
      ChannelHandle require_channel(const std::string& key) { return require_unlocked_channel(key); }
      void broadcast(const std::string& key, const std::string& message);
      void cleanup();
      void cleanup(const std::string& key);

    private:
      std::mutex channels_mutex;
      std::map<std::string, Channel*> channels;
    };
  }
}

#endif
