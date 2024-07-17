#pragma once

#import <Metal/Metal.h>

/**
 * Wait for MTLSharedEvent signaled value for specified amount of time
 * @return true if event signaled value equals input value after sleep timeout and false if not.
 */
inline bool WaitForMTLSharedEventValue(id<MTLSharedEvent> event, uint64_t value, size_t timeoutMS) noexcept {
    // TODO: Uncommit after MacOS 15 [:harold:]
    // return [event waitUntilSignaledValue:value timeoutMS:timeoutMS];

    constexpr size_t SLEEP_SESSION_TIME_MS = 10;
    size_t totalWaited = 0;
    while (event.signaledValue != value && totalWaited < timeoutMS) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_SESSION_TIME_MS));
        totalWaited += SLEEP_SESSION_TIME_MS;
    }
    return event.signaledValue == value;
}
