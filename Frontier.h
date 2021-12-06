//
// Created by kliment on 12/6/21.
//

#ifndef SURF_FRONTIER_H
#define SURF_FRONTIER_H


#include <utility>
#include <vector>
#include <string>
#include <cassert>

using namespace std;

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


template<typename ParamsType>
class FrontierPoint
{
    bool is_erased_ = false;
    ParamsType params;
    vector<double> score;
public:
    FrontierPoint(ParamsType _params, vector<double> _score): params(_params), score(std::move(_score))
    {}

    bool is_erased()
    {
        return is_erased_;
    }

    void erase()
    {
        assert(!is_erased());
        is_erased_ = true;
    }

    double operator [](size_t dim_id)
    {
        assert(!is_erased());
        assert(0 <= dim_id && dim_id < score.size());
        return score[dim_id];
    }

    const vector<double>& get_score()
    {
        assert(!is_erased());
        return score;
    }

    string to_string()
    {
        assert(!is_erased());
        string ret = params.to_string() + "\tscore\t";
        for(int i = 0;i<score.size();i++)
        {
            ret+=std::to_string(score[i])+" ";
        }
        return ret;
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
    vector<FrontierPoint<ParamsType> > frontier;
public:
    explicit Frontier(size_t _num_objectives): num_objectives(_num_objectives){}

/**

    def insert(self, the_bot, new_point):
        assert isinstance(new_point, dict)
        for obj in self.objectives:
            assert obj in new_point
        for obj in new_point:
            assert obj in self.objectives
*/
    bool insert(ParamsType params, const vector<double>& score)
    {
        assert(num_objectives == score.size());

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
        for(FrontierPoint<ParamsType>& point: frontier) {
            if(point.is_erased()) {
                continue;
            }
            bool point_dominates_new_point = true;
            for(size_t dim_id = 0; dim_id < num_objectives; dim_id++)
            {
                if(score[dim_id] > point[dim_id]) {
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
            for(FrontierPoint<ParamsType>& point: frontier) {
                if(point.is_erased()) {
                    continue;
                }
                bool point_is_dominated = true;
                for(size_t i = 0; i<num_objectives; i++)
                {
                    if(point[i] > score[i])
                    {
                        point_is_dominated = false;
                        break;
                    }
                }
                if(point_is_dominated) {
                    point.erase();
                }
            }
            FrontierPoint<ParamsType> new_point = FrontierPoint<ParamsType>(params, score);
            frontier.push_back(new_point);
            return true;
        }
        else
        {
            return false;
        }

    }

/**
    def remove_erased(self):
        new_frontier = []
        for point in self.frontier:
            if not point.erased:
                new_frontier.append(point)
        self.frontier = new_frontier
*/
    void remove_erased()
    {
        vector<FrontierPoint<ParamsType> > new_frontier;
        for(auto point : frontier)
        {
            if(!point.is_erased())
            {
                new_frontier.push_back(point);
            }
        }
        frontier = new_frontier;
    }

/**
    def __len__(self):
        ret = 0
        for point in self.frontier:
            if not point.erased:
                ret += 1
        return ret
*/
    size_t get_size()
    {
        size_t ret = 0;
        for(auto point : frontier)
        {
            if(!point.is_erased())
            {
                ret+=1;
            }
        }
        return ret;
    }

/**
    def __iter__(self):
        for point in self.frontier:
            if not point.erased:
                yield point
 */
    const vector<FrontierPoint<ParamsType> >& get_frontier()
    {
        return frontier;
    }

 };




#endif //SURF_FRONTIER_H
