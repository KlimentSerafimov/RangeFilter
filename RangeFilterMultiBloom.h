//
// Created by kliment on 12/4/21.
//

#ifndef SURF_RANGEFILTERMULTIBLOOM_H
#define SURF_RANGEFILTERMULTIBLOOM_H

#include "bloom/bloom_filter.hpp"
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <cstring>
#include <cassert>
#include <set>
#include "RangeFilterTemplate.h"
#include "PointQuery.h"

using namespace std;

class MultiBloomParams: public PointQueryParams
{
    double seed_fpr{};
    int cutoff{};
public:

    int get_cutoff() const
    {
        return cutoff;
    }

    double get_seed_fpr() const
    {
        return seed_fpr;
    }

    MultiBloomParams() = default;

    MultiBloomParams(double _seed_fpr, int _cutoff): seed_fpr(_seed_fpr), cutoff(_cutoff)
    {

    }

    string to_string() override
    {
        return "seed_fpr\t" + std::to_string(seed_fpr) + "\tcutoff\t"+std::to_string(cutoff);
    }
};

class MultiBloom: public PointQuery
{
    vector<bloom_filter> bfs;

    int max_length{};
    vector<set<string> > unique_prefixes_per_level;
    vector<int> num_prefixes_per_level;
    MultiBloomParams seed_params;

    void calc_metadata(const vector<string>& dataset, const vector<pair<string, string> >& workload, bool do_print)
    {
        const int calc_unique_prefixes_lvl = 9;

        for(size_t i = 0;i<dataset.size();i++)
        {
            max_length = max(max_length, (int)dataset[i].size());
            while((int)num_prefixes_per_level.size() <= max_length)
            {
                num_prefixes_per_level.push_back(0);
            }
            string prefix = "";
            for(size_t j = 0;j<dataset[i].size();j++)
            {
                prefix+=dataset[i][j];
                num_prefixes_per_level[j]+=1;
                if(j < calc_unique_prefixes_lvl)
                {
                    while(unique_prefixes_per_level.size() <= j)
                    {
                        unique_prefixes_per_level.emplace_back();
                    }
                    unique_prefixes_per_level[j].insert(prefix);
                }
            }
            size_t j = dataset[i].size();
            num_prefixes_per_level[j]+=1;
            if(j < calc_unique_prefixes_lvl)
            {
                while(unique_prefixes_per_level.size() <= j)
                {
                    unique_prefixes_per_level.emplace_back();
                }
                unique_prefixes_per_level[j].insert(prefix);
            }
        }

        for(int i = 0;i<calc_unique_prefixes_lvl;i++)
        {
            num_prefixes_per_level[i] = min(num_prefixes_per_level[i], (int)unique_prefixes_per_level[i].size());
        }

        if(do_print) {
            cout << "MULTI_BLOOM STATS" << endl;
            for(size_t i = 0;i<num_prefixes_per_level.size();i++)
            {
                cout << "num_prefixes_lvl_"<<i <<": " << num_prefixes_per_level[i] << endl;
            }
        }
    }

    size_t lvl(string s)
    {
        return s.size()-1;
    }

    void init(const vector<pair<int, double> >& params)
    {
        vector<bloom_filter> parameters = vector<bloom_filter>();
        vector<int> num_inserts_per_level;
        bfs = vector<bloom_filter>();
        size_t max_lvl = params.size();
        if(seed_params.get_cutoff() != -1)
        {
            max_lvl = (size_t)seed_params.get_cutoff();
        }
        for(size_t i = 0;i<max_lvl;i++)
        {
            parameters.emplace_back(get_bloom_parameters(params[i].first, params[i].second));
            bfs.emplace_back(bloom_filter(parameters[i]));
            num_inserts_per_level.emplace_back(0);
        }
    }

public:

    MultiBloom()= default;

    MultiBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, MultiBloomParams _seed_params, bool do_print = false):
    seed_params(std::move(_seed_params)){
        calc_metadata(dataset, workload, do_print);

        vector<pair<int, double> > params;
        for(size_t i = 0;i<num_prefixes_per_level.size();i++){
            params.emplace_back(num_prefixes_per_level[i], seed_params.get_seed_fpr());
        }

        init(params);
    }

    bool contains(string s) override
    {
        if(seed_params.get_cutoff() != -1 && (int)lvl(s) >= seed_params.get_cutoff())
        {
            return true;
        }
        if(lvl(s) >= bfs.size())
        {
            return false;
        }
        else
        {
            return bfs[lvl(s)].contains(s);
        }
    }

    void insert(string s ) override
    {
        if(seed_params.get_cutoff() != -1 && (int)lvl(s) >= seed_params.get_cutoff())
        {
            //assume inserted;
        }
        else {
            if (lvl(s) >= bfs.size()) {
                cout << "NOT ENOUGH BLOOM FILTERS!" << endl;
                assert(false);
            } else {
                bfs[lvl(s)].insert(s);
            }
        }
    }

    unsigned long long get_memory() override {
        unsigned long long ret = 0;
        for(size_t i = 0;i<bfs.size();i++)
        {
            ret+=bfs[i].get_memory();
        }
        return ret + sizeof(MultiBloom);
    }

    void clear() override
    {
        for(size_t i = 0; i<bfs.size();i++)
        {
            bfs[i].clear_memory();
        }
        bfs.clear();
    }
};


#endif //SURF_RANGEFILTERMULTIBLOOM_H
