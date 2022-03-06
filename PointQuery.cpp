//
// Created by kliment on 3/1/22.
//

#include "PointQuery.h"


string RangeFilterScore::to_string() const  {
    string ret;
    ret += "SCORE\tbpk "+std::to_string(bits_per_key())+" fpr "+std::to_string(false_positive_rate())+
           "\tMETAPARAMS\t"+params->to_string();
    return ret;
}

int RangeFilterScore::get_num_false_positives() const {
    return num_false_positives;
}

unsigned long long RangeFilterScore::get_memory() const {
    return total_num_bits;
}

RangeFilterScore::RangeFilterScore(PointQuery *_params, int _num_keys, int _num_queries, int _num_false_positives,
                                   int _num_negatives, int _num_false_negatives, unsigned long long int _total_num_bits) :
        params(_params),
        num_keys(_num_keys),
        num_queries(_num_queries),
        num_false_positives(_num_false_positives),
        num_negatives(_num_negatives),
        num_false_negatives(_num_false_negatives),
        total_num_bits(_total_num_bits){

    _params->set_score(*this);
}

int RangeFilterScore::get_num_negatives() const {
    return num_negatives;
}

int RangeFilterScore::get_num_false_negatives() const {
    return num_false_negatives;
}
