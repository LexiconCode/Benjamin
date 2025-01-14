# Defining Kaldi root directory
export KALDI_ROOT=`pwd`/../../KalB
KALDI_BIN=`pwd`/../../Kal_Bin
# Setting paths to useful tools
export PATH=$PWD/utils/:$KALDI_ROOT/src/bin:$KALDI_ROOT/tools/openfst-1.7.2/bin:$KALDI_ROOT/src/fstbin/:$KALDI_ROOT/src/gmmbin/:$KALDI_ROOT/src/featbin/:$KALDI_ROOT/src/lmbin/:$KALDI_ROOT/src/sgmm2bin/:$KALDI_ROOT/src/fgmmbin/:$KALDI_ROOT/src/latbin/:$KALDI_ROOT/src/online2bin:$KALDI_ROOT/src/onlinebin:$PWD:$PATH
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-}:$KALDI_ROOT/tools/openfst-1.7.2/lib
# Variable needed for proper data sorting
export LC_ALL=C

## If WSL binary exist add them to the path
if [[ -e "$KALDI_BIN" ]]; then

	# echo "Kaldi Binary discovered"
	# Add Kaldi binary to path
	export PATH=$KALDI_BIN/bin:$KALDI_BIN/fst/bin:$PATH
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-}:$KALDI_BIN/fst/lib

fi

SD="scripts/decode" #Script Decode
ST="scripts/train" #Script Train
SI="scripts/interpreter" #Script Interpreter
SK="scripts/kaldi" #Script Kaldi
