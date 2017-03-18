# 那天说的推荐算法的朴素实现

嗯，我来胡搞了。

考虑了一下复杂度，根据我自己的 Pixiv 使用经验，我现在有 ~10k Bookmark， 比较受关注的图片可以有 ~10k Bookmark，所以如果使用朴素算法，那么需要统计的路径有：

10k * 10k * 10k ~ 1e12

这么多条

嗯，以一般计算机的计算能力，这个需要至少上百秒，不谈从数据库里面拿数据的时间。

一个可行的方案：

先从自己的 Bookmark 里面选出一部分，比如说，1000 个图片，然后根据这些图片找出所有收藏它们的人，并且统计重合数量。然后再从这些人里（或者随机选一些）出发，找出最终的搜索结果

考虑到和我喜欢类似风格的图片的人应该更多会收藏类似的图片，所以这个比例应该远小于 1。

# 测试算法
[main.cpp](main.cpp) 实现了这个算法，没有使用数据库，只是保存在内存内。随机生成数据，所以会比预期要更分散一些。

嗯由于生成数据太慢了，所以调整了一下参数，并没有那么大的用户量之类的，平均收藏只有 1k，所以选出的样本选的是 200。

我并没有对我的实现正确性做测试...

因为没什么好办法做测试...只能用另一种方法实现一次然后比对。

希望没什么问题吧...

注释写了一些，嗯，希望够用。

## 算法说明

大概意思就是：

首先，从用户的收藏中，随机选出至多 MAX\_BATCH\_SIZE 这么多个作品，然后去统计它们所有的收藏者。

然后从这些收藏者中，找出那些至少和进行搜索的用户有 SOURCE\_SIMILARITY\_LOWERBOUND 这么多个共同收藏的人。把他们根据共同收藏个数排序，较大的放前面，取前 MAX\_SOURCE\_COUNT 名（如果不够就全取）。

统计它们所有的收藏，把这些作品按照被收藏的人数排序，取前 size 名，返回。

## 如何编译
使用了一些 C++14 的语法，如果在 MacOS 下，需要：

```bash
clang++ -o main main.cpp -std=c++14 -Wall
```

如果用 Windows 的话，直接用最新版 VS 就可以了。

# 测试结果

用的是代码里的参数

```
Generating tags...
Tags generated.
Generating bookmarks...
Progress: 0.1
Progress: 0.2
Progress: 0.3
Progress: 0.4
Progress: 0.5
Progress: 0.600001
Progress: 0.700001
Progress: 0.800001
Progress: 0.900001
---Complete---
---Random Testing---
---At batch of 10---

Running batch 0...
Batch 0 finished.
Time: 88.8157ms

Running batch 1...
Batch 1 finished.
Time: 91.2432ms

Running batch 2...
Batch 2 finished.
Time: 90.8127ms

Running batch 3...
Batch 3 finished.
Time: 81.9563ms

Running batch 4...
Batch 4 finished.
Time: 92.5687ms

Running batch 5...
Batch 5 finished.
Time: 90.5018ms

Running batch 6...
Batch 6 finished.
Time: 88.1404ms

Running batch 7...
Batch 7 finished.
Time: 87.7172ms

Running batch 8...
Batch 8 finished.
Time: 92.5108ms

Running batch 9...
Batch 9 finished.
Time: 83.8949ms

---All Complete---
Avg. time per query: 8.88162ms
```

嗯还是挺给力的。

本来想换大一点的参数再测一次，结果直接把电脑跑炸了...内存是瓶颈，换到数据库上，性能会有很大的下降。
