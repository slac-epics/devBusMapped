#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

#include "../../multicastBLDLib/src/multicastBLDLib.h"

extern "C" 
{
// forward declarations
int testBldAPI_C(char* sInterfaceIp); 
int testBldAPI_CPP(char* sInterfaceIp); 
	
int testBldClient(int iTestType, char* sInterfaceIp)
{	
	if ( iTestType == 0 )
	{	
		testBldAPI_CPP(sInterfaceIp);
	}
	else if ( iTestType == 1 )
	{
		testBldAPI_C(sInterfaceIp); 
	}
	
	return 0;
}

int testBldAPI_CPP(char* sInterfaceIp)
{
	const unsigned int uAddr = 239<<24 | 255<<16 | 0<<8 | 1; // multicast address
	const unsigned int uPort = 50000;
	const unsigned int uMaxDataSize = 256; // in bytes
	const unsigned char ucTTL = 32; /// minimum: 1 + (# of routers in the middle)
	
	const char lcData[] = "MULTICAST BLD TEST"; // The sizeof(lcData) < uMaxDataSize
	
	EpicsBld::BldClientInterface* pBldClient = 
		EpicsBld::BldClientFactory::createBldClient(uAddr, uPort, uMaxDataSize, 
			ucTTL, sInterfaceIp);			

	printf("Bld send to %x port %d Data String %s\n", uAddr, uPort, lcData);
	pBldClient->sendRawData(sizeof(lcData), lcData);
		
	delete pBldClient;
	
	return 0;
}

int testBldAPI_C(char* sInterfaceIp)
{
	const unsigned int uAddr = 239<<24 | 255<<16 | 0<<8 | 1; // multicast address
	const unsigned int uPort = 50000;
	const unsigned int uMaxDataSize = 256; // in bytes
	const unsigned char ucTTL = 32; /// minimum: 1 + (# of routers in the middle)
	
	void* pVoidBldClient = NULL;
	BldClientInitByInterfaceName(uAddr, uPort, uMaxDataSize, ucTTL, sInterfaceIp, 
		&pVoidBldClient);
	
	if ( pVoidBldClient == NULL ) return 1;
	
	printf( "Beginning Multicast Client Testing. Press Ctrl+C to Exit...\n" );
	
	// Allocate data buffer
	unsigned int uIntDataSize = (uMaxDataSize/sizeof(int));
	int* liData = (int*) malloc(uIntDataSize * sizeof(int));
	int iTestValue = 1000;
	
	const int iSleepInterval = 3; // sleeps some seconds after sending each Bld data
	while ( 1 )  
	{
		for (unsigned int uIndex=0; uIndex<uIntDataSize; uIndex++)
			liData[uIndex] = iTestValue;
			
		printf("Bld send to %x port %d Value %d\n", uAddr, uPort, iTestValue);		
		BldClientSendRawData(pVoidBldClient, uIntDataSize*sizeof(int), 
			(char*) liData);
		
		iTestValue++;		
		sleep(iSleepInterval);
		// Waiting for keyboard interrupt to break the infinite loop
	}	
	
	free(liData);	
	BldClientRelease(pVoidBldClient);
	
	return 0;
}

} // extern "C" 
