
#pragma once

#include <string>
#include <unordered_map>
#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <memory>
#include <atomic>
#include <chrono>
#include <optional>

// Avoid global using namespace std; explicitly qualify.

// -------- Enums --------
enum class State {
    IDLE,
    BUSY,
    OFFLINE,
    MAINTENANCE,
    UPDATING,
    RECOVERING,
    ERROR
};

enum class ActionType {
    SOFTWARE_UPDATE
};

enum class ActionStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED
};

// -------- Action --------
class Action {
    ActionType _action_type { ActionType::SOFTWARE_UPDATE };
    std::atomic<ActionStatus> _action_status { ActionStatus::PENDING };
    std::string _action_param;
    int _action_id { 0 };

    static std::atomic<int> _id_gen;

public:
    Action() = delete;

    Action(ActionType action_type, const std::string& action_param)
        : _action_type(action_type),
          _action_param(action_param)
    {
        _action_id = _id_gen.fetch_add(1, std::memory_order_relaxed);
    }

    int id() const noexcept { return _action_id; }
    ActionType type() const noexcept { return _action_type; }
    ActionStatus status() const noexcept { return _action_status.load(std::memory_order_relaxed); }
    const std::string& param() const noexcept { return _action_param; }

    void set_status(ActionStatus st) noexcept {
        _action_status.store(st, std::memory_order_relaxed);
    }
};
std::atomic<int> Action::_id_gen { 1 };

// -------- Device --------
class Device {
    std::string _device_info;
    State _device_state { State::IDLE };
    std::unordered_map<int, Action> _actions;
    mutable std::mutex _mtx; // per-device mutex

public:
    explicit Device(State st) : _device_state(st) {}

    // If you want to set info, expose a setter.
    void set_info(const std::string& info) {
        std::lock_guard<std::mutex> lock(_mtx);
        _device_info = info;
    }

    void set_state(State st) {
        std::lock_guard<std::mutex> lock(_mtx);
        _device_state = st;
    }

    State get_state() const {
        std::lock_guard<std::mutex> lock(_mtx);
        return _device_state;
    }

    std::string get_info() const {
        std::lock_guard<std::mutex> lock(_mtx);
        return _device_info;
    }

    // Adds or updates an action
    void add_action(const Action& ac) {
        std::lock_guard<std::mutex> lock(_mtx);
        _actions[ac.id()] = ac;
    }

    std::optional<ActionStatus> get_action_status(int id) const {
        std::lock_guard<std::mutex> lock(_mtx);
        auto it = _actions.find(id);
        if (it == _actions.end()) return std::nullopt;
        return it->second.status();
    }

    // Update action status safely
    bool update_action_status(int id, ActionStatus st) {
        std::lock_guard<std::mutex> lock(_mtx);
        auto it = _actions.find(id);
        if (it == _actions.end()) return false;
        it->second.set_status(st);
        return true;
    }
};

// -------- DevicePool (thread-safe singleton) --------
class DevicePool {
    std::unordered_map<int, std::shared_ptr<Device>> _pool;
    std::mutex _pool_mtx;

    DevicePool() = default;

public:
    DevicePool(const DevicePool&) = delete;
    DevicePool& operator=(const DevicePool&) = delete;

    static DevicePool* getInstance() {
        // C++11 guarantees thread-safe static local initialization
        static DevicePool instance;
        return &instance;
    }

    // Register or replace device
    void RegisterDevice(int device_id, State state) {
        auto dev = std::make_shared<Device>(state);
        std::lock_guard<std::mutex> lock(_pool_mtx);
        _pool[device_id] = std::move(dev);
    }

    // Returns true on success, false if not found
    bool SetDeviceStatus(int device_id, State state) {
        std::shared_ptr<Device> dev;
        {
            std::lock_guard<std::mutex> lock(_pool_mtx);
            auto it = _pool.find(device_id);
            if (it == _pool.end()) return false;
            dev = it->second;
        }
        dev->set_state(state);
        return true;
    }

    // Returns shared_ptr (can be null if not found)
    std::shared_ptr<Device> GetDevice(int device_id) {
        std::lock_guard<std::mutex> lock(_pool_mtx);
        auto it = _pool.find(device_id);
        if (it == _pool.end()) return nullptr;
        return it->second;
    }

    // Initiates an action asynchronously. Returns the action_id or -1 on not found.
    int Initiate(int device_id, ActionType action_type, const std::string& action_param) {
        std::shared_ptr<Device> dev;
        {
            std::lock_guard<std::mutex> lock(_pool_mtx);
            auto it = _pool.find(device_id);
            if (it == _pool.end()) {
                return -1; // device not found
            }
            dev = it->second;
        }

        // Create action in PENDING, register it, then run asynchronously.
        Action action(action_type, action_param);
        const int action_id = action.id();

        // Mark device updating and add action
        dev->set_state(State::UPDATING);
        action.set_status(ActionStatus::RUNNING);
        dev->add_action(action);

        // Run the work asynchronously WITHOUT holding any locks.
        std::async(std::launch::async, this, dev, action_id {
            try {
                // Simulate work
                std::this_thread::sleep_for(std::chrono::seconds(3));

                // Mark completed
                dev->update_action_status(action_id, ActionStatus::COMPLETED);

            } catch (...) {
                // Mark failed if any exception
                dev->update_action_status(action_id, ActionStatus::FAILED);
            }

            // Reset device state (choose appropriate state depending on action/type/result)
            dev->set_state(State::IDLE);
        });

        return action_id;
    }

    // Returns optional status. empty => device/action not found.
    std::optional<ActionStatus> GetDeviceAction(int device_id, int action_id) {
        std::shared_ptr<Device> dev;
        {
            std::lock_guard<std::mutex> lock(_pool_mtx);
            auto it = _pool.find(device_id);
            if (it == _pool.end()) return std::nullopt;
            dev = it->second;
        }
        return dev->get_action_status(action_id);
    }
};

// -------- Convenience adapter functions (matching your original signatures) --------

// Keep these adapter functions if other parts of your code call the int-based API.
// Prefer enum-based APIs if possible.

inline void RegisterDevice(int device_id, int state) {
    DevicePool::getInstance()->RegisterDevice(device_id, static_cast<State>(state));
}

inline bool SetDeviceStatus(int device_id, int state) {
    return DevicePool::getInstance()->SetDeviceStatus(device_id, static_cast<State>(state));
}

inline std::shared_ptr<Device> GetDevice(int device_id) {
    return DevicePool::getInstance()->GetDevice(device_id);
}

inline int Initiate(int device_id, int action_type, const std::string& action_param) {
    return DevicePool::getInstance()->Initiate(
        device_id, static_cast<ActionType>(action_type), action_param
    );
}

inline std::optional<ActionStatus> GetDeviceAction(int device_id, int action_id) {
    return DevicePool::getInstance()->GetDeviceAction(device_id, action_id);
}
