#ifndef ENN_CHAPAR_H
#define ENN_CHAPAR_H

#include "backend.h"
#include "enn_network.h"

#define ENN_LEARN_MODE 1
#define ENN_TEST_MODE  2

class EnnChapar
{
public:
    EnnChapar(int mode, float l_rate);
    ~EnnChapar();

    void testMode();
    void learnMode(float l_rate);

private:
};

#endif // ENenn_listDirs_H
