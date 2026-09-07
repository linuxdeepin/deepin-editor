// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "load_libs.h"

#include <dlfcn.h>

void PrintError(){
    char *error;
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s ", error);
    }
}
static LoadLibNames g_ldnames;
static LoadLibs *pLibs = NULL;
static LoadLibs *newClass(void)
{
    pLibs = (LoadLibs *)malloc(sizeof(LoadLibs));
    pLibs->m_document_clip_copy = NULL;
    pLibs->m_document_close = NULL;
//    RTLD_NOW：在dlopen返回前，解析出全部没有定义的符号，解析不出来返回NULL。
//    RTLD_LAZY：暂缓决定，等有需要时再解出符号
    void *handle = NULL;
    if (g_ldnames.chZPDDLL != NULL) {
        handle = dlopen(g_ldnames.chZPDDLL, RTLD_NOW);
        if (handle == NULL) {
            PrintError();
        }
    } else {
        fprintf(stderr, "Error: Library path is NULL\n");
    }
    if (handle == NULL) {
        return pLibs;
    }

    pLibs->m_document_clip_copy = (uos_document_clip_copy)dlsym(handle, "document_clip_copy");
    PrintError();
    if (pLibs->m_document_clip_copy == NULL) {
        fprintf(stderr, "Error: dlsym resolve document_clip_copy failed\n");
    }
    pLibs->m_document_close = (uos_document_close)dlsym(handle, "document_close");
    PrintError();
    if (pLibs->m_document_close == NULL) {
        fprintf(stderr, "Error: dlsym resolve document_close failed\n");
    }

    assert(pLibs != NULL);
    return pLibs;
}

/**
 * 饿汉式
 * 支持延迟加载，但是为了多线程安全，性能有所降低
 * 注意：方法内部要加锁，防止多线程多次创建
 * */
LoadLibs *getLoadZPDLibsInstance()
{    
    static pthread_mutex_t mutex;
    //双检锁
    if (pLibs == NULL) {
        // 这里要对pLibs加锁
        pthread_mutex_lock(&mutex);
        if (pLibs == NULL)
            pLibs = newClass();

        //退出时解锁
        pthread_mutex_unlock(&mutex);
    }

    return pLibs;
}

void setLibNames(LoadLibNames tmp)
{
    if(tmp.chZPDDLL == NULL) {
        g_ldnames.chZPDDLL = NULL;
    } else {
        // 重复调用时先释放旧的分配，避免内存泄漏
        if (g_ldnames.chZPDDLL != NULL) {
            free(g_ldnames.chZPDDLL);
        }
        g_ldnames.chZPDDLL = ( char*)malloc(strlen(tmp.chZPDDLL)+1);
        strcpy(g_ldnames.chZPDDLL,tmp.chZPDDLL);
    }
}
