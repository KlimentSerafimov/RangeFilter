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
#include "PointQuery.h"
#include "RangeFilterTemplate.h"

using namespace std;

class OneBloom: public PointQuery
{
    bloom_filter bf;

    long long total_num_chars;
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

    OneBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, double fpr, bool do_print = false) {
        calc_metadata(dataset, workload, do_print);
        bloom_parameters params = get_bloom_parameters(total_num_chars, fpr);
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

class RangeFilterOneBloom: public RangeFilterTemplate<OneBloom>
{

public:


    RangeFilterOneBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, double fpr, bool do_print = false):
            RangeFilterTemplate<OneBloom>(dataset, workload, fpr, do_print)
    {
    }

};


#endif //SURF_RANGEFILTERONEBLOOM_H
