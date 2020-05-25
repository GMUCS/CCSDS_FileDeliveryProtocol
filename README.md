This is not finished! - currently working on it!

This is a FTP protocol that adheres to the CCSDS (Consultative Committee for Space Data Systems) spec for sending files into space. This project is being built for a student lead space initiative at the Univerity of Alberta called ABsat. 

Supported operation systems:
- Posix,
- FREE-RTOS

Supported network stacks:
- tcp
- udp 
- csp (cube sate protocol)
- generic (other, including no network at all!)


# Compilation Notes:

If you are compiling on linux, it should compile posix compliant by default.

### Compiling CSP:
first, one needs to build the .a file for your specific cpu architecture. 
instructions can be found here. https://github.com/libcsp/libcsp

archive file: 
Once one has the .a file. simply link to it my adding the path to 
STATIC_FILES in our makefile: STATIC_FILES += /path/to/libcsp.a

.h files:
We need to include the .h files to our include path. We can do this 
by linking the .h files to our INCLUDES in our makefile. 
INCLUDES += /path/to/libcsp/include

The last thing you need to do to let this app integrate seemlessly with csp,
is to make sure that CSP_NETWORK is defined in port.h

run make to compile!

### Compiling with FreeRTOS:
The best way to compile with FreeRTOS is to do the same thing as we did 
with libscp -- create a .a file, and link to the .h files.

There are examples to help you with linking in the makefile.

Once again, make sure that FREE_RTOS_PORT is definted in port.h
and that POSIX_PORT is not defined. 


# Getting started:

The app has some startup requirements. init the app with:

    FTP *app = init_ftp(my_cfdp_id);
    if (app == NULL) {
        return 1;
    }

this will spin up a thread in posix, or a task in FreeRTOS. 
if you want to join this thread (make the current executing thread wait)
and it is a posix thread you can run:
ssp_thread_join(app->server_handle);

if you want to exit this task for any reason set app->close = 1;
this will run the exiting subroutines for us.

if you wish to send a file to a peer:

params:
id of destination,
source file name,
name of the file as it will arrive at destination,
ACKNOWLEDGED_MODE/UN_ACKNOWLEDGED_MODE (ACKNOWLEDGED_MODE will allow for acks/naks to be sent.),
app.

    example:
    Request *req = put_request(<destination id>, "pictures/src.jpg", "pictures/desination.jpg", ACKNOWLEDGED_MODE, app);

    start_request(req);
    
    
if you wish to get a file from a peer, we need to add a 'message' onto a put request,
we can specify that there is no associated put request if we set the filenames to NULL:

params:
id of destination,
source file name,
name of the file as it will arrive at destination,
ACKNOWLEDGED_MODE/UN_ACKNOWLEDGED_MODE (ACKNOWLEDGED_MODE will allow for acks/naks to be sent.),
app.

example:

    Request *req = put_request(<cfid of destination>, NULL, NULL, ACKNOWLEDGED_MODE, app);

    add_proxy_message_to_request(<cfid of proxy destination>, <length_of_id_in_octets>, "source_file_name" "destination_file_name", req);

    start_request(req);
    
    hint, if you want to 'get' a file, set the <cfid of proxy destination> to your id.


# MIB (management information base)
    
This base holds the configuration information for users (peers or applications).
This data is used for determining how and who we can send file or commands
to. Files will not be sent to users that are not included in the MIB.

The MIB is currently configured as key value pairs. These pairs are formatted
in the JSON format, the name of the file is peer_<cfid_id>. 

Here is an example of a MIB entry:
    {
    "cfdp_id": 1,
    "UT_address" : 2130706433,
    "UT_port" : 1111,
    "type_of_network" : 1,
    "default_transmission_mode" : 1,
    "MTU" : 1500,

    "one_way_light_time" : 123,
    "total_round_trip_allowance" : 123,
    "async_NAK_interval" : 123,
    "async_keep_alive_interval" : 123,
    "async_report_interval" : 123,
    "immediate_nak_mode_enabled" : 123,
    "prompt_transmission_interval" : 123,
    "disposition_of_incomplete" : 123,
    "CRC_required" : 0,
    "keep_alive_discrepancy_limit" : 8,
    "positive_ack_timer_expiration_limit" : 123,
    "nak_timer_expiration_limit" : 123,
    "transaction_inactivity_limit" : 123
    }


Below are the meanings of the fields for the MIB

cfdp_id
    This is the unique identifier of a peer on the network. We can start it at 1
    and increment it from there. This is an unsigned integer (32 bit) value;


UT_address
    This is an Underlying Transmission address. For example, in an IP stack, this
    would be an IP4 Ip address. This value is a decimal represenstation of an IP
    address. This particular one is 127.0.0.1. 


UT_port
    This os an Underlying Transmission port. For example, in an IP stack, this
    would be a 16 bit value -- like port 8080. Combinded with the UT_address, 
    together they form a complete UT address. The one above would be equal to
    127.0.0.1:1111. This is an unsigned integer (16 bit) value

type_of_network
    This number represents what type of network the underlying UT address takes.
    currently, the only acceptable values are 0 and 1.

default_transmission_mode:
    not implmented

MTU
    This number represents the 'maximum transmissable unit' -- this will also
    take the form of a buffer in the program. This value is the maximum size 
    packet that the application will receive. 



if you want to get in contact with me
email me at evangiese77@gmail.com
