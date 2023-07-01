#ifndef  CRAILS_SYNC_TASK_HPP
# define CRAILS_SYNC_TASK_HPP

# include <string>
# include <crails/datatree.hpp>

namespace Crails
{
  namespace Sync
  {
    class Task
    {
      std::string  scope, id;
      unsigned int last_notification = 0;
      unsigned int task_progress     = 0;
      unsigned int task_count        = 1;
      unsigned int notification_step_perc = 5;
      bool         mute = false;
    public:
      DataTree     metadata;

      struct Settings
      {
        static const std::string    hostname;
        static const unsigned short port;
        static const bool           ssl;
      };

      Task(unsigned int task_count);
      Task(const std::string& scope, unsigned int task_count);
      Task(const std::string& scope, const std::string& id, unsigned int task_count);
      Task(const Task&) = delete;
      ~Task();

      std::string uri() const { return scope + '/' + id; }
      const std::string& get_scope() const { return scope; }
      const std::string& get_id() const { return id; }
      bool is_mute() const { return mute; }
      void set_mute(bool value) { mute = value; }
      void set_task_count(unsigned int);
      void set_notification_step(unsigned int value) { notification_step_perc = value; }
      unsigned int notification_step() const;
      void increment(unsigned int progress = 1);
      void notify();
      void notify(const std::string& message);
    };
  }
}

#endif
