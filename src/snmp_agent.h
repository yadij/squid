/*
 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

/* DEBUG: section 49    SNMP Interface */

#ifndef SQUID_SRC_SNMP_AGENT_H
#define SQUID_SRC_SNMP_AGENT_H

#if SQUID_SNMP

#include "cache_snmp.h"
#include "snmp_vars.h"

variable_list *snmp_confFn(variable_list *, snint *);
variable_list *snmp_sysFn(variable_list *, snint *);
variable_list *snmp_prfSysFn(variable_list *, snint *);
variable_list *snmp_prfProtoFn(variable_list *, snint *);
variable_list *snmp_netIpFn(variable_list *, snint *);
variable_list *snmp_netFqdnFn(variable_list *, snint *);
variable_list *snmp_netDnsFn(variable_list *, snint *);
variable_list *snmp_meshPtblFn(variable_list *, snint *);

#endif /* SQUID_SNMP */
#endif /* SQUID_SRC_SNMP_AGENT_H */

