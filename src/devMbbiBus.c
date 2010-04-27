/* devMbbiBus.c */

/* devMbboBus derived from base/devMbbiSoftRaw
 * by Till Straumann <strauman@slac.stanford.edu>, 11/2002 
 */

/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 */
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"alarm.h"
#include	"dbDefs.h"
#include	"dbAccess.h"
#include	"recGbl.h"
#include	"recSup.h"
#include	"devSup.h"
#include	"mbbiRecord.h"
#include        "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

/* Create the dset for devMbbiBus */
static long init_record();
static long read_mbbi();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_mbbi;
}devMbbiBus={
	5,
	NULL,
	NULL,
	init_record,
	devBusMappedGetIointInfo,
	read_mbbi
};
epicsExportAddress(dset, devMbbiBus);

static long init_record(mbbiRecord *pmbbi)
{
DevBusMappedPvt pvt;

   	if ( devBusVmeLinkInit(&pmbbi->inp, 0, (dbCommon*) pmbbi) ) {
		recGblRecordError(S_db_badField,(void *)pmbbi,
			"devMbbiBus (init_record) Illegal INP field");
		return(S_db_badField);
	}

	pvt = pmbbi->dpvt; /* set by devBusVmeLinkInit() */
 
    /*to preserve old functionality*/
    if(pmbbi->nobt == 0) pmbbi->mask = 0xffffffff;
    pmbbi->mask <<= pmbbi->shft;
    return(0);
}

static long read_mbbi(mbbiRecord *pmbbi)
{
DevBusMappedPvt pvt = pmbbi->dpvt;
long			rval;
epicsUInt32		v;

    rval = devBusMappedGetVal(pvt, &v, (dbCommon*)pmbbi);
	if ( rval >=0 ) {
    	pmbbi->rval = v & pmbbi->mask;
	}
    return(rval);
}
