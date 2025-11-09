#pragma once

#include <mutex>
#include <map>
#include <string>
#include <chrono>
#include <climits>

namespace Utility {

    class Time_Utils {

        private:

        std::mutex _track_lock;

        std::map<std::string, double> _time_starts;

        std::map<std::string, std::pair<double, int>> _time_tracks;

        std::map<std::string, std::pair<double, double>> _time_min_max_tracks;

        static std::mutex _instance_lock;

        static Time_Utils* _instance;

        public:

        double now_at_second();

        void start_track(const std::string& key);

        void end_track(const std::string& key);

        void clear_track(const std::string& key);

        std::pair<double, int> get_time_track_info(const std::string& key);

        std::pair<double, double> get_time_min_max_track_info(const std::string &key);

        static Time_Utils* get();
    };

}