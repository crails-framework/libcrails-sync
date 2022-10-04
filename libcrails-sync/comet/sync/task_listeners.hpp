#ifndef  CRAILS_SYNC_TASK_LISTENERS_HPP
# define CRAILS_SYNC_TASK_LISTENERS_HPP

# include "task_listener.hpp"
# include <map>

namespace Comet
{
  namespace Sync
  {
    template<typename TASK_LISTENER = Comet::Sync::TaskListener>
    class TaskListeners : public std::map<std::string, std::shared_ptr<TASK_LISTENER>>
    {
      typedef std::map<std::string, std::shared_ptr<TASK_LISTENER>> Super;
      std::string scope;
    public:
      Signal<typename TaskListeners::iterator> added;
      Signal<typename TaskListeners::iterator> removed;

      TaskListeners(const std::string& scope) : scope(scope)
      {
      }

      const std::string& get_scope() const { return scope; }

      std::shared_ptr<TASK_LISTENER> add_task(const std::string& id)
      {
        auto existing_task = Super::find(id);

        if (existing_task == Super::end())
        {
          auto task   = std::make_shared<TASK_LISTENER>(scope, id);
          auto result = Super::emplace(id, task);
          auto finished_callback = std::bind(&TaskListeners::erase, this, result.first);

          added.trigger(result.first);
          task->finished.connect(finished_callback);
          task->aborted.connect(finished_callback);
          return task;
        }
        return existing_task->second;
      }

      typename TaskListeners::iterator erase(typename TaskListeners::iterator iterator)
      {
        if (Super::find(iterator->first) != Super::end())
        {
          removed.trigger(iterator);
          return Super::erase(iterator);
        }
        return Super::end();
      }
    };
  }
}

#endif
