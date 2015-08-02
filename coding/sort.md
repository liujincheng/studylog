
#冒泡排序
冒泡排序，两次遍历。外层，遍历整个序列。内层，以外层为参考点，通过交换的方式，将最大或最小值放到队列的末尾。
```c
/*按从小到大排序*/
void sort(int *pt, int len)
{
 int temp, i, j;
 
 for(i=len - 1; i>=0; i--)
 {
  for(j=0; j<i; j++)
  {
   if(pt[j] > pt[i])
   {
    temp = pt[j] ;
    pt[j] = pt[i] ;
    pt[i] = temp ;
   }
  }
 }
}
```

#快速排序
http://blog.csdn.net/morewindows/article/details/6684558

快速排序（Quicksort）是对冒泡排序的一种改进。由C. A. R. Hoare在1962年提出。它的基本思想是：通过一趟排序将要排序的数据分割成独立的两部分，其中一部分的所有数据都比另外一部分的所有数据都要小，然后再按此方法对这两部分数据分别进行快速排序，整个排序过程可以递归进行，以此达到整个数据变成有序序列。（递归，一定要有终止条件）
```c
 /*
一趟快速排序的算法是：
1）设置两个变量i、j，排序开始的时候：i=0，j=N-1；
2）以第一个数组元素作为关键数据，赋值给key，即key=A[0]；
3）从j开始向前搜索，即由后开始向前搜索(j--)，找到第一个小于key的值A[j]，将A[j]赋给A[i]；
4）从i开始向后搜索，即由前开始向后搜索(i++)，找到第一个大于key的A[i]，将A[i]赋给A[j]；
5）重复第3、4步，直到i=j； (3,4步中，没找到符合条件的值，即3中A[j]不小于key,4中A[i]不大于key的时候改变j、i的值，使得j=j-1，i=i+1，直至找到为止。找到符合条件的值，进行交换的时候i， j指针位置不变。另外，i==j这一过程一定正好是i+或j-完成的时候，此时令循环结束）。
  */ 
快速排序中，用到了数据交换，让我想到了简单的交换两个数：
temp = x; x = y; y = temp;
/*从小到大排列*/
int quickSort(int *pi, int numSize)
{
 int i = 0 ; 
 int j = numSize - 1 ;
 int val = pi[0] ;
 
 printf("%d\n", numSize) ;
 if(numSize <= 1)
  return ;
 
 while(i < j)
 {
  for(; j > i; j--)
   if(pi[j] < val)
   {
    pi[i] = pi[j] ;
    break ;
   }
 
  for(; i<j; i++)
   if(pi[i] > val)
   {
    pi[j] = pi[i] ;
    break ;
   }
 }
 pi[i] = val ;
 quickSort(pi, i) ;
 quickSort(pi + i + 1, numSize - 1 - i) ;
}
```

#归并排序
原理：将原序列划分为有序的两个序列，然后利用归并算法进行合并，合并之后即为有序序列。

要点：归并、分治

实现：
```c
Void MergeSort(Node L[],int m,int n)
{
	Int k;
	If(m<n)
	{
		K=(m+n)/2;
		MergeSort(L,m,k);
		MergeSort(L,k+1,n);
		Merge(L,m,k,n);
	}
}
```

堆排序

