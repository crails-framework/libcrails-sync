#ifndef  ODB_SYNC_CONNECTION_HPP
# define ODB_SYNC_CONNECTION_HPP

# include <crails/odb/connection.hpp>
# include "../sync/transaction.hpp"

namespace Crails
{
  namespace Odb
  {
    namespace Sync
    {
      class Connection : public Odb::Connection
      {
      public:
        Connection() : Odb::Connection(), sync_transaction(Crails::Sync::Transaction::get())
        {}

        virtual ~Connection() {}

        void commit()
        {
          Odb::Connection::commit();
          if (use_sync_transaction)
            sync_transaction.commit();
        }

        void rollback()
        {
          Odb::Connection::rollback();
          if (use_sync_transaction)
            sync_transaction.rollback();
        }

        template<typename MODEL>
        void save(MODEL& model)
        {
          Odb::Connection::save<MODEL>(model);
          if (use_sync_transaction)
            sync_transaction.save(model);
        }

        template<typename MODEL>
        void destroy(MODEL& model)
        {
          Odb::Connection::destroy<MODEL>(model);
          if (use_sync_transaction)
            sync_transaction.destroy(model);
        }

        template<typename MODEL>
        void destroy(odb::query<MODEL> query = odb::query<MODEL>(true))
        {
          odb::result<MODEL> results;

          if (Odb::Connection::find(results, query))
          {
            for (auto& model : results)
              destroy(model);
          }
        }

        bool use_sync_transaction = true;
        Crails::Sync::Transaction& sync_transaction;
      };
    }
  }
}

#endif
