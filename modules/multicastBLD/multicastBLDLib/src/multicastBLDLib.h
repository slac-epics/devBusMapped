namespace EpicsBld
{
	
/**
 * Abastract Interface of Bld Multicast Client 
 */
class BldClientInterface
{
public:
	virtual void sendRawData(int iSizeData, char* pData) = 0;	
	
	virtual ~BldClientInterface() {}
};

/**
 * Factory class of Bld Multicast Client 
 */
class BldClientFactory
{
public:
	virtual BldClientInterface* getBldClient(unsigned uAddr, 
		unsigned uPort, unsigned int uMaxDataSize, unsigned char ucTTL = 32, 
		char* sInteraceIp = 0);
		
	virtual BldClientInterface* getBldClient(unsigned uAddr, 
		unsigned uPort, unsigned int uMaxDataSize, unsigned char ucTTL = 32, 
		unsigned int uInteraceIp = 0);
		
	virtual ~BldClientFactory() {}
};

} // namespace EpicsBld

extern "C"
{

/**
 * Bld Client basic test function
 *
 * Will continuously send out the multicast packets to a default
 * address with default values. Need to be stop manually from keyboard
 * by pressing Ctrl+C
 */
int BldClientTestSendBasic(int iDataSeed);

/**
 * Bld Client test function with IP interface selection
 *
 * Similar to BldClientTestSendBasic(), but with the argument (sInterfaceIp)
 * to specify the IP interface for sending multicast.
 *
 * Will continuously send out the multicast packets to a default
 * address with default values. Need to be stop manually from keyboard
 * by pressing Ctrl+C
 */
int BldClientTestSendInterface(int iDataSeed, char* sInterfaceIp=0);


/* 
 * The following functions provide C wrappers for accesing EpicsBld::BldClientInterface
 * and EpicsBld::BldClientFactory
 */
 
/**
 * Init function: Use EpicsBld::BldClientFactory to construct the BldClient
 * and save the pointer in (*ppVoidBldClient)
 */
int BldClientInitByInterfaceName(unsigned uAddr, unsigned uPort, 
	unsigned int uMaxDataSize, unsigned char ucTTL, char* sInterfaceIp, 
	void** ppVoidBldClient);
int BldClientInitByInterfaceAddress(unsigned uAddr, unsigned uPort, 
	unsigned int uMaxDataSize, unsigned char ucTTL, unsigned int uInterfaceIp, 
	void** ppVoidBldClient);	

/**
 * Release function: Call C++ delete operator to delete the BldClient
 */
int BldClientRelease(void* pVoidBldClient);	

/**
 * Call the Send function defined in EpicsBld::BldClientInterface 
 */
int BldClientSendRawData(void* pVoidBldClient, int iSizeData, char* pData);

} // extern "C"


