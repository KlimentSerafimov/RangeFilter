//
// Created by kliment on 12/6/21.
//

#ifndef SURF_POINTQUERY_H
#define SURF_POINTQUERY_H

#include <string>
using namespace std;

class PointQueryParams
{
public:
    virtual string to_string()
    {
        assert(false);
    }
};

class PointQuery
{
public:
    virtual bool contains(string s)
    {
        assert(false);
    }

    virtual void insert(string s)
    {
        assert(false);
    }
    virtual unsigned long long get_memory() {
        assert(false);
    }

    virtual void clear()
    {
        assert(false);
    }
};


#endif //SURF_POINTQUERY_H
