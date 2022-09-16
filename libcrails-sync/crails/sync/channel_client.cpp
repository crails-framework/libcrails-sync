#include "channel_client.hpp"
#include <crails/context.hpp>

using namespace Crails::Sync;
using namespace std;

bool ChannelListener::acceptable(Crails::Context& context, ChannelHandle& channel)
{
  return channel->can_read(context.params["password"].defaults_to<string>(""));
}

bool ChannelClient::acceptable(Crails::Context& context, ChannelHandle& channel)
{
  return channel->can_write(context.params["password"].defaults_to<string>(""));
}

void ChannelListener::disconnected() const
{
  auto* channels = Channels::singleton::get();

  channel()->remove_listener(const_cast<ChannelListener&>(*this));
  channels->cleanup(channel_name);
}

void ChannelClient::received(const string& message, MessageType type)
{
  if (type == TextMessage)
    channel()->broadcast(message);
}
