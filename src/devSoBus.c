/* devSoBus.c */

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
#include	"stringoutRecord.h"
#include    "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

/* Create the dset for devSoBus */
static long init_record();
static long read_stringout();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_stringout;
}devSoBus={
	5,
	NULL,
	NULL,
	init_record,
	devBusMappedGetIointInfo,
	read_stringout
};
epicsExportAddress(dset, devSoBus);


static long init_record(stringoutRecord *prec)
{
DevBusMappedPvt pvt;
   	if ( devBusVmeLinkInit(&prec->out, 0, (dbCommon*)prec) ) {
		recGblRecordError(S_db_badField,(void *)prec,
			"devSoBus (init_record) Illegal OUT field");
		prec->pact = TRUE;
		return(S_db_badField);
	}
	pvt = prec->dpvt;
	if ( pvt->nargs < 1 || pvt->args[0]<1 ) {
		recGblRecordError(S_db_badField,(void *)prec,
			"devSoBus (init_record) Illegal OUT field: input method needs 'length' argument > 0");
		prec->pact = TRUE;
		return(S_db_badField);
	}
    return(0);
}

static long read_stringout(stringoutRecord *pstringout)
{
long            rval = 0; /* silence compiler warning */
int             i,l;
DevBusMappedPvt pvt = pstringout->dpvt;

	
	l = sizeof(pstringout->val);

	if ( pvt->args[0] < l )
		l = pvt->args[0];

	epicsMutexMustLock(pvt->dev->mutex);

		for ( i=0; i<l; i++ ) {
			rval = devBusMappedPutArrVal(pvt, pstringout->val[i], i, (dbCommon*)pstringout);
			if ( rval || 0 == pstringout->val[i] )
				break;
		}

	epicsMutexUnlock(pvt->dev->mutex);

	return rval;
}
