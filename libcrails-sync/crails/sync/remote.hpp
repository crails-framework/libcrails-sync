#ifndef  CRAILS_SYNC_REMOTE_HPP
# define CRAILS_SYNC_REMOTE_HPP

# include <string>

namespace Crails
{
  namespace Sync
  {
    // Header carrying Task::Settings::password in requests sent to the server.
    constexpr const char* password_header = "X-Sync-Password";

    // Blocking POST of a JSON body to the application server, as configured by
    // Task::Settings (hostname, port, ssl, password). Used by processes other
    // than the server (tasks, sidekix) to have the server broadcast for them.
    // Returns false (after logging the reason) if the server did not accept it.
    bool post_to_server(const std::string& path, const std::string& json_body);
  }
}

#endif
