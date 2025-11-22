#include <utility/time_utils.h>

std::mutex Utility::Time_Utils::_instance_lock;

Utility::Time_Utils* Utility::Time_Utils::_instance = nullptr;

double Utility::Time_Utils::now_at_second() {

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    auto duration_since_epoch = now.time_since_epoch();

    auto milisecond_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch);

    return static_cast<double>(milisecond_since_epoch.count()) / 10e6;
}

void Utility::Time_Utils::start_track(const std::string& key) {

    std::unique_lock<std::mutex> lock(_track_lock);

    _time_starts[key] = now_at_second();
}

void Utility::Time_Utils::end_track(const std::string& key) {
    
    std::unique_lock<std::mutex> lock(_track_lock);

    if (_time_starts.find(key) == _time_starts.end()) {
        return;
    }

    if (_time_tracks.find(key) == _time_tracks.end()) {
        _time_tracks[key] = {};
        _time_tracks[key].first = 0;
        _time_tracks[key].second = 0;
    }

    if (_time_min_max_tracks.find(key) == _time_min_max_tracks.end()) {
        _time_min_max_tracks[key] = {};
        _time_min_max_tracks[key].first = -1.0f;
        _time_min_max_tracks[key].second = std::numeric_limits<double>::max();
    }

    if (_time_tracks[key].second >= INT_MAX) {
        _time_tracks[key].first = 0;
        _time_tracks[key].second = 0;

        if (_time_min_max_tracks.find(key) != _time_min_max_tracks.end()) {
            _time_min_max_tracks[key].first = -1.0f;
            _time_min_max_tracks[key].second = std::numeric_limits<double>::max();
        }
    }

    double time = now_at_second() - _time_starts[key];
    if (time > _time_min_max_tracks[key].first) {
        _time_min_max_tracks[key].first = time;
    }
    if (time < _time_min_max_tracks[key].second) {
        _time_min_max_tracks[key].second = time;
    }
    _time_tracks[key].first += time;
    _time_tracks[key].second++;
}

void Utility::Time_Utils::clear_track(const std::string& key) {

    std::unique_lock<std::mutex> lock(_track_lock);
    
    if (_time_tracks.find(key) != _time_tracks.end()) {

        _time_tracks[key].first = 0;
        _time_tracks[key].second = 0;

        if (_time_min_max_tracks.find(key) != _time_min_max_tracks.end()) {
            _time_min_max_tracks[key].first = -1.0f;
            _time_min_max_tracks[key].second = std::numeric_limits<double>::max();
        }
    }
}

std::pair<double, int> Utility::Time_Utils::get_time_track_info(const std::string& key) {

    std::unique_lock<std::mutex> lock(_track_lock);

    if (_time_tracks.find(key) == _time_tracks.end()) {
        return {-1.0f, -1};
    }
    return _time_tracks[key];
}

std::pair<double, double> Utility::Time_Utils::get_time_min_max_track_info(const std::string &key) {

    std::unique_lock<std::mutex> lock(_track_lock);

    if (_time_min_max_tracks.find(key) == _time_min_max_tracks.end()) {
        return {-1.0f, -1};
    }
    return _time_min_max_tracks[key];
}

Utility::Time_Utils* Utility::Time_Utils::get() {

    std::unique_lock<std::mutex> lock(_instance_lock);

    if(_instance == nullptr) {
        _instance = new Time_Utils();
    }

    return _instance;
}