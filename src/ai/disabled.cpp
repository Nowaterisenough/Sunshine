/**
 * Optional AI boundary for the native panel distribution.
 * Keeps the streaming host's call sites unchanged and its existing ABR fallback active.
 * Does not read credentials, create worker threads, or perform network requests.
 */
#include "http.h"
#include "http_stream.h"
#include "proxy.h"
#include "src/nvhttp/ai_api.h"

namespace ai {
  bool isAiEnabled() {
    return false;
  }

  AiProxyResult processAiChat(const std::string &) {
    return {404, R"({"error":{"message":"AI support is not included in this build"}})", "application/json"};
  }

  AiProxyResult processAiChatStream(const std::string &body,
    std::function<bool(const char *, size_t)>, std::function<bool()>) {
    return processAiChat(body);
  }
}

namespace ai::http {
  void register_routes(confighttp::https_server_t &) {}
  void shutdown_streams() {}
}

namespace nvhttp::ai_api {
  void completions(resp_https_t response, req_https_t) {
    response->write(web::StatusCode::not_found);
  }
}
