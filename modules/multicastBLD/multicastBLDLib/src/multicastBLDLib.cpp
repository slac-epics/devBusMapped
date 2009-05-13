#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/uio.h>
/*
 * Common Configuration for Bld Server and Client
 */
namespace EpicsBld
{

namespace ConfigurationMulticast
{
	static const unsigned uTestAddr = 239<<24 | 255<<16 | 0<<8 | 1; // multicast
	static const unsigned uTestPort = 50000;
	static const unsigned char ucTTL = 2;
};

/**
 * class Ins
 *
 * imported from ::Pds::Ins
 */
class Ins
{
public:
	Ins();
	enum Option {DoNotInitialize};
	explicit Ins(Option);
	explicit Ins(unsigned short port);
	explicit Ins(int address);
	Ins(int address, unsigned short port);
	Ins(const Ins& ins, unsigned short port);
	explicit Ins(const sockaddr_in& sa);

	int                   address()     const; 
	void                  address(int address);
	unsigned short        portId()      const; 

	int operator==(const Ins& that)  const;

protected:
	int      _address;
	unsigned _port;
};

/**
 * class Sockaddr
 *
 * imported from ::Pds::Sockaddr
 */

class Sockaddr {
public:
  Sockaddr() {
    _sockaddr.sin_family      = AF_INET;
}
  
  Sockaddr(const Ins& ins) {
    _sockaddr.sin_family      = AF_INET;
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId()); 
}
  
  void get(const Ins& ins) {
    _sockaddr.sin_addr.s_addr = htonl(ins.address());
    _sockaddr.sin_port        = htons(ins.portId());     
}

  sockaddr* name() const {return (sockaddr*)&_sockaddr;}
  inline int sizeofName() const {return sizeof(_sockaddr);}

private:
  sockaddr_in _sockaddr;
};

/**
 * class Port
 *
 * imported from ::Pds::Port
 */
class Port : public Ins
{
  public:
    enum Type {ClientPort = 0, ServerPort = 2, VectoredServerPort = 3};
    ~Port();
    Port(Port::Type, 
            const Ins&, 
            int sizeofDatagram, 
            int maxPayload,
            int maxDatagrams);
    Port(Port::Type, 
            int sizeofDatagram, 
            int maxPayload,
            int maxDatagrams);
    int  sizeofDatagram() const;
    int  maxPayload()     const;
    int  maxDatagrams()   const;
    Type type()           const;

    // Construction may potentially fail. If the construction is
    // successful error() returns 0. If construction fails, error()
    // returns a non-zero value which corresponds to the reason (as an
    // "errno").
    int  error() const;
  protected:
    void error(int value);
    int _socket;
  private:
    enum {Class_Server = 2, Style_Vectored = 1};
    void   _open(Port::Type, 
		 const Ins&, 
		 int sizeofDatagram, 
		 int maxPayload,
		 int maxDatagrams);
    int    _bind(const Ins&);
    void   _close();
    Type   _type;
    int    _sizeofDatagram;
    int    _maxPayload;
    int    _maxDatagrams;
    int    _error;
};

class Client : public Port
{
  public:
    Client(int sizeofDatagram,
	   int maxPayload,
	   int maxDatagrams = 1);
    
    ~Client();

  public:
    int send(char* datagram, char* payload, int sizeofPayload, const Ins&);
		 
    /*
	 * Multicast functions
	 */
    void use(unsigned interface); 
	int multicastSetTTL(unsigned char ucTTL);
		 
  private:
     enum {SendFlags = 0};
#ifdef ODF_LITTLE_ENDIAN
    // Would prefer to have _swap_buffer in the stack, but it triggers
    // a g++ bug (tried release 2.96) with pointers to member
    // functions of classes which are bigger than 0x7ffc bytes; this
    // bug appeared in Outlet which points to OutletWire member
    // functions; hence OutletWire must be <= 0x7ffc bytes; if
    // Client is too large the problem appears in OutletWire
    // since it contains an Client. 
    // Later note: g++ 3.0.2 doesn't show this problem.
    char* _swap_buffer;
    void _swap(const iovec* iov, unsigned msgcount, iovec* iov_swap);
#endif
};

/**
 * Bld Multicast Client
 */
class ClientTest {
public:
	ClientTest(unsigned uAddr, unsigned uPort, unsigned char ucTTL = 1,
		char* sInteraceIp = NULL);
	~ClientTest();
	
	void send(int iSeed = 0);
private:
	enum {_Nwords=16};
	unsigned _data[_Nwords];

	Client   _outlet; // socket
	Ins      _dst;    // port and address
};

} // namespace EpicsBld

extern "C" int testEpicsBld(char* sInterfaceIp);

int testEpicsBld(char* sInterfaceIp)
{
	using namespace EpicsBld;
	using namespace ConfigurationMulticast;
	ClientTest bldClient(uTestAddr, uTestPort, ucTTL, sInterfaceIp);

	printf( "Beginning Multicast Client Testing. Press Any Key to Exit...\n" );
	
	int iSendSeed = 0;
	while ( 1 )  
	{
		bldClient.send(++iSendSeed);
		sleep(5);	
	}
	
	printf("\r\n"); // move the cursor to the beginning of line
	
	return 0;
}

/**
 * class member definitions
 *
 */
namespace EpicsBld
{
/**
 * class Ins
 */
inline int Ins::operator==(const Ins& that) const{
  return (_address == that._address && _port == that._port);
}

inline Ins::Ins(Option) 
{
}

inline Ins::Ins() 
{
	_address = INADDR_ANY;
	_port    = 0;
}

inline Ins::Ins(unsigned short port) 
{
    _address = INADDR_ANY;
    _port    = port;
}

inline Ins::Ins(int address, unsigned short port) 
{
    _address = address;
    _port    = port;
}

inline Ins::Ins(const Ins& ins, unsigned short port) 
{
    _address = ins._address;
    _port    = port;
}

inline Ins::Ins(const sockaddr_in& sa) 
{
    _address = ntohl(sa.sin_addr.s_addr);
    _port    = ntohs(sa.sin_port);
}

inline Ins::Ins(int address) 
{
    _address = address;
    _port    = 0;
}

inline unsigned short Ins::portId() const 
{
    return _port;  
}

inline void Ins::address(int address) 
{
    _address = address;
}

inline int Ins::address() const 
{
    return _address;
}	

/**
 * class Port
 */
inline Port::~Port() 
{
  _close();
}

inline Port::Port(Port::Type type, 
                        int sizeofDatagram, 
                        int maxPayload,
                        int maxDatagrams) : 
  Ins(Ins::DoNotInitialize)
{
  Ins ins; _open(type, ins, sizeofDatagram, maxPayload, maxDatagrams);
}

inline Port::Port(Port::Type type, 
		       const Ins& ins, 
		       int sizeofDatagram, 
		       int maxPayload,
		       int maxDatagrams) : 
Ins(Ins::DoNotInitialize)
{
_open(type, ins, sizeofDatagram, maxPayload, maxDatagrams);
}

inline Port::Type Port::type() const
{
  return _type;
}

inline int Port::sizeofDatagram() const
{
  return _sizeofDatagram;
}

inline int Port::maxPayload() const
{
  return _maxPayload;
}


inline int Port::maxDatagrams() const
{
  return _maxDatagrams;
}

inline int Port::error() const
{
  return _error;
}

inline void Port::error(int value)
{
  _error = value;
}

void Port::_open(Port::Type type,
                    const Ins& ins,
                    int sizeofDatagram,
                    int maxPayload,
                    int maxDatagrams)
{
  _socket         = socket(AF_INET, SOCK_DGRAM, 0);
  _sizeofDatagram = sizeofDatagram;
  _maxPayload     = maxPayload;
  _maxDatagrams   = maxDatagrams;
  _type           = type;
  if(!(_error = (_socket != -1) ? _bind(ins) : errno))
    {
      sockaddr_in name;
#ifdef VXWORKS
      int length = sizeof(name);
#else
      socklen_t length = sizeof(name);
#endif
      if(getsockname(_socket, (sockaddr*)&name, &length) == 0) {
	_address = ntohl(name.sin_addr.s_addr);
	_port = (unsigned)ntohs(name.sin_port);
      } else {
	_error = errno;
	_close();
      }
    }
  else
    _close();
  if (_error) {
    printf("*** Port failed to open address 0x%x port %i: %s\n", 
	   ins.address(), ins.portId(), strerror(errno));
}
}



int Port::_bind(const Ins& ins)
{
  int s = _socket;

  int yes = 1;
  if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&yes, sizeof(yes)) == -1)
    return errno;
	
//	struct ip_mreq	mreq;

//	mreq.imr_multiaddr.s_addr = 239<<24 | 255<<16 | 0<<8 | 1;
//	mreq.imr_interface.s_addr = 172<<24 | 21<<16 | 9<<8 | 23;
//	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
												//sizeof(mreq)) == -1 )
//		printf( "Error adding multicast group\n" );
	

  if(_type == Port::ClientPort)
    {
    int parm = (_sizeofDatagram + _maxPayload + sizeof(struct sockaddr_in)) *
               _maxDatagrams;

#ifdef VXWORKS                           // Out for Tornado II 7/21/00 - RiC
    // The following was found exprimentally with ~claus/bbr/test/sizeTest
    // The rule may well change if the mBlk, clBlk or cluster parameters change
    // as defined in usrNetwork.c - RiC
    parm = parm + (88 + 32 * ((parm - 1993) / 2048));
#endif

    if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char*)&parm, sizeof(parm)) == -1)
      return errno;
    }

  if(_type & Class_Server)
    {
    int parm = (_sizeofDatagram + _maxPayload + sizeof(struct sockaddr_in)) *
               _maxDatagrams;
#ifdef __linux__
    parm += 2048; // 1125
#endif
    if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char*)&parm, sizeof(parm)) == -1)
      return errno;
    if(ins.portId() && (_type & Style_Vectored))
      {
      int y = 1;
#ifdef VXWORKS 
      if(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (char*)&y, sizeof(y)) == -1)
#else
      if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&y, sizeof(y)) == -1)
#endif
        return errno;
      }
    }

#ifdef VXWORKS
  Sockaddr sa(Ins(ins.portId()));
#else
  Sockaddr sa(ins);
#endif
  return (bind(s, sa.name(), sa.sizeofName()) == -1) ? errno : 0;
}

void Port::_close()
{
  if(_socket != -1)
    {
    close(_socket);

    _socket = -1;
    }
}

/**
 * class Client
 */

Client::Client(int sizeofDatagram,
                     int maxPayload,
                     int maxDatagrams) :
  Port(Port::ClientPort,
          sizeofDatagram,
          maxPayload,
          maxDatagrams)
{
#ifdef ODF_LITTLE_ENDIAN
    _swap_buffer = new char[sizeofDatagram+maxPayload];
#endif
}

/*
** ++
**
**    Delete swap buffer under little endian machines
**
** --
*/


Client::~Client() {
#ifdef ODF_LITTLE_ENDIAN
  delete _swap_buffer;
#endif
}

/*
** ++
**
**    Use specified interface for sending multicast addresses
**
** --
*/

void Client::use(unsigned interface)
{
  in_addr address;
  address.s_addr = htonl(interface);
  if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_IF, (char*)&address,
                 sizeof(in_addr)) < 0) 
    error(errno);
}

/**
 * Set the multicast TTL value
 *
 * @param ucTTL  The TTL value
 * @return 0 if successful, 1 if error 
 */
int Client::multicastSetTTL(unsigned char ucTTL)
{
	if (setsockopt(_socket, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ucTTL,
				 sizeof(ucTTL)) < 0) 
		return 1;
	
	return 0;	
}

/*
** ++
**
**    This function is used to transmit the specified datagram to the
**    address specified by the "dst" argument.
**    The datagram is specified in two parts: "datagram", which is a
**    pointer the datagram's fixed size header, and "payload", which
**    is a pointer to the datagrams' variable sized portion. The size
**    of the payload (in bytes) is specified by the "sizeofPayload"
**    argument. The function returns the transmission status. A value
**    of zero (0) indicates the transfer was sucessfull. A positive
**    non-zero value is the reason (as an "errno"value) the transfer
**    failed.
**
** --
*/


int Client::send(char*         datagram,
                    char*         payload,
                    int           sizeofPayload,
                    const Ins& dst)
{
    struct msghdr hdr;
    struct iovec  iov[2];

  if(datagram)
    {
    iov[0].iov_base = (caddr_t)(datagram);
    iov[0].iov_len  = sizeofDatagram();
    iov[1].iov_base = (caddr_t)(payload);
    iov[1].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 2;
    }
  else
    {
    iov[0].iov_base = (caddr_t)(payload);
    iov[0].iov_len  = sizeofPayload;
    hdr.msg_iovlen  = 1;
    }

  Sockaddr sa(dst);
  hdr.msg_name         = (caddr_t)sa.name();
  hdr.msg_namelen      = sa.sizeofName();
  hdr.msg_control      = (caddr_t)0;
  hdr.msg_controllen   = 0;


#ifdef ODF_LITTLE_ENDIAN
  struct iovec iov_swap;
  _swap(iov, hdr.msg_iovlen, &iov_swap);
  hdr.msg_iov       = &iov_swap;
  hdr.msg_iovlen    = 1;
#else
  hdr.msg_iov       = &iov[0];
#endif

  int length = sendmsg(_socket, &hdr, SendFlags);
  return (length == - 1) ? errno : 0;
}

/*
** ++
**
**
**
** --
*/

#ifdef ODF_LITTLE_ENDIAN
void  Client::_swap(const iovec* iov, unsigned msgcount, iovec* iov_swap) {
  unsigned* dst = (unsigned*)_swap_buffer;  
  const iovec* iov_end = iov+msgcount;
  unsigned total = 0;
  do {
    unsigned  len = iov->iov_len;
    unsigned* src = (unsigned*)(iov->iov_base);
    unsigned* end = src + (len >> 2);
    while (src < end) *dst++ = htonl(*src++);
    total += len;
} while (++iov < iov_end);
  iov_swap->iov_len = total;
  iov_swap->iov_base = _swap_buffer; 
}
#endif

/**
 * class ClientTest
 */
ClientTest::ClientTest(unsigned uAddr, unsigned uPort, unsigned char ucTTL,
	char* sInterfaceIp) : _outlet(0,_Nwords*sizeof(unsigned)), _dst(uAddr,uPort)
{
	_outlet.multicastSetTTL(ucTTL);
  
	if ( sInterfaceIp != NULL) 
	{
		printf( "multicast interface IP: %s\n", sInterfaceIp );
		_outlet.use( ntohl(inet_addr(sInterfaceIp)) );
	}  
}

ClientTest::~ClientTest() 
{
}

void ClientTest::send(int iSeed)
{
  for (int i=0;i<_Nwords;i++) _data[i]=  iSeed*100+i;
  printf("\n\rcalling send uAddr %x port %d seed %d\n",
	_dst.address(),_dst.portId(), iSeed);
  _outlet.send(NULL,
	       reinterpret_cast<char*>(&_data), 
	       _Nwords * sizeof(unsigned), 
	       _dst);
}

} // namespace EpicsBld