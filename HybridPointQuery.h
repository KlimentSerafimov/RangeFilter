//
// Created by kliment on 2/25/22.
//

#ifndef SURF_HYBRIDPOINTQUERY_H
#define SURF_HYBRIDPOINTQUERY_H

#include <string>
#include <cassert>
#include "RangeFilterMultiBloom.h"

using namespace std;

class HybridPointQueryParams: public PointQueryParams
{
protected:
    size_t n = 0;
    vector<string> splits;
    vector<PointQueryParams*> sub_point_query_params;
    const DatasetAndWorkloadMetaData& meta_data;

    bool invariant() const {
        assert(n >= 1);
        assert(splits.size() == n-1);
        assert(sub_point_query_params.size() == n);
        for(size_t i = 1;i<splits.size();i++) {
            assert(splits[i] >= splits[i-1]);
        }
        return true;
    }

public:
    explicit HybridPointQueryParams(const DatasetAndWorkloadMetaData& _meta_data);
    HybridPointQueryParams(const HybridPointQueryParams& to_copy);
    virtual HybridPointQueryParams* clone() const override
    {
        return new HybridPointQueryParams(*this);
    }
    string to_string() const override;
};

class HybridPointQuery: public HybridPointQueryParams, public PointQuery {

    vector<PointQuery*> sub_point_query;
public:
    HybridPointQuery(const DatasetAndWorkload& dataset_and_workload, const string& midpoint, int left_cutoff, float left_fpr, int right_cutoff, float right_fpr, bool do_print);

    HybridPointQuery(const string& midpoint, const DatasetAndWorkloadMetaData& meta_data, PointQuery* left_pq, PointQuery* right_pq, bool do_print = false):
            HybridPointQueryParams(meta_data)
    {
        n = 2;
        splits.push_back(midpoint);

        sub_point_query.push_back(left_pq);
        sub_point_query_params.push_back(*sub_point_query.rbegin());
        sub_point_query.push_back(right_pq);
        sub_point_query_params.push_back(*sub_point_query.rbegin());

        assert(invariant());
    }

    HybridPointQueryParams* clone() const override{
        return HybridPointQueryParams::clone();
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

    void clear() override {
        for(size_t i = 0;i<sub_point_query_params.size();i++) {
            sub_point_query[i]->clear();
        }
        sub_point_query_params.clear();
        for(size_t i =0;i<splits.size();i++) {
            splits[i].clear();
        }
        splits.clear();
    }

    string to_string() const override
    {
        return HybridPointQueryParams::to_string();
    }
};


#endif //SURF_HYBRIDPOINTQUERY_H
