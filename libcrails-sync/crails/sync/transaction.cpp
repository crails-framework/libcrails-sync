#include "transaction.hpp"
#include <crails/logger.hpp>

using namespace Crails;
using namespace Crails::Sync;
using namespace std;

namespace Crails
{
  namespace Sync
  {
    bool enabled = true;
    function<void (DataTree&)> Transaction::on_commit;
  }
}

bool Transaction::is_enabled()
{
  return Sync::enabled;
}

void Transaction::commit()
{
  if (channel && updates.size() + removals.size() > 0)
  {
    DataTree message;
    vector<string> removal_uids;

    for (auto update : updates)
      update->render(message["updates"]);
    for (auto removal : removals)
      removal_uids.push_back(removal->uid());
    message["removals"].from_vector<string>(removal_uids);
    if (on_commit)
      on_commit(message);
    {
      lock_guard<mutex> lock(channel->mutex());

      channel->broadcast(message.to_json());
    }
    rollback();
    logger << Logger::Debug << "Crails::Sync::Transaction::commit: successfully broadcasted sync udpate" << Logger::endl;
  }
  else if (!channel)
    logger << Logger::Debug << "Crails::Sync::Transaction::commit: no channel specified" << Logger::endl;
  else
    logger << Logger::Debug << "Crails::Sync::Transaction::commit: nothing to commit" << Logger::endl;
}

void Transaction::rollback()
{
  updates.clear();
  removals.clear();
}
