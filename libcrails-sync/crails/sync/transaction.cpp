#include "transaction.hpp"

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
    channel->broadcast(message.to_json());
    rollback();
  }
}

void Transaction::rollback()
{
  updates.clear();
  removals.clear();
}
