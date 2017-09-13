#ifndef	__INCmisc_routinesh
#define	__INCmisc_routinesh

#include "wlantype.h"
#include "athreg.h"
#include "manlib.h"

A_UINT32 m_send_frame_and_recv(
   A_UINT32 devIndex, 
   A_UINT8 *pBuffer, 
   A_UINT32 tx_desc_ptr, 
   A_UINT32 tx_buf_ptr, 
   A_UINT32 rx_desc_ptr, 
   A_UINT32 rx_buf_ptr,
   A_UINT32 rate_code
);

A_UINT32 m_recv_frame_and_xmit(
   A_UINT32 devIndex, 
   A_UINT32 tx_desc_ptr, 
   A_UINT32 tx_buf_ptr, 
   A_UINT32 rx_desc_ptr, 
   A_UINT32 rx_buf_ptr,
   A_UINT32 rate_code
);

#endif
