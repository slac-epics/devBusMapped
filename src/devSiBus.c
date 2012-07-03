/* devSiBus.c */

/* based on devLiSoft.c -
 * modified by Till Straumann <strauman@slac.stanford.edu>, 2002/11/11
 */

/*
 *      Author:		Janet Anderson
 *      Date:   	09-23-91
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
 * .02	03-13-92	jba	ANSI C changes
 * .03  10-10-92        jba     replaced code with recGblGetLinkValue call
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
#include	"stringinRecord.h"
#include    "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

typedef struct SiDpvtRec_ {
	DevBusMappedPvtRec pvt;
	int                lmax;
} SiDpvtRec, *SiDpvt;

/* Create the dset for devSiBus */
static long init_record();
static long read_stringin();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_stringin;
}devSiBus={
	5,
	NULL,
	NULL,
	init_record,
	devBusMappedGetIointInfo,
	read_stringin
};
epicsExportAddress(dset, devSiBus);


static long init_record(stringinRecord *prec)
{
SiDpvt          dpvt;

	if ( ! (dpvt = malloc(sizeof(*dpvt))) ) {
		recGblRecordError(S_rec_outMem,(void *)prec,
			"devSiBus (init_record) No memory for DPVT struct");
		prec->pact = TRUE;
		return S_rec_outMem;
	}
	prec->dpvt = dpvt;

   	if ( devBusVmeLinkInit(&prec->inp, &dpvt->pvt, (dbCommon*)prec) ) {
		recGblRecordError(S_db_badField,(void *)prec,
			"devSiBus (init_record) Illegal INP field");
		prec->pact = TRUE;
		return(S_db_badField);
	}
	dpvt->lmax = sizeof(prec->val) - 1;
	if (    dpvt->pvt.nargs > 0
         && dpvt->pvt.args[0] < dpvt->lmax
	     && dpvt->pvt.args[0] > 0 ) {
		dpvt->lmax = dpvt->pvt.args[0];
	}

    return(0);
}

static long read_stringin(stringinRecord *pstringin)
{
long            rval = 0; /* keep compiler happy */
epicsUInt32     v;
int             i,l;
SiDpvt          dpvt = pstringin->dpvt;
DevBusMappedPvt pvt  = &dpvt->pvt;

	
	epicsMutexMustLock(pvt->dev->mutex);

		for ( i=0; i<dpvt->lmax; i++ ) {
			rval = devBusMappedGetArrVal(pvt, &v, i, (dbCommon*)pstringin);
			if ( rval || 0 == v )
				break;
			pstringin->val[i] = v;
		}
		pstringin->val[i] = 0;

	epicsMutexUnlock(pvt->dev->mutex);

	devBusMappedTSESetTime((dbCommon*)pstringin);

	return rval;
}
