#include "SqlUtils.h"

using organicdump_proto::PeripheralType;

namespace organicdump
{

bool ToTableName(PeripheralType type, const char **out_name) {
  switch (type) {
    case PeripheralType::SOIL_MOISTURE_SENSOR:
      *out_name = SOIL_MOISTURE_SENSOR_TABLE;
      return true;
    default:
      *out_name = nullptr;
      return false;
  }
}

} // namespace organicdump
