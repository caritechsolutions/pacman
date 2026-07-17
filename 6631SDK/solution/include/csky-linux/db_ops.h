/*
 * =====================================================================================
 *
 *       Filename:  db_ops.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年04月26日 13时11分57秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __DB_OPS_H__
#define __DB_OPS_H__

#include "db_type.h"
#include "module/pm/gxprogram_manage_berkeley.h"


#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

typedef struct _DB db_ops;
struct _DB
{
    DB *db[3];
    int (*open)  (const char*, DB**);
    int (*close) (DB*);
    int (*write) (DB* p, uint32_t pk, void *pd, size_t size);
    int (*read)   (DB* p, uint32_t pk, void *pd, size_t size);
    int (*sync)   (DB* p);
#define SAT 0
#define TP 1
#define PROG 2
};

extern db_ops db;


DLLEXPORT void berkeley_2_default(const char *path, const char *file);

#endif

