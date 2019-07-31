#include <arpa/inet.h>
#include <stdio.h>
//#include "protocol_interface.h"

extern "C" {

const static unsigned PKT_MAX_LEN = (32<<20);

int longconn_complete_func(const void* data, unsigned data_len);

int net_complete_func(const void* data, unsigned data_len)
{

    //printf("data_len = %u\n", data_len);
    if (data_len > PKT_MAX_LEN)
    {
        printf("data_len > PKT_MAX_LEN\n");
        return -1;
    }

    int ret = longconn_complete_func(data, data_len);

    if (ret == -1)
    {
        ret = *((int *)data);
    }

    return ret;
}

int longconn_complete_func(const void* data, unsigned data_len)
{
    //printf("Enter complete, data_len: %u\n", data_len);
	unsigned total_len = 0;

	if ( data_len < 4 ) {
		return 0;
	}

	total_len = *((unsigned *)data);


    //printf("total_len before ntohl: %d sizeof(unsigned)=%lu\n", total_len, sizeof(unsigned));

	total_len = ntohl(total_len);

    //printf("total_len: %d\n", total_len);

	if ( total_len > PKT_MAX_LEN ) {
        printf("longcon, total_len(%u) > PKT_MAX_LEN(%u)\n", total_len, PKT_MAX_LEN);
		return -1;
	}

	if ( data_len < total_len ) {
		return 0;
	}

	return total_len;
}

/*
 * mcd_route_func():	MCD route by message call back. Must named "mcd_route_func" in so file.
 * @pdata:			Network message.
 * @data_len:			Length of message.
 * @flow:			Flow number.
 * @listen_port:		Listen port which create the connection.
 * @req_mq_cnt:		Request mq count.
 * Returns:			Return the request mq index to enqueue when 0 <= return_value < req_mq_cnt.
 *					Otherwise ccd will choose last MCD.
 */
unsigned
mcd_route_func(void *pdata, unsigned data_len, unsigned long long flow, unsigned short listen_port, unsigned req_mq_cnt)
{
	static unsigned ret_index = 0;

	//printf("in mcd_route_func data_len:%u\n", data_len);

	return ((++ret_index) % req_mq_cnt);
}

}

