//
// Created by kliment on 12/6/21.
//

#ifndef SURF_POINTQUERY_H
#define SURF_POINTQUERY_H

#include <string>
#include <vector>
#include "cassert"
using namespace std;

class PointQueryParams
{
public:
    PointQueryParams() = default;
    virtual string to_string() const
    {
        assert(false);
        return "";
    }
    virtual PointQueryParams* clone() const
    {
        assert(false);
        return nullptr;
    }
};

class PointQuery;

class RangeFilterStats
{
public:
    PointQuery* params;
    int num_keys;
    int num_queries;
    int num_false_positives;
    int num_negatives;
    int num_false_negatives;
    unsigned long long total_num_bits;

    bool operator == (const RangeFilterStats& other) const
    {
        return
            num_keys == other.num_keys &&
            num_queries == other.num_queries &&
            num_false_positives == other.num_false_positives &&
            num_negatives == other.num_negatives &&
            total_num_bits == other.total_num_bits;
    }

    RangeFilterStats() = default;

    RangeFilterStats(
            PointQuery* _params,
            int _num_keys,
            int _num_queries,
            int _num_false_positives,
            int _num_negatives,
            int _num_false_negatives,
            unsigned long long _total_num_bits);

    string to_string() const;

    double false_positive_rate() const
    {
        return (double)num_false_positives/num_negatives;
    }

    double bits_per_key() const
    {
        return ((double)total_num_bits / num_keys);
    }

    vector<double> get_score_as_vector() const
    {
        vector<double> ret;
        ret.emplace_back(bits_per_key());
        ret.emplace_back(false_positive_rate());
        return ret;
    }


    PointQuery* get_params() const
    {
        return params;
    }

    int get_num_false_positives();

    unsigned long long int get_memory();

    int get_num_negatives();

    int get_num_false_negatives();
};

#include <map>

class PointQuery: virtual public PointQueryParams
{
protected:
    bool has_range_query = false;
public:
    PointQuery(): PointQueryParams() {}

    bool get_has_range_query() const
    {
        return has_range_query;
    }

//    bool assert_contains = false;
//    map<string, bool> memoized_contains;

    void memoize_contains(const string& s, bool ret)
    {
//        if(memoized_contains.find(s) != memoized_contains.end()) {
//            assert(memoized_contains[s] == ret);
//        }
//        else {
//            assert(!assert_contains);
//            memoized_contains[s] = ret;
//        }
    }

    virtual bool contains(const string& s, const string& left_str, const string& right_str)
    {
        return contains(s);
    }

    virtual bool contains(const string& s)
    {
        assert(false);
        return true;
    }

    virtual bool range_query(const string& left_str, const string& right_str)
    {
        assert(false);
        return true;
    }

    virtual void insert(const string& s)
    {
        assert(false);
    }

    virtual void insert(const string& s, bool is_leaf) {
        insert(s);
    }

    virtual unsigned long long get_memory() {
        assert(false);
        return 0;
    }

    virtual void clear()
    {
        assert(false);
    }

private:
    bool eval_stats_set = false;
    RangeFilterStats eval_stats;
public:

    void set_score(RangeFilterStats _eval_stats) {
        eval_stats_set = true;
        eval_stats = _eval_stats;
    }

    int get_num_false_positives()
    {
        assert(eval_stats_set);
        return eval_stats.get_num_false_positives();
    }

    int get_num_false_negatives()
    {
        assert(eval_stats_set);
        return eval_stats.get_num_false_negatives();
    }

    int get_memory_from_score()
    {
        assert(eval_stats_set);
        assert(get_memory()*8 == eval_stats.get_memory());
        return eval_stats.get_memory();
    }

    int get_num_negatives() {
        assert(eval_stats_set);
        return eval_stats.get_num_negatives();
    }
};


#endif //SURF_POINTQUERY_H
