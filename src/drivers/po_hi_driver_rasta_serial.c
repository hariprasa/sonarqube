/*
 * This is a part of PolyORB-HI-C distribution, a minimal
 * middleware written for generated code from AADL models.
 * You should use it with the Ocarina toolsuite.
 *
 * Copyright (C) 2010, European Space Agency
 */

#include <deployment.h>
/* Generated code header */

#ifdef __PO_HI_NEED_DRIVER_SERIAL_RASTA

#include <activity.h>
#include <marshallers.h>

#include <po_hi_debug.h>
#include <po_hi_transport.h>
#include <po_hi_gqueue.h>
#include <po_hi_messages.h>
#include <po_hi_utils.h>
#include <drivers/po_hi_rtems_utils.h>
#include <drivers/po_hi_driver_rasta_serial.h>
#include <drivers/po_hi_driver_rasta_common.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
/* POSIX-style files */

#include <pci.h>
#include <rasta.h>
#include <apbuart_rasta.h>
/* Rasta includes from GAISLER drivers */

#define __PO_HI_DRIVER_SERIAL_RASTA_DEVICE "/dev/apburasta0"
#define __PO_HI_DRIVER_SERIAL_RASTA_BAUDRATE 19200

int po_hi_c_driver_rasta_serial_fd;

__po_hi_msg_t        __po_hi_c_driver_rasta_serial_msg;
__po_hi_request_t    __po_hi_c_driver_rasta_serial_request;

void __po_hi_c_driver_serial_rasta_poller (void)
{

   int n;
   int ts;
   int tr;
   uint8_t* ptr;

   __DEBUGMSG ("[RASTA SERIAL] Poller waits for incoming message !\n");
   ts = __PO_HI_MESSAGES_MAX_SIZE;
   tr = 0;
   ptr = &(__po_hi_c_driver_rasta_serial_msg.content[0]);
   __po_hi_msg_reallocate (&__po_hi_c_driver_rasta_serial_msg);
   while (ts > 0)
   {
      n = read (po_hi_c_driver_rasta_serial_fd, ptr, ts); 
      ptr += n;

      if (n == -1)
      {
         __DEBUGMSG("[RASTA SERIAL] Cannot read on socket !\n");
         return;
      }
      ts -= n;
   tr += n;
   }

   __DEBUGMSG ("[RASTA SERIAL] read() returns total %d, max message size=%d\n", tr, __PO_HI_MESSAGES_MAX_SIZE);

   __DEBUGMSG ("[RASTA SERIAL] Message received by poller: 0x");
   for (ts = 0 ; ts < __PO_HI_MESSAGES_MAX_SIZE ; ts++)
   {
      __DEBUGMSG ("%x", __po_hi_c_driver_rasta_serial_msg.content[ts]);
   }
   __DEBUGMSG ("\n");

   __po_hi_c_driver_rasta_serial_msg.length = tr;

   __po_hi_unmarshall_request (&__po_hi_c_driver_rasta_serial_request,
                               &__po_hi_c_driver_rasta_serial_msg);

   __po_hi_main_deliver (&__po_hi_c_driver_rasta_serial_request);
}

void __po_hi_c_driver_serial_rasta_init (__po_hi_device_id id)
{
   __po_hi_c_driver_rasta_common_init ();
   po_hi_c_driver_rasta_serial_fd = open (__PO_HI_DRIVER_SERIAL_RASTA_DEVICE, O_RDWR);

   if (po_hi_c_driver_rasta_serial_fd < 0)
   {
      __DEBUGMSG ("[RASTA SERIAL] Error while opening device %s\n", __PO_HI_DRIVER_SERIAL_RASTA_DEVICE);
   }

  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_SET_BAUDRATE, __PO_HI_DRIVER_SERIAL_RASTA_BAUDRATE); /* stream mode */
  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_SET_BLOCKING, APBUART_BLK_RX | APBUART_BLK_TX | APBUART_BLK_FLUSH);
  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_SET_TXFIFO_LEN, 64);  /* Transmitt buffer 64 chars */
  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_SET_RXFIFO_LEN, 256); /* Receive buffer 256 chars */
  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_SET_ASCII_MODE, 0); /* Make \n go \n\r or \r\n */
  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_CLR_STATS, 0);
  __PO_HI_DRIVERS_RTEMS_UTILS_IOCTL(po_hi_c_driver_rasta_serial_fd, APBUART_START, 0);

  if (tcflush (po_hi_c_driver_rasta_serial_fd, TCIOFLUSH) != 0)
  {
     __DEBUGMSG("[RASTA SERIAL] Error when trying to flush\n");
  }
}

int __po_hi_c_driver_serial_rasta_sender (const __po_hi_task_id task_id, const __po_hi_port_t port)
{
   int n;
   int ts;
   __po_hi_local_port_t local_port;
   __po_hi_request_t* request;
   __po_hi_msg_t msg;
   __po_hi_port_t destination_port;

   local_port = __po_hi_get_local_port_from_global_port (port);

   request = __po_hi_gqueue_get_most_recent_value (task_id, local_port);

   destination_port     = __po_hi_gqueue_get_destination (task_id, local_port, 0);

   __po_hi_msg_reallocate (&msg);

   request->port = destination_port;

   __po_hi_marshall_request (request, &msg);

   __DEBUGMSG  ("[RASTA SERIAL] Message sent: 0x");
   for (ts = 0 ; ts < __PO_HI_MESSAGES_MAX_SIZE ; ts++)
   {
      __DEBUGMSG ("%x", msg.content[ts]);
   }
   __DEBUGMSG ("\n");

   n = write (po_hi_c_driver_rasta_serial_fd, &msg, __PO_HI_MESSAGES_MAX_SIZE);

   __DEBUGMSG ("[RASTA SERIAL] write() returns %d\n", n);
   return 1;
}

#endif

