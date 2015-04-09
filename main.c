/***************
  Copyright (c) 2015, MedicineYeh
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************/

#define DATA_SIZE 16
#include <stdio.h>
#include <stdlib.h>    
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

#define checkErr(fun, statement) err = fun;\
                                 if (err != CL_SUCCESS) {statement}
#define checkExit(value, message) if (value == 0) {printf(message); goto release;}

cl_program load_program(cl_context context, cl_device_id device, const char* filename)
{
    FILE *fp = fopen(filename, "rt");
    size_t length;
    char *data;
    char *build_log;
    size_t ret_val_size;
    cl_program program = 0;
    cl_int status = 0;

    if(!fp) return 0;

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
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
        build_log = (char *)malloc(ret_val_size + 1);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
        build_log[ret_val_size] = '\0';
        printf("Building Log:\n%s", build_log);
        return 0;
    }

    return program;
}

int main(int argc, char **argv)
{
    cl_int err = 0;
    cl_uint num = 0;
    cl_platform_id *platforms = NULL;
    cl_context_properties prop[3] = {0};
    cl_context context = 0;
    cl_device_id *devices = NULL;
    cl_command_queue queue = 0;
    cl_program program = 0;
    cl_mem cl_a = 0, cl_b = 0, cl_res = 0;
    cl_kernel adder = 0;
    int num_total_devices = 0;
    char devname[16][256] = {{0}};
    size_t cb, work_size;
    cl_float a[DATA_SIZE], b[DATA_SIZE], res[DATA_SIZE];
    int i;

    //Initialize array
    srand(time(0));
    for(i = 0; i < DATA_SIZE; i++) {
        a[i] = (rand() % 100) / 100.0;
        b[i] = (rand() % 100) / 100.0;
        res[i] = 0;
    }

    checkErr(clGetPlatformIDs(0, 0, &num), 
             printf("Unable to get platforms\n");
             return 0;
             );

    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num);
    checkErr(clGetPlatformIDs(num, platforms, NULL), 
             printf("Unable to get platform ID\n");
             return 0;
             );

    checkErr(clGetPlatformIDs(0, 0, &num), 
             printf("Unable to get platforms\n");
             return 0;
             );

    prop[0] = CL_CONTEXT_PLATFORM;
    prop[1] = (cl_context_properties)platforms[0];
    prop[2] = 0;
    context = clCreateContextFromType(prop, CL_DEVICE_TYPE_ALL, NULL, NULL, NULL);
    checkExit(context, "Can't create OpenCL context\n");

    clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
    devices = (cl_device_id *)malloc(cb);
    clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, devices, 0);
    checkExit(cb, "Can't get devices\n");
    num_total_devices = cb / sizeof(cl_device_id);

    for (i = 0; i < num_total_devices; i++) {
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL, &cb);
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, cb, devname, 0);
        printf("Device(%d/%d): %s\n", i, num_total_devices, devname[i]);
    }

    queue = clCreateCommandQueue(context, devices[0], 0, 0);
    checkExit(queue, "Can't create command queue\n");

    program = load_program(context, devices[0], "shader.cl");
    checkExit(program, "Fail to build program\n");

    cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &a[0], NULL);
    cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * DATA_SIZE, &b[0], NULL);
    cl_res = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(cl_float) * DATA_SIZE, &res[0], NULL);
    if (cl_a == 0 || 
        cl_b == 0 || 
        cl_res == 0) {
        printf("Can't create OpenCL buffer\n");
        goto release;
    }

    checkErr(clEnqueueWriteBuffer(queue, cl_a, CL_TRUE, 0, 
                                  sizeof(cl_float) * DATA_SIZE, a, 0, 0, 0),
             printf("ERROR ! WRITE BUFFER 1\n");
             goto release;
            );
    checkErr(clEnqueueWriteBuffer(queue, cl_b, CL_TRUE, 0, 
                                  sizeof(cl_float) * DATA_SIZE, b, 0, 0, 0),
             printf("ERROR ! WRITE BUFFER 2\n");
             goto release;
            );

    adder = clCreateKernel(program, "adder", &err);
    if (err == CL_INVALID_KERNEL_NAME) printf("CL_INVALID_KERNEL_NAME\n");
    checkExit(adder, "Can't load kernel\n");

    clSetKernelArg(adder, 0, sizeof(cl_mem), &cl_a);
    clSetKernelArg(adder, 1, sizeof(cl_mem), &cl_b);
    clSetKernelArg(adder, 2, sizeof(cl_mem), &cl_res);

    work_size = DATA_SIZE;

    checkErr(clEnqueueNDRangeKernel(queue, adder, 1, 0, &work_size, 0, 0, 0, 0),
             printf("Can't enqueue kernel\n");
            );
    checkErr(clEnqueueReadBuffer(queue, cl_res, CL_TRUE, 0, sizeof(cl_float) * DATA_SIZE, res, 0, 0, 0),
             printf("Can't enqueue read buffer\n");
            );

    for (i = 0; i < DATA_SIZE; i++) {
        printf("%f + %f = %f(answer %f)\n", a[i], b[i], res[i], a[i] + b[i]);
    }
    printf("------\n");

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

