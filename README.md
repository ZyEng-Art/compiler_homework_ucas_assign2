# 函数指针定值分析器
## 简介
这是国科大“编译程序高级教程”课程作业二，要求实现在每个函数调用处指出其可能调用哪些函数。一个简单的例子如下：
```
int plus(int a, int b) {
   return a+b;
}

int clever() {
    int (*a_fptr)(int, int) = plus;

    int op1 =1,  op2=2;

    unsigned result = a_fptr(op1, op2);
    return 0;
}


///  10 : plus

```
## 安装方式
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=/usr/lib/llvm10ra .. 
&& cmake --build .
 ```

## 实现方式
因为分析的是函数指针，需要分析过程间的数据流，一个函数指针可能是一个尚未分析到的函数的返回值，因此使用标准的数据流分析框架（类似于指针的指向分析）。

实现中还基于指针分析构建了过程调用图。

## 扩展
### 考虑控制流进行更精确的分析

以下是两个测例
* test01:
```
#include <stdlib.h>
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (*t_fptr)(int, int) = 0;

    int op1=1, op2=2;

    if (x == 3) {
       t_fptr = a_fptr;
    } 

    if (t_fptr != NULL) {
       unsigned result = t_fptr(op1, op2);
    }  
   return 0;
}

/// 22 : plus (it is OK to print 21 : plus, NULL) 

```

* test14:
```
#include <stdlib.h>
int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

int (*foo(int a, int b, int (*a_fptr)(int, int), int(*b_fptr)(int, int) ))(int, int) {
   return a_fptr;
}
int (*clever(int a, int b, int (*a_fptr)(int, int), int(*b_fptr)(int, int) ))(int, int) {
   return b_fptr;
}
int moo(char x, int op1, int op2) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int (* (*goo_ptr)(int, int, int (*)(int, int), int(*)(int, int)))(int, int)=foo;
    int (*t_fptr)(int, int) = 0;

    if(x == '+')
    {
        t_fptr = goo_ptr(op1, op2, a_fptr, s_fptr);
        s_fptr=a_fptr;
    }else
    {
        goo_ptr=clever;
    }

    t_fptr = goo_ptr(op1, op2, s_fptr, a_fptr); 
    t_fptr(op1, op2);
    
    return 0;
}


// 24 : foo  
// 31 : clever,foo
// 32 : plus

```

这两个测例可以基于控制流得到更精确的结果，我的想法是可以先进行指令调度，
如果将支配边界之后的指令移动到之前不影响结果，可以进行移动，进而无效在某个
分支可以不执行某些指令。