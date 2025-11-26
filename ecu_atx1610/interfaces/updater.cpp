#include "../common/logging.hpp"

namespace interfaces {

class Updater {
public:
    void check_for_updates()
    {
        LOG_INFO("Updater", "Verificando actualizaciones OTA simuladas");
    }
};

} // namespace interfaces
