#ifndef  CRAILS_SYNC_CHANNEL_HPP
# define CRAILS_SYNC_CHANNEL_HPP

# include <crails/websocket.hpp>
# include <crails/utils/singleton.hpp>

namespace Crails
{
  namespace Sync
  {
    class Channel
    {
      typedef Crails::WebSocket Listener;
    public:
      enum ClientMode { ReadMode, WriteMode, ReadWriteMode };

      std::mutex& mutex() { return object_mutex; }
      void add_listener(Listener& listener);
      void remove_listener(Listener& listener);
      void broadcast(const std::string& message);
      void set_password(const std::string& password, ClientMode mode);
      bool can_read(const std::string& password) const { return read_password == password; }
      bool can_write(const std::string& password) const { return write_password == password; }
      std::size_t count() const;

    private:
      std::mutex object_mutex;
      std::list<std::shared_ptr<Listener>> listeners;
      std::string read_password, write_password;
    };

    struct ChannelHandle
    {
      ChannelHandle(Channel& channel) : channel(channel) { channel.mutex().lock(); }
      ChannelHandle(const ChannelHandle& copy) : channel(copy.channel) { copy.owner = false; }
      ~ChannelHandle() { if (owner) channel.mutex().unlock(); }
      Channel* operator->() { return &channel; }
    private:
      Channel& channel;
      mutable bool owner = true;
    };

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
