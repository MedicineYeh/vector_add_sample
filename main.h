#ifndef _MAIN_H__
#define _MAIN_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <CL/cl.h>

double get_event_exec_time(cl_event event);
cl_program load_program(cl_context context, cl_device_id device, const char* filename);
bool get_cl_context(cl_context* context, cl_device_id** devices, int num_platform);

static inline void check_err(size_t err_num, const char* statement)
{
    if (err_num) {
        printf("%s\n", statement);
        exit(err_num);
    }
}

#endif
