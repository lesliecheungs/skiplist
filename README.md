# SkipList

## 1.跳表的意义

​		当给定一个有序数组，我们要想对其进行高效的查询，可以自然而然的想到二分查找。

​		但我们所需的元素可能往往不是一次给完的，可能需要动态的维护这个数组的有序以及增减，但这并不是数组所擅长的，所以我们就得用链表了。

​		如果想在一个链表上进行高效的查询，我们可能知道为了二分树，平衡树（二分树可能退化），红黑树（平衡树条件过于苛刻，从而频繁旋转）。但红黑树当数据量过大时，其条件过于放松，从而树的高度很可能导致查找速度慢，而且红黑树本身实现就过于复杂。

​		于是我们就引出了比红黑树效率更高，实现更简单的方法----**跳表**。





