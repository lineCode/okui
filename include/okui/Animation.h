/**
* Copyright 2017 BitTorrent Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <okui/config.h>

#include <okui/interpolation.h>

#include <chrono>

namespace okui {

template <typename T, typename Clock = std::chrono::steady_clock>
class Animation {
public:
    using Duration = typename Clock::duration;
    using Interpolator = T(*)(const T&, const T&, double);
    using TimePoint = typename Clock::time_point;

    Animation(T initial = T{});

    void target(T target, Duration duration, Interpolator interpolator = interpolation::Linear<T>);
    void reset(T value);

    T current() const;

    bool isComplete() const { return current() == target(); }

    const T& initial() const         { return _initial; }
    const T& target() const          { return _target; }
    const TimePoint& start() const   { return _start; }
    TimePoint end() const            { return _start + _duration; }
    const Duration& duration() const { return _duration; }

private:
    T            _initial;
    T            _target;
    TimePoint    _start;
    Duration     _duration = Clock::duration::zero();
    Interpolator _interpolator = nullptr;
};

template<typename T, typename Clock>
Animation<T, Clock>::Animation(T initial)
    : _initial{std::move(initial)}
    , _target{_initial}
{}

template<typename T, typename Clock>
void Animation<T, Clock>::target(T target, typename Clock::duration duration, Interpolator interpolator) {
    _initial      = current();
    _target       = std::move(target);
    _start        = Clock::now();
    _duration     = std::move(duration);
    _interpolator = std::move(interpolator);
}

template<typename T, typename Clock>
void Animation<T, Clock>::reset(T value) {
    _initial      = std::move(value);
    _target       = _initial;
    _start        = {};
    _duration     = Clock::duration::zero();
    _interpolator = nullptr;
}

template<typename T, typename Clock>
T Animation<T, Clock>::current() const {
    if (!_interpolator) {
        return _target;
    }

    auto elapsed = Clock::now() - _start;

    if (elapsed >= _duration) {
        return _target;
    }

    auto t = std::chrono::duration<double>(elapsed).count(),
         d = std::chrono::duration<double>(_duration).count();

    return _interpolator(_initial, _target-_initial, t/d);
}

} // namespace okui
