#include "io.h"

#include <stdio.h>
#include <string.h>
#include <switch.h>

/* callback function to mimic fread using a memory buffer */
size_t buffer_read(void * out_buffer, size_t size, void * user_data) {
    buffer_context * ctxt = (buffer_context *)user_data;
    if (ctxt->current_address >= ctxt->start_address &&
        ctxt->current_address + size <= ctxt->start_address + ctxt->buffer_size) {

        memcpy(out_buffer, ctxt->current_address, size);
        ctxt->current_address += size;
        return size;
    }
    else {
        return 0;
    }
}

/* callback function to mimic fwrite using a memory buffer */
size_t buffer_write(const void * in_buffer, size_t size, void * user_data) {
    buffer_context * ctxt = (buffer_context *)user_data;
    if (ctxt->current_address >= ctxt->start_address &&
        ctxt->current_address + size <= ctxt->start_address + ctxt->buffer_size) {

        memcpy(ctxt->current_address, in_buffer, size);
        ctxt->current_address += size;
        return size;
    }
    else {
        return 0;
    }
}

/* callback function to read data from a file stream */
size_t file_read(void * out_buffer, size_t size, void * user_data) {
    return fread(out_buffer, sizeof(uint8_t), size, (FILE *)user_data);
}

/* callback function to write data to a file stream. Ignore fflush and fsync errors in case of failure */
size_t file_write(const void *in_buffer, size_t size, void *user_data)
{
	int ret = 0;
	ret = fwrite(in_buffer, sizeof(uint8_t), size, (FILE *)user_data);
    if (ret == 0) return ret;

	if(fflush((FILE *)user_data) != 0){
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unable to flush data!\n");
        return ret;
    }
    if(fsync(fileno((FILE *)user_data)) < 0){
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Unable to fsync data!\n");
        return ret;
    }
    return ret;
}
