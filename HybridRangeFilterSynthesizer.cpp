//
// Created by kliment on 3/5/22.
//

#include "HybridRangeFilterSynthesizer.h"


Frontier<RichMultiBloomParams> *
simulated_annealing(const DatasetAndWorkload &dataset_and_workload, ofstream &frontiers, int meta_iter,
                    Frontier<RichMultiBloomParams> *frontier_p)
{
    const vector<string>& dataset = dataset_and_workload.get_dataset();
    double seed_fpr = 0.0001;

    int seed_cutoff = dataset_and_workload.get_max_length_of_dataset();
    bool do_print = false;


    vector<string> dim_names;
    dim_names.emplace_back("bpk");
    dim_names.emplace_back("fpr");

    int output_step_count_every = 1000;
    int output_frontier_every = 2000;
    int hard_copy_every = 2500;


    if(frontier_p == nullptr) {
        frontier_p = new Frontier<RichMultiBloomParams>(2);
    }
    Frontier<RichMultiBloomParams>& frontier = *frontier_p;

    RichMultiBloom *_pq = new RichMultiBloom(dataset_and_workload, seed_fpr, seed_cutoff, do_print);
    RangeFilterTemplate *_rf = new RangeFilterTemplate(dataset_and_workload, _pq, do_print);

    const RangeFilterScore& _ret = *dataset_and_workload.test_range_filter(_rf, do_print);

    size_t annealing_epoch = 0;
    size_t iter = 1;
    size_t total_num_inserts = 0;
    RichMultiBloom& tmp_rmb_pq = *(RichMultiBloom*)_ret.get_params();
    RichMultiBloomParams& tmp_params = *(tmp_rmb_pq.add_iter_and_epoch(iter, annealing_epoch));
    total_num_inserts += frontier.insert(tmp_params, _ret.get_score_as_vector()) != nullptr;


    cout << endl;
    cout << _ret.to_string() << endl;

    RichMultiBloom *pq = nullptr;
    RangeFilterTemplate *rf = nullptr;

    if(true) {
        for (const auto &it: frontier.get_frontier()) {
            ((RichMultiBloomParams *) &it->get_params())->add_iter_and_epoch(1, 0);
            ((RichMultiBloomParams *) &it->get_params())->reset_reinint_count();
        }

        assert(frontier.get_size() >= 1);

        pq = new RichMultiBloom(dataset_and_workload, (*frontier.get_frontier().begin())->get_params(), do_print);
        assert(pq->get_is_score_set());
        rf = new RangeFilterTemplate(dataset_and_workload, pq, do_print);

    }
    else
    {
        pq = _pq;
        rf = _rf;
    }

    double prev_area = frontier.get_area();
    double prev_line_length = frontier.get_line_length();

    annealing_epoch+=1;

    size_t success_count = 0;
    size_t explore_more_success_count_threshold = 3;

    size_t stagnation_count = 0;

    assert(meta_iter >= 1);
    const size_t stagnation_count_cutoff_for_annealing_epoch_transition = 3*meta_iter;
    const size_t max_reinitialization_count = 0;

    while(true)
    {
        iter+=1;

        int dim_id = rand() % pq->RichMultiBloomParams::get_params().get_cutoff();
        double mult = pow(0.5, 1.0/(double)annealing_epoch);
        if(rand()%2 == 0)
        {
            mult = pow(2.0, 1.0/(double)annealing_epoch);
        }

        pq->perturb(dim_id, mult, rf->get_is_leaf_char()); // save score, and if no improvement reset it back at undo, otherwise update it.

        const RangeFilterScore& ret = *dataset_and_workload.test_range_filter(rf, do_print);

        if(iter % output_step_count_every == 0) {
            cout << endl;
            cout << "ITER " << iter << endl;
            cout << "EPOCH " << annealing_epoch << endl;
        }

        assert(pq == (RichMultiBloom*)ret.get_params());

//        RichMultiBloomParams& local_tmp_params = *((RichMultiBloom*)ret.get_params())->add_iter_and_epoch(iter, annealing_epoch);
        RichMultiBloomParams& local_tmp_params = *(pq)->add_iter_and_epoch(iter, annealing_epoch);

        local_tmp_params.set_score(ret);

        if(frontier.insert(local_tmp_params, ret.get_score_as_vector()))
        {
            pq = pq->clone(dataset_and_workload);
            rf->set_point_query(pq);
            double frontier_area = frontier.get_area();

            success_count+=1;
            total_num_inserts+=1;
            stagnation_count = 0;

            frontiers << "iter\t" << iter <<"\t|frontier.area|" << frontier_area << endl;

            if(iter % output_step_count_every == 0) {
                cout << "INSERT & CONTINUE" << endl;
                cout << "|frontier.area|" << frontier_area << endl;
                cout << "|frontier.line_len|" << frontier.get_line_length() << endl;

                cout << "|frontier| = " << frontier.get_size() << endl;
                cout << "|inserts| = " << total_num_inserts << endl;
                cout << "|betterment| = " << success_count << endl;
            }

        }
        else
        {
            stagnation_count+=1;
            success_count = 0;
            if(iter % output_step_count_every == 0) {
                cout << "UNDO" << endl << "|stagnation| " << stagnation_count << endl;
                double frontier_area = frontier.get_area();
                cout << "|frontier.area|" << frontier_area << endl;
                cout << "|frontier.line_len|" << frontier.get_line_length() << endl;

                cout << "|frontier| = " << frontier.get_size() << endl;
                cout << "|inserts| = " << total_num_inserts << endl;
            }
            pq->undo();
        }

        if(iter % output_step_count_every == 0) {
            cout << ret.to_string() << endl;
        }

        if(success_count >= explore_more_success_count_threshold && annealing_epoch >= 2)
        {
//            cout << endl;
//            cout << "EXPLORE MORE" << endl;
            assert(annealing_epoch >= 2);
            annealing_epoch = annealing_epoch-1;
//            cout << "NEW EPOCH " << annealing_epoch << endl;
            success_count = 0;
        }

        bool break_asap = false;
        if(stagnation_count >= stagnation_count_cutoff_for_annealing_epoch_transition)
        {
//            cout << endl;

            vector<FrontierPoint<RichMultiBloomParams>* >& vec = frontier.get_frontier_to_modify();
            pair<size_t, int> oldest_params = make_pair(iter, -1);

            for(size_t reinitialization_count = 0;
                reinitialization_count <= max_reinitialization_count && oldest_params.second == -1;
                reinitialization_count++) {
//                if(reinitialization_count >= 1)
//                {
//                    cout << "REINITIALIZATION_COUNT " << reinitialization_count << endl;
//                    cout << "MAX_REINITIALIZATION_COUNT " << max_reinitialization_count << endl;
//                }
                for (size_t i = 0; i < vec.size(); i++) {
                    if (vec[i]->get_params().get_used_for_reinit_count() <= reinitialization_count) {
                        oldest_params = min(oldest_params, make_pair(vec[i]->get_params().get_iter_id(), (int) i));
                    }
                }
            }

            if(oldest_params.second == -1)
            {
                cout << "BREAK ASAP" << endl;
                break_asap = true;
            }
            else {

                assert(oldest_params.second != -1);

//                cout << "REINITIALIZE TO" << endl;
//                cout << vec[oldest_params.second]->to_string(dim_names) << endl;

                if (pq->is_cleared()) {
                    assert(false);
                    pq = new RichMultiBloom(dataset_and_workload, vec[oldest_params.second]->get_params());
                } else {
                    pq->reinitialize(vec[oldest_params.second]->get_params());
                }
                rf->insert_prefixes(dataset);

                RichMultiBloomParams &params = vec[oldest_params.second]->get_params_to_modify();
                params.used_for_reinit();

                annealing_epoch = params.get_epoch();

//                cout << "NEW EPOCH " << annealing_epoch << endl;

                stagnation_count = 0;
                success_count = 0;
            }

        }

        if(iter % output_frontier_every == 0 || break_asap)
        {
            const vector<FrontierPoint<RichMultiBloomParams>* >& vec = frontier.get_frontier();

            frontiers << "ITER " << iter << endl;
            frontiers << "EPOCH " << annealing_epoch << endl;
            frontiers << "|inserts| = " << total_num_inserts << endl;
            frontiers << "|betterment| = " << success_count << endl;
            frontiers << "|stagnation| = " << stagnation_count << endl;
            frontiers << "|frontier| = " << frontier.get_size() << endl;

            vector<size_t> reinit_counts;
            for(size_t i = 0; i <= max_reinitialization_count+1; i++)
            {
                reinit_counts.push_back(0);
            }

            for(size_t i = 0;i<vec.size();i++)
            {
                size_t reinit_count = vec[i]->get_params().get_used_for_reinit_count();
                assert(reinit_count <= reinit_counts.size());
                reinit_counts[reinit_count]+=1;
            }

            for(size_t i = 0; i <= max_reinitialization_count+1; i++){
                if(reinit_counts[i] >= 1) {
                    frontiers << "|reinit_counts = " << i << "| = " << reinit_counts[i] << endl;
                }
            }
            for(size_t i = 0;i<vec.size();i++){
                frontiers << vec[i]->to_string(dim_names) << endl;
            }

            frontier.print(frontiers, -1, false);
            frontiers << endl;

        }

        if(iter % output_step_count_every == 0){
            double frontier_area = frontier.get_area();
            double frontier_line_length = frontier.get_line_length();

            if (prev_line_length == frontier_line_length) {
                cout << "line_len NOT IMPROVED" << endl;
                cout << "delta: " << prev_area - frontier_area << endl;
                cout << "cutoff: " << frontier_area * 0.09 / 20 << endl;
                if (prev_area - frontier_area < frontier_area * 0.09 / 20) {
                    frontiers << "IMPROVEMENT TOO SMALL; BREAK;" << endl << endl;
                    cout << "IMPROVEMENT TOO SMALL; BREAK;" << endl << endl;
                    break_asap = true;
                }
            } else {
                cout << "line_len IMPROVED" << endl;
                assert(prev_line_length < frontier_line_length);
            }

            prev_area = frontier_area;
            prev_line_length = frontier_line_length;
        }

        if(iter % hard_copy_every == 0 || break_asap) {
            ofstream latest_frontier("latest_frontier_hard1k_iter" + std::to_string(iter) + "_metaiter"+std::to_string(meta_iter)+".out");
            frontier.print(latest_frontier, -1, false);
            latest_frontier.close();
        }

        if(break_asap)
        {
            cout << endl;
            cout << "ITER " << iter << endl;
            cout << "BREAK" << endl;
            frontiers << "BREAK" << endl;
            break;
        }
    }

    cout << "DONE" << endl;
    frontiers << "DONE" << endl;

    return frontier_p;
}
