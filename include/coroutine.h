#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <iostream>
#include <ucontext.h>
#include <functional>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

enum CO_STATUS{
    COROUTINE_DEAD=0,
    COROUTINE_READY=1,
    COROUTINE_SUSPEND=2,
    COROUTINE_RUNNING=3
};
#define STACKSIZE (1024*1024)
#define DEFAULT_COROUTINE 16

class coroutine{
    using Func = std::function<void()>;
    struct cell_coroutine{
        Func func;
        uint32_t size;
        uint32_t cap;
        char* stack;//in real heap!
        int status;
        ucontext_t ctx;
    };
    struct schedule{
        char stack[STACKSIZE];//in real stack!
        uint32_t nco;//now co used
        uint32_t cap;//capcity
        int running;//running cell_corotine's id in 
        ucontext_t main;//always run in this ctx
        cell_coroutine** co;//rotine list{cell_corotine*,cell_corotine*,...}
    };
    schedule* _sch;//only 1
public:
    coroutine() :_sch(nullptr) {init();}
    void init(){
        _sch = new schedule{};
        assert(_sch);
        _sch->running = -1;
        _sch->nco = 0;
        _sch->cap = DEFAULT_COROUTINE;
        memset(_sch->stack,0,STACKSIZE);//clear stack
        _sch->co = new cell_coroutine* [_sch->cap]{};//default 0
        assert(_sch->co);
    }
    cell_coroutine* co_new(Func fun){
        cell_coroutine*c = new cell_coroutine;
        assert(c);
        c->cap = 0;//capcity when running
        c->size = 0;//
        c->func = fun;
        c->stack = 0;
        c->status = COROUTINE_READY;//waiting for schedule
        c->ctx = ucontext_t{};
        return c;
    }
    static void co_delete(cell_coroutine* c){
        if(c){
            if(c->stack)
                free(c->stack);
            c->stack = nullptr;    
            free(c);
        }
    }
    static void mainfunc(void*sch){
        schedule* s = (schedule*)sch;
        int id = s->running;
        assert(id>=0 && id<s->cap);
        cell_coroutine* c = s->co[id];
        c->func();//only continue when return,
        co_delete(c);//free c when it has end
        s->co[id] = nullptr;
        s->running = -1;
        --s->nco;
    }
    int  coroutine_create(Func fun){//add new_co c to _sch
        cell_coroutine* c = co_new(fun);
        if(_sch->nco >= _sch->cap){
            int id = _sch->nco;
            cell_coroutine** co_tmp = (cell_coroutine**)realloc(_sch->co,sizeof(cell_coroutine*) * _sch->cap * 2);
            assert(co_tmp);
            _sch->co = co_tmp;//must have this
            memset(_sch->co+_sch->cap, 0, sizeof(cell_coroutine*) * _sch->cap);// set 1/2 in last to 0
            _sch->co[id] = c;//install c
            ++_sch->nco;
            _sch->cap *= 2;
            return id;
        }else{
            for(int id=0;id<_sch->cap;++id){
                if(_sch->co[id]==nullptr){
                    _sch->co[id] = c;
                    ++_sch->nco;
                    return id;
                }
            }
        }
        assert(0);
        return -1;
    }
    void save_stack(cell_coroutine* c,char*top){//save _sche's stack into c's stack //top is real stack_buttom!
        char dummy;//buttom of _sche's stack
        assert(top-&dummy <= STACKSIZE);
        uint32_t size = top-&dummy;
        if(c->cap < size){
            free(c->stack);
            c->cap = size;
            c->stack = (char*)malloc(size);
        }
        c->size = top-&dummy;
        memcpy(c->stack,&dummy,size);
    }
    void coroutine_resume(int id){//resume co[id]
        assert(_sch->running==-1);
        assert(id>=0 && id<_sch->cap);
        cell_coroutine* c = _sch->co[id];
        if(!c) return;//if co[id]==nullptr, not resume
        int status = c->status;
        switch(status){
            case COROUTINE_READY:{
                getcontext(&c->ctx);
                c->ctx.uc_stack.ss_sp = _sch->stack;
                c->ctx.uc_stack.ss_size = STACKSIZE;
                c->ctx.uc_link = &_sch->main;
                c->status = COROUTINE_RUNNING;
                _sch->running = id;
                makecontext(&c->ctx,(void(*)())mainfunc,1,(uint64_t)_sch);
                swapcontext(&_sch->main,&c->ctx);
                break;
            }
            case COROUTINE_SUSPEND:{
                c->status = COROUTINE_RUNNING;
                _sch->running = id;
                memcpy(_sch->stack+STACKSIZE-c->size,c->stack,c->size);
                swapcontext(&_sch->main,&c->ctx);
                break;
            }
            default:break;
        }
    }
    void coroutine_yield(){
        int id = _sch->running;
        assert(id>=0 && id<_sch->cap);
        cell_coroutine* c = _sch->co[id];
        //save running stack into c's stack
        assert((char*)&c > _sch->stack);
        save_stack(c,_sch->stack+STACKSIZE);//top is real stack_buttom = _sch->stack+STACKSIZE;

        c->status = COROUTINE_SUSPEND;
        _sch->running = -1;//wait for coroutine_resume(id),must change to -1
        swapcontext(&c->ctx,&_sch->main);//save registers(ctx) to c's ctx 
    }
    int coroutine_status(int id){
        assert(id>=0 && id<_sch->cap);
        if(!_sch->co[id])
            return COROUTINE_DEAD;
        return _sch->co[id]->status;
    }
    int coroutine_running(){
        return _sch->running;
    }
    int coroutine_nco(){
        return _sch->nco;
    }
    void destroy(){
        if(_sch){
            if(_sch->co){
                for(int id=0;id<_sch->cap;++id){
                    if(_sch->co[id]){
                        free(_sch->co[id]->stack);
                        _sch->co[id]->stack = nullptr;
                    }
                }
                free(_sch->co);
                _sch->co=nullptr;                
            }
            free(_sch);
            _sch=nullptr;
        }
    }
    ~coroutine(){
        destroy();
        std::cout << "_sch has been destroyed.\n";
    }
};

#endif