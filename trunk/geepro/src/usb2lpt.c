#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>        /* this is libusb */
#include "opendevice.h" /* common code moved to separate module */
#include "./firmware/requests.h"   /* custom request numbers */
#include "./firmware/usbconfig.h"  /* device's VID/PID and names */

typedef struct 
{
    usb_dev_handle *handle;
    char *buffer;    
    int  bfsize;
} s_usb2lpt;

char usb2lpt_get(s_usb2lpt *usb, char *buff, int buff_len, int req, int rcv)
{
    int cnt;

    if( !usb ) return 0;
    cnt = usb_control_msg(usb->handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, req, 0, 0, buff, buff_len, 5000);
    if(cnt < rcv){
        if(cnt < 0){
            fprintf(stderr, "USB error: %s\n", usb_strerror());
        }else{
            fprintf(stderr, "only %d of %d bytes received.\n", cnt, rcv);
        }
	return 0;
    }
    return 1;
}        

char usb2lpt_input(s_usb2lpt *usb)
{
    if( !usb ) return 0;
    if(usb2lpt_get( usb, usb->buffer, usb->bfsize, USB_RQ_INPUT, 1 )) return usb->buffer[0];
    return 0;
}

char *usb2lpt_get_id(s_usb2lpt *usb)
{
    if( !usb ) return NULL;
    if(usb2lpt_get( usb, usb->buffer, usb->bfsize, USB_RQ_ID, 7 )) return usb->buffer;
    return NULL;
}

char *usb2lpt_get_rev(s_usb2lpt *usb)
{
    if( !usb ) return NULL;
    if(usb2lpt_get( usb, usb->buffer, usb->bfsize, USB_RQ_REV, 2 )) return usb->buffer;
    return NULL;
}

char usb2lpt_set(s_usb2lpt *usb, int buff_len, int req)
{
    int cnt;
    
    if( !usb ) return 0;
    cnt = usb_control_msg(usb->handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, req, 0, 0, usb->buffer, buff_len, 5000);
    if(cnt != buff_len){
        fprintf(stderr, "USB error: %s\n", usb_strerror());
        return 0;
    }
    return 1;
}

char usb2lpt_output(s_usb2lpt *usb, int data)
{
    int cnt;
    if( !usb ) return 0;
    cnt = usb_control_msg(usb->handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, USB_RQ_OUTPUT, 0, data, usb->buffer, 0, 5000);
    if(cnt != 0){
        fprintf(stderr, "USB error: %s\n", usb_strerror());
        return 0;
    }
    return 1;
}

char usb2lpt_test( s_usb2lpt *usb ) // return TRUE if error
{
    if( !usb ) return 1;
    usb->buffer[0] = 'E';
    usb->buffer[1] = 'c';
    usb->buffer[2] = 'h';
    usb->buffer[3] = 'o';
    usb2lpt_set(usb, 2, USB_RQ_ECHO );
    return !strncmp(usb->buffer, "Echo", 2);
}

static void usb2lpt_usb_detach(s_usb2lpt *usb)
{    
    int retries = 1, usbConfiguration = 1, usbInterface = 0;
    int len;

    if( !usb ) return;
    if( usb_set_configuration(usb->handle, usbConfiguration )){
        fprintf(stderr, "Warning: could not set configuration: %s\n", usb_strerror());
    }
    while((len = usb_claim_interface(usb->handle, usbInterface)) != 0 && retries-- > 0){
        if(usb_detach_kernel_driver_np(usb->handle, 0) < 0){
            fprintf(stderr, "Warning: could not detach kernel driver: %s\n", usb_strerror());
        }
    }
}

void usb2lpt_open( s_usb2lpt *usb )
{
    const unsigned char rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
    char  vendor[] = {USB_CFG_VENDOR_NAME, 0}, product[] = {USB_CFG_DEVICE_NAME, 0};
    int   vid, pid;

    if( !usb ) return;
    /* compute VID/PID from usbconfig.h so that there is a central source of information */
    vid = rawVid[1] * 256 + rawVid[0];
    pid = rawPid[1] * 256 + rawPid[0];
    /* The following function is in opendevice.c: */
    if(usbOpenDevice(&usb->handle, vid, vendor, pid, product, NULL, NULL, NULL) != 0){
        fprintf(stderr, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x. Have you got permissions to USB ?\n", product, vid, pid);
        exit(1);
    }
}

s_usb2lpt *usb2lpt_init()
{
    s_usb2lpt *tmp;
    
    if(!(tmp = (s_usb2lpt *)malloc( sizeof(s_usb2lpt)))){
	printf("Malloc!\n");
	return NULL;
    }
    memset( tmp, 0, sizeof( s_usb2lpt ) );
    tmp->bfsize = 8;
    if(!(tmp->buffer = (char *)malloc( tmp->bfsize ))){
	printf("Malloc!\n");
	free( tmp );
	return NULL;
    }
    usb_init();
    usb2lpt_open( tmp );
    usb2lpt_usb_detach( tmp );
    return tmp;
}

void usb2lpt_free(s_usb2lpt *usb )
{
    if( !usb ) return;
    usb_close( usb->handle );
    if(usb->buffer) free( usb->buffer );
    free( usb );
}

/*
int main(int argc, char **argv)
{
    int i;

    s_usb2lpt *usb;
    
    usb = usb2lpt_init( );

//    printf("ID:%s\n", usb2lpt_get_id( usb ));
//    x = usb2lpt_get_rev( usb ); 
//    printf("Rev:%i.%i\n", x[0], x[1]);

    for(i = 0; i < 11; i++){
	printf("Data: 0x%x\n", usb2lpt_input( usb ));
	usb2lpt_output( usb, 0);
//	printf("Echo: %i\n", usb2lpt_test(usb));
	usleep( 200000);
    }

    usb2lpt_free( usb );
    return 0;
}
*/
