//
// Created by kliment on 2/25/22.
//

#ifndef SURF_HYBRIDPOINTQUERY_H
#define SURF_HYBRIDPOINTQUERY_H

#include <string>
#include <cassert>
#include "RangeFilterMultiBloom.h"

using namespace std;

class HybridPointQueryParams: virtual public PointQueryParams
{
protected:
    size_t n = 0;
    vector<string> splits;
    vector<PointQueryParams*> sub_point_query_params;
    const DatasetAndWorkloadMetaData& meta_data;

protected:
    virtual bool invariant() const {
        if(n == 0) {
            assert(splits.size() == 0);
            assert(sub_point_query_params.size() == 0);
        }
        else {
            assert(n >= 1);
            assert(splits.size() == n - 1);
            assert(sub_point_query_params.size() == n);
            for (size_t i = 1; i < splits.size(); i++) {
                assert(splits[i] >= splits[i - 1]);
            }
        }
        return true;
    }

public:
    void clear()
    {
        _clear();
    }
    void _clear() override {
        assert(!is_cleared());
        sub_point_query_params.clear();
        splits.clear();
        set_cleared_to(true);
    }
    explicit HybridPointQueryParams(const DatasetAndWorkloadMetaData& _meta_data);
    HybridPointQueryParams(const HybridPointQueryParams& to_copy);
    virtual HybridPointQueryParams* clone_params() const override
    {
        return new HybridPointQueryParams(*this);
    }
    string to_string() const override;
};

class HybridPointQuery: public HybridPointQueryParams, public PointQuery {

    vector<PointQuery*> sub_point_query;
public:

    bool invariant() const override
    {
        assert(HybridPointQueryParams::invariant());
        assert(sub_point_query.size() == sub_point_query_params.size());
        for(size_t i = 0; i< sub_point_query.size();i++)
        {
            assert(sub_point_query[i] == sub_point_query_params[i]);
        }
        return true;
    }

    HybridPointQuery(const DatasetAndWorkload& dataset_and_workload, const string& midpoint, int left_cutoff, float left_fpr, int right_cutoff, float right_fpr, bool do_print);

    HybridPointQuery(
            const string& midpoint, const DatasetAndWorkloadMetaData& meta_data,
            PointQuery* left_pq, PointQuery* right_pq):
            HybridPointQueryParams(meta_data)
    {
        n = 0;
        set_split(midpoint, left_pq, right_pq);
    }

    HybridPointQuery(const DatasetAndWorkloadMetaData& meta_data): HybridPointQueryParams(meta_data) {
        n = 0;
        assert(invariant());
    }

//    HybridPointQuery* clone() override
//    {
//        assert(n == 2);
//        assert(invariant());
//        return new HybridPointQuery(
//                splits[0],
//                meta_data,
//                sub_point_query[0]->clone(),
//                sub_point_query[1]->clone());
//    }


    void populate_params(vector<MultiBloomParams *> &ret_params) override {
        for(auto it: sub_point_query)
        {
            it->populate_params(ret_params);
        }
    }

    void set_score(const RangeFilterScore& _eval_stats) override {
        HybridPointQueryParams::set_score(_eval_stats);
        PointQuery::set_score(_eval_stats);
    }


    HybridPointQueryParams* clone_params() const override{
        return HybridPointQueryParams::clone_params();
    }

    bool contains(const string& s, const string& left_str, const string& right_str) override
    {
//        if(!(left_str < s && s < right_str)) {
//            if(!(left_str.substr(0, s.size()) == s || right_str.substr(0, s.size()) == s))
//            {
//                assert(s.rbegin() == is_leaf_char);
//            }
//        }
        assert(left_str < right_str);
        assert(invariant());
        bool found = false;
        bool ret = false;
        for(size_t i = 0;i<splits.size();i++) {
            assert(splits[i] != s);
            if(right_str <= splits[i]) {
                ret = sub_point_query[i]->contains(s, left_str, right_str);
                found = true;
                break;
            }
            else {
                assert(splits[i] <= left_str);
            }
        }
        if(!found)
        {
            ret = sub_point_query[splits.size()]->contains(s, left_str, right_str);
            found = true;
        }
        assert(found);

        PointQuery::memoize_contains(s, ret);

        return ret;
    }

    void insert(const string& s, bool is_leaf) override
    {
        string base_str = s;
        if(is_leaf)
        {
            base_str = s.substr(0, s.size()-1);
        }
        assert(invariant());

        bool enter = false;
        for(size_t i = 0;i<splits.size();i++) {
            if(splits[i].substr(0, base_str.size()) == base_str) {
                if(!enter) {
                    sub_point_query[i]->insert(s, is_leaf);
                }
                assert(i+1 < sub_point_query.size());
                sub_point_query[i+1]->insert(s, is_leaf);
                enter = true;
            }
            else if(enter) {
                assert(base_str > splits[i]);
                break;
            } else if(base_str > splits[i]) {
                break;
            }
        }
        if(enter) {
            return;
        }
        for(size_t i = 0;i<splits.size();i++) {
            if(splits[i] > s) {
                sub_point_query[i]->insert(s, is_leaf);
                return;
            }
        }
        sub_point_query[splits.size()]->insert(s, is_leaf);
    }

    unsigned long long get_memory() override {
        assert(invariant());
        unsigned long long ret = 0;
        for(size_t i = 0;i<sub_point_query_params.size();i++)
        {
            ret+=sub_point_query[i]->get_memory();
        }
        for(size_t i =0;i<splits.size();i++)
        {
            ret+=splits[i].size()*sizeof(char);
        }
        return ret;
    }

    void _clear() override {
        assert(invariant());
        HybridPointQueryParams::_clear();
        for(auto it: sub_point_query)
        {
            it->clear();
        }
        sub_point_query.clear();
    }

    string to_string() const override
    {
        return HybridPointQueryParams::to_string();
    }

    void set_split(const string& midpoint, PointQuery *left_pq, PointQuery *right_pq);
};


#endif //SURF_HYBRIDPOINTQUERY_H
