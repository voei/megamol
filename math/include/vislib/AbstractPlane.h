/*
 * AbstractPlane.h
 *
 * Copyright (C) 2006 by Universitaet Stuttgart (VIS). Alle Rechte vorbehalten.
 */

#ifndef VISLIB_ABSTRACTPLANE_H_INCLUDED
#define VISLIB_ABSTRACTPLANE_H_INCLUDED
#if (_MSC_VER > 1000)
#pragma once
#endif /* (_MSC_VER > 1000) */


#include "vislib/assert.h"
#include "vislib/mathfunctions.h"
#include "vislib/Point3D.h"
#include "vislib/types.h"
#include "vislib/Vector3D.h"


namespace vislib {
namespace math {


    /**
     * This is the abstract superclass for planes and shallow planes.
     */
    template<class T, class S> class AbstractPlane {

    public:

        /** Enumeration for halfspaces of the plane. */
        enum Halfspace { 
            NEGATIVE_HALFSPACE = -1,    // Halfspace opposite of normal.
            IN_PLANE = 0,               // No halfspace, but plane itself.
            POSITIVE_HALFSPACE = 1      // Halfspace in normal direction.
        };

        /** Result for intersection tests. */
        enum IntersectionCount { 
            NONE = 0,                   // No intersection.
            ONE,                        // Exactly one intersection.
            ALL                         // All points intersect.
        };

        /** Dtor. */
        ~AbstractPlane(void);

        /**
         * Answer the parameter a in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter a.
         */
        inline const T& A(void) const {
            return this->parameters[IDX_A];
        }

        /**
         * Answer the parameter b in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter b.
         */
        inline const T& B(void) const {
            return this->parameters[IDX_B];
        }

        /**
         * Answer the parameter c in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter c.
         */
        inline const T& C(void) const {
            return this->parameters[IDX_C];
        }

        /**
         * Answer whether 'point' is in the plane.
         *
         * @param point The point to be tested.
         *
         * @return true, if 'point' satisfies the plane equation, false 
         *         otherwise.
         */
        template<class Tp, class Sp>
        inline bool Contains(const AbstractPoint3D<Tp, Sp>& point) const {
            return IsEqual<T>(this->Distance(point), static_cast<T>(0));
        }
        

        /**
         * Answer the parameter d in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter d.
         */
        inline const T& D(void) const {
            return this->parameters[IDX_D];
        }

        /**
         * Answer the distance of 'point' to the plane.
         *
         * @param point The point to compute the distance of.
         *
         * @return The distance of 'point' to the plane.
         */
        template<class Tp, class Sp>
        T Distance(const AbstractPoint3D<Tp, Sp>& point) const;

        /**
         * Answer the parameter a in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter a.
         */
        inline const T& GetA(void) const {
            return this->parameters[IDX_A];
        }

        /**
         * Answer the parameter b in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter b.
         */
        inline const T& GetB(void) const {
            return this->parameters[IDX_B];
        }

        /**
         * Answer the parameter c in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter c.
         */
        inline const T& GetC(void) const {
            return this->parameters[IDX_C];
        }

        /**
         * Answer the parameter d in the plane equation ax + by + cz + d = 0.
         *
         * @param The parameter d.
         */
        inline const T& GetD(void) const {
            return this->parameters[IDX_D];
        }

        /**
         * Answer in which halfspace 'point' lies in respect to the plane.
         *
         * @param point The point to be tested.
         *
         * @return The halfspace the point lies in.
         */
        template<class Tp, class Sp>
        Halfspace InHalfspace(const AbstractPoint3D<Tp, Sp>& point);

        //template<class Tp, class Sp1, class Sp2>
        //IntersectionCount Intersect(const Line3D<Tp, Sp1>& line, AbstractPoint3D<T, Sp2>& outWhere) const;

        /**
         * Answer the normal of the plane. The vector returned is guaranteed to
         * be normalised.
         *
         * @return The normal of the plane.
         */
        inline Vector3D<T> Normal(void) const {
            Vector3D<T> retval(this->parameters);
            retval.Normalise();
            return retval;
        }

        /**
         * Normalise the plane parameters.
         */
        void Normalise(void) const;

        /**
         * Answer a point in the plane.
         *
         * @return A point in the plane.
         */
        Point3D<T> Point(void) const;

        /**
         * Answer three points in the plane.
         *
         * @param outP1 Receives the first point.
         * @param outP2 Receives the second point.
         * @param outP3 Receives the third point.
         */
        void Points(Point3D<T>& outP1, Point3D<T>& outP2, 
            Point3D<T>& outP3) const;

        /** 
         * Change the plane equation.
         *
         * @param a The parameter a in the equation ax + by + cz + d = 0.
         * @param b The parameter b in the equation ax + by + cz + d = 0.
         * @param c The parameter c in the equation ax + by + cz + d = 0.
         * @param d The parameter d in the equation ax + by + cz + d = 0.
         */
        inline void Set(const float a, const float b, const float c, 
                const float d) {
            this->parameters[IDX_A] = a;
            this->parameters[IDX_B] = b;
            this->parameters[IDX_C] = c;
            this->parameters[IDX_D] = d;
        }

        /**
         * Change the plane equation using a point on the plane and its formal.
         *
         * @param point  A point on the plane.
         * @param normal The plane normal
         */
        template<class Tp1, class Sp1, class Tp2, class Sp2>
        void Set(const AbstractPoint3D<Tp1, Sp1>& point, 
            const AbstractVector3D<Tp2, Sp2>& normal);

        /** 
         * Set the parameter a in the plan equation ax + by + cz + d = 0.
         *
         * @param a The parameter a.
         */
        inline void SetA(const T& a) {
            this->parameters[IDX_A] = a;
        }

        /** 
         * Set the parameter b in the plan equation ax + by + cz + d = 0.
         *
         * @param b The parameter b.
         */
        inline void SetB(const T& b) {
            this->parameters[IDX_B] = b;
        }

        /** 
         * Set the parameter c in the plan equation ax + by + cz + d = 0.
         *
         * @param c The parameter c.
         */
        inline void SetC(const T& c) {
            this->parameters[IDX_C] = c;
        }

        /** 
         * Set the parameter d in the plan equation ax + by + cz + d = 0.
         *
         * @param d The parameter d.
         */
        inline void SetD(const T& d) {
            this->parameters[IDX_D] = d;
        }

        /**
         * Assignment.
         *
         * @param rhs The right hand side operand.
         *
         * @return *this.
         */
        AbstractPlane& operator =(const AbstractPlane& rhs);

        /**
         * Assignment. This operator allows arbitrary plane to plane 
         * conversions.
         *
         * @param rhs The right hand side operand.
         *
         * @return *this.
         */
        template<class Tp, class Sp>
        AbstractPlane& operator =(const AbstractPlane<Tp, Sp>& rhs);

        /**
         * Test for equality. The IsEqual function is used for this.
         *
         * @param rhs The right hand side operand.
         *
         * @return true, if *this and 'rhs' are equal, false otherwise.
         */
        bool operator ==(const AbstractPlane& rhs) const;

        /**
         * Test for equality. This operator allows comparing planes that
         * have been instantiated for different scalar types. The IsEqual<T>
         * function for the scalar type of the left hand side operand is used
         * as comparison operator.
         *
         * @param rhs The right hand side operand.
         *
         * @return true, if *this and 'rhs' are equal, false otherwise.
         */
        template<class Tp, class Sp>
        bool operator ==(const AbstractPlane<Tp, Sp>& rhs) const;

        /**
         * Test for inequality. The IsEqual function is used for this.
         *
         * @param rhs The right hand side operand.
         *
         * @return true, if *this and 'rhs' are not equal, false otherwise.
         */
        inline bool operator !=(const AbstractPlane& rhs) const {
            return !(*this == rhs);
        }

        /**
         * Test for inequality. This operator allows comparing planes that
         * have been instantiated for different scalar types. The IsEqual<T>
         * function for the scalar type of the left hand side operand is used
         * as comparison operator.
         *
         * @param rhs The right hand side operand.
         *
         * @return true, if *this and 'rhs' are not equal, false otherwise.
         */
        template<class Tp, class Sp>
        inline bool operator !=(const AbstractPlane<Tp, Sp>& rhs) const {
            return !(*this == rhs);
        }


    protected:

        /** The index of the parameter a. */
        static const UINT_PTR IDX_A;

        /** The index of the parameter b. */
        static const UINT_PTR IDX_B;

        /** The index of the parameter c. */
        static const UINT_PTR IDX_C;

        /** The index of the parameter d. */
        static const UINT_PTR IDX_D;

        /** 
         * Disallow instances of this class.
         */
        inline AbstractPlane(void) {}

        /**
         * Answer the normalised plane parameters.
         *
         * @param outA Receives the normalised a parameter.
         * @param outB Receives the normalised b parameter.
         * @param outC Receives the normalised c parameter.
         * @param outD Receives the normalised d parameter.
         */
        void normalise(T& outA, T& outB, T& outC, T& outD) const;

        /** The parameters defining the plane. */
        S parameters;

    };


    /*
     * AbstractPlane<T, S>::~AbstractPlane
     */
    template<class T, class S> AbstractPlane<T, S>::~AbstractPlane(void) {
    }


    /*
     * AbstractPlane<T, S>::Distance
     */
    template<class T, class S>
    template<class Tp, class Sp>
    T AbstractPlane<T, S>::Distance(
            const AbstractPoint3D<Tp, Sp>& point) const {
        T a, b, c, d;           // Normalised plane parameters.
        
        this->normalise(a, b, c, d);
        return (a * static_cast<T>(point.X()) + b * static_cast<T>(point.Y())
            + c * static_cast<T>(point.Z()) + d);
    }


    /* 
     * AbstractPlane<T, S>::InHalfspace
     */
    template<class T, class S>
    template<class Tp, class Sp>
    typename AbstractPlane<T, S>::Halfspace AbstractPlane<T, S>::InHalfspace(
            const AbstractPoint3D<Tp, Sp>& point) {
        T dist = this->Distance(point);

        if (IsEqual(dist, static_cast<T>(0))) {
            return IN_PLANE;

        } else if (dist > static_cast<T>(0)) {
            return NEGATIVE_HALFSPACE;

        } else if (dist < static_cast<T>(0)) {
            return POSITIVE_HALFSPACE;
    
        } else {
            ASSERT(false);      // Should never happen.
            return IN_PLANE;
        }        
    }


    ///*
    // * AbstractPlane<T, S>::Intersect
    // */
    //template<class T, class S>
    //template<class Tp, class Sp1, class Sp2>
    //typename AbstractPlane<T, S>::IntersectCount AbstractPlane<T, S>::Intersect(
    //        const Line3D<Tp, Sp1>& line, 
    //        AbstractPoint3D<T, Sp2>& outWhere) const {
    //    T dx0 = this->parameters[IDX_A] * x0.X() 
    //        + this->parameters[IDX_B] * x0.Y() 
    //        + this->parameters[IDX_C] * x0.Z();
    //    T dx1 = this->parameters[IDX_A] * x1.X() 
    //        + this->parameters[IDX_B] * x1.Y() 
    //        + this->parameters[IDX_C] * x1.Z();
    //
    //    if (IsEqual(dx0, -this->d) && IsEqual(dx1, -this->d)) {
    //        outWhere = x0;
    //        return ALL;

    //    } else if (IsEqual(dx0, dx1)) {
    //        return NONE;

    //    } else {
    //        outWhere = x0 + (((dx0 + this->d) / (dx0 - dx1)) * (x1 - x0));
    //        return ONE;
    //    }
    //}


    /*
     * AbstractPlane<T, S>::Normalise
     */
    template<class T, class S>
    void AbstractPlane<T, S>::Normalise(void) const {
        T a, b, c, d;
        this->normalise(a, b, c, d);
        this->Set(a, b, c, d);
    }


    /*
     * AbstractPlane<T, S>::Point
     */
    template<class T, class S>
    Point3D<T> AbstractPlane<T, S>::Point(void) const {
        T a, b, c, d;
        this->normalise(a, b, c, d);
        return Point3D<T>(-d * a, -d * b, -d * c);
    }


    /*
     * AbstractPlane<T, S>::Points
     */
    template<class T, class S>
    void AbstractPlane<T, S>::Points(Point3D<T>& outP1, Point3D<T>& outP2,
            Point3D<T>& outP3) const {
        T a, b, c, d;
        this->normalise(a, b, c, d);
        
        outP1 = outP3 = Point3D<T>(-d * a, -d * b, -d * c);
        outP2 = Point3D<T>(outP1.X() - b, outP1.Y() + a, outP1.Z());
        outP3 += (outP2 - outP1).Cross(this->Normal());   

        ASSERT(this->Contains(outP1));
        ASSERT(this->Contains(outP2));
        ASSERT(this->Contains(outP3));
    }


    /*
     * AbstractPlane<T, S>::Set
     */
    template<class T, class S>
    template<class Tp1, class Sp1, class Tp2, class Sp2>
    void AbstractPlane<T, S>::Set(const AbstractPoint3D<Tp1, Sp1>& point, 
            const AbstractVector3D<Tp2, Sp2>& normal) {
        this->parameters[IDX_A] = normal.X();
        this->parameters[IDX_B] = normal.Y();
        this->parameters[IDX_C] = normal.Z();
        this->parameters[IDX_D] = -1.0f * (this->parameters[IDX_A] * point.X()
            + this->parameters[IDX_B] * point.Y() 
            + this->parameters[IDX_C] * point.Z());
    }

    /*
     * AbstractPlane<T, S>::operator =
     */
    template<class T, class S>
    AbstractPlane<T, S>& AbstractPlane<T, S>::operator =(
            const AbstractPlane& rhs) {
        if (this != &rhs) {
            ::memcpy(this->parameters, rhs.parameters, 4 * sizeof(T));
        }

        return *this;
    }


    /*
     * AbstractPlane<T, S>::operator =
     */
    template<class T, class S>
    template<class Tp, class Sp>
    AbstractPlane<T, S>& AbstractPlane<T, S>::operator =(
            const AbstractPlane<Tp, Sp>& rhs) {
        if (static_cast<void *>(this) != static_cast<void *>(&rhs)) {
            this->parameters[IDX_A] = static_cast<T>(rhs.A());
            this->parameters[IDX_B] = static_cast<T>(rhs.B());
            this->parameters[IDX_C] = static_cast<T>(rhs.C());
            this->parameters[IDX_D] = static_cast<T>(rhs.D());
        }

        return *this;
    }


    /*
     * AbstractPlane<T, S>::operator ==
     */
    template<class T, class S>
    bool AbstractPlane<T, S>::operator ==(const AbstractPlane& rhs) const {
        return (IsEqual(this->parameters[IDX_A], rhs.parameters[IDX_A])
            && IsEqual(this->parameters[IDX_B], rhs.parameters[IDX_B])
            && IsEqual(this->parameters[IDX_C], rhs.parameters[IDX_C])
            && IsEqual(this->parameters[IDX_D], rhs.parameters[IDX_D]));
    }


    /*
     * AbstractPlane<T, S>::operator ==
     */
    template<class T, class S>
    template<class Tp, class Sp>
    bool AbstractPlane<T, S>::operator ==(
            const AbstractPlane<Tp, Sp>& rhs) const {
        return (IsEqual<T>(this->parameters[IDX_A], rhs.parameters[IDX_A])
            && IsEqual<T>(this->parameters[IDX_B], rhs.parameters[IDX_B])
            && IsEqual<T>(this->parameters[IDX_C], rhs.parameters[IDX_C])
            && IsEqual<T>(this->parameters[IDX_D], rhs.parameters[IDX_D]));
    }


    /*
     * vislib::math::AbstractPlane<T, S>::IDX_A
     */
    template<class T, class S> 
    const UINT_PTR AbstractPlane<T, S>::IDX_A = 0;


    /*
     * vislib::math::AbstractPlane<T, S>::IDX_B
     */
    template<class T, class S> 
    const UINT_PTR AbstractPlane<T, S>::IDX_B = 1;


    /*
     * vislib::math::AbstractPlane<T, S>::IDX_C
     */
    template<class T, class S> 
    const UINT_PTR AbstractPlane<T, S>::IDX_C = 2;


    /*
     * vislib::math::AbstractPlane<T, S>::IDX_D
     */
    template<class T, class S> 
    const UINT_PTR AbstractPlane<T, S>::IDX_D = 3;


    /*
     * AbstractPlane<T, S>::normalise
     */
    template<class T, class S> 
    void AbstractPlane<T, S>::normalise(T& outA, T& outB, T& outC, 
            T& outD) const {
        T len = Sqrt(Sqr(this->parameters[IDX_A]) 
            + Sqr(this->parameters[IDX_B]) + Sqr(this->parameters[IDX_C]));

        if (!IsEqual(len, static_cast<T>(0))) {
            outA = this->parameters[IDX_A] / len;
            outB = this->parameters[IDX_B] / len;
            outC = this->parameters[IDX_C] / len;
            outD = this->parameters[IDX_D] / len;
        } else {
            outA = outB = outC = outD = static_cast<T>(0);
        }        
    }

} /* end namespace math */
} /* end namespace vislib */

#endif /* VISLIB_ABSTRACTPLANE_H_INCLUDED */
