//
// Created by kliment on 3/1/22.
//

#ifndef SURF_SURFPOINTQUERY_H
#define SURF_SURFPOINTQUERY_H

#include "PointQuery.h"

class SurfPointQueryParams: public PointQueryParams
{
    int trie_size;

protected:
    string to_string() const override
    {
        return std::to_string(trie_size);
    }
    PointQueryParams* clone() const override
    {
        return new SurfPointQueryParams(trie_size);
    }

    SurfPointQueryParams(int _trie_size): trie_size(_trie_size) {}
};

namespace surf
{
    class SuRF;
}

class SurfPointQuery: public SurfPointQueryParams, public PointQuery
{
    surf::SuRF* surf_real= nullptr;
public:
    SurfPointQuery(const vector<string>& dataset, int _trie_size);
    bool range_query(const string& left_key, const string& right_key) override;
    void insert(const string& s){} // do nothing since it's using surf_real as range filter.
    unsigned long long get_memory() override;
    void clear() override;
    string to_string() const override
    {
        return "trie_size " + SurfPointQueryParams::to_string();
    }
};


#endif //SURF_SURFPOINTQUERY_H
