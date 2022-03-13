//
// Created by kliment on 12/6/21.
//

#ifndef SURF_POINTQUERY_H
#define SURF_POINTQUERY_H

#include <string>
#include <vector>
#include "cassert"
#include "iostream"
#include "RangeFilterScore.h"
using namespace std;

static int pqp_global_id = 0;

class PointQueryParams
{
    void init()
    {
        pqp_id = pqp_global_id++;
//        cout << "pqp_id: "<< pqp_id << endl;
    }
protected:
    explicit PointQueryParams(const PointQueryParams* to_clone): is_score_set(to_clone->is_score_set), score(to_clone->score) {
        init();
    }
    explicit PointQueryParams(const PointQueryParams& to_clone):
    is_score_set(to_clone.is_score_set), score(to_clone.score) {
        assert(!to_clone.cleared);
        init();
    }
public:
    bool is_cleared()
    {
        return cleared;
    }
    int pqp_id = -1;
    int modify_cleared_id = -1;
    PointQueryParams() {init();};
    virtual string to_string() const
    {
        assert(false);
        return "";
    }
    virtual PointQueryParams* clone_params() const
    {
        assert(false);
        return nullptr;
    }

    void set_cleared_to(bool set_to)
    {
        if(modify_cleared_id == -1) {
            modify_cleared_id = 0;
        }
        modify_cleared_id+=1;
        assert(cleared == !set_to);
        cleared = set_to;
    }

private:
    bool cleared = false;
    bool is_score_set = false;
    RangeFilterScore score;
    bool has_prev_score = false;
    RangeFilterScore prev_score;
public:

    virtual void _clear()
    {
        assert(false);
    }
    bool get_is_score_set() const
    {
        return is_score_set;
    }

    virtual void set_score(const RangeFilterScore& _eval_stats) {
        if(is_score_set)
        {
            assert(score == _eval_stats);
        }
        else {
            assert(!is_score_set);
            is_score_set = true;
            score = _eval_stats;
        }
    }

    const RangeFilterScore& get_score() const
    {
        return score;
    }

    void reset_score()
    {
        assert(is_score_set);
        prev_score = score;
        has_prev_score = true;

        is_score_set = false;
        score.clear();
    }

    void undo_reset_score()
    {
        assert(is_score_set);
        assert(has_prev_score);
        score.clear();
        score = prev_score;
        prev_score.clear();
        has_prev_score = false;
    }

    int get_num_false_positives()
    {
        assert(is_score_set);
        return score.get_num_false_positives();
    }

    int get_num_false_negatives()
    {
        assert(is_score_set);
        return score.get_num_false_negatives();
    }

    virtual unsigned long long get_memory_from_score()
    {
        assert(is_score_set);
        return score.get_memory();
    }

    int get_num_negatives() {
        assert(is_score_set);
        return score.get_num_negatives();
    }
};

#include <map>

class MultiBloomParams;

class PointQuery: virtual public PointQueryParams
{
    int num_shared_ptr = 1;
protected:
    bool has_range_query = false;
public:

    void increment_num_shared_ptr()
    {
        num_shared_ptr+=1;
    }

//    virtual PointQuery* clone()
//    {
//        assert(false);
//    }

    virtual void populate_params(vector<MultiBloomParams *> &ret_params)
    {
        assert(false);
    }

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

    virtual void clear() final {
        decrement_shared_ptr();
    }

    virtual void _clear()
    {
        assert(false);
    }

    void decrement_shared_ptr()
    {
        assert(num_shared_ptr >= 1);
        num_shared_ptr--;
        if(num_shared_ptr == 0)
        {
            _clear();
        }
    }


    unsigned long long get_memory_from_score() override {
        unsigned long long ret = PointQueryParams::get_memory_from_score();
        assert(get_memory()*8 == ret);
        return ret;
    }

};


#endif //SURF_POINTQUERY_H
