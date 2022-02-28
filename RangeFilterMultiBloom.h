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

class MultiBloomParams: public PointQueryParams {
protected:
    vector<pair<int, double> > params;
public:

    MultiBloomParams* clone() const override
    {
        return new MultiBloomParams(params);
    }

    explicit MultiBloomParams() = default;

    explicit MultiBloomParams(vector<pair<int, double> > _fprs) : params(std::move(_fprs)) {}

    virtual const vector<pair<int, double> > &get_params() const {
        return params;
    };

    string to_string() const override
    {
        string ret;
        for (size_t i = 0; i < params.size(); i++) {
            ret += "lvl" + std::to_string(i) + "(keys " + std::to_string(params[i].first) + ", fpr " +
                   std::to_string(params[i].second) + ") ";
        }
        return ret;
    }
};

class RichMultiBloomParams: virtual public MultiBloomParams
{

private:
    size_t iter_id{};
    size_t epoch_id{};
    size_t used_for_reinit_count = 0;
public:

    RichMultiBloomParams* clone() const override
    {
        RichMultiBloomParams* ret = new RichMultiBloomParams(params);
        ret->iter_id = iter_id;
        ret->epoch_id = epoch_id;
        ret->used_for_reinit_count = ret->used_for_reinit_count;
        return ret;
    }

    const vector<pair<int, double> >& get_params() const override
    {
        return MultiBloomParams::get_params();
    }

    explicit RichMultiBloomParams(vector<pair<int, double> > _fprs): MultiBloomParams(std::move(_fprs)){}

    string to_string() const override {
        string ret;
        ret = "METAPARAMS\titer " + std::to_string(iter_id) + " epoch " + std::to_string(epoch_id) + " reinit " +
              std::to_string(used_for_reinit_count);
        ret += "\tPARAMS\t";
        ret += MultiBloomParams::to_string();
        return ret;
    }

    RichMultiBloomParams *add_iter_and_epoch(size_t _iter_id, size_t _epoch_id) {
        iter_id = _iter_id;
        epoch_id = _epoch_id;
        return this;
    }

    const size_t &get_iter_id() const {
        return iter_id;
    }

    void used_for_reinit() {
        used_for_reinit_count+=1;
        epoch_id+=1;
    }

    size_t get_used_for_reinit_count() const {
        return used_for_reinit_count;
    }

    size_t get_epoch() const {
        return epoch_id;
    }
};


class MultiBloom: public PointQuery, virtual public MultiBloomParams
{

    int max_length{};
    vector<set<string> > unique_prefixes_per_level;
    vector<int> num_prefixes_per_level;
    double seed_fpr{};

protected:
    vector<bloom_filter> bfs;
    int cutoff{};
private:

    void calc_metadata(const vector<string>& dataset, bool do_print)
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

        for(int i = 0;i<min(calc_unique_prefixes_lvl, (int)num_prefixes_per_level.size());i++)
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

protected:
    static size_t lvl(const string& s)
    {
        return s.size()-1;
    }

    void init()
    {
        vector<bloom_filter> parameters = vector<bloom_filter>();
        bfs = vector<bloom_filter>();
        size_t max_lvl = params.size();
        if(cutoff != -1)
        {
            assert(max_lvl <= (size_t)cutoff);
        }
        assert(bfs.empty());
        for(size_t i = 0;i<max_lvl;i++)
        {
            parameters.emplace_back(get_bloom_parameters(params[i].first, params[i].second));
            bfs.emplace_back(bloom_filter(parameters[i]));
        }
    }

public:

    MultiBloomParams* clone() const override
    {
        return MultiBloomParams::clone();
    }

    MultiBloom()= default;

    MultiBloom(const vector<string>& dataset, double _seed_fpr, int _cutoff = -1, bool do_print = false):
    seed_fpr(_seed_fpr), cutoff(_cutoff){
//        for(int i = 0;i<dataset.size();i++)
//        {
//            cout << dataset[i] << endl;
//        }
        calc_metadata(dataset, do_print);
        size_t max_lvl = num_prefixes_per_level.size();
        if(cutoff != -1)
        {
            max_lvl = min(max_lvl, (size_t)cutoff);
        }
        size_t min_num_elements = 4;
        for(size_t i = 0;i<max_lvl;i++){
            params.emplace_back(num_prefixes_per_level[i]+min_num_elements, seed_fpr);
        }

        init();
    }

    bool contains(const string& s) override
    {
        if(cutoff != -1 && (int)lvl(s) >= cutoff)
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

    void insert(const string& s ) override
    {
        if(cutoff != -1 && (int)lvl(s) >= cutoff)
        {
            //assume inserted;
        }
        else {
            if (lvl(s) >= bfs.size()) {
                cout << "NOT ENOUGH BLOOM FILTERS!" << endl;
                assert(false);
            } else {
                if(bfs[lvl(s)].size())
                bfs[lvl(s)].insert(s);
            }
        }
    }

    unsigned long long get_memory() override {
        unsigned long long ret = 0;
        for(size_t i = 0;i<bfs.size();i++)
        {
            float capacity = (1.0-params[i].second)*(float)params[i].first;
//            cout << params[i].first <<" " << params[i].second << " "<< capacity << endl;
            if(capacity >= 1.0) {
                ret += bfs[i].get_memory();
            }
        }
        return ret + sizeof(vector<bloom_filter>) + 2*sizeof(int) + sizeof(vector<set<string> >) + sizeof(vector<int>) + sizeof(double) +
        sizeof(vector<pair<int, double> >) + params.size()*sizeof(pair<int, double>);
    }

    void clear() override
    {
        for(size_t i = 0; i<bfs.size();i++)
        {
            bfs[i].clear_memory();
        }
        bfs.clear();
    }

    string to_string() const override
    {
        return MultiBloomParams::to_string();
    }
};

class RichMultiBloom: public MultiBloom, public RichMultiBloomParams {
    const vector<string>& dataset;

    bool has_prev = false;
    size_t prev_changed_dim_id{};
    double prev_fpr_at_prev_changed_dim_id{};
    char prev_is_leaf_char{};

public:

    RichMultiBloom* clone() const override
    {
        assert(false);
    }

    string to_string() const override
    {
        return RichMultiBloomParams::to_string();
    }

    RichMultiBloom(const DatasetAndWorkload& dataset_and_workload, double _seed_fpr,
                   int _cutoff = -1, bool do_print = false);

    void reinitialize(const RichMultiBloomParams& new_params)
    {
        MultiBloomParams::params = new_params.get_params();
        clear();
        init();
    }

    void perturb(size_t dim_id, double mult, char is_leaf_char) {
        assert(cutoff >= 0);
        assert(0 <= dim_id && dim_id < (size_t)cutoff);
        const double min_mult = 0.5;
        const double max_mult = 2.0;
        assert(min_mult > 0 && min_mult < 1.0);
        assert(max_mult > 1.0);
        assert(min_mult <= mult && mult <= max_mult);

        assert(dim_id < params.size());

        if(has_prev)
        {
            assert(prev_is_leaf_char == is_leaf_char);
        }
        else
        {
            prev_is_leaf_char = is_leaf_char;
        }

        has_prev = true;
        prev_changed_dim_id = dim_id;
        prev_fpr_at_prev_changed_dim_id = params[dim_id].second;

        const double half = 1.0/max_mult;
        if(mult >= 1.0) {
            if (params[dim_id].second > half) {
                double inv_mult = 1.0/mult;
                params[dim_id].second = params[dim_id].second + (1.0 - params[dim_id].second)* inv_mult;
            }
            else
            {
                params[dim_id].second *= mult;
            }
        }
        else
        {
            params[dim_id].second *= mult;
        }

        bfs[dim_id].clear_memory();
        bfs[dim_id] = bloom_filter(get_bloom_parameters(params[dim_id].first, params[dim_id].second));

        for(size_t i = 0;i<dataset.size(); i++)
        {
            string super_str = dataset[i] + is_leaf_char;
            if(lvl(super_str) >= dim_id)
            {
                insert(super_str.substr(0, dim_id+1));
            }
        }
    }

    void undo() {
        assert(has_prev);

        size_t dim_id = prev_changed_dim_id;
        double fpr = prev_fpr_at_prev_changed_dim_id;
        char is_leaf_char = prev_is_leaf_char;

        params[dim_id].second = fpr;

        bfs[dim_id].clear_memory();
        bfs[dim_id] = bloom_filter(get_bloom_parameters(params[dim_id].first, params[dim_id].second));

        for(size_t i = 0;i<dataset.size(); i++)
        {
            string super_str = dataset[i] + is_leaf_char;
            if(lvl(super_str) >= dim_id)
            {
                insert(super_str.substr(0, dim_id+1));
            }
        }
    }
};


#endif //SURF_RANGEFILTERMULTIBLOOM_H
