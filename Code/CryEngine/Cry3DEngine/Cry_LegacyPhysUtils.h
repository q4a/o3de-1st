/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// Copied utils functions from CryPhysics that are used by non-physics systems
// This functions will be eventually removed, DO *NOT* use these functions
// TO-DO: Re-implement users using new code
// LY-109806

#pragma once

#include "stridedptr.h"
#include "Cry_Math.h"
#include "Cry_Vector3.h"

namespace LegacyCryPhysicsUtils
{
    typedef int index_t;

    // Workaround for bug in GCC 4.8. The kind of access patterns here leads to an internal
    // compiler error in GCC 4.8 when optimizing with debug symbols. Two possible solutions
    // are available, compile in Profile mode without debug symbols or remove optimizations
    // in the code where the bug occurs
    // see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=59776
#if defined(_PROFILE) && !defined(__clang__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 8)
    // Cannot use #pragma GCC optimize("O0") because it causes a system crash when using
    // the gcc compiler for another platform
#define CRY_GCC48_AVOID_OPTIMIZE __attribute__((optimize("-O0")))
#else
#define CRY_GCC48_AVOID_OPTIMIZE
#endif
    // unused_marker deliberately fills a variable with invalid data,
    // so that later is_unused() can check whether it was initialized
    // (this is used in all physics params/status/action structures)
    class unused_marker
    {
    public:
        union f2i
        {
            float f;
            uint32 i;
        };
        union d2i
        {
            double d;
            uint32 i[2];
        };
        unused_marker() {}
        unused_marker& operator,(float& x) CRY_GCC48_AVOID_OPTIMIZE;
        unused_marker& operator,(double& x) CRY_GCC48_AVOID_OPTIMIZE;
        unused_marker& operator,(int& x) CRY_GCC48_AVOID_OPTIMIZE;
        unused_marker& operator,(unsigned int& x) CRY_GCC48_AVOID_OPTIMIZE;
        template<class ref>
        unused_marker& operator,(ref*& x) { x = (ref*)-1; return *this; }
        template<class F>
        unused_marker& operator,(Vec3_tpl<F>& x) { return *this, x.x; }
        template<class F>
        unused_marker& operator,(Quat_tpl<F>& x) { return *this, x.w; }
        template<class F>
        unused_marker& operator,(strided_pointer<F>& x) { return *this, x.data; }
    };
    inline unused_marker& unused_marker::operator,(float& x) { *alias_cast<int*>(&x) = 0xFFBFFFFF; return *this; }
    inline unused_marker& unused_marker::operator,(double& x) { (alias_cast<int*>(&x))[false ? 1 : 0] = 0xFFF7FFFF; return *this; }
    inline unused_marker& unused_marker::operator,(int& x) { x = 1 << 31; return *this; }
    inline unused_marker& unused_marker::operator,(unsigned int& x) { x = 1u << 31; return *this; }

#undef CRY_GCC48_AVOID_OPTIMIZE

    inline bool is_unused(const float& x) { unused_marker::f2i u; u.f = x; return (u.i & 0xFFA00000) == 0xFFA00000; }

    inline bool is_unused(int x) { return x == 1 << 31; }
    inline bool is_unused(unsigned int x) { return x == 1u << 31; }
    template<class ref>
    bool is_unused(ref* x) { return x == (ref*)-1; }
    template<class ref>
    bool is_unused(strided_pointer<ref> x) { return is_unused(x.data); }
    template<class F>
    bool is_unused(const Ang3_tpl<F>& x) { return is_unused(x.x); }
    template<class F>
    bool is_unused(const Vec3_tpl<F>& x) { return is_unused(x.x); }
    template<class F>
    bool is_unused(const Quat_tpl<F>& x) { return is_unused(x.w); }
    inline bool is_unused(const double& x) { unused_marker::d2i u; u.d = x; return (u.i[eLittleEndian ? 1 : 0] & 0xFFF40000) == 0xFFF40000; }
#ifndef MARK_UNUSED
#define MARK_UNUSED LegacyCryPhysicsUtils::unused_marker(),
#endif
    typedef void* (*qhullmalloc)(size_t);
    namespace qhull_IMPL
    {
        struct ptitem
        {
            ptitem* next, * prev;
        };

        struct qhtritem
        {
            qhtritem* next, * prev;
            ptitem* ptassoc;
            Vec3 n, pt0;
            int idx[3];
            qhtritem* buddy[3];
            int deleted;
        };

        inline bool e_cansee(const Vec3& dp, const Vec3& n, float e = 0.002f) { return sqr_signed(dp * n) > sqr_signed(e)* dp.len2()* n.len2(); }

        inline void relocate_ptritem(qhtritem*& ptr, intptr_t diff)
        {
            ptr = (qhtritem*)((intptr_t)ptr + diff & ~- iszero((intptr_t)ptr));    // offset the pointer, but leave out 0s
        }

        inline void relocate_tritem(qhtritem* ptr, intptr_t diff)
        {
            relocate_ptritem(ptr->next, diff);
            relocate_ptritem(ptr->prev, diff);
            relocate_ptritem(ptr->buddy[0], diff);
            relocate_ptritem(ptr->buddy[1], diff);
            relocate_ptritem(ptr->buddy[2], diff);
        }

        template<class item>
        void delete_item_from_list(item* p)
        {
            if (p->prev)
            {
                p->prev->next = p->next;
            }
            if (p->next)
            {
                p->next->prev = p->prev;
            }
            p->prev = p->next = 0;
        }
        template<class item>
        void add_item_to_list(item*& pin, item* pnew)
        {
            if (!pin)
            {
                pin = pnew->next = pnew->prev = pnew;
            }
            else
            {
                pnew->next = pin->next;
                pnew->prev = pin;
                pin->next->prev = pnew;
                pin->next = pnew;
            }
        }
        template<class item>
        void merge_lists(item*& plist, item* pnew)
        {
            if (!pnew)
            {
                return;
            }
            if (!plist)
            {
                plist = pnew;
            }
            else
            {
                plist->next->prev = pnew->prev;
                pnew->prev->next = plist->next;
                plist->next = pnew;
                pnew->prev = plist;
            }
        }

        struct Vec3mem
            : Vec3
        {
            Vec3 operator-(const Vec3& op) const { return sub(op); }
            float operator*(const Vec3& op) const { return dot(op); }
            Vec3 operator^(const Vec3& op) const { return cross(op); }
        };

        void associate_ptlist_with_trilist(ptitem* ptlist, qhtritem* trilist, ptitem* pt0, strided_pointer<Vec3mem> pvtx);

        inline void swap(int* v, void** p, int i1, int i2)
        {
            int ti = v[i1];
            v[i1] = v[i2];
            v[i2] = ti;
            void* tp = p[i1];
            p[i1] = p[i2];
            p[i2] = tp;
        }

        void qsort(int* v, void** p, int left, int right);

        int bin_search(int* v, int n, int idx);

        int qhull(strided_pointer<Vec3> _pts, int npts, index_t*& pTris, qhullmalloc qmalloc);

    } // namespace qhull_IMPL

    int qhull(strided_pointer<Vec3> _pts, int npts, index_t*& pTris, qhullmalloc qmalloc);

    namespace TriangulatePoly_IMPL
    {
        static int g_bSaferBooleans = 1;
        static int g_nTriangulationErrors;
        static int g_bBruteforceTriangulation = 0;

        struct vtxthunk
        {
            vtxthunk* next[2];
            vtxthunk* jump;
            vector2df* pt;
            int bProcessed;
        };

        int TriangulatePolyBruteforce(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf);

        int TriangulatePoly(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf);

    } // namespace TriangulatePoly_IMPL

    int TriangulatePoly(vector2df* pVtx, int nVtx, int* pTris, int szTriBuf);

    namespace polynomial_tpl_IMPL
    {
        template<class ftype, int degree>
        class polynomial_tpl
        {
        public:
            explicit polynomial_tpl() { denom = (ftype)1; };
            explicit polynomial_tpl(ftype op) { zero(); data[degree] = op; }
            AZ_FORCE_INLINE polynomial_tpl& zero()
            {
                for (int i = 0; i <= degree; i++)
                {
                    data[i] = 0;
                }
                denom = (ftype)1;
                return *this;
            }
            polynomial_tpl(const polynomial_tpl<ftype, degree>& src) { *this = src; }
            polynomial_tpl& operator=(const polynomial_tpl<ftype, degree>& src)
            {
                denom = src.denom;
                for (int i = 0; i <= degree; i++)
                {
                    data[i] = src.data[i];
                }
                return *this;
            }
            template<int degree1>
            AZ_FORCE_INLINE polynomial_tpl& operator=(const polynomial_tpl<ftype, degree1>& src)
            {
                int i;
                denom = src.denom;
                for (i = 0; i <= min(degree, degree1); i++)
                {
                    data[i] = src.data[i];
                }
                for (; i < degree; i++)
                {
                    data[i] = 0;
                }
                return *this;
            }
            AZ_FORCE_INLINE polynomial_tpl& set(ftype* pdata)
            {
                for (int i = 0; i <= degree; i++)
                {
                    data[degree - i] = pdata[i];
                }
                return *this;
            }

            AZ_FORCE_INLINE ftype& operator[](int idx) { return data[idx]; }

            void calc_deriviative(polynomial_tpl<ftype, degree>& deriv, int curdegree = degree) const;

            AZ_FORCE_INLINE polynomial_tpl& fixsign()
            {
                ftype sg = sgnnz(denom);
                denom *= sg;
                for (int i = 0; i <= degree; i++)
                {
                    data[i] *= sg;
                }
                return *this;
            }

            int findroots(ftype start, ftype end, ftype* proots, int nIters = 20, int curdegree = degree, bool noDegreeCheck = false) const;
            int nroots(ftype start, ftype end) const;

            AZ_FORCE_INLINE ftype eval(ftype x) const
            {
                ftype res = 0;
                for (int i = degree; i >= 0; i--)
                {
                    res = res * x + data[i];
                }
                return res;
            }
            AZ_FORCE_INLINE ftype eval(ftype x, int subdegree) const
            {
                ftype res = data[subdegree];
                for (int i = subdegree - 1; i >= 0; i--)
                {
                    res = res * x + data[i];
                }
                return res;
            }

            AZ_FORCE_INLINE polynomial_tpl& operator+=(ftype op) { data[0] += op * denom; return *this; }
            AZ_FORCE_INLINE polynomial_tpl& operator-=(ftype op) { data[0] -= op * denom; return *this; }
            AZ_FORCE_INLINE polynomial_tpl operator*(ftype op) const
            {
                polynomial_tpl<ftype, degree> res;
                res.denom = denom;
                for (int i = 0; i <= degree; i++)
                {
                    res.data[i] = data[i] * op;
                }
                return res;
            }
            AZ_FORCE_INLINE polynomial_tpl& operator*=(ftype op)
            {
                for (int i = 0; i <= degree; i++)
                {
                    data[i] *= op;
                }
                return *this;
            }
            AZ_FORCE_INLINE polynomial_tpl operator/(ftype op) const
            {
                polynomial_tpl<ftype, degree> res = *this;
                res.denom = denom * op;
                return res;
            }
            AZ_FORCE_INLINE polynomial_tpl& operator/=(ftype op) { denom *= op;   return *this; }

            AZ_FORCE_INLINE polynomial_tpl<ftype, degree * 2> sqr() const { return *this * *this; }

            ftype denom;
            ftype data[degree + 1];
        };

        template <class ftype>
        struct tagPolyE
        {
            inline static ftype polye() { return (ftype)1E-10; }
        };

        template<>
        inline float tagPolyE<float>::polye() { return 1e-6f; }

        template <class ftype>
        inline ftype polye() { return tagPolyE<ftype>::polye(); }

        // Don't use this macro; use AZStd::max instead. This is only here to make the template const arguments below readable
        // and because Visual Studio 2013 doesn't have a const_expr version of std::max
        #define deprecated_degmax(degree1, degree2) (((degree1) > (degree2)) ? (degree1) : (degree2))

        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator+(const polynomial_tpl<ftype, degree>& pn, ftype op)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] += op * res.denom;
            return res;
        }
        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator-(const polynomial_tpl<ftype, degree>& pn, ftype op)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] -= op * res.denom;
            return res;
        }

        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator+(ftype op, const polynomial_tpl<ftype, degree>& pn)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] += op * res.denom;
            return res;
        }
        template<class ftype, int degree>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree> operator-(ftype op, const polynomial_tpl<ftype, degree>& pn)
        {
            polynomial_tpl<ftype, degree> res = pn;
            res.data[0] -= op * res.denom;
            for (int i = 0; i <= degree; i++)
            {
                res.data[i] = -res.data[i];
            }
            return res;
        }
        template<class ftype, int degree>
        polynomial_tpl<ftype, degree * 2> AZ_FORCE_INLINE psqr(const polynomial_tpl<ftype, degree>& op) { return op * op; }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> operator+(const polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> res;
            int i;
            for (i = 0; i <= min(degree1, degree2); i++)
            {
                res.data[i] = op1.data[i] * op2.denom + op2.data[i] * op1.denom;
            }
            for (; i <= degree1; i++)
            {
                res.data[i] = op1.data[i] * op2.denom;
            }
            for (; i <= degree2; i++)
            {
                res.data[i] = op2.data[i] * op1.denom;
            }
            res.denom = op1.denom * op2.denom;
            return res;
        }
        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> operator-(const polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            polynomial_tpl<ftype, deprecated_degmax(degree1, degree2)> res;
            int i;
            for (i = 0; i <= min(degree1, degree2); i++)
            {
                res.data[i] = op1.data[i] * op2.denom - op2.data[i] * op1.denom;
            }
            for (; i <= degree1; i++)
            {
                res.data[i] = op1.data[i] * op2.denom;
            }
            for (; i <= degree2; i++)
            {
                res.data[i] = op2.data[i] * op1.denom;
            }
            res.denom = op1.denom * op2.denom;
            return res;
        }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1>& operator+=(polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            for (int i = 0; i < min(degree1, degree2); i++)
            {
                op1.data[i] = op1.data[i] * op2.denom + op2.data[i] * op1.denom;
            }
            op1.denom *= op2.denom;
            return op1;
        }
        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1>& operator-=(polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            for (int i = 0; i < min(degree1, degree2); i++)
            {
                op1.data[i] = op1.data[i] * op2.denom - op2.data[i] * op1.denom;
            }
            op1.denom *= op2.denom;
            return op1;
        }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1 + degree2> operator*(const polynomial_tpl<ftype, degree1>& op1, const polynomial_tpl<ftype, degree2>& op2)
        {
            polynomial_tpl<ftype, degree1 + degree2> res;
            res.zero();
            int j;
            switch (degree1)
            {
            case 8:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[8 + j] += op1.data[8] * op2.data[j];
                }
            case 7:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[7 + j] += op1.data[7] * op2.data[j];
                }
            case 6:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[6 + j] += op1.data[6] * op2.data[j];
                }
            case 5:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[5 + j] += op1.data[5] * op2.data[j];
                }
            case 4:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[4 + j] += op1.data[4] * op2.data[j];
                }
            case 3:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[3 + j] += op1.data[3] * op2.data[j];
                }
            case 2:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[2 + j] += op1.data[2] * op2.data[j];
                }
            case 1:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[1 + j] += op1.data[1] * op2.data[j];
                }
            case 0:
                for (j = 0; j <= degree2; j++)
                {
                    res.data[0 + j] += op1.data[0] * op2.data[j];
                }
            }
            res.denom = op1.denom * op2.denom;
            return res;
        }


        template <class ftype>
        AZ_FORCE_INLINE void polynomial_divide(const polynomial_tpl<ftype, 8>& num, const polynomial_tpl<ftype, 8>& den, polynomial_tpl<ftype, 8>& quot,
            polynomial_tpl<ftype, 8>& rem, int degree1, int degree2)
        {
            int i, j, k, l;
            ftype maxel;
            for (i = 0; i <= degree1; i++)
            {
                rem.data[i] = num.data[i];
            }
            for (i = 0; i <= degree1 - degree2; i++)
            {
                quot.data[i] = 0;
            }
            for (i = 1, maxel = fabs_tpl(num.data[0]); i <= degree1; i++)
            {
                maxel = max(maxel, num.data[i]);
            }
            for (maxel *= polye<ftype>(); degree1 >= 0 && fabs_tpl(num.data[degree1]) < maxel; degree1--)
            {
                ;
            }
            for (i = 1, maxel = fabs_tpl(den.data[0]); i <= degree2; i++)
            {
                maxel = max(maxel, den.data[i]);
            }
            for (maxel *= polye<ftype>(); degree2 >= 0 && fabs_tpl(den.data[degree2]) < maxel; degree2--)
            {
                ;
            }
            rem.denom = num.denom;
            quot.denom = (ftype)1;
            if (degree1 < 0 || degree2 < 0)
            {
                return;
            }

            for (k = degree1 - degree2, l = degree1; l >= degree2; l--, k--)
            {
                quot.data[k] = rem.data[l] * den.denom;
                quot.denom *= den.data[degree2];
                for (i = degree1 - degree2; i > k; i--)
                {
                    quot.data[i] *= den.data[degree2];
                }
                for (i = degree2 - 1, j = l - 1; i >= 0; i--, j--)
                {
                    rem.data[j] = rem.data[j] * den.data[degree2] - den.data[i] * rem.data[l];
                }
                for (; j >= 0; j--)
                {
                    rem.data[j] *= den.data[degree2];
                }
                rem.denom *= den.data[degree2];
            }
        }

        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree1 - degree2> operator/(const polynomial_tpl<ftype, degree1>& num, const polynomial_tpl<ftype, degree2>& den)
        {
            polynomial_tpl<ftype, degree1 - degree2> quot;
            polynomial_tpl<ftype, degree1> rem;
            polynomial_divide((polynomial_tpl<ftype, 8>&)num, (polynomial_tpl<ftype, 8>&)den, (polynomial_tpl<ftype, 8>&)quot,
                (polynomial_tpl<ftype, 8>&)rem, degree1, degree2);
            return quot;
        }
        template <class ftype, int degree1, int degree2>
        AZ_FORCE_INLINE polynomial_tpl<ftype, degree2 - 1> operator%(const polynomial_tpl<ftype, degree1>& num, const polynomial_tpl<ftype, degree2>& den)
        {
            polynomial_tpl<ftype, degree1 - degree2> quot;
            polynomial_tpl<ftype, degree1> rem;
            polynomial_divide((polynomial_tpl<ftype, 8>&)num, (polynomial_tpl<ftype, 8>&)den, (polynomial_tpl<ftype, 8>&)quot,
                (polynomial_tpl<ftype, 8>&)rem, degree1, degree2);
            return (polynomial_tpl<ftype, degree2 - 1>&)rem;
        }

        template <class ftype, int degree>
        AZ_FORCE_INLINE void polynomial_tpl<ftype, degree>::calc_deriviative(polynomial_tpl<ftype, degree>& deriv, int curdegree) const
        {
            for (int i = 0; i < curdegree; i++)
            {
                deriv.data[i] = data[i + 1] * (i + 1);
            }
            deriv.denom = denom;
        }

        template<typename to_t, typename from_t>
        to_t* convert_type(from_t* input)
        {
            typedef union
            {
                to_t* to;
                from_t* from;
            } convert_union;
            convert_union u;
            u.from = input;
            return u.to;
        }

        template <class ftype, int degree>
        AZ_FORCE_INLINE int polynomial_tpl<ftype, degree>::nroots(ftype start, ftype end) const
        {
            polynomial_tpl<ftype, degree> f[degree + 1];
            int i, j, sg_a, sg_b;
            ftype val, prevval;

            calc_deriviative(f[0]);
            polynomial_divide(*convert_type<polynomial_tpl<ftype, 8> >(this), *convert_type< polynomial_tpl<ftype, 8> >(&f[0]), *convert_type<polynomial_tpl<ftype, 8> >(&f[degree]),
                *convert_type<polynomial_tpl<ftype, 8> >(&f[1]), degree, degree - 1);
            f[1].denom = -f[1].denom;
            for (i = 2; i < degree; i++)
            {
                polynomial_divide(*convert_type<polynomial_tpl<ftype, 8> >(&f[i - 2]), *convert_type<polynomial_tpl<ftype, 8> >(&f[i - 1]), *convert_type<polynomial_tpl<ftype, 8> >(&f[degree]),
                    *convert_type<polynomial_tpl<ftype, 8> >(&f[i]), degree + 1 - i, degree - i);
                f[i].denom = -f[i].denom;
                if (fabs_tpl(f[i].denom) > (ftype)1E10)
                {
                    for (j = 0; j <= degree - 1 - i; j++)
                    {
                        f[i].data[j] *= (ftype)1E-10;
                    }
                    f[i].denom *= (ftype)1E-10;
                }
            }

            prevval = eval(start) * denom;
            for (i = sg_a = 0; i < degree; i++, prevval = val)
            {
                val = f[i].eval(start, degree - 1 - i) * f[i].denom;
                sg_a += isneg(val * prevval);
            }

            prevval = eval(end) * denom;
            for (i = sg_b = 0; i < degree; i++, prevval = val)
            {
                val = f[i].eval(end, degree - 1 - i) * f[i].denom;
                sg_b += isneg(val * prevval);
            }

            return fabs_tpl(sg_a - sg_b);
        }

        template<class ftype>
        AZ_FORCE_INLINE ftype cubert_tpl(ftype x) { return fabs_tpl(x) > (ftype)1E-20 ? exp_tpl(log_tpl(fabs_tpl(x)) * (ftype)(1.0 / 3)) * sgnnz(x) : x; }
        template<class ftype>
        AZ_FORCE_INLINE ftype pow_tpl(ftype x, ftype pow) { return fabs_tpl(x) > (ftype)1E-20 ? exp_tpl(log_tpl(fabs_tpl(x)) * pow) * sgnnz(x) : x; }
        template<class ftype>
        AZ_FORCE_INLINE void swap(ftype* ptr, int i, int j) { ftype t = ptr[i]; ptr[i] = ptr[j]; ptr[j] = t; }

        template <class ftype, int maxdegree>
        int polynomial_tpl<ftype, maxdegree>::findroots(ftype start, ftype end, ftype* proots, [[maybe_unused]] int nIters, int degree, bool noDegreeCheck) const
        {
            AZ_UNUSED(nIters);
            int i, j, nRoots = 0;
            ftype maxel;
            if (!noDegreeCheck)
            {
                for (i = 1, maxel = fabs_tpl(data[0]); i <= degree; i++)
                {
                    maxel = max(maxel, data[i]);
                }
                for (maxel *= polye<ftype>(); degree > 0 && fabs_tpl(data[degree]) <= maxel; degree--)
                {
                    ;
                }
            }

            if constexpr (maxdegree >= 1)
            {
                if (degree == 1)
                {
                    proots[0] = data[0] / data[1];
                    nRoots = 1;
                }
            }

            if constexpr (maxdegree >= 2)
            {
                if (degree == 2)
                {
                    ftype a, b, c, d, bound[2], sg;

                    a = data[2];
                    b = data[1];
                    c = data[0];
                    d = aznumeric_cast<ftype>(sgnnz(a));
                    a *= d;
                    b *= d;
                    c *= d;
                    d = b * b - a * c * 4;
                    bound[0] = start * a * 2 + b;
                    bound[1] = end * a * 2 + b;
                    sg = aznumeric_cast<ftype>((sgnnz(bound[0] * bound[1]) + 1) >> 1);
                    bound[0] *= bound[0];
                    bound[1] *= bound[1];
                    bound[isneg(fabs_tpl(bound[1]) - fabs_tpl(bound[0]))] *= sg;

                    if (isnonneg(d) & inrange(d, bound[0], bound[1]))
                    {
                        d = sqrt_tpl(d);
                        a = (ftype)0.5 / a;
                        proots[nRoots] = (-b - d) * a;
                        nRoots += inrange(proots[nRoots], start, end);
                        proots[nRoots] = (-b + d) * a;
                        nRoots += inrange(proots[nRoots], start, end);
                    }
                }
            }

            if constexpr (maxdegree >= 3)
            {
                if (degree == 3)
                {
                    ftype t, a, b, c, a3, p, q, Q, Qr, Ar, Ai, phi;

                    t = (ftype)1.0 / data[3];
                    a = data[2] * t;
                    b = data[1] * t;
                    c = data[0] * t;
                    a3 = a * (ftype)(1.0 / 3);
                    p = b - a * a3;
                    q = (a3 * b - c) * (ftype)0.5 - cube(a3);
                    Q = cube(p * (ftype)(1.0 / 3)) + q * q;
                    Qr = sqrt_tpl(fabs_tpl(Q));

                    if (Q > 0)
                    {
                        proots[0] = cubert_tpl(q + Qr) + cubert_tpl(q - Qr) - a3;
                        nRoots = 1;
                    }
                    else
                    {
                        phi = atan2_tpl(Qr, q) * (ftype)(1.0 / 3);
                        t = pow_tpl(Qr * Qr + q * q, (ftype)(1.0 / 6));
                        Ar = t * cos_tpl(phi);
                        Ai = t * sin_tpl(phi);
                        proots[0] = 2 * Ar - a3;
                        proots[1] = aznumeric_cast<ftype>(-Ar + Ai * sqrt3 - a3);
                        proots[2] = aznumeric_cast<ftype>(-Ar - Ai * sqrt3 - a3);
                        i = idxmax3(proots);
                        swap(proots, i, 2);
                        i = isneg(proots[0] - proots[1]);
                        swap(proots, i, 1);
                        nRoots = 3;
                    }
                }
            }

            if constexpr (maxdegree >= 4)
            {
                if (degree == 4)
                {
                    ftype t, a3, a2, a1, a0, y, R, D, E, subroots[3];
                    const ftype e = (ftype)1E-9;

                    t = (ftype)1.0 / data[4];
                    a3 = data[3] * t;
                    a2 = data[2] * t;
                    a1 = data[1] * t;
                    a0 = data[0] * t;
                    polynomial_tpl<ftype, 3> p3aux;
                    ftype kp3aux[] = { 1, -a2, a1 * a3 - 4 * a0, 4 * a2 * a0 - a1 * a1 - a3 * a3 * a0 };
                    p3aux.set(kp3aux);
                    if (!p3aux.findroots((ftype)-1E20, (ftype)1E20, subroots))
                    {
                        return 0;
                    }
                    R = a3 * a3 * (ftype)0.25 - a2 + (y = subroots[0]);

                    if (R > -e)
                    {
                        if (R < e)
                        {
                            D = E = a3 * a3 * (ftype)(3.0 / 4) - 2 * a2;
                            t = y * y - 4 * a0;
                            if (t < -e)
                            {
                                return 0;
                            }
                            t = 2 * sqrt_tpl(max((ftype)0, t));
                        }
                        else
                        {
                            R = sqrt_tpl(max((ftype)0, R));
                            D = E = a3 * a3 * (ftype)(3.0 / 4) - R * R - 2 * a2;
                            t = (4 * a3 * a2 - 8 * a1 - a3 * a3 * a3) / R * (ftype)0.25;
                        }
                        if (D + t > -e)
                        {
                            D = sqrt_tpl(max((ftype)0, D + t));
                            proots[nRoots++] = a3 * (ftype)-0.25 + (R - D) * (ftype)0.5;
                            proots[nRoots++] = a3 * (ftype)-0.25 + (R + D) * (ftype)0.5;
                        }
                        if (E - t > -e)
                        {
                            E = sqrt_tpl(max((ftype)0, E - t));
                            proots[nRoots++] = a3 * (ftype)-0.25 - (R + E) * (ftype)0.5;
                            proots[nRoots++] = a3 * (ftype)-0.25 - (R - E) * (ftype)0.5;
                        }
                        if (nRoots == 4)
                        {
                            i = idxmax3(proots);
                            if (proots[3] < proots[i])
                            {
                                swap(proots, i, 3);
                            }
                            i = idxmax3(proots);
                            swap(proots, i, 2);
                            i = isneg(proots[0] - proots[1]);
                            swap(proots, i, 1);
                        }
                    }
                }
            }

            if constexpr (maxdegree > 4)
            {
                if (degree > 4)
                {
                    ftype roots[maxdegree + 1], prevroot, val, prevval[2], curval, bound[2], middle;
                    polynomial_tpl<ftype, maxdegree> deriv;
                    int nExtremes, iter, iBound;
                    calc_deriviative(deriv);

                    // find a subset of deriviative extremes between start and end
                    for (nExtremes = deriv.findroots(start, end, roots + 1, nIters, degree - 1) + 1; nExtremes > 1 && roots[nExtremes - 1] > end; nExtremes--)
                    {
                        ;
                    }
                    for (i = 1; i < nExtremes && roots[i] < start; i++)
                    {
                        ;
                    }
                    roots[i - 1] = start;
                    PREFAST_ASSUME(nExtremes < maxdegree + 1);
                    roots[nExtremes++] = end;

                    for (prevroot = start, prevval[0] = eval(start, degree), nRoots = 0; i < nExtremes; prevval[0] = val, prevroot = roots[i++])
                    {
                        val = eval(roots[i], degree);
                        if (val * prevval[0] < 0)
                        {
                            // we have exactly one root between prevroot and roots[i]
                            bound[0] = prevroot;
                            bound[1] = roots[i];
                            iter = 0;
                            do
                            {
                                middle = (bound[0] + bound[1]) * (ftype)0.5;
                                curval = eval(middle, degree);
                                iBound = isneg(prevval[0] * curval);
                                bound[iBound] = middle;
                                prevval[iBound] = curval;
                            } while (++iter < nIters);
                            proots[nRoots++] = middle;
                        }
                    }
                }
            }

            for (i = 0; i < nRoots && proots[i] < start; i++)
            {
                ;
            }
            for (; nRoots > i&& proots[nRoots - 1] > end; nRoots--)
            {
                ;
            }
            for (j = i; j < nRoots; j++)
            {
                proots[j - i] = proots[j];
            }

            return nRoots - i;
        }
    } // namespace polynomial_tpl_IMPL
    template<class ftype, int degree>
    using polynomial_tpl = polynomial_tpl_IMPL::polynomial_tpl<ftype, degree>;

    typedef polynomial_tpl<real, 3> P3;
    typedef polynomial_tpl<real, 2> P2;
    typedef polynomial_tpl<real, 1> P1;
    typedef polynomial_tpl<float, 3> P3f;
    typedef polynomial_tpl<float, 2> P2f;
    typedef polynomial_tpl<float, 1> P1f;
} // namespace LegacyCryPhysicsUtils
