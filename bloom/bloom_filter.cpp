//
// Created by kliment on 12/6/21.
//


#include "bloom_filter.hpp"
bloom_parameters get_bloom_parameters(long long size, double fpr)
{

    bloom_parameters parameters;

    parameters.projected_element_count = size;

    parameters.false_positive_probability = fpr;

    parameters.random_seed = 0xA5A5A5A5;

    if (!parameters)
    {
        std::cout << "Error - Invalid set of bloom filter parameters!" << std::endl;
        assert(false);
        return parameters;
    }

    parameters.compute_optimal_parameters();

    return parameters;
}