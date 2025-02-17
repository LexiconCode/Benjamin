#!/bin/bash
. ./path.sh || exit 1
. ./scripts/kaldi/cmd.sh || exit 1

nj=1       # number of parallel jobs
lm_order=1 # language model order (n-gram quantity)

. utils/parse_options.sh || exit 1
# Removing previously created data (from last run.sh execution)
rm -rf exp mfcc data/train/spk2utt data/train/cmvn.scp data/train/feats.scp data/train/split1 data/test/spk2utt data/test/cmvn.scp data/test/feats.scp data/test/split1 data/local/lang data/lang data/local/tmp data/local/dict/lexiconp.txt
echo
echo "===== PREPARING ACOUSTIC DATA ====="
echo

utils/utt2spk_to_spk2utt.pl data/train/utt2spk > data/train/spk2utt
utils/utt2spk_to_spk2utt.pl data/test/utt2spk > data/test/spk2utt

echo
echo "===== FEATURES EXTRACTION ====="
echo
# Making feats.scp files
mfccdir=mfcc
steps/make_mfcc.sh --nj $nj --cmd "$train_cmd" data/train exp/make_mfcc/train $mfccdir
steps/make_mfcc.sh --nj $nj --cmd "$train_cmd" data/test exp/make_mfcc/test $mfccdir
# Making cmvn.scp files
steps/compute_cmvn_stats.sh data/train exp/make_mfcc/train $mfccdir
steps/compute_cmvn_stats.sh data/test exp/make_mfcc/test $mfccdir

echo
echo "===== PREPARING LANGUAGE DATA ====="
echo

utils/prepare_lang.sh data/local/dict "<UNK>" data/local/lang data/lang

echo
echo "===== LANGUAGE MODEL CREATION ====="
echo "===== MAKING lm.arpa ====="
echo

local=data/local
mkdir $local/tmp
steps/ngram-count.sh > $local/tmp/lm.arpa
echo
echo "===== MAKING G.fst ====="
echo
lang=data/lang
arpa2fst --disambig-symbol=#0 --read-symbol-table=$lang/words.txt $local/tmp/lm.arpa $lang/G.fst
echo
echo "===== MONO TRAINING ====="
echo

steps/train_mono.sh --nj $nj --cmd "$train_cmd" data/train data/lang exp/mono

echo "steps/train_mono.sh --nj $nj --cmd "$train_cmd" data/train data/lang exp/mono"
# exit 1
echo
echo "===== MONO DECODING ====="
echo
utils/mkgraph.sh --mono data/lang exp/mono exp/mono/graph || exit 1
steps/decode.sh --config conf/decode.config --nj $nj --cmd "$decode_cmd" exp/mono/graph data/test exp/mono/decode
echo
echo "===== MONO ALIGNMENT ====="
echo
steps/align_si.sh --nj $nj --cmd "$train_cmd" data/train data/lang exp/mono exp/mono_ali || exit
echo
echo "===== TRI1 (first triphone pass) TRAINING ====="
echo
steps/train_deltas.sh --cmd "$train_cmd" 2000 11000 data/train data/lang exp/mono_ali exp/tri1 || exit 1
echo
echo
echo "===== TRI1 Prepare Online Decoding ====="
echo
steps/online/prepare_online_decoding.sh data/train data/lang exp/tri1 exp/tri1/final.mdl exp/tri1_online
echo
echo "===== TRI1 (first triphone pass) ALIGNMENT ====="
echo
steps/align_si.sh --nj $nj --cmd "$train_cmd" data/train data/lang exp/tri1 exp/tri1_ali
echo
echo "===== TRI1 (first triphone pass) DECODING ====="
echo
utils/mkgraph.sh data/lang exp/tri1 exp/tri1/graph || exit 1
steps/decode.sh --config conf/decode.config --nj $nj --cmd "$decode_cmd" exp/tri1/graph data/test exp/tri1/decode
echo
echo "===== Copy Models to BaTool ====="
echo
steps/copy.sh
#echo "===== TRI2 (LDA+MLLT) TRAINING ====="
#echo
#steps/train_lda_mllt.sh --cmd "$train_cmd" 4000 50000 data/train data/lang exp/tri1_ali exp/tri2 || exit 1
#echo
#echo "===== TRI2 (LDA+MLLT) DECODING ====="
#echo
#utils/mkgraph.sh data/lang exp/tri2 exp/tri2/graph || exit 1
#steps/decode.sh --config conf/decode.config --nj $nj --cmd "$decode_cmd" exp/tri2/graph data/test exp/tri2/decode
#echo
#echo "===== TRI2 (LDA+MLLT) ALIGNMENT ====="
#echo
#steps/align_si.sh --nj $nj --cmd "$train_cmd" data/train data/lang exp/tri2 exp/tri2_ali || exit 1
#echo
#echo "===== TRI3 (LDA+MLLT+SAT) TRAINING ====="
#echo
#steps/train_sat.sh --cmd "$train_cmd" 5000 100000 data/train data/lang exp/tri2_ali exp/tri3 || exit 1
#echo
#echo "===== TRI3 (LDA+MLLT+SAT) DECODING ====="
#echo
#utils/mkgraph.sh data/lang exp/tri3 exp/tri3/graph || exit 1
#steps/decode.sh --config conf/decode.config --nj $nj --cmd "$decode_cmd" exp/tri3/graph data/test exp/tri3/decode
