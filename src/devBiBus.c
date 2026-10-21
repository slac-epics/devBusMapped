/* devBiBus.c */

/* based on devBiSoftRaw.c -
 * modified by Till Straumann <strauman@slac.stanford.edu>, 2002/11/11
 */

/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .01  11-11-91        jba     Moved set of alarm stat and sevr to macros
 * .02  02-05-92	jba	Changed function arguments from paddr to precord 
 * .03	03-13-92	jba	ANSI C changes
 * .04  10-10-92        jba     replaced code with recGblGetLinkValue call
 *      ...
 */
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	"alarm.h"
#include	"dbDefs.h"
#include	"dbAccess.h"
#include	"recGbl.h"
#include        "recSup.h"
#include	"devSup.h"
#include	"biRecord.h"
#include        "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

/* Create the dset for devBiBus */
static long init_record(struct dbCommon*);
static long read_bi(struct biRecord*);

#ifndef HAS_bidset
typedef struct bidset {
	dset common;
	long (*read_bi)(struct biRecord*);
} bidset;
#endif

bidset devBiBus = {
	.common = {
		.number = 5,
		.report = NULL,
		.init = NULL,
		.init_record = init_record,
		.get_ioint_info = devBusMappedGetIointInfo,
	},
	.read_bi = read_bi
};
epicsExportAddress(dset, devBiBus);

static long init_record(dbCommon *prec)
{
	struct biRecord* biRec = (struct biRecord*)prec;
   	if ( devBusVmeLinkInit(&biRec->inp, 0, (dbCommon*)prec) ) {
		recGblRecordError(S_db_badField,(void *)prec,
			"devBiBus (init_record) Illegal INP field");
		return(S_db_badField);
	}

	prec->udf  = 0;

    return(0);
}

static long read_bi(biRecord *pbi)
{
epicsUInt32		v;
long			rval;
DevBusMappedPvt pvt = pbi->dpvt;
	rval = devBusMappedGetVal(pvt, &v, (dbCommon*)pbi);
	if ( rval >= 0 ) {
		if ( pbi->mask )
		 	v &= pbi->mask;
		pbi->rval = v;
	}
    return(rval);
}
