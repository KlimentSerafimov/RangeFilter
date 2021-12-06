//
// Created by kliment on 12/6/21.
//

#ifndef SURF_POINTQUERY_H
#define SURF_POINTQUERY_H

#include <string>
using namespace std;

class PointQueryParams
{
public:
    virtual string to_string()
    {
        assert(false);
    }
};

class PointQuery
{
public:
    virtual bool contains(string s)
    {
        assert(false);
    }

    virtual void insert(string s)
    {
        assert(false);
    }

    virtual unsigned long long get_memory() {
        assert(false);
    }

    virtual void clear()
    {
        assert(false);
    }

    virtual PointQueryParams* get_params()
    {
        assert(false);
    }
};


class RangeFilterStats
{
public:
    PointQueryParams* params;
    int num_keys;
    int num_queries;
    int num_false_positives;
    int num_negatives;
    unsigned long long total_num_bits;
    RangeFilterStats(
            PointQueryParams* _params,
            int _num_keys,
            int _num_queries,
            int _num_false_positives,
            int _num_negatives,
            unsigned long long _total_num_bits):
            params(_params),
            num_keys(_num_keys),
            num_queries(_num_queries),
            num_false_positives(_num_false_positives),
            num_negatives(_num_negatives),
            total_num_bits(_total_num_bits){}

    string to_string() const {
        string ret;
        ret += params->to_string()+"\tbits_per_key\t"+std::to_string(bits_per_key())+"\tfalse_positive_rate(%)\t"+std::to_string(false_positive_rate());
        return ret;
    }

    double false_positive_rate() const
    {
        return (double)num_false_positives/num_negatives*100.0;
    }

    double bits_per_key() const
    {
        return ((double)total_num_bits / num_keys);
    }
};



#endif //SURF_POINTQUERY_H