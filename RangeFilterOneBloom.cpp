//
// Created by kliment on 3/13/22.
//

#include "DatasetAndWorkload.h"
#include "RangeFilterOneBloom.h"

OneBloom::OneBloom(const Dataset &dataset, double _seed_fpr, int _cutoff) :
        OneBloomParams(dataset.get_num_inserts_in_point_query(_cutoff), _seed_fpr, _cutoff){
    assert(num_inserts >= 1);
    int edge_case_num_inserts = 4;
    bloom_parameters params = get_bloom_parameters(num_inserts+edge_case_num_inserts, seed_fpr);
    bf = bloom_filter(params);
    bf_defined = true;
}
