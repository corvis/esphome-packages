#include <esphome.h>

namespace JBHeating {
    std::vector<output::OutputSwitch*> valves;
    int valvesSize, stateSize;
    int countdownInterval = 30;

    void init(int interval, const std::vector<output::OutputSwitch*> &v) {
        valves = v;
        valvesSize = valves.size();
        stateSize = sizeof(id(jb_valves_countdown)) / sizeof(int);
        countdownInterval = interval;
    }

    bool isFailureDetected() {
        for (int i = 0; i < stateSize; i++) if (id(jb_valves_countdown)[i] != 0) return true;
        return false;
    }

    void handleCountdown() {
        if (valvesSize > stateSize) {
            ESP_LOGE("jb_irrigation", "Invalid invocation of JBHeating::handle_countdown. Check the number of valves passed");
            ESP_LOGD("jb_irrigation", "valves len: %i state len: %i, other: %i", valvesSize, stateSize, sizeof(id(jb_valves_countdown)));
            return;
        }

        for (int i=0; i<valvesSize; i++) {
            int counter = id(jb_valves_countdown)[i];
            if (counter == 0) continue;
            if (counter > 0)  {
                id(jb_valves_countdown)[i] -= countdownInterval;
                ESP_LOGD("jb_irrigation", "Countdown status: valve %s, counter: %i", valves[i]->get_object_id().c_str(), id(jb_valves_countdown)[i]);
            }
            if (counter <= 0){
                id(jb_valves_countdown)[i] = 0;
                id(valves[i]).turn_off();
                ESP_LOGI("jb_irrigation", "Irrigation cycle finished for valve %s", valves[i]->get_object_id().c_str());
            }
        }
    }

    bool startIrrigation(int duration, std::string valve_id) {
        int valve_index = -1;
        for (int i=0; i<valvesSize; i++) {
            if (valves[i]->get_object_id().compare(valve_id) == 0) {
                valve_index = i;
                break;
            }
        }
        if (valve_index < 0) {
            ESP_LOGE("jb_irrigation", "Unable to schedule irrigation. Valve %s doesn't exist.", valve_id.c_str());
            return false;
        }
        if (duration < countdownInterval) {
            ESP_LOGE("jb_irrigation", "Unable to schedule irrigation. duration can't be less then %i seconds", countdownInterval);
            return false;
        }
        ESP_LOGI("jb_irrigation", "Starting irrigation valve: %s, duration %i sec", valve_id.c_str(), duration);
        id(jb_valves_countdown)[valve_index] = duration;
        id(valves[valve_index]).turn_on();
        return true;
    }
}