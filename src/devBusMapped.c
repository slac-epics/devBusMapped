/* helper routine for parsing a VMEIO link address */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <epicsMutex.h>
#include <ellLib.h>
#include <alarm.h>
#include <dbAccess.h>
#include <errlog.h>
#include <drvSup.h>
#include <epicsExport.h>

#define DEV_BUS_MAPPED_PVT
#include <devBusMapped.h>
#include <basicIoOps.h>

typedef struct DBMNodeRec_ {
	ELLNODE            node;
	union {
	DevBusMappedDev    dev;
	DevBusMappedAccess acc;
	IOSCANPVT          scn;
	}                  data;
	char               name[];
} DBMNodeRec, *DBMNode;

static struct ELLLIST devList   = ELLLIST_INIT;
static struct ELLLIST ioList    = ELLLIST_INIT;
static struct ELLLIST ioscnList = ELLLIST_INIT;

static epicsMutexId    mtx;

static DBMNode
dbmFind(const char *name, ELLLIST *l)
{
DBMNode  n;
	for ( n = (DBMNode)ellFirst(l); n; n = (DBMNode)ellNext(&n->node) ) {
		if ( 0 == strcmp(name, n->name) ) {
			return n;
		}
	}
	return 0;
}

static DBMNode
dbmAdd(const char *name, ELLLIST *l)
{
DBMNode n;
	if ( dbmFind(name, l) )
		return 0;
	if ( (n = malloc(sizeof(*n) + strlen(name) + 1)) ) {
		strcpy(n->name, name);
		ellAdd(l, &n->node);
	}
	return n;
}

int
dbmIterate(ELLLIST *l, int (*fn)(DBMNode n, void *arg), void *arg)
{
DBMNode n;
int     i;

	epicsMutexMustLock( mtx );
		for ( i=0, n = (DBMNode)ellFirst(l); n; n = (DBMNode)ellNext(&n->node), i++ ) {
			if ( fn(n, arg) )
				break;
		}
	epicsMutexUnlock(mtx);
	return i;
}

/* Find the 'devBusMappedDev' of a registered device by name */
DevBusMappedDev
devBusMappedFind(const char *name)
{
DBMNode n;

	epicsMutexMustLock( mtx );
		n = dbmFind(name, &devList);
	epicsMutexUnlock( mtx );

	return n ? n->data.dev : 0;
}

static DevBusMappedAccess
findIo(const char *name)
{
DBMNode n;

	epicsMutexMustLock( mtx );
		n = dbmFind(name, &ioList);
	epicsMutexUnlock( mtx );

	return n ? n->data.acc : 0;
}

static IOSCANPVT
findIoScn(const char *name)
{
DBMNode n;

	epicsMutexMustLock( mtx );
		n = dbmFind(name, &ioscnList);
	epicsMutexUnlock( mtx );

	return n ? n->data.scn : 0;
}



#define DECL_INP(name) static int name(DevBusMappedPvt pvt, epicsUInt32 *pv, dbCommon *prec)
#define DECL_OUT(name) static int name(DevBusMappedPvt pvt, epicsUInt32 v, dbCommon *prec)

DECL_INP(inbe32)
	{ *pv = in_be32(pvt->addr);							return 0; }
DECL_INP(inle32)
	{ *pv = in_le32(pvt->addr);							return 0; }
DECL_INP(inbe16)
	{ *pv = (uint16_t)(in_be16(pvt->addr) & 0xffff);	return 0; }
DECL_INP(inle16)
	{ *pv = (uint16_t)(in_le16(pvt->addr) & 0xffff);	return 0; }
DECL_INP(in8)
	{ *pv = (uint8_t)(in_8(pvt->addr) & 0xff);			return 0; }

DECL_INP(inbe16s)
	{ *pv = (int16_t)(in_be16(pvt->addr) & 0xffff);		return 0; }
DECL_INP(inle16s)
	{ *pv = (int16_t)(in_le16(pvt->addr) & 0xffff);		return 0; }
DECL_INP(in8s)
	{ *pv = (int8_t)(in_8(pvt->addr) & 0xff);			return 0; }

DECL_INP(inm32)
	{ *pv = *(uint32_t *) pvt->addr;					return 0; }
DECL_INP(inm16)
	{ *pv = *(uint16_t *)pvt->addr;						return 0; }
DECL_INP(inm8)
	{ *pv = *(uint8_t  *)pvt->addr;						return 0; }

DECL_INP(inm16s)
	{ *pv = *(int16_t  *)pvt->addr;						return 0; }
DECL_INP(inm8s)
	{ *pv = *(int8_t   *)pvt->addr;						return 0; }
	
DECL_OUT(outbe32)
	{ out_be32(pvt->addr,v);							return 0; }
DECL_OUT(outle32)
	{ out_le32(pvt->addr,v);							return 0; }
DECL_OUT(outbe16)
	{ out_be16(pvt->addr,v&0xffff);						return 0; }
DECL_OUT(outle16)
	{ out_le16(pvt->addr,v&0xffff);						return 0; }
DECL_OUT(out8)
	{ out_8(pvt->addr, v&0xff);							return 0; }

DECL_OUT(outm32)
	{ *(uint32_t *)pvt->addr = v;						return 0; }
DECL_OUT(outm16)
	{ *(uint16_t *)pvt->addr = v & 0xffff;				return 0; }
DECL_OUT(outm8)
	{ *(uint8_t  *)pvt->addr = v & 0xff;				return 0; }

static DevBusMappedAccessRec m32   = { inm32, outm32 };
static DevBusMappedAccessRec be32  = { inbe32, outbe32 };
static DevBusMappedAccessRec le32  = { inle32, outle32 };
static DevBusMappedAccessRec m16   = { inm16, outm16 };
static DevBusMappedAccessRec be16  = { inbe16, outbe16 };
static DevBusMappedAccessRec le16  = { inle16, outle16 };
static DevBusMappedAccessRec m8    = { inm8, outm8 };
static DevBusMappedAccessRec io8   = { in8, out8 };
static DevBusMappedAccessRec m16s  = { inm16s, outm16 };
static DevBusMappedAccessRec be16s = { inbe16s, outbe16 };
static DevBusMappedAccessRec le16s = { inle16s, outle16 };
static DevBusMappedAccessRec m8s   = { inm8s, outm8 };
static DevBusMappedAccessRec io8s  = { in8s, out8 };

unsigned long
devBusVmeLinkInit(DBLINK *l, DevBusMappedPvt pvt, dbCommon *prec)
{
char          *plus,*comma,*comma1,*cp;
unsigned long offset = 0;
uintptr_t     rval   = 0;
char          *base  = 0;
char          *endp;

	if ( !pvt ) {
		assert( pvt = malloc( sizeof(*pvt) ) );
		prec->dpvt = pvt;
	}

	pvt->prec = prec;
	pvt->acc = &be32;

    switch (l->type) {

    case (CONSTANT) :
		{
			unsigned long long ull;
			char               *endp;
			if ( l->value.constantStr ) {
				ull = strtoull(l->value.constantStr, &endp, 0);
				if ( endp > l->value.constantStr ) {
					rval = ull;
				} else {
					errlogPrintf("devBusVmeLinkInit: Invalid CONSTANT link value\n");
					rval = 0;
				}
			} else {
				rval = 0;
			}
		}
        break;

    case (VME_IO) :

			base = cp = malloc(strlen(l->value.vmeio.parm) + 1);
			strcpy(cp,l->value.vmeio.parm);

			if ( (plus=strchr(cp,'+')) ) {
				*plus++=0;
				cp = plus;
			}
			if ( (comma=strchr(cp,',')) ) {
				*comma++=0;
				cp = comma;
			}
			if ( (comma1=strchr(cp,',')) ) {
				*comma1++=0;
				cp = comma1;
			}

			if ( plus ) {
				offset = strtoul(plus, &endp, 0);
				if ( !*plus || *endp ) {
					recGblRecordError(S_db_badField, (void*)prec,
									  "devXXBus (init_record) Invalid OFFSET string");
					break;
				}
			}

			rval = strtoul(base, &endp, 0);
			if ( *base && !*endp ) {
				int  i;
				char buf[15];
				/* they specified a number; create a list entry on the fly... */

				/* make a canonical name */
				sprintf(buf,"0x%"PRIXPTR,rval);

				/* try to find; if that fails, try to create; if this fails, try
				 * to find again - someone else might have created in the meantime...
				 */
				for ( i = 0;  ! (pvt->dev = devBusMappedFind(buf)) && i < 1; i++ ) {
					if ( (pvt->dev = devBusMappedRegister(buf, (volatile void *)rval)) )
						break;
				}
			} else {
				pvt->dev = devBusMappedFind(base);
			}

			if ( pvt->dev ) {
				rval  = (uintptr_t)pvt->dev->baseAddr;
				rval += l->value.vmeio.card << l->value.vmeio.signal;
			} else {
				rval = 0;
			}

			if ( comma ) {
				DevBusMappedAccess found;
				if ( (found = findIo( comma )) ) {
					pvt->acc = found;
				} else
				if ( !strncmp(comma,"m32",3) ) {
					pvt->acc = &m32;
				} else
				if ( !strncmp(comma,"be32",4) ) {
					pvt->acc = &be32;
				} else
				if ( !strncmp(comma,"le32",4) ) {
					pvt->acc = &le32;
				} else
				if ( !strncmp(comma,"m16",3) ) {
					pvt->acc = ('s'==comma[3] ? &m16s : &m16);
				} else
				if ( !strncmp(comma,"be16",4) ) {
					pvt->acc = ('s'==comma[4] ? &be16s : &be16);
				} else
				if ( !strncmp(comma,"le16",4) ) {
					pvt->acc = ('s'==comma[4] ? &le16s : &le16);
				} else
				if ( !strncmp(comma,"m8",2) ) {
					pvt->acc = ('s'==comma[2] ? &m8s : &m8);
				} else
				if ( !strncmp(comma,"be8",3) ) {
					pvt->acc = ('s'==comma[3] ? &io8s : &io8);
				} else {
					recGblRecordError(S_db_badField, (void*)prec,
									  "devXXBus (init_record) Invalid ACCESS string");
					break;
				}
			}

			pvt->scan = 0;
			if ( comma1 ) {
				IOSCANPVT found;
				if ( (found = findIoScn( comma1 )) ) {
					pvt->scan = found;
				} else {
					recGblRecordError(S_db_badField, (void*)prec,
									  "devXXBus (init_record) Invalid IOSCANPVT string");
				}
			}

    default :
		break;
    }

	free(base);

	if (rval)
		rval += offset;

	pvt->addr = (volatile void*)rval;

	if ( 0 == rval )
		prec->pact = TRUE;

	return !rval;
}

/* invoke the access method and do common work
 * (raise alarms)
 */
int
devBusMappedGetVal(DevBusMappedPvt pvt, epicsUInt32 *pvalue, dbCommon *prec)
{
int rval = pvt->acc->rd(pvt, pvalue, prec);
	if ( rval )
		recGblSetSevr( prec, READ_ALARM, INVALID_ALARM );
	return rval;
}

int
devBusMappedPutVal(DevBusMappedPvt pvt, epicsUInt32 value, dbCommon *prec)
{
int rval = pvt->acc->wr(pvt, value, prec);
	if ( rval )
		recGblSetSevr( prec, WRITE_ALARM, INVALID_ALARM );
	return rval;
}


/* Register a device's base address and return a pointer to a
 * freshly allocated 'DevBusMappedDev' struct or NULL on failure.
 */
DevBusMappedDev
devBusMappedRegister(const char *name, volatile void * baseAddress)
{
DevBusMappedDev	rval = 0, d;
DBMNode         n;

	if ( (d = malloc(sizeof(*rval))) ) {

		/* pre-load the allocated structure */

		d->baseAddr = baseAddress;
		if ( (d->mutex = epicsMutexCreate()) ) {
			epicsMutexMustLock( mtx );
			if ( (n = dbmAdd(name, &devList)) ) {
				rval = n->data.dev = d;
				d = 0;
				rval->name = n->name;
			}
			epicsMutexUnlock( mtx );
		}
	}

	if (d) {
		if (d->mutex)
			epicsMutexDestroy(d->mutex);
		free(d);
	}
	return rval;
}

int
devBusMappedRegisterIO(const char *name, DevBusMappedAccess acc)
{
DBMNode n;

	epicsMutexMustLock( mtx );
		if ( ( n = dbmAdd(name, &ioList) ) ) {
			n->data.acc = acc;
		}
	epicsMutexUnlock( mtx );

	return n ? 0 : -1;
}

int
devBusMappedRegisterIOScan(const char *name, IOSCANPVT scan)
{
DBMNode n;

	epicsMutexMustLock( mtx );
		if ( ( n = dbmAdd(name, &ioscnList) ) ) {
			n->data.scn = scan;
		}
	epicsMutexUnlock( mtx );

	return n ? 0 : -1;
}

long
devBusMappedGetIointInfo(int delFrom, dbCommon *prec, IOSCANPVT *ppvt)
{
DevBusMappedPvt pvt = prec->dpvt;
	/* a 'pvt' with no valid 'scan'  (scan==NULL) will yield an
	 * error (dbScan.c)
	 */
	*ppvt = pvt->scan;
	return 0;
}

static int
prDev(DBMNode n, void *unused)
{
	/* Caller holds the mutex - we unlock while printing (assuming 'n' can't go away) */
	epicsMutexUnlock(mtx);
		errlogPrintf("  @%p: %s\n", n->data.dev->baseAddr, n->data.dev->name);
	epicsMutexMustLock(mtx);
	/* List structure may have changed here but the node is still somewhere in the list */
	return 0;
}

static int
prNam(DBMNode n, void *unused)
{
	/* Caller holds the mutex - we unlock while printing (assuming 'n' can't go away) */
	epicsMutexUnlock(mtx);
		errlogPrintf("  %s\n", n->name);
	epicsMutexMustLock(mtx);
	/* List structure may have changed here but the node is still somewhere in the list */
	return 0;
}

int
devBusMappedDump(DevBusMappedDev dev)
{
	if ( dev ) {
		errlogPrintf("@%p: %s\n", dev->baseAddr, dev->name);
		return 1;
	}
	return dbmIterate( &devList, prDev, 0 );
}

/* In this routine we don't hold the lock while printing (slow).
 * This means that the list could change but it can not become
 * inconsistent. We must the lock while stepping around.
 */
long
devBusMappedReport(int level)
{
	errlogPrintf("devBusMapped - generic device support for memory-mapped devices\n");
	if ( level > 0 ) {
		errlogPrintf("Registered base addresses:\n");
		if ( 0 == devBusMappedDump( 0 ) )
			errlogPrintf("  <NONE>\n");
		errlogPrintf("Registered IO methods:\n");
		if ( 0 == dbmIterate( &ioList, prNam, 0 ) )
			errlogPrintf("  <NONE>\n");
		errlogPrintf("Registered IOSCAN lists:\n");
		if ( 0 == dbmIterate( &ioscnList, prNam, 0 ) )
			errlogPrintf("  <NONE>\n");
	}

	return 0;
}

/* Must initialize the facility via the registrar -- we want to be able to
 * use it *before* iocInit()!
 */
static void
devBusMappedInit()
{
	mtx = epicsMutexMustCreate();
}

/* We only create a 'driver' entry for sake of the 'report' routine */
struct drvet devBusMapped = {
	number: 2,
	report: devBusMappedReport,
	init:   0
};

epicsExportAddress(drvet, devBusMapped);
epicsExportRegistrar(devBusMappedInit);
