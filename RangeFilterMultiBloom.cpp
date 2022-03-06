//
// Created by kliment on 12/4/21.
//

#include "RangeFilterMultiBloom.h"
#include "DatasetAndWorkload.h"

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, double _seed_fpr, int _cutoff,
                               bool do_print)  :
        MultiBloomParams(_cutoff),
        RichMultiBloomParams(MultiBloomParams(_cutoff)),
        MultiBloom(dataset_and_workload.get_dataset(), _seed_fpr, _cutoff, do_print),
        dataset(dataset_and_workload.get_dataset()) {
    assert((int)RichMultiBloomParams::params.size() == RichMultiBloomParams::cutoff);
}

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, const MultiBloomParams& _params,
                               bool do_print) :
        PointQueryParams(_params),
        MultiBloomParams(_params),
        RichMultiBloomParams(_params),
        MultiBloom(dataset_and_workload.get_dataset(), _params, do_print),
        dataset(dataset_and_workload.get_dataset()) {
    assert((int)RichMultiBloomParams::params.size() == RichMultiBloomParams::cutoff);
}

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, const string &line, bool do_print):
        MultiBloomParams(init_from_string(line)),
        RichMultiBloomParams(MultiBloomParams(init_from_string(line))),
        MultiBloom(dataset_and_workload.get_dataset(), line, do_print),
        dataset(dataset_and_workload.get_dataset()){
    assert((int)RichMultiBloomParams::params.size() == RichMultiBloomParams::cutoff);

};