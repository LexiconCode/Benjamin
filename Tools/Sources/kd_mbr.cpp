#include "kd_mbr.h"
#include <QDebug>

using namespace kaldi;


struct GammaCompare
{
    // should be like operator <.  But we want reverse order
    // on the 2nd element (posterior), so it'll be like operator
    // > that looks first at the posterior.
    bool operator () (const std::pair<int32, float> &a,
                      const std::pair<int32, float> &b) const {
        if (a.second > b.second) return true;
        else if (a.second < b.second) return false;
        else return a.first > b.first;
    }
};

void KdMBR::MbrDecode()
{
    for( size_t counter=0 ; ; counter++ )
    {
        NormalizeEps(&R_);
        AccStats(); // writes to gamma_
        double delta_Q = 0.0; // change in objective function.

        one_best_times_.clear();
        one_best_confidences_.clear();

        // Caution: q in the line below is (q-1) in the algorithm
        // in the paper; both R_ and gamma_ are indexed by q-1.
        for (size_t q = 0; q < R_.size(); q++)
        {
            if (opts.decode_mbr)
            { // This loop updates R_ [indexed same as gamma_].
                // gamma_[i] is sorted in reverse order so most likely one is first.
                const std::vector<std::pair<int32, BaseFloat> > &this_gamma = gamma_[q];
                double old_gamma = 0, new_gamma = this_gamma[0].second;
                int32 rq = R_[q], rhat = this_gamma[0].first; // rq: old word, rhat: new.
                for (size_t j = 0; j < this_gamma.size(); j++)
                    if (this_gamma[j].first == rq) old_gamma = this_gamma[j].second;
                delta_Q += (old_gamma - new_gamma); // will be 0 or negative; a bound on
                // change in error.
                if (rq != rhat)
                    KALDI_VLOG(2) << "Changing word " << rq << " to " << rhat;
                R_[q] = rhat;
            }
            // build the outputs (time, confidences),
            if (R_[q] != 0 || opts.print_silence)
            {
                // see which 'item' from the sausage-bin should we select,
                // (not necessarily the 1st one when MBR decoding disabled)
                int32 s = 0;
                for (int32 j=0; j<gamma_[q].size(); j++) {
                    if (gamma_[q][j].first == R_[q]) {
                        s = j;
                        break;
                    }
                }
                one_best_times_.push_back(times_[q][s]);
                // post-process the times,
                size_t i = one_best_times_.size();
                if (i > 1 && one_best_times_[i-2].second > one_best_times_[i-1].first)
                {
                    // It's quite possible for this to happen, but it seems like it would
                    // have a bad effect on the downstream processing, so we fix it here.
                    // We resolve overlaps by redistributing the available time interval.
                    float prev_right = i > 2 ? one_best_times_[i-3].second : 0.0;
                    float left = std::max(prev_right,
                                              std::min(one_best_times_[i-2].first,
                                              one_best_times_[i-1].first));
                    float right = std::max(one_best_times_[i-2].second,
                            one_best_times_[i-1].second);
                    float first_dur =
                            one_best_times_[i-2].second - one_best_times_[i-2].first;
                    float second_dur =
                            one_best_times_[i-1].second - one_best_times_[i-1].first;
                    float mid = first_dur > 0 ? left + (right - left) * first_dur /
                                                    (first_dur + second_dur) : left;
                    one_best_times_[i-2].first = left;
                    one_best_times_[i-2].second = one_best_times_[i-1].first = mid;
                    one_best_times_[i-1].second = right;
                }
                float confidence = 0.0;
                for (int32 j = 0; j < gamma_[q].size(); j++) {
                    if (gamma_[q][j].first == R_[q]) {
                        confidence = gamma_[q][j].second;
                        break;
                    }
                }
                one_best_confidences_.push_back(confidence);
            }
        }
        KALDI_VLOG(2) << "Iter = " << counter << ", delta-Q = " << delta_Q;
        if (delta_Q == 0) break;
        if (counter > 100)
        {
            KALDI_WARN << "Iterating too many times in MbrDecode; stopping.";
            break;
        }
    }

    if( !opts.print_silence )
    {
        RemoveEps(&R_);
    }
}

void KdMBR::RemoveEps(std::vector<int32> *vec)
{
    for( int i=0 ; i<vec->size() ; i++ )
    {
        if( vec->at(i)==0 )
        {
            vec->erase(vec->begin()+i);
            i--;
        }
    }
}

// static
void KdMBR::NormalizeEps(std::vector<int32> *vec)
{
    RemoveEps(vec);
    vec->resize(1 + vec->size() * 2);
    int32 s = vec->size();
    for (int32 i = s/2 - 1; i >= 0; i--)
    {
        (*vec)[i*2 + 1] = (*vec)[i];
        (*vec)[i*2 + 2] = 0;
    }
    (*vec)[0] = 0;
}

double KdMBR::EditDistance(int32 N, int32 Q,
                           Vector<double> &alpha,
                           Matrix<double> &alpha_dash,
                           Vector<double> &alpha_dash_arc)
{
    alpha(1) = 0.0; // = log(1).  Line 5.
    alpha_dash(1, 0) = 0.0; // Line 5.
    for (int32 q = 1; q <= Q; q++)
    {
        alpha_dash(1, q) = alpha_dash(1, q-1) + l_distance(0, R_[q-1]); // Line 7.
    }
    for (int32 n = 2; n <= N; n++)
    {
        double alpha_n = kLogZeroDouble;
        for (size_t i = 0; i < pre_[n].size(); i++)
        {
            const KdMBRArc &arc = arcs_[pre_[n][i]];
            alpha_n = LogAdd(alpha_n, alpha(arc.start_node) + arc.loglike);
        }
        alpha(n) = alpha_n; // Line 10.
        // Line 11 omitted: matrix was initialized to zero.
        for (size_t i = 0; i < pre_[n].size(); i++)
        {
            const KdMBRArc &arc = arcs_[pre_[n][i]];
            int32 s_a = arc.start_node, w_a = arc.word;
            BaseFloat p_a = arc.loglike;
            for (int32 q = 0; q <= Q; q++)
            {
                if (q == 0)
                {
                    alpha_dash_arc(q) = // line 15.
                            alpha_dash(s_a, q) + l_distance(w_a, 0, true);
                }
                else
                {  // a1,a2,a3 are the 3 parts of min expression of line 17.
                    int32 r_q = R_[q-1];;
                    double a1 = alpha_dash(s_a, q-1) + l_distance(w_a, r_q),
                            a2 = alpha_dash(s_a, q) + l_distance(w_a, 0, true),
                            a3 = alpha_dash_arc(q-1) + l_distance(0, r_q);
                    alpha_dash_arc(q) = std::min(a1, std::min(a2, a3));
                }
                // line 19:
                alpha_dash(n, q) += Exp(alpha(s_a) + p_a - alpha(n)) * alpha_dash_arc(q);
            }
        }
    }
    return alpha_dash(N, Q); // line 23.
}

// Figure 5 in the paper.
void KdMBR::AccStats()
{
    using std::map;

    int32 N = static_cast<int32>(pre_.size()) - 1,
            Q = static_cast<int32>(R_.size());

    Vector<double> alpha(N+1); // index (1...N)
    Matrix<double> alpha_dash(N+1, Q+1); // index (1...N, 0...Q)
    Vector<double> alpha_dash_arc(Q+1); // index 0...Q
    Matrix<double> beta_dash(N+1, Q+1); // index (1...N, 0...Q)
    Vector<double> beta_dash_arc(Q+1); // index 0...Q
    std::vector<char> b_arc(Q+1); // integer in {1,2,3}; index 1...Q
    std::vector<map<int32, double> > gamma(Q+1); // temp. form of gamma.
    // index 1...Q [word] -> occ.

    // The tau maps below are the sums over arcs with the same word label
    // of the tau_b and tau_e timing quantities mentioned in Appendix C of
    // the paper... we are using these to get averaged times for both the
    // the sausage bins and the 1-best output.
    std::vector<map<int32, double> > tau_b(Q+1), tau_e(Q+1);

    double Ltmp = EditDistance(N, Q, alpha, alpha_dash, alpha_dash_arc);
    if (L_ != 0 && Ltmp > L_) { // L_ != 0 is to rule out 1st iter.
        KALDI_WARN << "Edit distance increased: " << Ltmp << " > "
                   << L_;
    }
    L_ = Ltmp;
    KALDI_VLOG(2) << "L = " << L_;
    // omit line 10: zero when initialized.
    beta_dash(N, Q) = 1.0; // Line 11.
    for (int32 n = N; n >= 2; n--) {
        for (size_t i = 0; i < pre_[n].size(); i++) {
            const KdMBRArc &arc = arcs_[pre_[n][i]];
            int32 s_a = arc.start_node, w_a = arc.word;
            BaseFloat p_a = arc.loglike;
            alpha_dash_arc(0) = alpha_dash(s_a, 0) + l_distance(w_a, 0, true); // line 14.
            for (int32 q = 1; q <= Q; q++)
            { // this loop == lines 15-18.
                int32 r_q = R_[q-1];;
                double a1 = alpha_dash(s_a, q-1) + l_distance(w_a, r_q),
                        a2 = alpha_dash(s_a, q) + l_distance(w_a, 0, true),
                        a3 = alpha_dash_arc(q-1) + l_distance(0, r_q);
                if (a1 <= a2)
                {
                    if (a1 <= a3)
                    {
                        b_arc[q] = 1;
                        alpha_dash_arc(q) = a1;
                    }
                    else
                    {
                        b_arc[q] = 3;
                        alpha_dash_arc(q) = a3;
                    }
                }
                else
                {
                    if (a2 <= a3)
                    {
                        b_arc[q] = 2;
                        alpha_dash_arc(q) = a2;
                    }
                    else
                    {
                        b_arc[q] = 3;
                        alpha_dash_arc(q) = a3;
                    }
                }
            }
            beta_dash_arc.SetZero(); // line 19.
            for (int32 q = Q; q >= 1; q--)
            {
                // line 21:
                beta_dash_arc(q) += Exp(alpha(s_a) + p_a - alpha(n)) * beta_dash(n, q);
                switch (static_cast<int>(b_arc[q]))
                { // lines 22 and 23:
                case 1:
                    beta_dash(s_a, q-1) += beta_dash_arc(q);
                    // next: gamma(q, w(a)) += beta_dash_arc(q)
                    AddToMap(w_a, beta_dash_arc(q), &(gamma[q]));
                    // next: accumulating times, see decl for tau_b,tau_e
                    AddToMap(w_a, state_times_[s_a] * beta_dash_arc(q), &(tau_b[q]));
                    AddToMap(w_a, state_times_[n] * beta_dash_arc(q), &(tau_e[q]));
                    break;
                case 2:
                    beta_dash(s_a, q) += beta_dash_arc(q);
                    break;
                case 3:
                    beta_dash_arc(q-1) += beta_dash_arc(q);
                    // next: gamma(q, epsilon) += beta_dash_arc(q)
                    AddToMap(0, beta_dash_arc(q), &(gamma[q]));
                    // next: accumulating times, see decl for tau_b,tau_e
                    // WARNING: there was an error in Appendix C.  If we followed
                    // the instructions there the next line would say state_times_[sa], but
                    // it would be wrong.  I will try to publish an erratum.
                    AddToMap(0, state_times_[n] * beta_dash_arc(q), &(tau_b[q]));
                    AddToMap(0, state_times_[n] * beta_dash_arc(q), &(tau_e[q]));
                    break;
                default:
                    KALDI_ERR << "Invalid b_arc value"; // error in code.
                }
            }
            beta_dash_arc(0) += Exp(alpha(s_a) + p_a - alpha(n)) * beta_dash(n, 0);
            beta_dash(s_a, 0) += beta_dash_arc(0); // line 26.
        }
    }
    beta_dash_arc.SetZero(); // line 29.
    for (int32 q = Q; q >= 1; q--)
    {
        beta_dash_arc(q) += beta_dash(1, q);
        beta_dash_arc(q-1) += beta_dash_arc(q);
        AddToMap(0, beta_dash_arc(q), &(gamma[q]));
        // the statements below are actually redundant because
        // state_times_[1] is zero.
        AddToMap(0, state_times_[1] * beta_dash_arc(q), &(tau_b[q]));
        AddToMap(0, state_times_[1] * beta_dash_arc(q), &(tau_e[q]));
    }
    for (int32 q = 1; q <= Q; q++)
    { // a check (line 35)
        double sum = 0.0;
        for (map<int32, double>::iterator iter = gamma[q].begin();
             iter != gamma[q].end(); ++iter) sum += iter->second;
        if (fabs(sum - 1.0) > 0.1)
            KALDI_WARN << "sum of gamma[" << q << ",s] is " << sum;
    }
    // The next part is where we take gamma, and convert
    // to the class member gamma_, which is using a different
    // data structure and indexed from zero, not one.
    gamma_.clear();
    gamma_.resize(Q);
    for (int32 q = 1; q <= Q; q++)
    {
        for (map<int32, double>::iterator iter = gamma[q].begin();
             iter != gamma[q].end(); ++iter)
            gamma_[q-1].push_back(
                        std::make_pair(iter->first, static_cast<BaseFloat>(iter->second)));
        // sort gamma_[q-1] from largest to smallest posterior.
        GammaCompare comp;
        std::sort(gamma_[q-1].begin(), gamma_[q-1].end(), comp);
    }
    // We do the same conversion for the state times tau_b and tau_e:
    // they get turned into the times_ data member, which has zero-based
    // indexing.
    times_.clear();
    times_.resize(Q);
    sausage_times_.clear();
    sausage_times_.resize(Q);
    for (int32 q = 1; q <= Q; q++)
    {
        double t_b = 0.0, t_e = 0.0;
        for (std::vector<std::pair<int32, BaseFloat>>::iterator iter = gamma_[q-1].begin();
             iter != gamma_[q-1].end(); ++iter) {
            double w_b = tau_b[q][iter->first], w_e = tau_e[q][iter->first];
            if (w_b > w_e)
                KALDI_WARN << "Times out of order";  // this is quite bad.
            times_[q-1].push_back(
                        std::make_pair(static_cast<BaseFloat>(w_b / iter->second),
                                       static_cast<BaseFloat>(w_e / iter->second)));
            t_b += w_b;
            t_e += w_e;
        }
        sausage_times_[q-1].first = t_b;
        sausage_times_[q-1].second = t_e;
        if (sausage_times_[q-1].first > sausage_times_[q-1].second)
            KALDI_WARN << "Times out of order";  // this is quite bad.
        if (q > 1 && sausage_times_[q-2].second > sausage_times_[q-1].first) {
            // We previously had a warning here, but now we'll just set both
            // those values to their average.  It's quite possible for this
            // condition to happen, but it seems like it would have a bad effect
            // on the downstream processing, so we fix it.
            sausage_times_[q-2].second = sausage_times_[q-1].first =
                    0.5 * (sausage_times_[q-2].second + sausage_times_[q-1].first);
        }
    }
}

std::vector<int32> KdMBR::GetOneBest()
{
    return R_;
}

void KdMBR::AddToMap(int32 i, double d, std::map<int32, double> *gamma)
{
    if (d == 0) return;
    std::pair<const int32, double> pr(i, d);
    std::pair<std::map<int32, double>::iterator, bool> ret = gamma->insert(pr);
    if (!ret.second) // not inserted, so add to contents.
        ret.first->second += d;
}

// gives edit-distance function l(a,b)
double KdMBR::l_distance(int32 a, int32 b, bool penalize)
{
    if (a == b)
    {
        return 0.0;
    }
    else if( penalize )
    {
        return 1.0 + KD_MBR_DELTA;
    }
    else
    {
        return 1.0;
    }
}

// These time intervals may overlap.
std::vector<std::vector<std::pair<float, float> > > KdMBR::GetTimes()
{
    return times_;
}

//the times returned by this method do not overlap
std::vector<std::pair<float, float> > KdMBR::GetSausageTimes()
{
    return sausage_times_;
}

// returns times for entry in the one-best output.
std::vector<std::pair<float, float> > KdMBR::GetOneBestTimes()
{
    return one_best_times_;
}

QVector<BtWord> KdMBR::getResult(QVector<QString> lexicon)
{
    QVector<BtWord> result;
    std::vector<float> conf = GetOneBestConfidences();
    std::vector<int32> words = GetOneBest();
    std::vector<std::pair<float, float>> times = GetOneBestTimes();

    for( int i = 0; i<words.size() ; i++ )
    {
        BtWord word_buf;
        word_buf.conf = conf[i];
        word_buf.start = times[i].first/100.0;
        word_buf.end = times[i].second/100.0;
        word_buf.time = (word_buf.start+word_buf.end)/2;
        word_buf.word = lexicon[words[i]];

        result.push_back(word_buf);
    }

    return result;
}

std::vector<float> KdMBR::GetOneBestConfidences()
{
    return one_best_confidences_;
}

void KdMBR::PrepareLatticeAndInitStats(CompactLattice *clat)
{
    KALDI_ASSERT(clat != NULL);

    CreateSuperFinal(clat); // Add super-final state to clat... this is
    // one of the requirements of the MBR algorithm, as mentioned in the
    // paper (i.e. just one final state).

    // Topologically sort the lattice, if not already sorted.
    kaldi::uint64 props = clat->Properties(fst::kFstProperties, false);
    if (!(props & fst::kTopSorted)) {
        if (fst::TopSort(clat) == false)
            KALDI_ERR << "Cycles detected in lattice.";
    }
    CompactLatticeStateTimes(*clat, &state_times_); // work out times of
    // the states in clat
    state_times_.push_back(0); // we'll convert to 1-based numbering.
    for (size_t i = state_times_.size()-1; i > 0; i--)
        state_times_[i] = state_times_[i-1];

    // Now we convert the information in "clat" into a special internal
    // format (pre_, post_ and arcs_) which allows us to access the
    // arcs preceding any given state.
    // Note: in our internal format the states will be numbered from 1,
    // which involves adding 1 to the OpenFst states.
    int32 N = clat->NumStates();
    pre_.resize(N+1);

    // Careful: "Arc" is a class-member struct, not an OpenFst type of arc as one
    // would normally assume.
    for (int32 n = 1; n <= N; n++)
    {
        for (fst::ArcIterator<CompactLattice> aiter(*clat, n-1);
             !aiter.Done(); aiter.Next())
        {
            const CompactLatticeArc &carc = aiter.Value();
            KdMBRArc arc; // in our local format.
            arc.word = carc.ilabel; // == carc.olabel
            arc.start_node = n;
            arc.end_node = carc.nextstate + 1; // convert to 1-based.
            arc.loglike = - (carc.weight.Weight().Value1() +
                             carc.weight.Weight().Value2());
            // loglike: sum graph/LM and acoustic cost, and negate to
            // convert to loglikes.  We assume acoustic scaling is already done.

            pre_[arc.end_node].push_back(arcs_.size()); // record index of this arc.
            arcs_.push_back(arc);
        }
    }
}

KdMBR::KdMBR(CompactLattice *clat_in)
{
    CompactLattice clat(*clat_in); // copy.
    opts.decode_mbr = true;

    PrepareLatticeAndInitStats(&clat);

    // We don't need to look at clat.Start() or clat.Final(state):
    // we know clat.Start() == 0 since it's topologically sorted,
    // and clat.Final(state) is Zero() except for One() at the last-
    // numbered state, thanks to CreateSuperFinal and the topological
    // sorting.

    { // Now set R_ to one best in the FST.
        RemoveAlignmentsFromCompactLattice(&clat); // will be more efficient
        // in best-path if we do this.
        Lattice lat;
        ConvertLattice(clat, &lat); // convert from CompactLattice to Lattice.
        fst::VectorFst<fst::StdArc> fst;
        ConvertLattice(lat, &fst); // convert from lattice to normal FST.
        fst::VectorFst<fst::StdArc> fst_shortest_path;
        fst::ShortestPath(fst, &fst_shortest_path); // take shortest path of FST.
        std::vector<int32> alignment, words;
        fst::TropicalWeight weight;
        GetLinearSymbolSequence(fst_shortest_path, &alignment, &words, &weight);
        KALDI_ASSERT(alignment.empty()); // we removed the alignment.
        R_ = words;
        L_ = 0.0; // Set current edit-distance to 0 [just so we know
        // when we're on the 1st iter.]
    }

    MbrDecode();

}