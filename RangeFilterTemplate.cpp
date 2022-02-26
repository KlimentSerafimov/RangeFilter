//
// Created by kliment on 2/26/22.
//

#include "RangeFilterTemplate.h"
#include "DatasetAndWorkload.h"

void RangeFilterTemplate::calc_metadata(const DatasetAndWorkload &dataset_and_workload, bool do_print) {

    max_char = dataset_and_workload.get_max_char();
    min_char = dataset_and_workload.get_min_char();
    max_length = dataset_and_workload.get_max_length();

    is_leaf_char = (char) ((int) min_char - 1);

    if (do_print) {
        cout << "RANGE FILTER STATS" << endl;
        cout << "num_strings " << (int) dataset_and_workload.get_dataset().size() << endl;
        cout << "max_length " << max_length << endl;
        cout << "min_char " << min_char << endl;
        cout << "max_char " << max_char << endl;
        cout << "max_char-min_char+1 " << max_char - min_char + 1 << endl;
        cout << "is_leaf_char " << is_leaf_char << endl;
    }
}

RangeFilterTemplate::RangeFilterTemplate(const DatasetAndWorkload &dataset_and_workload, PointQuery *_pq,
                                         bool do_print):
        pq(_pq)
{
    calc_metadata(dataset_and_workload, do_print);
    insert_prefixes(dataset_and_workload.get_dataset());
}
