/* helper routine for parsing a VMEIO link address */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <epicsMutex.h>
#include <registry.h>
#include <alarm.h>

#define DEV_BUS_MAPPED_PVT
#include <devBusMapped.h>
#include <basicIoOps.h>

/* just any unique address */
static void	*registryId = (void*)&registryId;
static void	*ioRegistryId = (void*)&ioRegistryId;
static void	*ioscanRegistryId = (void*)&ioscanRegistryId;


#define DECL_INP(name) static int name(DevBusMappedPvt pvt, unsigned *pv, dbCommon *prec)
#define DECL_OUT(name) static int name(DevBusMappedPvt pvt, unsigned v, dbCommon *prec)

DECL_INP(inbe32)
	{ *pv = in_be32(pvt->addr);								return 0; }
DECL_INP(inle32)
	{ *pv = in_le32(pvt->addr);								return 0; }
DECL_INP(inbe16)
	{ *pv = (unsigned short)(in_be16(pvt->addr) & 0xffff);	return 0; }
DECL_INP(inle16)
	{ *pv = (unsigned short)(in_le16(pvt->addr) & 0xffff);	return 0; }
DECL_INP(in8)
	{ *pv = (unsigned char)(in_8(pvt->addr) & 0xff);		return 0; }

DECL_INP(inbe16s)
	{ *pv = (signed short)(in_be16(pvt->addr) & 0xffff);	return 0; }
DECL_INP(inle16s)
	{ *pv = (signed short)(in_le16(pvt->addr) & 0xffff);	return 0; }
DECL_INP(in8s)
	{ *pv = (signed char)(in_8(pvt->addr) & 0xff);			return 0; }

DECL_INP(inm32)
	{ *pv = *(unsigned *) pvt->addr;						return 0; }
DECL_INP(inm16)
	{ *pv = *(unsigned short *)pvt->addr;					return 0; }
DECL_INP(inm8)
	{ *pv = *(unsigned char *)pvt->addr;					return 0; }

DECL_INP(inm16s)
	{ *pv = *(signed short *)pvt->addr;						return 0; }
DECL_INP(inm8s)
	{ *pv = *(signed char *)pvt->addr;						return 0; }
	
DECL_OUT(outbe32)
	{ out_be32(pvt->addr,v);		return 0; }
DECL_OUT(outle32)
	{ out_le32(pvt->addr,v);		return 0; }
DECL_OUT(outbe16)
	{ out_be16(pvt->addr,v&0xffff);	return 0; }
DECL_OUT(outle16)
	{ out_le16(pvt->addr,v&0xffff);	return 0; }
DECL_OUT(out8)
	{ out_8(pvt->addr, v&0xff);		return 0; }

DECL_OUT(outm32)
	{ *(unsigned *)pvt->addr = v;					return 0; }
DECL_OUT(outm16)
	{ *(unsigned short *)pvt->addr = v & 0xffff;	return 0; }
DECL_OUT(outm8)
	{ *(unsigned char *)pvt->addr = v & 0xff;		return 0; }

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
unsigned long rval   = 0;
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
			recGblInitConstantLink(l, DBF_ULONG, &rval);
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
				/* they specified a number; create a registry entry on the fly... */

				/* make a canonical name */
				sprintf(buf,"0x%08lX",rval);

				/* try to find; if that fails, try to create; if this fails, try
				 * to find again - someone else might have created in the meantime...
				 */
				for ( i = 0;  ! (pvt->dev = (DevBusMappedDev)registryFind(registryId, buf)) && i < 1; i++ ) {
					if ( (pvt->dev = devBusMappedRegister(buf, (volatile void *)rval)) )
						break;
				}
			} else {
				pvt->dev = (DevBusMappedDev)registryFind(registryId, base);
			}

			if ( pvt->dev ) {
				rval  = (unsigned)pvt->dev->baseAddr;
				rval += l->value.vmeio.card << l->value.vmeio.signal;
			} else {
				rval = 0;
			}

			if ( comma ) {
				void *found;
				if ( (found = registryFind( ioRegistryId, comma )) ) {
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
				void *found;
				if ( (found = registryFind( ioscanRegistryId, comma1 )) ) {
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
devBusMappedGetVal(DevBusMappedPvt pvt, unsigned *pvalue, dbCommon *prec)
{
int rval = pvt->acc->rd(pvt, pvalue, prec);
	if ( rval )
		recGblSetSevr( prec, READ_ALARM, INVALID_ALARM );
	return rval;
}

int
devBusMappedPutVal(DevBusMappedPvt pvt, unsigned value, dbCommon *prec)
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
devBusMappedRegister(char *name, volatile void * baseAddress)
{
DevBusMappedDev	rval = 0, d;

	if ( (d = malloc(sizeof(*rval) + strlen(name))) ) {
		/* pre-load the allocated structure -  'registryAdd()'
		 * is atomical...
		 */
		d->baseAddr = baseAddress;
		strcpy((char*)d->name, name);
		if ( (d->mutex = epicsMutexCreate()) ) {
			/* NOTE: the registry keeps a pointer to the name and
			 *       does not copy the string, therefore we keep one.
			 *       (_must_ pass d->name, not 'name'!!)
			 */
			if ( registryAdd( registryId, d->name, d ) ) {
				rval = d; d = 0;
			}
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
devBusMappedRegisterIO(char *name, DevBusMappedAccess acc)
{
char *n;

	/* EPICS registry doesn't copy the name string, so we do */
	if ( 0 == (n=malloc(strlen(name)+1)) )
		return 0;

	strcpy(n,name);

	return ! registryAdd( ioRegistryId, n, (void*) acc );
}

int
devBusMappedRegisterIOScan(char *name, IOSCANPVT scan)
{
char *n;

	/* EPICS registry doesn't copy the name string, so we do */
	if ( 0 == (n=malloc(strlen(name)+1)) )
		return 0;

	strcpy(n,name);

	return ! registryAdd( ioscanRegistryId, n, (void*) scan );
}

/* Find the 'devBusMappedDev' of a registered device by name */
DevBusMappedDev
devBusMappedFind(char *name)
{
	return registryFind(registryId, name);
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
