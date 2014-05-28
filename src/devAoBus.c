/* devAoBus.c */

/* based on devAoSoft.c -
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
 *              Advanced Photon Lource
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
#include	"aoRecord.h"
#include        "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

/* Create the dset for devAoBus */
static long init_record();
static long write_ao();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_ao;
	DEVSUPFUN	special_linconv;
}devAoBus={
	6,
	NULL,
	NULL,
	init_record,
	devBusMappedGetIointInfo,
	write_ao,
	NULL
};
epicsExportAddress(dset, devAoBus);


static long init_record(aoRecord *prec)
{
long        rval = 0;
epicsUInt32 v;

	if ( devBusVmeLinkInit(&prec->out, 0, (dbCommon*)prec) ) {
		recGblRecordError(S_db_badField,(void *)prec,
			"devAoBus (init_record) Illegal OUT field");
		return(S_db_badField);
	}


	if (!prec->pini) {
		DevBusMappedPvt pvt = prec->dpvt;
		if ( devBusMappedGetVal(pvt, &v, (dbCommon*)prec) )
			recGblSetSevr( prec, READ_ALARM, INVALID_ALARM );
		prec->rval = (epicsInt32)v;
		recGblResetAlarms(prec);
		/* convert */
	} else {
		/* If they want pini then they want to define an intial VAL.
		 * However, the aoRecord code will use the initial RVAL 
		 * unless we tell it NOT to convert!
		 */
		rval = 2;
	}
    return(rval);
}

static long write_ao(aoRecord	*pao)
{
DevBusMappedPvt pvt = pao->dpvt;
long			rval;
epicsMutexLock(pvt->dev->mutex);
	rval = devBusMappedPutVal(pvt,pao->rval, (dbCommon*)pao);
epicsMutexUnlock(pvt->dev->mutex);
	return rval;
}

