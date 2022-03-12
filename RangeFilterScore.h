//
// Created by kliment on 3/10/22.
//

#ifndef SURF_RANGEFILTERSCORE_H
#define SURF_RANGEFILTERSCORE_H

#include <vector>
#include <string>

using namespace std;

class PointQuery;

class RangeFilterScore
{
public:
    PointQuery* params = nullptr;
    int num_keys = -1;
    int num_queries = -1;
    int num_false_positives = -1;
    int num_negatives = -1;
    int num_false_negatives = -1;
    unsigned long long total_num_bits = 0;

    void clear(){
        params = nullptr;
        num_keys = -1;
        num_queries = -1;
        num_false_positives = -1;
        num_negatives = -1;
        num_false_negatives = -1;
        total_num_bits = 0;
    }

    bool operator == (const RangeFilterScore& other) const
    {
        return
                num_keys == other.num_keys &&
                num_queries == other.num_queries &&
                num_false_positives == other.num_false_positives &&
                num_negatives == other.num_negatives &&
                total_num_bits == other.total_num_bits;
    }

    RangeFilterScore() = default;

    RangeFilterScore(
            PointQuery* _params,
            int _num_keys,
            int _num_queries,
            int _num_false_positives,
            int _num_negatives,
            int _num_false_negatives,
            unsigned long long _total_num_bits);

    string to_string() const;

    double false_positive_rate() const
    {
        return (double)num_false_positives/num_negatives;
    }

    double bits_per_key() const
    {
        return ((double)total_num_bits / num_keys);
    }

    vector<double> get_score_as_vector() const
    {
        vector<double> ret;
        ret.emplace_back(bits_per_key());
        ret.emplace_back(false_positive_rate());
        return ret;
    }


    PointQuery* get_params() const
    {
        return params;
    }

    int get_num_false_positives() const;

    unsigned long long int get_memory() const;

    int get_num_negatives() const;

    int get_num_false_negatives() const;
};


#endif //SURF_RANGEFILTERSCORE_H
