//
// Created by kliment on 11/16/21.
//

#ifndef RANGEFILTER_TRIE_H
#define RANGEFILTER_TRIE_H


#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <cassert>

using namespace std;

#define CHAR_SIZE 128

class Trie {

    bool isleaf = false;
    Trie* parent = nullptr;
    Trie *character[CHAR_SIZE]{};

    char _last_char;
    char _init_char;
    int _max_length;

    char* last_char;
    char* init_char;
    int* max_length;

    int count_ret_false = 0;

    bool ret(bool val)
    {
        if(!val)
        {
            count_ret_false+=1;
        }
        return val;
    }

public:

    bool do_breakpoint = false;

    void clear()
    {
        for(int i = 0; i< CHAR_SIZE; i++)
        {
            if(character[i] != nullptr)
            {
                character[i]->clear();
                character[i] = nullptr;
            }
        }
        isleaf = false;
        parent = nullptr;

        *last_char = 0;
        *init_char = 0;
        *max_length = 0;

        count_ret_false = 0;
    }

    int sum_count_ret_false()
    {
        int ret = count_ret_false;
        for(int i = 0;i<CHAR_SIZE;i++)
        {
            if(character[i] != nullptr)
            {
                ret += character[i]->sum_count_ret_false();
            }
        }
        return ret;
    }

    // constructor
    Trie(): _last_char((char)0),
            _init_char((char)127),
            _max_length(0), last_char(&_last_char),
            init_char(&_init_char), max_length(&_max_length){

        this->isleaf = false;
        for (int i = 0; i <CHAR_SIZE; i ++){
            this->character[i] = nullptr;
        }
    }

    Trie(Trie* _parent): Trie()
    {
        parent = _parent;
        if(parent != nullptr)
        {
            last_char = parent->last_char;
            init_char = parent->init_char;
            max_length = parent->max_length;

        }
    }

private:
public:
    bool deletion(Trie*&, string);
    bool contains(string key);
    bool query(string x, string y);
    bool haveChildren(Trie const* p);

    void insert(string basicString);

    explicit Trie(const vector<string>& dataset): Trie()
    {

//        max_char = (char)0;
//        min_char = (char)127;
//        max_length = 0;
//
//        long long total_num_chars = 0;
//
////        int char_count[127];
////        memset(char_count, 0, sizeof(char_count));
//
//        for(int i = 0;i< (int)dataset.size();i++)
//        {
//            max_length = max(max_length, (int)dataset[i].size());
//            total_num_chars+=(int)dataset[i].size();
//            for(int j = 0;j<(int)dataset[i].size();j++)
//            {
//                max_char = (char)max((int)max_char, (int)dataset[i][j]);
//                min_char = (char)min((int)min_char, (int)dataset[i][j]);
////                char_count[(int)dataset[i][j]] += 1;
//            }
//        }


        for(int i = 0;i<(int)dataset.size();i++)
        {
            insert(dataset[i]);
        }
    }

    string get_init_char();

    unsigned long long get_memory();
};

#endif //RANGEFILTER_TRIE_H
