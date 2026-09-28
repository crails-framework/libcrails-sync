#ifndef  CRAILS_SYNC_CHANNEL_HPP
# define CRAILS_SYNC_CHANNEL_HPP

# include <crails/websocket.hpp>
# include <memory>
# include <mutex>
# include <list>

namespace Crails
{
  namespace Sync
  {
    class Channel : public std::enable_shared_from_this<Channel>
    {
      typedef Crails::WebSocket Listener;
    public:
      enum ClientMode { ReadMode, WriteMode, ReadWriteMode };

      Channel(const std::string& name) : name(name) {}

      std::mutex& mutex() { return object_mutex; }
      void add_listener(Listener& listener);
      void remove_listener(Listener& listener);
      void broadcast(const std::string& message);
      void set_password(const std::string& password, ClientMode mode);
      bool can_read(const std::string& password) const { return read_password == password; }
      bool can_write(const std::string& password) const { return write_password == password; }
      std::size_t count() const;

    private:
      const std::string name;
      std::mutex object_mutex;
      std::list<std::shared_ptr<Listener>> listeners;
      std::string read_password, write_password;
    };

    // Locks the channel for as long as the handle lives, and keeps the channel
    // itself alive for as long as the handle lives.
    struct ChannelHandle
    {
      ChannelHandle(std::shared_ptr<Channel> value) : channel(std::move(value)) { channel->mutex().lock(); }
      ChannelHandle(Channel& value) : ChannelHandle(value.shared_from_this()) {}
      ChannelHandle(const ChannelHandle& copy) : channel(copy.channel) { copy.owner = false; }
      ~ChannelHandle() { if (owner) channel->mutex().unlock(); } // unlock first, release the reference after
      Channel* operator->() { return channel.get(); }
    private:
      std::shared_ptr<Channel> channel;
      mutable bool owner = true;
    };
  }
}

#endif
