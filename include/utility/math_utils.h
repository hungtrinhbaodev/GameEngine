#pragma once
#include <iostream>
#include <random> // For random number generation

namespace Utility {
    
    class Math_Utils {

        private:

        static std::default_random_engine _generator;

        public:

        static double rand() {
            std::uniform_real_distribution<double> distribution(0.0, 1.0f);
            double rand_number = distribution(_generator); 
            return rand_number;
        }

    };

}