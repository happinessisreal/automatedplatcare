#include "plant_config.h"
#include "sensors.h"
#include "state.h"
#include "tasks.h"

void lightTask(void*) {
  for (;;) {
    int pct = lightPercent(lightRaw());
    stateUpdate([&](State& s) { s.light = pct; });
    vTaskDelay(pdMS_TO_TICKS(LIGHT_PERIOD_MS));
  }
}
