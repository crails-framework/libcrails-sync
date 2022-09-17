#ifndef  CRAILS_SYNC_CHANNEL_CLIENT_HPP
# define CRAILS_SYNC_CHANNEL_CLIENT_HPP

# include "channels.hpp"

namespace Crails
{
  namespace Sync
  {
    class ChannelListener : public Crails::WebSocket
    {
      const std::string channel_name;
    public:
      ChannelListener(Crails::Context& context, const std::string& channel_name) : Crails::WebSocket(context), channel_name(channel_name) {}

      static bool   acceptable(Crails::Context& context, ChannelHandle& channel);
      ChannelHandle channel() const { return Channels::singleton::get()->require_unlocked_channel(channel_name); }
      virtual void  disconnected() const override;
    };

    class ChannelClient : public ChannelListener
    {
    public:
      ChannelClient(Crails::Context& context, const std::string& channel_name) : ChannelListener(context, channel_name) {}

      static bool  acceptable(Crails::Context& context, ChannelHandle& channel);
      virtual void received(const std::string& message, MessageType type) override;
    };
  }
}

#endif
