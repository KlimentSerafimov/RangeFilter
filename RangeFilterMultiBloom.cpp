//
// Created by kliment on 12/4/21.
//

#include "RangeFilterMultiBloom.h"
#include "DatasetAndWorkload.h"

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, double _seed_fpr, int _cutoff,
                               bool do_print)  :
        MultiBloom(dataset_and_workload.get_dataset(), _seed_fpr, _cutoff, do_print),
        RichMultiBloomParams(MultiBloom::params), dataset(dataset_and_workload.get_dataset()) {

}

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, vector<pair<int, double>> _params,
                               bool do_print) :
        MultiBloom(dataset_and_workload.get_dataset(), std::move(_params), do_print), RichMultiBloomParams(MultiBloom::params),
        dataset(dataset_and_workload.get_dataset()) {
}

RichMultiBloom::RichMultiBloom(const DatasetAndWorkload &dataset_and_workload, const string &line, bool do_print):
        MultiBloom(dataset_and_workload.get_dataset(), line, do_print), RichMultiBloomParams(MultiBloom::params),
        dataset(dataset_and_workload.get_dataset()){

};