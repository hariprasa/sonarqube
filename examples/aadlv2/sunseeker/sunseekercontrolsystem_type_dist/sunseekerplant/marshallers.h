#ifndef __OCARINA_GENERATED_MARSHALLERS_H_
#define __OCARINA_GENERATED_MARSHALLERS_H_ 
#include <types.h>
#include <po_hi_messages.h>
#include <po_hi_types.h>
#include <request.h>
/*****************************************************/

/*  This file was automatically generated by Ocarina */

/*  Do NOT hand-modify this file, as your            */

/*  changes will be lost when you re-run Ocarina     */

/*****************************************************/

void __po_hi_marshall_type_single_float 
    (beacon_standard__single_float value,
    __po_hi_msg_t* message,
    __po_hi_uint32_t* offset);

void __po_hi_unmarshall_type_single_float 
    (beacon_standard__single_float* value,
    __po_hi_msg_t* message,
    __po_hi_uint32_t* offset);

void __po_hi_marshall_request_plant_controllerinput 
    (__po_hi_request_t* request,
    __po_hi_msg_t* message,
    __po_hi_uint32_t* offset);

void __po_hi_unmarshall_request_plant_controllerinput 
    (__po_hi_request_t* request,
    __po_hi_msg_t* message,
    __po_hi_uint32_t* offset);

void __po_hi_marshall_request_plant_outputfeedback 
    (__po_hi_request_t* request,
    __po_hi_msg_t* message,
    __po_hi_uint32_t* offset);

void __po_hi_unmarshall_request_plant_outputfeedback 
    (__po_hi_request_t* request,
    __po_hi_msg_t* message,
    __po_hi_uint32_t* offset);

void __po_hi_marshall_request 
    (__po_hi_request_t* request,
    __po_hi_msg_t* message);

void __po_hi_unmarshall_request 
    (__po_hi_request_t* request,
    __po_hi_msg_t* message);

#endif