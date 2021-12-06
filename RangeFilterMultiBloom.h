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


class MultiBloom: public PointQuery
{
    vector<bloom_filter> bfs;

    int max_length{};
    vector<set<string> > unique_prefixes_per_level;
    vector<int> num_prefixes_per_level;

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

public:

    MultiBloom()= default;

    MultiBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, double fpr, bool do_print = false) {
        calc_metadata(dataset, workload, do_print);

        vector<pair<int, double> > params;
        for(size_t i = 0;i<num_prefixes_per_level.size();i++){
            params.emplace_back(num_prefixes_per_level[i], fpr);
        }

        vector<bloom_filter> parameters = vector<bloom_filter>();
        vector<int> num_inserts_per_level;
        bfs = vector<bloom_filter>();
        for(size_t i = 0;i<params.size();i++)
        {
            parameters.emplace_back(get_bloom_parameters(params[i].first, params[i].second));
            bfs.emplace_back(bloom_filter(parameters[i]));
            num_inserts_per_level.emplace_back(0);
        }
    }

    MultiBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, vector<pair<int, double> > params, bool do_print = false) {
        calc_metadata(dataset, workload, do_print);

        vector<bloom_filter> parameters = vector<bloom_filter>();
        vector<int> num_inserts_per_level;
        bfs = vector<bloom_filter>();
        for(size_t i = 0;i<params.size();i++)
        {
            parameters.emplace_back(get_bloom_parameters(params[i].first, params[i].second));
            bfs.emplace_back(bloom_filter(parameters[i]));
            num_inserts_per_level.emplace_back(0);
        }
    }

    bool contains(string s) override
    {
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
        if(lvl(s) >= bfs.size())
        {
            cout << "NOT ENOUGH BLOOM FILTERS!" << endl;
            assert(false);
        }
        else
        {
            bfs[lvl(s)].insert(s);
        }
    }

    unsigned long long get_memory() {
        unsigned long long ret = 0;
        for(size_t i = 0;i<bfs.size();i++)
        {
            ret+=bfs[i].get_memory();
        }
        return ret + sizeof(MultiBloom);
    }

    void clear()
    {
        for(size_t i = 0; i<bfs.size();i++)
        {
            bfs[i].clear_memory();
        }
        bfs.clear();
    }
};

class RangeFilterMultiBloom: public RangeFilterTemplate<MultiBloom>
{
public:

    RangeFilterMultiBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, double fpr, bool do_print = false):
            RangeFilterTemplate<MultiBloom>(dataset, workload, fpr, do_print)
    {
    }

    RangeFilterMultiBloom(const vector<string>& dataset, const vector<pair<string, string> >& workload, vector<pair<int, double> > params, bool do_print = false)
    :RangeFilterTemplate<MultiBloom>(dataset, workload, std::move(params), do_print){}

};


#endif //SURF_RANGEFILTERMULTIBLOOM_H
