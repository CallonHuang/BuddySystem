# BuddySystem
1. list.c : 基础数据结构，是个带头节点的双向循环链表
2. buddy.c : 伙伴系统的实现，包含移动合并实现的高阶申请函数
3. main.c : 测试函数
- 注：BuddyInit和BuddyDestroy设计了alloc和release的函数指针，用户可以不局限于使用它来管理malloc内存，其他类型（映射虚拟地址的）内存管理，如共享类的内存（shmget等）也可以用
