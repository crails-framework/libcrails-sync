#ifndef  CRAILS_SYNC_CHANNEL_ACTIONS_HPP
# define CRAILS_SYNC_CHANNEL_ACTIONS_HPP

# include "channel_client.hpp"
# include <crails/context.hpp>
# include <crails/logger.hpp>

# define match_sync_channel(path, websocket) \
  match("GET", path, [](Crails::Context& context, std::function<void()> callback) \
  { Crails::Sync::ChannelRoute<websocket>::trigger(context, callback); })

# define match_sync_task(path) \
  match("POST", path, [](Crails::Context& context, std::function<void()> callback) \
  { Crails::Sync::BroadcastRoute::trigger(context, callback); }) \
 .match_sync_channel(path, Crails::Sync::ChannelListener)

namespace Crails
{
  namespace Sync
  {
    template<typename WEBSOCKET = ChannelListener>
    class ChannelRoute
    {
      typedef WEBSOCKET WebSocketType;
    public:
      static void trigger(Crails::Context& context, std::function<void()> callback)
      {
        const HttpRequest& request      = context.connection->get_request();
        const std::string  channel_name = context.params["uri"];
        ChannelHandle      channel(Channels::singleton::get()->require_unlocked_channel(channel_name));

        if (boost::beast::websocket::is_upgrade(request) && WebSocketType::acceptable(context, channel))
        {
          auto  websocket = std::make_shared<WebSocketType>(context, channel_name);

          context.response.set_status_code(HttpStatus::switching_protocols);
          websocket->accept(request);
          websocket->read();
          channel->add_listener(*websocket);
        }
        else
          context.response.set_status_code(HttpStatus::bad_request);
        callback();
      }
    };

    class BroadcastRoute
    {
    public:
      static void trigger(Crails::Context& context, std::function<void()> callback)
      {
        auto self = context.shared_from_this();

        context.connection->get_body([self, &context, callback](std::string_view body)
        {
          const std::string channel_name = context.params["uri"];
          Channels*         channels = Channels::singleton::get();
          bool              has_listeners;

          has_listeners = try_to_broadcast(context, channels, channel_name, body);
          context.response.send();
          callback();
          if (!has_listeners)
            channels->cleanup(channel_name);
        });
      }

    private:
      static bool try_to_broadcast(Crails::Context& context, Channels* channels, const std::string& channel_name, std::string_view body)
      {
        ChannelHandle channel(channels->require_unlocked_channel(channel_name));
        std::size_t   listener_count = channel->count();

        if (ChannelClient::acceptable(context, channel) && listener_count)
        {
          context.response.set_status_code(Crails::HttpStatus::ok);
          channel->broadcast(body.data());
        }
        else
        {
          logger << "Crails::Sync::BroadcastRoute: rejecting broadcast attempt on channel "
                 << channel_name << " (" << listener_count << " listeners)" << Logger::endl;
          context.response.set_status_code(Crails::HttpStatus::forbidden);
        }
        return listener_count != 0;
      }
    };
  }
}

#endif
