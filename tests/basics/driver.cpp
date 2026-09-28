#include <crails/program_options.hpp>
#include <crails/logger.hpp>
#include <crails/session_store/no_session_store.hpp>
#include <crails/sync/channels.hpp>
#include <crails/sync/channel_actions.hpp>
#include <crails/sync/transaction.hpp>
#include <crails/sync/task.hpp>
#include "test_server.hpp"

#include <atomic>
#include <future>
#include <thread>
#include <vector>

#undef NODEBUG
#include <cassert>

using namespace Crails;
using namespace Crails::Sync;
using namespace std;

const std::string    Sync::Task::Settings::hostname = "0.0.0.0";
const unsigned short Sync::Task::Settings::port = 3001;
const bool           Sync::Task::Settings::ssl = false;

int driver()
{
  SingletonInstantiator<TestServer> server;
  SingletonInstantiator<Crails::NoSessionStore::Factory> store;
  SingletonInstantiator<Channels> channels;

  // A channel name always maps to the same channel
  {
    auto a  = channels->require_unlocked_channel("same-channel");
    auto a2 = channels->require_unlocked_channel("same-channel");
    auto b  = channels->require_unlocked_channel("other-channel");

    assert(a == a2);
    assert(a != b);
  }

  // Channels are created without passwords, and passwords can be set for reading and writing separately
  {
    auto channel = channels->require_unlocked_channel("passwords");

    assert(channel->can_read(""));
    assert(channel->can_write(""));
    channel->set_password("read-secret", Channel::ReadMode);
    assert(channel->can_read("read-secret"));
    assert(!channel->can_read(""));
    assert(channel->can_write(""));
    channel->set_password("both-secret", Channel::ReadWriteMode);
    assert(channel->can_read("both-secret"));
    assert(channel->can_write("both-secret"));
    assert(!channel->can_read("read-secret"));
  }

  // Listeners are counted as they are added and removed
  {
    auto listener = make_test_listener(*server);
    ChannelHandle channel(channels->require_unlocked_channel("listeners"));

    assert(channel->count() == 0);
    channel->add_listener(*listener);
    assert(channel->count() == 1);
    channel->remove_listener(*listener);
    assert(channel->count() == 0);
    channel->remove_listener(*listener); // removing an unknown listener is harmless
    assert(channel->count() == 0);
  }

  // Cleanup removes channels without listeners
  {
    weak_ptr<Channel> weak;

    {
      auto channel = channels->require_unlocked_channel("cleanup-empty");
      weak = channel;
    }
    assert(!weak.expired()); // channels remain registered until they are cleaned up
    channels->cleanup("cleanup-empty");
    assert(weak.expired());
  }

  // Cleanup removes every empty channel when called without a key
  {
    weak_ptr<Channel> weak_a, weak_b;

    {
      auto a = channels->require_unlocked_channel("cleanup-all-a");
      auto b = channels->require_unlocked_channel("cleanup-all-b");
      weak_a = a;
      weak_b = b;
    }
    channels->cleanup();
    assert(weak_a.expired());
    assert(weak_b.expired());
  }

  // Cleanup keeps channels with listeners, and removes them once the last listener is gone
  {
    auto listener = make_test_listener(*server);
    weak_ptr<Channel> weak;

    {
      ChannelHandle channel(channels->require_unlocked_channel("cleanup-listened"));
      channel->add_listener(*listener);
    }
    {
      // Get a weak reference without keeping the channel alive
      auto channel = channels->require_unlocked_channel("cleanup-listened");
      weak = channel;
    }
    channels->cleanup("cleanup-listened");
    channels->cleanup();
    assert(!weak.expired());
    {
      ChannelHandle channel(channels->require_unlocked_channel("cleanup-listened"));
      assert(channel->count() == 1);
      channel->remove_listener(*listener);
    }
    channels->cleanup("cleanup-listened");
    assert(weak.expired());
  }

  // Regression: cleanup must not delete a channel somebody holds a reference to
  // (a request between looking up the channel and locking it used to hit a deleted channel)
  {
    weak_ptr<Channel> weak;
    auto channel = channels->require_unlocked_channel("cleanup-referenced");

    weak = channel;
    channels->cleanup("cleanup-referenced");
    channels->cleanup();
    assert(!weak.expired());
    assert(channel == channels->require_unlocked_channel("cleanup-referenced"));
    channel.reset();
    channels->cleanup("cleanup-referenced");
    assert(weak.expired());
  }

  // Regression: same as above, while another thread has the channel locked through a ChannelHandle
  {
    weak_ptr<Channel> weak;
    promise<void>     locked, release;
    auto              released = release.get_future();
    thread            holder;

    {
      auto channel = channels->require_unlocked_channel("cleanup-locked");
      weak = channel;
      holder = thread([channel, &locked, &released]() mutable
      {
        ChannelHandle handle(std::move(channel));

        locked.set_value();
        released.wait();
      });
    }
    locked.get_future().wait();
    channels->cleanup("cleanup-locked");
    channels->cleanup();
    assert(!weak.expired());
    release.set_value();
    holder.join();
    channels->cleanup();
    assert(weak.expired());
  }

  // BEGIN-TRANSACTION
  // A transaction keeps its channel alive
  {
    weak_ptr<Channel> weak;
    auto& transaction = Transaction::get();

    {
      auto channel = channels->require_unlocked_channel("cleanup-transaction");
      weak = channel;
      transaction.set_channel(channel);
    }
    channels->cleanup();
    assert(!weak.expired());
    assert(transaction.get_channel() == weak.lock().get());
    transaction.set_channel(*channels->require_unlocked_channel("cleanup-transaction-2"));
    channels->cleanup();
    assert(weak.expired());
  }
  // END-TRANSACTION

  // BEGIN-ROUTE
  // A rejected connection attempt does not leave an empty channel behind
  {
    weak_ptr<Channel> weak;
    auto context = make_test_context(*server, "/rejected-channel");

    {
      auto channel = channels->require_unlocked_channel("rejected-channel");
      weak = channel;
    }
    context->params["uri"] = string("rejected-channel");
    ChannelRoute<ChannelListener>::trigger(*context, []() {});
    assert(context->response.get_status_code() == HttpStatus::bad_request); // not a websocket upgrade
    assert(weak.expired());
  }
  // END-ROUTE

  // Concurrent lookups, listener registrations and cleanups
  {
    const unsigned int thread_count = 4;
    const unsigned int iterations   = 5000;
    vector<shared_ptr<TestListener>> listeners;
    vector<thread> threads;
    atomic<bool> stop{false};
    atomic<unsigned int> failures{0};

    for (unsigned int i = 0 ; i < thread_count ; ++i)
      listeners.push_back(make_test_listener(*server));
    for (unsigned int i = 0 ; i < thread_count ; ++i)
    {
      threads.emplace_back([&, i]()
      {
        for (unsigned int j = 0 ; j < iterations ; ++j)
        {
          const string name = "concurrent-" + to_string(j % 3);

          {
            ChannelHandle channel(channels->require_unlocked_channel(name));
            channel->add_listener(*listeners[i]);
          }
          {
            ChannelHandle channel(channels->require_unlocked_channel(name));
            if (channel->count() == 0)
              ++failures; // our own listener is registered, channel can't be empty
            channel->remove_listener(*listeners[i]);
          }
          channels->cleanup(name);
        }
      });
    }
    thread cleaner([&]() { while (!stop) channels->cleanup(); });

    for (auto& t : threads)
      t.join();
    stop = true;
    cleaner.join();
    assert(failures == 0);
    for (unsigned int i = 0 ; i < 3 ; ++i)
    {
      ChannelHandle channel(channels->require_unlocked_channel("concurrent-" + to_string(i)));
      assert(channel->count() == 0);
    }
  }
  return 0;
}

int main()
{
  int status = driver();
  Crails::Server::cleanup();
  return 0;
}
