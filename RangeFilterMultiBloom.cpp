//
// Created by kliment on 12/4/21.
//

#include "RangeFilterMultiBloom.h"
#include "DatasetAndWorkload.h"

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, double _seed_fpr, int _cutoff,
                               bool do_print)  :
        MultiBloom(dataset_and_workload.get_dataset(), _seed_fpr, _cutoff, do_print), dataset(dataset_and_workload.get_dataset()) {

};