#ifndef KD_CMVN_H
#define KD_CMVN_H

#include <QObject>
#include <QThread>

#include "feat/online-feature.h"

class KdCMVN
{
public:

    void GetFrame(int32 frame,
                  kaldi::RecyclingVector *o_features,
                  kaldi::VectorBase<float> *feat);

    //
    // Next, functions that are not in the interface.
    //

    /// Initializer that sets the cmvn state.  If you don't have previous
    /// utterances from the same speaker you are supposed to initialize the CMVN
    /// state from some global CMVN stats, which you can get from summing all cmvn
    /// stats you have in your training data using "sum-matrix".  This just gives
    /// it a reasonable starting point at the start of the file.
    /// If you do have previous utterances from the same speaker or at least a
    /// similar environment, you are supposed to initialize it by calling GetState
    /// from the previous utterance
    KdCMVN(kaldi::OnlineCmvnOptions &opts,
           kaldi::OnlineCmvnState &cmvn_state, int32 dim);

    // Outputs any state information from this utterance to "cmvn_state".
    // The value of "cmvn_state" before the call does not matter: the output
    // depends on the value of OnlineCmvnState the class was initialized
    // with, the input feature values up to cur_frame, and the effects
    // of the user possibly having called Freeze().
    // If cur_frame is -1, it will just output the unmodified original
    // state that was supplied to this object.
    void GetState(int32 cur_frame, kaldi::RecyclingVector *o_features,
                  kaldi::OnlineCmvnState *cmvn_state);

    // This function can be used to modify the state of the CMVN computation
    // from outside, but must only be called before you have processed any data
    // (otherwise it will crash).  This "state" is really just the information
    // that is propagated between utterances, not the state of the computation
    // inside an utterance.
    void SetState(kaldi::OnlineCmvnState cmvn_state);

    // From this point it will freeze the CMN to what it would have been if
    // measured at frame "cur_frame", and it will stop it from changing
    // further. This also applies retroactively for this utterance, so if you
    // call GetFrame() on previous frames, it will use the CMVN stats
    // from cur_frame; and it applies in the future too if you then
    // call OutputState() and use this state to initialize the next
    // utterance's CMVN object.
    void Freeze(int32 cur_frame,
                kaldi::RecyclingVector *o_features);

    virtual ~KdCMVN();
protected:

    /// Smooth the CMVN stats "stats" (which are stored in the normal format as a
    /// 2 x (dim+1) matrix), by possibly adding some stats from "global_stats"
    /// and/or "speaker_stats", controlled by the config.  The best way to
    /// understand the smoothing rule we use is just to look at the code.
    static void SmoothOnlineCmvnStats(const kaldi::MatrixBase<double> &speaker_stats,
                                      const kaldi::MatrixBase<double> &global_stats,
                                      const kaldi::OnlineCmvnOptions &opts,
                                      kaldi::MatrixBase<double> *stats);

    /// Get the most recent cached frame of CMVN stats.  [If no frames
    /// were cached, sets up empty stats for frame zero and returns that].
    void GetMostRecentCachedFrame(int32 frame,
                                  int32 *cached_frame,
                                  kaldi::MatrixBase<double> *stats);

    /// Cache this frame of stats.
    void CacheFrame(int32 frame, const kaldi::MatrixBase<double> &stats);

    /// Initialize ring buffer for caching stats.
    inline void InitRingBufferIfNeeded();

    /// Computes the raw CMVN stats for this frame, making use of (and updating if
    /// necessary) the cached statistics in raw_stats_.  This means the (x,
    /// x^2, count) stats for the last up to opts_.cmn_window frames.
    void ComputeStatsForFrame(int32 frame, kaldi::RecyclingVector *o_features,
                              kaldi::MatrixBase<double> *stats);


    kaldi::OnlineCmvnOptions opts_;
    std::vector<int32> skip_dims_; // Skip CMVN for these dimensions.  Derived from opts_.
    kaldi::OnlineCmvnState orig_state_;   // reflects the state before we saw this
    // utterance.
    kaldi::Matrix<double> frozen_state_;  // If the user called Freeze(), this variable
    // will reflect the CMVN state that we froze
    // at.

    // The variable below reflects the raw (count, x, x^2) statistics of the
    // input, computed every opts_.modulus frames.  raw_stats_[n / opts_.modulus]
    // contains the (count, x, x^2) statistics for the frames from
    // std::max(0, n - opts_.cmn_window) through n.
    std::vector<kaldi::Matrix<double>*> cached_stats_modulo_;
    // the variable below is a ring-buffer of cached stats.  the int32 is the
    // frame index.
    std::vector<std::pair<int32, kaldi::Matrix<double> > > cached_stats_ring_;

    // Some temporary variables used inside functions of this class, which
    // put here to avoid reallocation.
    kaldi::Matrix<double> temp_stats_;
    kaldi::Vector<float>  temp_feats_;
    kaldi::Vector<double> temp_feats_dbl_;
    int32 Dim; ///REMOVE THIS
};


#endif // BT_CMVN_H