/*
 * This is a part of PolyORB-HI-C distribution, a minimal
 * middleware written for generated code from AADL models.
 * You should use it with the Ocarina toolsuite.
 *
 * For more informations, please visit http://ocarina.enst.fr
 *
 * Copyright (C) 2010, European Space Agency
 */

#include <deployment.h>

#if (defined (__PO_HI_NEED_DRIVER_SOCKETS) || defined (__PO_HI_NEED_DRIVER_SOCKETS_ASN1))

#include <activity.h>

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>


#include <po_hi_debug.h>
#include <po_hi_transport.h>
#include <po_hi_common.h>
#include <po_hi_main.h>
#include <po_hi_time.h>
#include <po_hi_task.h>
#include <drivers/po_hi_driver_sockets_asn1.h>
#include <drivers/po_hi_driver_sockets_common.h>
#include <drivers/po_hi_driver_sockets.h>

#ifdef __PO_HI_NEED_DRIVER_SOCKETS
__po_hi_inetnode_t nodes[__PO_HI_NB_NODES];
__po_hi_inetnode_t rnodes[__PO_HI_NB_NODES];
#else
__po_hi_inetnode_t nodes[__PO_HI_NB_DEVICES];
__po_hi_inetnode_t rnodes[__PO_HI_NB_DEVICES];
#endif

__po_hi_device_id my_id;

#ifdef __PO_HI_NEED_DRIVER_SOCKETS_ASN1
void __po_hi_driver_sockets_init (__po_hi_device_id id)
{
   int                i;
   int                ret;
   int                reuse;
   char               *tmp;
   __po_hi_uint16_t   dev;
   __po_hi_time_t     mytime;
   struct sockaddr_in sa;
   struct hostent*    hostinfo;

   char* dev_conf;
   char dev_addr[16];
   int  dev_port;
   int node;

   my_id = id;

   memset (dev_addr, '\0', 16);

   for (node = 0 ; node < __PO_HI_NB_DEVICES ; node++)
   {
      nodes[node].socket = -1;
   }


   dev_conf = __po_hi_get_device_naming (id);
   dev_port = 0;

   if (sscanf (dev_conf, "eth %s %d", dev_addr, &dev_port) == 0)
   {
      __DEBUGMSG ("[DRIVER SOCKETS] Unable to parse device configuration (id=%d)\n", id);
   }

   __DEBUGMSG ("My configuration, addr=%s, port=%d\n", dev_addr, dev_port);

   /*
    * If the current node port has a port number, then it has to
    * listen to other nodes. So, we create a socket, bind it and
    * listen to other nodes.
    */
   if (dev_port != 0)
   {
      nodes[id].socket = socket (AF_INET, SOCK_STREAM, 0);

      if (nodes[id].socket == -1 )
      {
#ifdef __PO_HI_DEBUG
         __DEBUGMSG ("Cannot create socket for device %d\n", id);
#endif
         return;
      }

      reuse = 1;
      setsockopt (nodes[id].socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

      sa.sin_addr.s_addr = htonl (INADDR_ANY);   /* We listen on all adresses */
      sa.sin_family = AF_INET;                   
      sa.sin_port = htons (dev_port);   /* Port provided by the generated code */

      if( bind( nodes[id].socket , ( struct sockaddr * ) &sa , sizeof( struct sockaddr_in ) ) < 0 )
      {
#ifdef __PO_HI_DEBUG
         __DEBUGMSG ("Unable to bind socket and port on socket %d\n", nodes[id].socket);
#endif
      }

      if( listen( nodes[id].socket , __PO_HI_NB_ENTITIES ) < 0 )
      {
#ifdef __PO_HI_DEBUG
         __DEBUGMSG ("Cannot listen on socket %d\n", nodes[mynode].socket);
#endif
      }

      /* 
       * Create the thread which receive all data from other
       * nodes. This thread will execute the function
       * __po_hi_receiver_task
       */

      __po_hi_initialize_add_task ();

      __po_hi_create_generic_task 
         (-1, 0,__PO_HI_MAX_PRIORITY, 0, __po_hi_sockets_asn1_poller);
   }

   /*
    * For each node in the sytem that may communicate with the current
    * node we create a socket. This socket will be used to send data.
    */
   for (dev = 0 ; dev < __PO_HI_NB_DEVICES ; dev++ )
   {
      if (dev == id)
      {
         continue;
      }

      memset (dev_addr, '\0', 16);
      dev_port = 0;

      dev_conf = __po_hi_get_device_naming (dev);

      if (sscanf (dev_conf, "eth %s %d", dev_addr, &dev_port) == 0)
      {
         __DEBUGMSG ("[DRIVER SOCKETS] Unable to parse device configuration (id=%d)\n", id);
         continue;
      }

      __DEBUGMSG ("[DRIVER SOCKETS] Configuration for device %d, addr=%s, port=%d\n", dev, dev_addr, dev_port);

      if (dev_port == 0)
      {
         continue;
      }

      while (1)
      {
         nodes[dev].socket = socket (AF_INET, SOCK_STREAM, 0);

         if (nodes[dev].socket == -1 )
         {
#ifdef __PO_HI_DEBUG
            __DEBUGMSG ("[DRIVER SOCKETS] Socket for node %d is not created", node);
#endif
            return;
         }

         hostinfo = gethostbyname ((char*)dev_addr);

         if (hostinfo == NULL )
         {
            __DEBUGMSG ("[DRIVER SOCKETS] Error while getting host informations for device %d\n", dev);
         }

         sa.sin_port = htons (dev_port);
         sa.sin_family = AF_INET;

         /* The following lines are used to copy the
          * hostinfo->h_length to the sa.sin_addr member. Most
          * of program use the memcpy to do that, but the
          * RT-POSIX profile we use forbid the use of this
          * function.  We use a loop instead to perform the
          * copy.  So, these lines replace the code :
          *
          * memcpy( (char*) &( sa.sin_addr ) , (char*)hostinfo->h_addr , hostinfo->h_length );
          */

         tmp = (char*) &(sa.sin_addr);
         for (i=0 ; i<hostinfo->h_length ; i++)
         {
            tmp[i] = hostinfo->h_addr[i];
         }

         /*
          * We try to connect on the remote host. We try every
          * second to connect on.
          */

         ret = connect (nodes[dev].socket, 
                        (struct sockaddr*) &sa ,
                        sizeof (struct sockaddr_in));

         if (ret == 0)
         {

            __DEBUGMSG ("[DRIVER SOCKETS] Send my id (%d)\n", id);
            if (write (nodes[dev].socket, &id, sizeof (__po_hi_device_id)) != sizeof (__po_hi_device_id))
            {
               __DEBUGMSG ("[DRIVER SOCKETS] Device %d cannot send his id\n", id);
            }
            break;
         }

         if (close (nodes[dev].socket))
         {
            __DEBUGMSG ("[DRIVER SOCKETS] Cannot close socket %d\n", nodes[dev].socket);
         }

         /*
          * We wait 500ms each time we try to connect on the
          * remote host
          */

         __po_hi_get_time (&mytime);
         __DEBUGMSG ("[DRIVER SOCKETS] Cannot connect on device %d, wait 500ms\n", dev);
         __po_hi_delay_until (__po_hi_add_times (mytime, __po_hi_milliseconds (500)));
      }
   }
}
#else
void __po_hi_driver_sockets_init (__po_hi_device_id id)
{
   int                i;
   int                ret;
   int                reuse;
   char               *tmp;
   __po_hi_time_t     mytime;
   struct sockaddr_in sa;
   struct hostent*    hostinfo;

   char* dev_conf;
   char dev_addr[16];
   int  dev_port;
   int node;

   memset (dev_addr, '\0', 16);


   for (node = 0 ; node < __PO_HI_NB_NODES ; node++)
   {
      nodes[node].socket = -1;
   }

   /*
    * If the current node port has a port number, then it has to
    * listen to other nodes. So, we create a socket, bind it and
    * listen to other nodes.
    */
   if ( node_port[mynode] != __PO_HI_NOPORT )
   {
      nodes[mynode].socket = socket (AF_INET, SOCK_STREAM, 0);

      if (nodes[mynode].socket == -1 )
      {
#ifdef __PO_HI_DEBUG
         __DEBUGMSG ("Cannot create socket for node %d\n", mynode);
#endif
         return;
      }

      reuse = 1;
      setsockopt (nodes[mynode].socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

      sa.sin_addr.s_addr = htonl (INADDR_ANY);   /* We listen on all adresses */
      sa.sin_family = AF_INET;                   
      sa.sin_port = htons (node_port[mynode]);   /* Port provided by the generated code */

      if( bind( nodes[mynode].socket , ( struct sockaddr * ) &sa , sizeof( struct sockaddr_in ) ) < 0 )
      {
#ifdef __PO_HI_DEBUG
         __DEBUGMSG ("Unable to bind socket and port on socket %d\n", nodes[mynode].socket);
#endif
      }

      if( listen( nodes[mynode].socket , __PO_HI_NB_ENTITIES ) < 0 )
      {
#ifdef __PO_HI_DEBUG
         __DEBUGMSG ("Cannot listen on socket %d\n", nodes[mynode].socket);
#endif
      }

      /* 
       * Create the thread which receive all data from other
       * nodes. This thread will execute the function
       * __po_hi_receiver_task
       */

      __po_hi_initialize_add_task ();

      __po_hi_create_generic_task 
         (-1, 0,__PO_HI_MAX_PRIORITY, 0, __po_hi_sockets_receiver_task);
   }

   /*
    * For each node in the sytem that may communicate with the current
    * node we create a socket. This socket will be used to send data.
    */
   for (node = 0 ; node < __PO_HI_NB_NODES ; node++ )
   {
      if ( (node != mynode) && (node_port[node] != __PO_HI_NOPORT) && (nodes[node].socket == -1) )
      {
         while (1)
         {
            nodes[node].socket = socket (AF_INET, SOCK_STREAM, 0);

            if (nodes[node].socket == -1 )
            {
#ifdef __PO_HI_DEBUG
               __DEBUGMSG ("Socket for node %d is not created", node);
#endif
               return;
            }

            hostinfo = gethostbyname ((char*)node_addr[node]);

            if (hostinfo == NULL )
            {
#ifdef __PO_HI_DEBUG
               __DEBUGMSG ("Error while getting host informations for node %d\n", node);
#endif
            }

            sa.sin_port = htons( node_port[node] );
            sa.sin_family = AF_INET;

            /* The following lines are used to copy the
             * hostinfo->h_length to the sa.sin_addr member. Most
             * of program use the memcpy to do that, but the
             * RT-POSIX profile we use forbid the use of this
             * function.  We use a loop instead to perform the
             * copy.  So, these lines replace the code :
             *
             * memcpy( (char*) &( sa.sin_addr ) , (char*)hostinfo->h_addr , hostinfo->h_length );
             */

            tmp = (char*) &(sa.sin_addr);
            for (i=0 ; i<hostinfo->h_length ; i++)
            {
               tmp[i] = hostinfo->h_addr[i];
            }

            /*
             * We try to connect on the remote host. We try every
             * second to connect on.
             */

            ret = connect 
               (nodes[node].socket, ( struct sockaddr* ) &sa , sizeof( struct sockaddr_in ));

            if (ret == 0)
            {
               if (write (nodes[node].socket, &mynode, sizeof (__po_hi_node_t)) != sizeof (__po_hi_node_t))
               {
#ifdef __PO_HI_DEBUG
                  __DEBUGMSG ("Node %d cannot send his node-id\n", node);
#endif
               }
               break;
            }

            if (close (nodes[node].socket))
            {
#ifdef __PO_HI_DEBUG
               __DEBUGMSG ("Cannot close socket %d\n", nodes[node].socket);
#endif
            }

            /*
             * We wait 500ms each time we try to connect on the
             * remote host
             */

            __po_hi_get_time (&mytime);
            __po_hi_delay_until (__po_hi_add_times (mytime, __po_hi_milliseconds (500)));
         }
      }
   }
}
#endif

#endif /* __PO_HI_NEED_DRIVER_SOCKETS || __PO_HI_NEED_DRIVER_SOCKETS_ASN1 */

