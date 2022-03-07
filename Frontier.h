//
// Created by kliment on 12/6/21.
//

#ifndef SURF_FRONTIER_H
#define SURF_FRONTIER_H


#include <utility>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <functional>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

class RichMultiBloomParams;

/**
 * class FrontierPoint:
    def __init__(self, the_bot, point):
        self.bot = the_bot
        self.point = point
        self.erased = False

    def __getitem__(self, item):
        assert not self.erased
        return self.point[item]

    def __iter__(self):
        return self.point.__iter__()

    def __repr__(self):
        return str(self.bot) + " " + str(self.point)
*/


class LineLength: public pair<double, double>
{
public:
    LineLength(double _f, double _s): pair<double, double>(_f, _s) {}
    string to_string()
    {
        std::ostringstream streamObj;
        streamObj << setprecision(8) <<  first <<" "<< second;
        std::string strObj = streamObj.str();
        return strObj;
    }

    bool operator < (const LineLength& other) const {
        if(first < other.first && second < other.second) {
            return true;
        }
        else if((first <= other.first) != (second <= other.second)) {
            assert(false);
        }
        else if(first == other.first) {
            return second < other.second;
        }
        else if(second == other.second) {
            return first < other.first;
        }
        else {
            assert(false);
        }
    }
};


template<typename ParamsType>
class FrontierPoint
{
    mutable bool is_erased_ = false;
    ParamsType params;
    vector<double> score;
public:
    FrontierPoint(ParamsType _params, vector<double> _score): params(_params), score(std::move(_score))
    {}

    bool is_erased() const
    {
        return is_erased_;
    }

    void erase()
    {
        assert(!is_erased());
        is_erased_ = true;
        params.clear();
    }

    double operator [](size_t dim_id) const
    {
        assert(!is_erased());
        assert(0 <= dim_id && dim_id < score.size());
        return score[dim_id];
    }

    const vector<double>& get_score_as_vector()
    {
        assert(!is_erased());
        return score;
    }

    string to_string(const vector<string>& dim_names) const
    {
        assert(!is_erased());
        assert(dim_names.size() == score.size());
        string ret = "SCORE\t";
        for(size_t i = 0;i<score.size();i++)
        {
            ret+=dim_names[i]+" "+std::to_string(score[i])+" ";
        }
        ret += "\t" + params.to_string();
        return ret;
    }

    string to_string() const
    {
        assert(!is_erased());
        string ret;
        for(size_t i = 0;i<score.size();i++)
        {
            ret+=std::to_string(score[i])+" ";
        }
        return ret;
    }

    bool operator < (const FrontierPoint<ParamsType>& other) const
    {
        assert(other.score.size() == score.size());
        for(size_t i = 0;i<score.size();i++)
        {
            if(score[i] < other.score[i])
            {
                return true;
            }
            else if(score[i] > other.score[i])
            {
                return false;
            }
        }
        return false;
    }

    const ParamsType& get_params() const {
        return params;
    }

    ParamsType& get_params_to_modify() {
        return params;
    }

};

/**
class Frontier:
    def __init__(self, objectives):
        self.objectives = objectives
        self.frontier = []
        */

template<typename ParamsType>
class Frontier
{
    size_t num_objectives;
    vector<FrontierPoint<ParamsType>* > frontier;
    size_t num_erased = 0;

/**
    def remove_erased(self):
        new_frontier = []
        for point in self.frontier:
            if not point.erased:
                new_frontier.append(point)
        self.frontier = new_frontier
*/
    void _remove_erased()
    {
        if(num_erased == 0) {
            return;
        }
        vector<FrontierPoint<ParamsType>* > new_frontier;
        for(FrontierPoint<ParamsType>* point : frontier)
        {
            if(!point->is_erased())
            {
                new_frontier.push_back(point);
            }
        }
        frontier = new_frontier;
        num_erased = 0;
        sorted = false;
    }

public:

    bool changed = false;

    explicit Frontier(size_t _num_objectives): num_objectives(_num_objectives){}

    bool sorted = false;

    void sort()
    {
        if(sorted) {
            return;
        }
        _remove_erased();
        std::sort(frontier.begin(), frontier.end(),
             [ ]( const FrontierPoint<ParamsType>* lhs, const FrontierPoint<ParamsType>* rhs )
                {
                    return (*lhs) < (*rhs);
                });
        sorted = true;
    }

    void print(ostream& out, int print_top = -1, bool decorate = false)
    {
        sort();
        reverse(frontier.begin(), frontier.end());
        sorted = false;
        int init_print = 0;

        if(decorate)
        out << "frontier (";

        if(print_top != -1)
        {
            init_print = max(0, (int)frontier.size()-print_top);

        }

        if(decorate)
        out << "top " << frontier.size() - init_print << "/" << frontier.size() << ")" <<  endl;

        for(size_t i = init_print; i<frontier.size();i++)
        {
            out << frontier[i]->to_string() << " PARAMS " << frontier[i]->get_params().to_string() << endl;
        }

        if(decorate)
        out << "done printing_frontier" << endl;
    }

/**

    def insert(self, the_bot, new_point):
        assert isinstance(new_point, dict)
        for obj in self.objectives:
            assert obj in new_point
        for obj in new_point:
            assert obj in self.objectives
*/
    const FrontierPoint<ParamsType>* insert(const ParamsType& params, const vector<double>& score)
    {
        assert(num_objectives == score.size());

        double init_area = get_area();
        LineLength init_line_length = get_line_length();

/**
        new_point_is_dominated = False
        for point in self.frontier:
            if point.erased:
                continue
            point_dominates_new_point = True
            for obj in self.objectives:
                if new_point[obj] > point[obj]:
                    point_dominates_new_point = False
                    break
            if point_dominates_new_point:
                new_point_is_dominated = True
                break
 */

        bool new_point_is_dominated = false;
        for(const FrontierPoint<ParamsType>* point: frontier) {
            if(point->is_erased()) {
                continue;
            }
            bool point_dominates_new_point = true;
            for(size_t dim_id = 0; dim_id < num_objectives; dim_id++)
            {
                if(score[dim_id] < (*point)[dim_id]) {
                    point_dominates_new_point = false;
                    break;
                }
            }
            if(point_dominates_new_point) {
                new_point_is_dominated = true;
                break;
            }
        }

/**
        if not new_point_is_dominated:
            for point in self.frontier:
                if point.erased:
                    continue
                point_is_dominated = True
                for obj in self.objectives:
                    if point[obj] > new_point[obj]:
                        point_is_dominated = False
                        break
                if point_is_dominated:
                    point.erased = True
            self.frontier.append(FrontierPoint(the_bot, new_point))
*/
        if(!new_point_is_dominated)
        {
            for(FrontierPoint<ParamsType>* point: frontier) {
                if(point->is_erased()) {
                    continue;
                }
                bool point_is_dominated = true;
                for(size_t i = 0; i<num_objectives; i++)
                {
                    if((*point)[i] < score[i]) {
                        point_is_dominated = false;
                        break;
                    }
                }
                if(point_is_dominated) {
                    point->erase();
                    num_erased+=1;
                }
            }
            FrontierPoint<ParamsType>* new_point = new FrontierPoint<ParamsType>(params, score);
            frontier.push_back(new_point);

            if(num_erased*2 >= frontier.size())
            {
                _remove_erased();
            }

            changed = true;
            sorted = false;


            if(is_same<ParamsType, RichMultiBloomParams>::value) {
                if (init_area < get_area()) {
                    assert(init_line_length < get_line_length());
                } else {
                    assert(init_area >= get_area());
                }
            }

            return new_point;
        }
        else
        {
            return nullptr;
        }

    }


/**
    def __len__(self):
        ret = 0
        for point in self.frontier:
            if not point.erased:
                ret += 1
        return ret
*/
    size_t get_size() const
    {
        return frontier.size()-num_erased;
    }

/**
    def __iter__(self):
        for point in self.frontier:
            if not point.erased:
                yield point
 */
    const vector<FrontierPoint<ParamsType>* >& get_frontier()
    {
        sort();
        return frontier;
    }

    const vector<FrontierPoint<ParamsType>* >& get_frontier_const() const {
        return frontier;
    }

    vector<FrontierPoint<ParamsType>* >& get_frontier_to_modify() {
        sort();
        return frontier;
    }

    pair<vector<double>, ParamsType>* get_best_better_than(vector<double> constraint, std::function<double(vector<double>)> optimization_function) {

        sort();

        assert(constraint.size() == num_objectives);

        pair<double, pair<vector<double>, ParamsType> >* ret = nullptr;

        for(size_t i = 0;i<frontier.size();i++)
        {
            bool passes = true;
            for(size_t j = 0;j<constraint.size();j++)
            {
                if(constraint[j] < (*frontier[i])[j])
                {
                    passes = false;
                    break;
                }
            }

            if(passes)
            {
                auto tmp = new pair<double, pair<vector<double>, ParamsType> >(
                        optimization_function(frontier[i]->get_score_as_vector()),
                        make_pair(
                                frontier[i]->get_score_as_vector(),
                                frontier[i]->get_params()
                        )
                );
                if(ret == nullptr)
                {
                    ret = tmp;
                }
                else
                {
                    *ret = min(*ret, *tmp);
                }
            }
        }
        if(ret != nullptr) {
            return &ret->second;
        }
        else {
            return nullptr;
        }
    }

    long double get_area()
    {
        assert(num_objectives == 2);
        if(frontier.empty())
        {
            return numeric_limits<double>::max();
        }
        assert(frontier.size() >= 1);
        sort();
        double ret = 0;
        double left = 0;
        for(size_t i = 0;i<frontier.size();i++) {
            if(i == 0) {
                ret += (frontier[i]->get_score_as_vector()[0] - left) * frontier[i]->get_score_as_vector()[1];
                left = frontier[i]->get_score_as_vector()[0];
            }
            else {
                ret += (frontier[i]->get_score_as_vector()[0] - left) * frontier[i - 1]->get_score_as_vector()[1];
                left = frontier[i]->get_score_as_vector()[0];
            }
        }
        return ret;
    }

    LineLength get_line_length()
    {
        assert(num_objectives == 2);
        if(frontier.empty()) {
            return LineLength(-numeric_limits<double>::max(), -numeric_limits<double>::max());
        }
        sort();
        assert(frontier.size() >= 1);

        double x = (*frontier.begin())->get_score_as_vector()[0];
        double y = (*frontier.rbegin())->get_score_as_vector()[1];

        if(is_same<ParamsType, RichMultiBloomParams>::value) {
            assert(x >= 0);
            assert(y >= 0);
        }

        return LineLength(-x, -y);

//        if(x >= 0 && y >= 0)
//        {
//            return -x*y;
//        }
//        else if((x > 0) != (y > 0))
//        {
//            return -x*y;
//        }
//        else
//        {
//            assert(x < 0 && y < 0);
//            return x*y;
//        }
    }


};




#endif //SURF_FRONTIER_H
