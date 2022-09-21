#ifndef  CRAILS_SYNC_STREAM_HPP
# define CRAILS_SYNC_STREAM_HPP

# include <sstream>
# include <crails/sync/task.hpp>
# include <iostream>

namespace Crails
{
  namespace Sync
  {
    class Stream
    {
      std::stringstream stream;
    public:
      Sync::Task*       task   = nullptr;
      std::ostream*     output = nullptr;

      Stream(Sync::Task& task) : task(&task)
      {}

      Stream(std::ostream& output) : output(&output)
      {}

      Stream()
      {
        stream.setstate(std::ios_base::badbit);
      }

      void add_output(Sync::Task& value) { task = &value; }
      void add_output(std::ostream& value) { output = &value; }

      template<typename T>
      Stream& operator<<(const T& v)
      {
        stream << v;
        if (stream.str().find('\n') != std::string::npos)
          on_new_line();
        return *this;
      }

      Stream& put(char c)
      {
        stream.put(c);
        if (c == '\n')
          on_new_line();
        return *this;
      }

    private:
      void on_new_line()
      {
        if (task)
          task->notify(stream.str());
        if (output)
          *output << stream.str();
        stream.str(std::string());
      }
    };
  }
}

#endif

