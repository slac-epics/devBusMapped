/* helper routine for parsing a VMEIO link address */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <epicsMutex.h>
#include <epicsTime.h>
#include <ellLib.h>
#include <alarm.h>
#include <dbAccess.h>
#include <errlog.h>
#include <drvSup.h>
#include <epicsExport.h>
#include <cantProceed.h>

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

static DevBusMappedTSGet getTS = epicsTimeGetCurrent;

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



#define DECL_INP(name) static int name(DevBusMappedPvt pvt, epicsUInt32 *pv, int idx, dbCommon *prec)
#define DECL_OUT(name) static int name(DevBusMappedPvt pvt, epicsUInt32 v,   int idx, dbCommon *prec)

DECL_INP(inbe32)
	{ *pv = in_be32(pvt->addr + (idx<<2));			    			return 0; }
DECL_INP(inle32)
	{ *pv = in_le32(pvt->addr + (idx<<2));							return 0; }
DECL_INP(inbe16)
	{ *pv = (uint16_t)(in_be16(pvt->addr + (idx<<1)) & 0xffff);		return 0; }
DECL_INP(inle16)
	{ *pv = (uint16_t)(in_le16(pvt->addr + (idx<<1)) & 0xffff);		return 0; }
DECL_INP(in8)
	{ *pv = (uint8_t)(in_8(pvt->addr + idx) & 0xff);				return 0; }

DECL_INP(inbe16s)
	{ *pv = (int16_t)(in_be16(pvt->addr + (idx<<1)) & 0xffff);		return 0; }
DECL_INP(inle16s)
	{ *pv = (int16_t)(in_le16(pvt->addr + (idx<<1)) & 0xffff);		return 0; }
DECL_INP(in8s)
	{ *pv = (int8_t)(in_8(pvt->addr+idx) & 0xff);					return 0; }

DECL_INP(inm32)
	{ *pv = *(((uint32_t *)pvt->addr) + idx);						return 0; }
DECL_INP(inm16)
	{ *pv = *(((uint16_t *)pvt->addr) + idx);						return 0; }
DECL_INP(inm8)
	{ *pv = *(((uint8_t  *)pvt->addr) + idx);						return 0; }

DECL_INP(inm16s)
	{ *pv = *(((int16_t  *)pvt->addr) + idx);						return 0; }
DECL_INP(inm8s)
	{ *pv = *(((int8_t   *)pvt->addr) + idx);						return 0; }
	
DECL_OUT(outbe32)
	{ out_be32(pvt->addr + (idx<<2),v);								return 0; }
DECL_OUT(outle32)
	{ out_le32(pvt->addr + (idx<<2),v);								return 0; }
DECL_OUT(outbe16)
	{ out_be16(pvt->addr + (idx<<1),v&0xffff);						return 0; }
DECL_OUT(outle16)
	{ out_le16(pvt->addr + (idx<<1),v&0xffff);						return 0; }
DECL_OUT(out8)
	{ out_8(pvt->addr + idx, v&0xff);								return 0; }

DECL_OUT(outm32)
	{ *(((uint32_t *)pvt->addr)+idx) = v;							return 0; }
DECL_OUT(outm16)
	{ *(((uint16_t *)pvt->addr)+idx) = v & 0xffff;					return 0; }
DECL_OUT(outm8)
	{ *(((uint8_t  *)pvt->addr)+idx) = v & 0xff;					return 0; }

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

static char *
erasePreWhite(char *p)
{
	while ( ' ' == *p || '\t' == *p )
		*p-- = 0;
	return p;
}

static char *
eraseWhite(char *p)
{
	while ( ' ' == *p || '\t' == *p )
		*p++ = 0;
	return p;
}

static const char *
parseArgs(char **cp_p, long **args, int *nargs)
{
char *opar, *cpar, *comma, *arg, *endp;
char tag;
const char *rval = 0;

	*nargs = 0;
	*args  = 0;
	if ( ! (opar = strchr(*cp_p, '(')) ) {
		return 0;
	}
	*opar++ = 0;
	erasePreWhite(opar-2);
	opar = eraseWhite(opar);
	if ( ! (cpar = strchr(opar,')')) ) {
		return	"devXXBus (init_record) Invalid access method string - closing ')' missing";
	}

	/* mark end of argument list by a 'comma' overwriting the closing
	 * ')' (or the last whitespace preceding it).
	 * This makes parsing easier since an argument list then
 	 * has the format   { <arg> ',' }.
 	 * However, since we don't know what comes after the ')' we
	 * temporarily write a NULL char and save the original character.
	 * If the original string terminates after ')' this is still correct
	 * (tagging 0 with 0)...
	 */
	*cpar++ = ',';
	tag = *(cpar);
	*cpar  = 0;

	arg = opar;
	while ( (comma = strchr(arg, ',')) ) {
		*comma++=0;
		erasePreWhite(comma - 2);
		comma = eraseWhite(comma);

		if ( ! (*args = realloc(*args, sizeof(**args) * (*nargs+1))) ) {
			rval = "devXXBus (init_record) No memory for argument array";
			goto bail;
		}

		(*args)[*nargs] = strtol(arg, &endp, 0);
		if ( !*arg || *endp ) {
			rval = "devXXBus (init_record) Non-numerical argument in INP string";
			goto bail;
		}
		(*nargs)++;

		arg = comma;
	}

bail:
	/* restore original char */
	*cpar = tag;

	*cp_p = eraseWhite(cpar);

	if ( *args && (0 == *nargs || rval) ) {
		free( *args );
		*args  = 0;
		*nargs = 0;
	}

	return rval;
}

unsigned long
devBusVmeLinkInit(DBLINK *l, DevBusMappedPvt pvt, dbCommon *prec)
{
char          *plus,*comma,*comma1,*cp;
unsigned long offset = 0;
uintptr_t     rval   = 0;
char          *base  = 0;
char          *str   = 0;
char          *endp;
const char    *errmsg;

	if ( !pvt ) {
		pvt = malloc( sizeof(*pvt) );
		if ( !pvt )
			cantProceed("devBusMapped: no memory for Pvt struct\n");
		prec->dpvt = pvt;
	}

	pvt->prec  = prec;
	pvt->acc   = &be32;
	pvt->args  = 0;
	pvt->nargs = 0;
	pvt->udata = 0;
	pvt->scan  = 0;

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

			/* allocate one char at the beginning to store marker '0' which
             * makes sure erasePreWhite() can never write beyond the start
             * of the string.
			 */
			str = cp = malloc(strlen(l->value.vmeio.parm) + 2);
			*cp++=0;
			strcpy(cp,l->value.vmeio.parm);
			/* trim white-space off the end and start of the string */
			erasePreWhite(cp + strlen(cp)-1);
			base = cp = eraseWhite(cp);

			if ( (plus=strchr(cp,'+')) ) {
				*plus++=0;
				/* erase whitespace around '+' */
				erasePreWhite(plus-2);
				cp = plus = eraseWhite(plus);
			}
			if ( (comma=strchr(cp,',')) ) {
				*comma++=0;
				/* erase whitespace around ',' */
				erasePreWhite(comma-2);
				cp = comma = eraseWhite(comma);
				if ( plus > comma )
					plus = 0;
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

				if ( (errmsg = parseArgs(&cp, &pvt->args, &pvt->nargs )) ) {
					recGblRecordError(S_db_badField, (void*)prec, errmsg);
					break;
				}
{
int i;
	printf("ARGS:\n");
	for ( i = 0; i<pvt->nargs; i++) {
		printf("%li\n", pvt->args[i]);
	}
}

				if ( (comma1=strchr(cp,',')) ) {
					*comma1++=0;
					/* erase whitespace around ',' */
					erasePreWhite(comma1-2);
					cp = comma1 = eraseWhite(comma1);
				}

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

				if ( comma1 ) {
					IOSCANPVT found;
					if ( (found = findIoScn( comma1 )) ) {
						pvt->scan = found;
					} else {
						recGblRecordError(S_db_badField, (void*)prec,
										  "devXXBus (init_record) Invalid IOSCANPVT string");
					}
				}
			}

    default :
		break;
    }

	free(str);

	if (rval)
		rval += offset;

	pvt->addr = (volatile void*)rval;

	if ( 0 == rval )
		prec->pact = TRUE;

	return !rval;
}

DevBusMappedTSGet
devBusMappedTSGetInstall(DevBusMappedTSGet newGetTS)
{
DevBusMappedTSGet oldGetTS;

	epicsMutexMustLock( mtx );
		oldGetTS = getTS;
		if ( newGetTS ) {
			getTS = newGetTS;
		}
	epicsMutexUnlock( mtx );
	return oldGetTS;
}

/*
 * Set timestamp if TSE == epicsTimeEventDeviceTime
 */
void
devBusMappedTSESetTime(dbCommon *prec)
{
	if ( epicsTimeEventDeviceTime == prec->tse ) {
		getTS( &prec->time );
	}
}


/* invoke the access method and do common work
 * (raise alarms)
 */
int
devBusMappedGetVal(DevBusMappedPvt pvt, epicsUInt32 *pvalue, dbCommon *prec)
{
int rval = pvt->acc->rd(pvt, pvalue, 0, prec);
	if ( rval )
		recGblSetSevr( prec, READ_ALARM, INVALID_ALARM );
	devBusMappedTSESetTime(prec);
	return rval;
}

int
devBusMappedPutVal(DevBusMappedPvt pvt, epicsUInt32 value, dbCommon *prec)
{
int rval = pvt->acc->wr(pvt, value, 0, prec);
	if ( rval )
		recGblSetSevr( prec, WRITE_ALARM, INVALID_ALARM );
	devBusMappedTSESetTime(prec);
	return rval;
}

int
devBusMappedGetArrVal(DevBusMappedPvt pvt, epicsUInt32 *pvalue, int idx, dbCommon *prec)
{
int rval = pvt->acc->rd(pvt, pvalue, idx, prec);
	if ( rval )
		recGblSetSevr( prec, READ_ALARM, INVALID_ALARM );
	return rval;
}

int
devBusMappedPutArrVal(DevBusMappedPvt pvt, epicsUInt32 value, int idx, dbCommon *prec)
{
int rval = pvt->acc->wr(pvt, value, idx, prec);
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
