/* devAiBus.c */

/* based on devAiSoft.c -
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
#include	"aiRecord.h"
#include        "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

/* Create the dset for devAiBus */
static long init_record();
static long read_ai();

struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_ai;
	DEVSUPFUN	special_linconv;
}devAiBus={
	6,
	NULL,
	NULL,
	init_record,
	devBusMappedGetIointInfo,
	read_ai,
	NULL
};
epicsExportAddress(dset, devAiBus);


static long init_record(aiRecord *prec)
{

   	if ( devBusVmeLinkInit(&prec->inp, 0, (dbCommon*)prec) ) {
		recGblRecordError(S_db_badField,(void *)prec,
			"devAiBus (init_record) Illegal INP field");
		return(S_db_badField);
	}

    return(0);
}

static long read_ai(aiRecord *pai)
{
DevBusMappedPvt pvt = pai->dpvt;
	return devBusMappedGetVal(pvt, &pai->rval, (dbCommon*)pai);
}
