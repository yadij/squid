/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_BASE_CBCPOINTER_H
#define SQUID_SRC_BASE_CBCPOINTER_H

#include "base/TextException.h"
#include "cbdata.h"
#include "debug/Stream.h"

/**
 \ingroup CBDATAAPI
 *
 * Safely points to a cbdata-protected class (cbc), such as an AsyncJob.
 * When a cbc we communicate with disappears without
 * notice or a notice has not reached us yet, this class prevents
 * dereferencing the pointer to the gone cbc object.
 */
template<class Cbc>
class CbcPointer
{
public:
    CbcPointer(); // a nil pointer
    CbcPointer(Cbc *aCbc);
    CbcPointer(const CbcPointer &p);
    CbcPointer(CbcPointer &&);
    ~CbcPointer();

    Cbc *raw() const; ///< a temporary raw Cbc pointer; may be invalid
    Cbc *get() const; ///< a temporary valid raw Cbc pointer or NULL
    Cbc &operator *() const; ///< a valid Cbc reference or exception
    Cbc *operator ->() const; ///< a valid Cbc pointer or exception

    // no bool operator because set() != valid()
    bool set() const { return cbc != nullptr; } ///< was set but may be invalid
    Cbc *valid() const { return get(); } ///< was set and is valid
    bool operator !() const { return !valid(); } ///< invalid or was not set
    bool operator ==(const CbcPointer<Cbc> &o) const { return lock == o.lock; }

    CbcPointer &operator =(const CbcPointer &p);
    CbcPointer &operator =(CbcPointer &&);

    /// support converting a child cbc pointer into a parent cbc pointer
    template <typename Other>
    CbcPointer(const CbcPointer<Other> &o): cbc(o.raw()), lock(nullptr) {
        if (o.valid())
            lock = cbdataReference(o->toCbdata());
    }

    /// support assigning a child cbc pointer to a parent cbc pointer
    template <typename Other>
    CbcPointer &operator =(const CbcPointer<Other> &o) {
        if (this != &o) { // assignment to self
            clear();
            cbc = o.raw(); // so that set() is accurate
            if (o.valid())
                lock = cbdataReference(o->toCbdata());
        }
        return *this;
    }

    void clear(); ///< make pointer not set; does not invalidate cbdata

    std::ostream &print(std::ostream &os) const;

private:
    Cbc *cbc; // a possibly invalid pointer to a cbdata class
    void *lock; // a valid pointer to cbc's cbdata or nil
};

template <class Cbc>
inline
std::ostream &operator <<(std::ostream &os, const CbcPointer<Cbc> &p)
{
    return p.print(os);
}

// inlined methods

template<class Cbc>
CbcPointer<Cbc>::CbcPointer(): cbc(nullptr), lock(nullptr)
{
}

template<class Cbc>
CbcPointer<Cbc>::CbcPointer(Cbc *aCbc): cbc(aCbc), lock(nullptr)
{
    if (cbc)
        lock = cbdataReference(cbc->toCbdata());
}

template<class Cbc>
CbcPointer<Cbc>::CbcPointer(const CbcPointer &d): cbc(d.cbc), lock(nullptr)
{
    if (d.lock && cbdataReferenceValid(d.lock))
        lock = cbdataReference(d.lock);
}

template<class Cbc>
CbcPointer<Cbc>::CbcPointer(CbcPointer &&d): cbc(d.cbc), lock(d.lock)
{
    d.cbc = nullptr;
    d.lock = nullptr;
}

template<class Cbc>
CbcPointer<Cbc>::~CbcPointer()
{
    clear();
}

template<class Cbc>
CbcPointer<Cbc> &CbcPointer<Cbc>::operator =(const CbcPointer &d)
{
    if (this != &d) { // assignment to self
        clear();
        cbc = d.cbc;
        if (d.lock && cbdataReferenceValid(d.lock))
            lock = cbdataReference(d.lock);
    }
    return *this;
}

template<class Cbc>
CbcPointer<Cbc> &CbcPointer<Cbc>::operator =(CbcPointer &&d)
{
    if (this != &d) { // assignment to self
        clear();
        cbc = d.cbc;
        d.cbc = nullptr;
        lock = d.lock;
        d.lock = nullptr;
    }
    return *this;
}

template<class Cbc>
void
CbcPointer<Cbc>::clear()
{
    cbdataReferenceDone(lock); // lock may be nil before and will be nil after
    cbc = nullptr;
}

template<class Cbc>
Cbc *
CbcPointer<Cbc>::raw() const
{
    return cbc;
}

template<class Cbc>
Cbc *
CbcPointer<Cbc>::get() const
{
    return (lock && cbdataReferenceValid(lock)) ? cbc : nullptr;
}

template<class Cbc>
Cbc &
CbcPointer<Cbc>::operator *() const
{
    Cbc *c = get();
    assert(c);
    return *c;
}

template<class Cbc>
Cbc *
CbcPointer<Cbc>::operator ->() const
{
    Cbc *c = get();
    assert(c);
    return c;
}

template <class Cbc>
std::ostream &CbcPointer<Cbc>::print(std::ostream &os) const
{
    return os << cbc << '/' << lock;
}

#endif /* SQUID_SRC_BASE_CBCPOINTER_H */

