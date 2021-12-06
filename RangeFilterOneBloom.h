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

class OneBloomParams: public PointQueryParams
{
    double seed_fpr;
    int cutoff;
public:
    OneBloomParams(double _seed_fpr, int _cutoff):  seed_fpr(_seed_fpr), cutoff(_cutoff) {}
    string to_string() override
    {
        return "seed_fpr\t" + std::to_string(seed_fpr) + "\tcutoff\t" + std::to_string(cutoff);
    }
};

class OneBloom: public PointQuery
{
    bloom_filter bf;
    bool bf_defined = false;

    long long total_num_chars{};
    double seed_fpr;
    int cutoff;

    void calc_metadata(const vector<string>& dataset, const vector<pair<string, string> >& workload, bool do_print)
    {
        total_num_chars = 0;

        int char_count[127];
        memset(char_count, 0, sizeof(char_count));

        for(int i = 0;i<(int)dataset.size();i++)
        {
            if(cutoff != -1) {
                total_num_chars += min((int) dataset[i].size() + 1, cutoff);
            }
            else
            {
                total_num_chars += (int)dataset.size()+1;
            }
        }

        if(do_print) {
            cout << "ONE_BOOM STATS" << endl;
            cout << "num_chars " << total_num_chars << endl;
        }
    }

public:

    OneBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, double _seed_fpr, int _cutoff, bool do_print = false):
    seed_fpr(_seed_fpr), cutoff(_cutoff){
        calc_metadata(dataset, workload, do_print);
        if(total_num_chars > 0) {
            bloom_parameters params = get_bloom_parameters(total_num_chars, seed_fpr);
            bf = bloom_filter(params);
            bf_defined = true;
        }
    }

    void insert(string s) override
    {
        if(cutoff != -1 && (int)s.size() > cutoff)
        {
            //assume inserted
        }
        else {
            assert(bf_defined);
            bf.insert(s);
        }
    }

    bool contains(string s) override
    {
        if(cutoff != -1 && (int)s.size() > cutoff)
        {
            return true;
        }
        else {
            assert(bf_defined);
            return bf.contains(s);
        }
    }

    unsigned long long get_memory() override{
        return bf.get_memory() + sizeof(long long) + sizeof(double) + sizeof(int);
    }

    void clear() override
    {
        bf.clear_memory();
    }

    PointQueryParams* get_params() override
    {
        return new OneBloomParams(seed_fpr, cutoff);
    }

};


#endif //SURF_RANGEFILTERONEBLOOM_H
