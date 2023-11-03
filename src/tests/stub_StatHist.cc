/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#include "squid.h"

#define STUB_API "StatHist.cc"
#include "tests/STUB.h"

#include "StatHist.h"
StatHist::StatHist(const StatHist &) STUB_NOP
double StatHist::deltaPctile(const StatHist &, double) const STUB_RETVAL(0.0)
double StatHist::val(unsigned int) const STUB_RETVAL(0.0)
void StatHist::count(double) STUB_NOP
void StatHist::dump(StoreEntry *, StatHistBinDumper *) const STUB
void StatHist::logInit(unsigned int, double, double) STUB_NOP
void StatHist::enumInit(unsigned int) STUB_NOP
StatHist &StatHist::operator += (const StatHist &) STUB_RETREF(StatHist)
/*protected:
    void StatHist::init(unsigned int, hbase_f *, hbase_f *, double, double) STUB
    unsigned int StatHist::findBin(double) STUB_RETVAL(0)
*/
double statHistDeltaMedian(const StatHist &, const StatHist &) STUB_RETVAL(0.0)
double statHistDeltaPctile(const StatHist &, const StatHist &, double) STUB_RETVAL(0.0)
