#ifndef  CRAILS_SYNC_TASK_LISTENER_HPP
# define CRAILS_SYNC_TASK_LISTENER_HPP

# include <comet/websocket.hpp>
# include <comet/signal.hpp>
# include <comet/datatree.hpp>
# include <memory>

namespace Comet
{
  namespace Sync
  {
    class TaskListener
    {
      typedef std::unique_ptr<Comet::WebSocket> WebSocketPtr;

      std::string  scope, id;
      float        progress = 0.f;
      unsigned int task_count = 1;
      unsigned int task_progress = 1;
      WebSocketPtr socket;
    public:
      Signal<float> updated;
      Signal<void>  finished;

      TaskListener(const std::string& scope, const std::string& id) : scope(scope), id(id), socket(new Comet::WebSocket(url()))
      {
        socket->message_received.connect(std::bind(&TaskListener::on_update_received, this, std::placeholders::_1));
      }

      std::string url() const
      {
        client::Location* location = client::window.get_location();
        std::string protocol = Comet::String(location->get_protocol());
        std::string host = Comet::String(location->get_host());

        protocol = protocol == "http:" ? "ws:" : "wss:";
        return protocol + '/' + host + '/' + uri();
      }

      std::string        uri() const { return scope + '/' + id; }
      const std::string& get_scope() const { return scope; }
      const std::string& get_id() const { return id; }
      float              get_progress() const { return progress; }
      unsigned int       get_task_count() const { return task_count; }
      unsigned int       get_task_progress() const { return task_progress; }

      void on_update_received(const std::string& message)
      {
        DataTree metadata;

        metadata.from_json(message);
        progress      = metadata["progress"].as<float>();
        task_count    = metadata["item_count"];
        task_progress = metadata["item_progress"];
        updated.trigger(progress);
        if (progress >= 1.f)
          finished.trigger();
      }
    };
  }
}

#endif
