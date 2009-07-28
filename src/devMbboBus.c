/* devMbboBus.c */

/* devMbboBus derived from base/devMbboSoftRaw
 * by Till Straumann <strauman@slac.stanford.edu>, 11/2002 
 */

/*
 *      Author:		Janet Anderson
 *      Date:		3-28-92
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
 * .01  03-28-92        jba     Initial version
 * .02  10-10-92        jba     replaced code with recGblGetLinkValue call
 *      ...
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
#include	"mbboRecord.h"
#include        "epicsExport.h"

#define DEV_BUS_MAPPED_PVT
#include	"devBusMapped.h"

/* Create the dset for devMbboBus */
static long init_record();
static long write_mbbo();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_mbbo;
}devMbboBus={
	5,
	NULL,
	NULL,
	init_record,
	devBusMappedGetIointInfo,
	write_mbbo
};
epicsExportAddress(dset, devMbboBus);

static long init_record(mbboRecord *pmbbo)
{
DevBusMappedPvt pvt;
unsigned		v;

   	if ( devBusVmeLinkInit(&pmbbo->out, 0, (dbCommon*) pmbbo) ) {
		recGblRecordError(S_db_badField,(void *)pmbbo,
			"devMbboBus (init_record) Illegal OUT field");
		return(S_db_badField);
	}

	pvt = pmbbo->dpvt; /* set by devBusVmeLinkInit() */

    /*to preserve old functionality*/
    if(pmbbo->nobt == 0) pmbbo->mask = 0xffffffff;

    pmbbo->mask <<= pmbbo->shft;

	if ( devBusMappedGetVal(pvt,&v,(dbCommon*)pmbbo) )
		recGblSetSevr( pmbbo, READ_ALARM, INVALID_ALARM );
	pmbbo->rval  = pmbbo->rbv = v;
	pmbbo->rval &= pmbbo->mask;

	/* PINI means that the initial value shall be forced out to
	 * the device. We suppress conversion in this case.
	 * Otherwise (!PINI), the value read back from the device
	 * will be converted and put to 'val'
	 */
	if ( pmbbo->pini ) {
		return 2;
	} else {
		recGblResetAlarms(pmbbo);
	}
	return 0;
}

static long write_mbbo(mbboRecord *pmbbo)
{
DevBusMappedPvt pvt = pmbbo->dpvt;
unsigned 		data;
long			rval;

	/* we could maintain a 'per word' mutex but that would be
	 * too complicated...
	 */
epicsMutexLock(pvt->dev->mutex);

	if ( (rval = devBusMappedGetVal(pvt, &data, (dbCommon *)pmbbo)) < 0 ) {
		goto leave;
	}

	pmbbo->rbv = data;

	data &= ~pmbbo->mask;

    data |= (pmbbo->rval & pmbbo->mask);

	rval = devBusMappedPutVal(pvt, data, (dbCommon *)pmbbo);

leave:
epicsMutexUnlock(pvt->dev->mutex);

	return rval;
}
