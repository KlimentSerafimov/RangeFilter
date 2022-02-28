//
// Created by kliment on 12/6/21.
//

#ifndef SURF_POINTQUERY_H
#define SURF_POINTQUERY_H

#include <string>
#include <vector>
#include "cassert"
using namespace std;

class PointQueryParams
{
public:
    PointQueryParams() = default;
    virtual string to_string() const
    {
        assert(false);
    }
    virtual PointQueryParams* clone() const
    {
        assert(false);
    }
};

class PointQuery: virtual public PointQueryParams
{
public:
    PointQuery(): PointQueryParams() {}
    virtual bool contains(const string& s)
    {
        assert(false);
    }

    virtual void insert(const string& s)
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
};


class RangeFilterStats
{
public:
    PointQuery* params;
    int num_keys;
    int num_queries;
    int num_false_positives;
    int num_negatives;
    unsigned long long total_num_bits;
    RangeFilterStats(
            PointQuery* _params,
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
        ret += "SCORE\tbpk "+std::to_string(bits_per_key())+" fpr "+std::to_string(false_positive_rate())+
                "\n"+params->to_string();
        return ret;
    }

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
};



#endif //SURF_POINTQUERY_H
