#ifndef  SYNC_TRANSACTION_HPP
# define SYNC_TRANSACTION_HPP

# include <crails/datatree.hpp>
# include <crails/shared_vars.hpp>
# include <crails/renderer.hpp>
# include <crails/logger.hpp>
# include <boost/lexical_cast.hpp>
# include "channel.hpp"

namespace Crails
{
  namespace Sync
  {
    struct IUpdate
    {
      virtual void               render(Data)       = 0;
      virtual const std::string& scope()      const = 0;
      virtual std::string        uid()        const = 0;
    };

    template<typename T>
    struct Update : public IUpdate
    {
      T object;

      const std::string& scope() const { return T::scope; }

      std::string uid() const
      {
        return scope() + '.' + boost::lexical_cast<std::string>(object.get_id());
      }

      void render(Data data)
      {
        if (T::view != "")
        {
          DataTree params;
          Crails::RenderString result;
          Crails::SharedVars vars;

          params["headers"]["Accept"] = "application/json";
          vars["model"] = &object;
          try
          {
            Crails::Renderer::render(T::view, params.as_data(), result, vars);
            data[uid()] = result.value();
            return ;
          }
          catch (const Crails::MissingTemplate& error)
          {
            logger << Logger::Error << "Crails::Sync::Update: missing template: " << error.what() << Logger::endl;
          }
        }
        object.merge_data(data[uid()]);
      }
    };

    class Transaction
    {
      typedef std::list<std::shared_ptr<IUpdate> > UpdateList;
    public:
      void     set_channel(Channel& value) { channel = &value; }
      Channel* get_channel() const { return channel; }

      static bool is_enabled();
      static std::function<void (DataTree&)> on_commit;

      void commit();
      void rollback();

      template<typename MODEL>
      void save(MODEL& model)
      {
        if (is_enabled())
        {
          auto update = std::make_shared<Update<MODEL> >();

          update->object = model;
          updates.push_back(update);
        }
      }

      template<typename MODEL>
      void destroy(MODEL& model)
      {
        if (is_enabled())
        {
          auto update = std::make_shared<Update<MODEL> >();

          update->object = model;
          removals.push_back(update);
        }
      }

    private:
      Channel* channel = nullptr;
      UpdateList updates, removals;
    };
  }
}

#endif
