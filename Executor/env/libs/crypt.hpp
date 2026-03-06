#include "../../deps/server/base64.hpp"

namespace crypt {
    inline std::string handleBase64Encode(const std::string& data, const json& settings, DWORD pid) {
        return base64::to_base64(data);
    }

    inline std::string handleBase64Decode(const std::string& data, const json& settings, DWORD pid) {
        try {
            return base64::from_base64(data);
        } catch (...) {
            return "";
        }
    }

    inline void init() {
        registerBridgeMethod("crypt.base64encode", handleBase64Encode);
        registerBridgeMethod("crypt.base64decode", handleBase64Decode);
    }
}
