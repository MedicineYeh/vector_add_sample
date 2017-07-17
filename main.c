/***************
  Copyright (c) 2015, MedicineYeh
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are
 permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of
 conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of
 conditions and the following disclaimer in the documentation and/or other materials
 provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************/

#define DATA_SIZE 16
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>
#include "main.h"

#define check_release(value, message)                                                    \
    if (value) {                                                                         \
        printf(message "\n");                                                            \
        goto release;                                                                    \
    }

double get_event_exec_time(cl_event event)
{
    cl_ulong start_time, end_time;
    /*Get start device counter for the event*/
    clGetEventProfilingInfo(
      event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start_time, NULL);
    /*Get end device counter for the event*/
    clGetEventProfilingInfo(
      event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end_time, NULL);
    /*Convert the counter values to milli seconds*/
    double total_time = (end_time - start_time) * 1e-6;
    return total_time;
}

cl_program load_program(cl_context context, cl_device_id device, const char *filename)
{
    FILE *     fp = fopen(filename, "rt");
    size_t     length;
    char *     data;
    char *     build_log;
    size_t     ret_val_size;
    cl_program program = 0;
    cl_int     status  = 0;

    if (!fp) return 0;

    // get file length
    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // read program source
    data = (char *)malloc(length + 1);
    fread(data, sizeof(char), length, fp);
    data[length] = '\0';

    // create and build program
    program = clCreateProgramWithSource(context, 1, (const char **)&data, 0, 0);
    if (program == 0) return 0;

    status = clBuildProgram(program, 0, 0, 0, 0, 0);
    if (status != CL_SUCCESS) {
        printf("Error:  Building Program from file %s\n", filename);
        clGetProgramBuildInfo(
          program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
        build_log = (char *)malloc(ret_val_size + 1);
        clGetProgramBuildInfo(
          program, device, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
        build_log[ret_val_size] = '\0';
        printf("Building Log:\n%s", build_log);
        return 0;
    }

    return program;
}

bool get_cl_context(cl_context *context, cl_device_id **devices, int num_platform)
{
    if (context == NULL || devices == NULL) return false;
    cl_platform_id *platforms = NULL;
    // The iteration variable
    int i;

    cl_uint num;
    check_err(clGetPlatformIDs(0, 0, &num), "Unable to get platforms");

    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num);
    check_err(clGetPlatformIDs(num, platforms, NULL), "Unable to get platform ID");

    check_err(clGetPlatformIDs(0, 0, &num), "Unable to get platforms");

    printf("Found %d platforms:\n", num);
    for (i = 0; i < num; i++) {
        char str[1024];
        clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 1024, str, NULL);
        printf("\t%d: %s\n", i, str);
    }

    cl_context_properties prop[3];
    prop[0] = CL_CONTEXT_PLATFORM;
    prop[1] = (cl_context_properties)platforms[num_platform];
    prop[2] = 0;
    cl_int err;
    *context = clCreateContextFromType(prop, CL_DEVICE_TYPE_ALL, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        printf("Can't create OpenCL context\n");
        return false;
    }

    size_t size_b;
    int    num_total_devices;
    clGetContextInfo(*context, CL_CONTEXT_DEVICES, 0, NULL, &size_b);
    *devices = (cl_device_id *)malloc(size_b);
    clGetContextInfo(*context, CL_CONTEXT_DEVICES, size_b, *devices, 0);
    if (size_b == 0) {
        printf("Can't get devices\n");
        return false;
    }
    num_total_devices = size_b / sizeof(cl_device_id);

    printf("Found %d devices:\n", num_total_devices);
    for (i = 0; i < num_total_devices; i++) {
        char devname[16][256] = {};
        clGetDeviceInfo(*devices[i], CL_DEVICE_NAME, 256, devname[i], 0);
        printf("\t%d: %s", i, devname[i]);
        clGetDeviceInfo(*devices[i], // Set the device info
                        CL_DEVICE_MAX_COMPUTE_UNITS,
                        sizeof(int),
                        &size_b,
                        0);
        printf("  - %d\n", (int)size_b);
    }

    return true;
}

int main(int argc, char **argv)
{
    cl_int           err     = 0;
    cl_context       context = 0;
    cl_device_id *   devices = NULL;
    cl_command_queue queue   = 0;
    cl_program       program = 0;
    cl_mem           cl_a = 0, cl_b = 0, cl_res = 0;
    cl_kernel        adder = 0;
    cl_event         event;
    // The iteration variable
    int i;
    // Define our data set
    cl_float a[DATA_SIZE], b[DATA_SIZE], res[DATA_SIZE];

    // Initialize array
    srand(time(0));
    for (i = 0; i < DATA_SIZE; i++) {
        a[i]   = (rand() % 100) / 100.0;
        b[i]   = (rand() % 100) / 100.0;
        res[i] = 0;
    }

    check_release(get_cl_context(&context, &devices, 0) == false,
                  "Fail to create context");

    // Specify the queue to be profile-able
    queue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, 0);
    check_release(queue == NULL, "Can't create command queue");

    program = load_program(context, devices[0], "shader.cl");
    check_release(program == NULL, "Fail to build program");

    cl_a =
      clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float) * DATA_SIZE, NULL, NULL);
    cl_b =
      clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(cl_float) * DATA_SIZE, NULL, NULL);
    cl_res = clCreateBuffer(
      context, CL_MEM_WRITE_ONLY, sizeof(cl_float) * DATA_SIZE, NULL, NULL);
    if (cl_a == 0 || cl_b == 0 || cl_res == 0) {
        printf("Can't create OpenCL buffer\n");
        goto release;
    }

    check_release(clEnqueueWriteBuffer(
                    queue, cl_a, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, a, 0, 0, 0),
                  "Write Buffer 1");
    check_release(clEnqueueWriteBuffer(
                    queue, cl_b, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, b, 0, 0, 0),
                  "Write Buffer 2");

    adder = clCreateKernel(program, "adder", &err);
    if (err == CL_INVALID_KERNEL_NAME) printf("CL_INVALID_KERNEL_NAME\n");
    check_release(adder == NULL, "Can't load kernel");

    clSetKernelArg(adder, 0, sizeof(cl_mem), &cl_a);
    clSetKernelArg(adder, 1, sizeof(cl_mem), &cl_b);
    clSetKernelArg(adder, 2, sizeof(cl_mem), &cl_res);

    size_t work_size = DATA_SIZE;

    check_release(clEnqueueNDRangeKernel(queue, adder, 1, 0, &work_size, 0, 0, 0, &event),
                  "Can't enqueue kernel");
    check_release(
      clEnqueueReadBuffer(
        queue, cl_res, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, res, 0, 0, 0),
      "Can't enqueue read buffer");

    clWaitForEvents(1, &event);
    printf("Execution Time: %.04lf ms\n\n", get_event_exec_time(event));

    // Make sure everything is done before we do anything
    clFinish(queue);
    err = 0;
    for (i = 0; i < DATA_SIZE; i++) {
        if (res[i] != a[i] + b[i]) {
            printf("%f + %f = %f(answer %f)\n", a[i], b[i], res[i], a[i] + b[i]);
            err++;
        }
    }
    if (err == 0)
        printf("Validation passed\n");
    else
        printf("Validation failed\n");
    printf("------\n");

    //--------------------------------
    // Second test
    for (i = 0; i < DATA_SIZE; i++) {
        a[i]   = i;
        b[i]   = i;
        res[i] = 0;
    }

    check_err(clEnqueueWriteBuffer(
                queue, cl_a, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, a, 0, 0, 0),
              "Write Buffer 1");
    check_err(clEnqueueWriteBuffer(
                queue, cl_b, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, b, 0, 0, 0),
              "Write Buffer 2");

    check_err(clEnqueueNDRangeKernel(queue, adder, 1, 0, &work_size, 0, 0, 0, &event),
              "Can't enqueue kernel");
    check_err(clEnqueueReadBuffer(
                queue, cl_res, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, res, 0, 0, 0),
              "Can't enqueue read buffer");

    clWaitForEvents(1, &event);
    printf("Execution Time: %.04lf ms\n\n", get_event_exec_time(event));

    // Make sure everything is done before we do anything
    clFinish(queue);
    err = 0;
    for (i = 0; i < DATA_SIZE; i++) {
        if (res[i] != a[i] + b[i]) {
            printf("%f + %f = %f(answer %f)\n", a[i], b[i], res[i], a[i] + b[i]);
            err++;
        }
    }
    if (err == 0)
        printf("Validation passed\n");
    else
        printf("Validation failed\n");

release:
    clReleaseKernel(adder);
    clReleaseProgram(program);
    clReleaseMemObject(cl_a);
    clReleaseMemObject(cl_b);
    clReleaseMemObject(cl_res);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
