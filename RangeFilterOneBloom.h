//
// Created by kliment on 12/4/21.
//

#ifndef SURF_RANGEFILTERONEBLOOM_H
#define SURF_RANGEFILTERONEBLOOM_H

#include "bloom/bloom_filter.hpp"
#include <cassert>
#include <utility>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include "PointQuery.h"
#include "RangeFilterTemplate.h"

using namespace std;

class OneBloomParams: public PointQueryParams
{
    double seed_fpr{};
public:
    OneBloomParams() = default;
    explicit OneBloomParams(double _seed_fpr): seed_fpr(_seed_fpr) {}

    double get_seed_fpr() const
    {
        return seed_fpr;
    }

    string to_string() override
    {
        return "seed_fpr\t" + std::to_string(get_seed_fpr());
    }
};

class OneBloom: public PointQuery
{
    bloom_filter bf;
    OneBloomParams seed_params;

    long long total_num_chars{};
    void calc_metadata(const vector<string>& dataset, const vector<pair<string, string> >& workload, bool do_print)
    {
        total_num_chars = 0;

        int char_count[127];
        memset(char_count, 0, sizeof(char_count));

        for(int i = 0;i<(int)dataset.size();i++)
        {
            total_num_chars+=(int)dataset[i].size()+1;
        }


        if(do_print) {
            cout << "ONE_BOOM STATS" << endl;
            cout << "num_chars " << total_num_chars << endl;
        }
    }

public:

    OneBloom() = default;

    OneBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, OneBloomParams _seed_params, bool do_print = false):
            seed_params(std::move(_seed_params)){
        calc_metadata(dataset, workload, do_print);
        bloom_parameters params = get_bloom_parameters(total_num_chars, seed_params.get_seed_fpr());
        bf = bloom_filter(params);
    }

    void insert(string s) override
    {
        bf.insert(s);
    }

    bool contains(string s) override
    {
        return bf.contains(s);
    }

    unsigned long long get_memory() override {
        return bf.get_memory() + sizeof(OneBloom);
    }

    void clear() override
    {
        bf.clear_memory();
    }
};


#endif //SURF_RANGEFILTERONEBLOOM_H
