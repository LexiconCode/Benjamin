#ifndef KD_LATTICE_WEIGHT_H
#define KD_LATTICE_WEIGHT_H

#include <QDebug>
#include "bt_config.h"
#include "fst/fstlib.h"

class KdLatticeWeight
{
public:
    KdLatticeWeight();
    KdLatticeWeight(float a, float b);

    KdLatticeWeight &operator=(const KdLatticeWeight &w) {
        value1 = w.value1;
        value2 = w.value2;
        return *this;
    }

    KdLatticeWeight Reverse() const
    {
        return *this;
    }

    static const KdLatticeWeight Zero()
    {
        return KdLatticeWeight(std::numeric_limits<float>::infinity(),
                               std::numeric_limits<float>::infinity());
    }

    static const KdLatticeWeight One()
    {
        return KdLatticeWeight(0.0, 0.0);
    }

    static const std::string &Type()
    {
        static const std::string type = (sizeof(float) == 4 ? "lattice4" : "lattice8") ;
        return type;
    }

    static const KdLatticeWeight NoWeight()
    {
        return KdLatticeWeight(std::numeric_limits<float>::quiet_NaN(),
                               std::numeric_limits<float>::quiet_NaN());
    }

    bool isValid();
    bool isZero();
    float getCost();

    static constexpr uint64 Properties()
    {
        return fst::kLeftSemiring | fst::kRightSemiring | fst::kCommutative |
                fst::kPath | fst::kIdempotent;
    }

    ///FIXME: SHOULD BE REMOVED
    std::ostream &Write(std::ostream &strm) const
    {
        return strm;
    }

    float value1;
    float value2;
};

inline bool operator!=(const KdLatticeWeight &wa,
                       const KdLatticeWeight &wb)
{
    // Volatile qualifier thwarts over-aggressive compiler optimizations
    // that lead to problems esp. with NaturalLess().
    float va1 = wa.value1;
    float va2 = wa.value2;
    float vb1 = wb.value1;
    float vb2 = wb.value2;
    return (va1 != vb1 || va2 != vb2);
}

inline KdLatticeWeight Times(const KdLatticeWeight &w1,
                             const KdLatticeWeight &w2)
{
    return KdLatticeWeight(w1.value1+w2.value1, w1.value2+w2.value2);
}

inline int Compare (const KdLatticeWeight &w1,
                    const KdLatticeWeight &w2)
{
    float f1 = w1.value1 + w1.value2;
    float f2 = w2.value1 + w2.value2;
    if( f1 < f2)
    {
        return 1;
    } // having smaller cost means you're larger
    // in the semiring [higher probability]
    else if( f1 > f2)
    {
        return -1;
    }
    // mathematically we should be comparing (w1.value1_-w1.value2_ < w2.value1_-w2.value2_)
    // in the next line, but add w1.value1_+w1.value2_ = w2.value1_+w2.value2_ to both sides and
    // divide by two, and we get the simpler equivalent form w1.value1_ < w2.value1_.
    else if( w1.value1 < w2.value1)
    {
        return 1;
    }
    else if( w1.value1 > w2.value1)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

inline KdLatticeWeight Plus(const KdLatticeWeight &w1,
                            const KdLatticeWeight &w2)
{
    return (Compare(w1, w2) >= 0 ? w1 : w2);
}

// divide w1 by w2 (on left/right/any doesn't matter as
// commutative).
inline KdLatticeWeight Divide(const KdLatticeWeight &w1,
                              const KdLatticeWeight &w2,
                              fst::DivideType=fst::DIVIDE_LEFT)
{
    float a = w1.value1 - w2.value1;
    float b = w1.value2 - w2.value2;

    if( a != a || b != b || a == -std::numeric_limits<float>::infinity()
            || b == -std::numeric_limits<float>::infinity())
    {
        qDebug() << "LatticeWeightTpl::Divide, NaN or invalid number produced. "
                   << "[dividing by zero?]  Returning zero";
        return KdLatticeWeight::Zero();
    }
    if( a == std::numeric_limits<float>::infinity() ||
            b == std::numeric_limits<float>::infinity())
    {
        return KdLatticeWeight::Zero(); // not a valid number if only one is infinite.
    }
    return KdLatticeWeight(a, b);
}

inline bool operator==(const KdLatticeWeight &wa,
                       const KdLatticeWeight &wb)
{
    // Volatile qualifier thwarts over-aggressive compiler optimizations
    // that lead to problems esp. with NaturalLess().
    volatile float va1 = wa.value1, va2 = wa.value2,
            vb1 = wb.value1, vb2 = wb.value2;
    return (va1 == vb1 && va2 == vb2);
}

// to convert from Lattice to standard FST
void ConvertLatticeWeight(const KdLatticeWeight &w_in,
                                 fst::TropicalWeightTpl<float> *w_out);

#endif // KD_LATTICE_WEIGHT_H
