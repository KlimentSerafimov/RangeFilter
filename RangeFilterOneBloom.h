//
// Created by kliment on 12/4/21.
//

#ifndef SURF_RANGEFILTERONEBLOOM_H
#define SURF_RANGEFILTERONEBLOOM_H

#include "bloom/bloom_filter.hpp"
#include <cassert>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include "PointQuery.h"
#include "iomanip"

using namespace std;

class Dataset;

class OneBloomParams: public virtual PointQueryParams
{
protected:
    const int cutoff;
    const int num_inserts;
    const double seed_fpr;
public:
    OneBloomParams(const int _num_inserts, const double _seed_fpr, const int _cutoff):
        cutoff(_cutoff), num_inserts(_num_inserts), seed_fpr(_seed_fpr) {}
    string to_string() const override
    {
        std::ostringstream streamObj;
        streamObj << fixed << setprecision(9) << "cutoff " << cutoff<< " num_inserts " << num_inserts << " seed_fpr " << seed_fpr;
        std::string ret = streamObj.str();
        return ret;
//        return "cutoff " + std::to_string(cutoff) + " num_inserts " + std::to_string(num_inserts) + " seed_fpr " + std::to_string(seed_fpr);
    }
    void _clear() override {
        set_cleared_to(true);
    }
};

class OneBloom: public OneBloomParams, public PointQuery
{
    bloom_filter bf;
    bool bf_defined = false;

public:

    OneBloom(const Dataset &dataset, double _seed_fpr, int _cutoff);

    string to_string() const override
    {
        return OneBloomParams::to_string();
    }

    void insert(const string& s) override
    {
        if(cutoff != -1 && (int)s.size() > cutoff) {
            //assume inserted
        }
        else {
            assert(bf_defined);
            bf.insert(s);
        }
    }

    bool contains(const string& s) override
    {
        bool ret = true;
        if(cutoff != -1 && (int)s.size() > cutoff) {
            ret = true;
        }
        else {
            assert(bf_defined);
            ret = bf.contains(s);
        }
        PointQuery::memoize_contains(s, ret);
        return ret;
    }

    unsigned long long get_memory() override{
        return bf.get_memory() + sizeof(long long) + sizeof(double) + sizeof(int);
    }

    void _clear() override
    {
        bf.clear_memory();
        OneBloomParams::_clear();
        assert(is_cleared());
    }
};


#endif //SURF_RANGEFILTERONEBLOOM_H
