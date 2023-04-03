#include "./include/coroutine.h"

void fun1(coroutine*co){
    for(int i=0;i<10;++i){
        std::cout << "Runing is:"<<co->coroutine_running()<<" times "<<i<<std::endl;
        co->coroutine_yield();
    }
    //go DEAD
}
void fun2(coroutine*co){
    for(int i=0;i<1;++i){
        std::cout << "Runing is:"<<co->coroutine_running()<<" times "<<i<<std::endl;
        co->coroutine_yield();
    }
}

void test(coroutine*co){
    int id0 = co->coroutine_create(std::bind(fun1,co));
    int id1 = co->coroutine_create(std::bind(fun2,co));

    while(co->coroutine_status(id0) || co->coroutine_status(id1)){//only DEAD, break
        co->coroutine_resume(id0);
        co->coroutine_resume(id1);
    }
}

void test2(coroutine*co){
    int idList[20]={0};
    for(int i=0;i<sizeof(idList)/sizeof(int);++i){
        idList[i]=co->coroutine_create(std::bind(fun2,co));
    }
    while(co->coroutine_nco()>0){
        for(int i=0;i<sizeof(idList)/sizeof(int);++i){
            if(co->coroutine_status(idList[i])!=COROUTINE_DEAD)
                co->coroutine_resume(idList[i]);
        }
    }
}

int main(int, char**) {
    coroutine co;
    test2(&co);
    test(&co);
    std::cout<<"running nco: "<<co.coroutine_nco()<<std::endl;
}