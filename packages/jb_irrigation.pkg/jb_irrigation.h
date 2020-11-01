#include <esphome.h>

namespace JBHeating {
    std::vector<gpio::GPIOSwitch*> valves;
    int valvesSize, stateSize;
    int countdownInterval = 30;
    int maxIrrigationInterval = 2 * 3600;   // 2 hours

    void init(int interval, const std::vector<gpio::GPIOSwitch*> &v, int maxDuration) {
        valves = v;
        valvesSize = valves.size();
        stateSize = sizeof(id(jb_valves_countdown)) / sizeof(int);
        countdownInterval = interval;
        maxIrrigationInterval = maxDuration;

    }

    bool isFailureDetected() {
        bool result = false;
        for (int i = 0; i < valvesSize; i++) {
            if (id(valves[i]).state) {
                result = true;
                id(valves[i]).turn_off();
            }

        }
        return result;
    }

    int valveIndexByName(std::string valve_id) {
        int valve_index = -1;
        for (int i=0; i<valvesSize; i++) {
            if (valves[i]->get_object_id().compare(valve_id) == 0) {
                valve_index = i;
                break;
            }
        }
        return valve_index;
    }

    int valveIndexByValve(esphome::gpio::GPIOSwitch* valve) {
        int valve_index = -1;
        for (int i=0; i<valvesSize; i++) {
            if (valves[i]->get_object_id().compare(valve->get_object_id()) == 0) {
                valve_index = i;
                break;
            }
        }
        return valve_index;
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
            if (counter > maxIrrigationInterval) {
                ESP_LOGW("jb_irrigation", "Something went wrong. Current countdown value is higher then max. allowed duration of %i seconds. Turning off the valve %s", maxIrrigationInterval, valves[i]->get_object_id().c_str());
                counter = 0;
            }
            if (!id(valves[i]).state) {
                ESP_LOGW("jb_irrigation", "Valve %s is closed but the countdown is in progress. Resetting.", valves[i]->get_object_id().c_str());
                counter = 0;
            }
            if (counter > 0)  {
                counter -= countdownInterval;
                id(jb_valves_countdown)[i] = counter;
                ESP_LOGD("jb_irrigation", "Countdown status: valve %s, counter: %i", valves[i]->get_object_id().c_str(), id(jb_valves_countdown)[i]);
            }
            if (counter <= 0){
                id(jb_valves_countdown)[i] = 0;
                id(valves[i]).turn_off();
                ESP_LOGI("jb_irrigation", "Irrigation cycle finished for valve %s", valves[i]->get_object_id().c_str());
            }
        }
    }

    bool ensureValveOpenTimeIsLimited(esphome::gpio::GPIOSwitch* valve) {
        int valve_index = valveIndexByValve(valve);
        if (valve_index < 0) {
            ESP_LOGE("jb_irrigation", "Valve %s doesn't exist.", valve->get_object_id().c_str());
            return false;
        }
        int counter = id(jb_valves_countdown)[valve_index];
        if (counter > maxIrrigationInterval || counter == 0) {
            id(jb_valves_countdown)[valve_index] = maxIrrigationInterval;
            ESP_LOGE("jb_irrigation", "Irrigation for valve %s wasn't time limited. This typically indicates manual opening which is not recommended. Limit is automatically set to %i", valve->get_object_id().c_str(), maxIrrigationInterval);
            return false;
        }
        return true;
    }

    bool startIrrigation(int duration, std::string valve_id) {
        int valve_index = valveIndexByName(valve_id);
        if (valve_index < 0) {
            ESP_LOGE("jb_irrigation", "Unable to schedule irrigation. Valve %s doesn't exist.", valve_id.c_str());
            return false;
        }
        if (duration < countdownInterval) {
            ESP_LOGE("jb_irrigation", "Unable to schedule irrigation. Duration can't be less then %i seconds", countdownInterval);
            return false;
        }
        if (duration > maxIrrigationInterval) {
            ESP_LOGW("jb_irrigation", "Unable to schedule irrigation. Duration exceeds max allowed duration of %i seconds", maxIrrigationInterval);
            return false;
        }
        ESP_LOGI("jb_irrigation", "Starting irrigation valve: %s, duration %i sec", valve_id.c_str(), duration);
        id(jb_valves_countdown)[valve_index] = duration;
        id(valves[valve_index]).turn_on();
        return true;
    }
}