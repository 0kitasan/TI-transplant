# TI-transplant

## LCD(st7789)

直接暴力把原先在 st 上的静态链接库全部删除，否则不能编译，因为有些编译符号对不上

需要设置 CS/DC/RST 三个 GPIO 口来构造 st7789 类

经初步测试，目前还算稳定，至少比 7/29 上午稳定多了

## UART(printf redirect)

C 版本的可以直接重定向`printf`及其占位符

C++版本的只能不带占位符，我目前没找到什么好的解决方法

为了兼容 LCD 的 C++ 库，整个项目最好还是使用 C++；

解决方案是：自定义一个 char 数组，使用`sprintf`（在这里使用占位符）格式化字符串，最后直接用`printf`（不带占位符）输出

## THD mock

模拟赛的源代码，使用 float 形式的 fft，但是能跑

需要再去研究一下定点数的 fft
