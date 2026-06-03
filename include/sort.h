#pragma once

#include <vector>
#include <string>

// ============================================================================
// 排序模块 —— 提供三种经典排序算法，用于对学生成绩进行升序排列
// ============================================================================
// 本模块包含以下排序函数：
//   - bubbleSort       冒泡排序（简单直观，适合小数据量）
//   - selectionSort    选择排序（每次选最小元素放到前面）
//   - quickSort        快速排序（分治思想，效率最高，适合大数据量）
//   - quickSortWithIndex 带索引的快速排序（排序成绩的同时记录原始位置）
// ============================================================================

namespace sort {

    // 冒泡排序：对数组进行升序排列
    // 原理：依次比较相邻两个元素，如果顺序不对就交换，
    //       每轮遍历后最大的元素会"冒"到最后面。
    void bubbleSort(std::vector<double>& arr);

    // 选择排序：对数组进行升序排列
    // 原理：每一轮从未排序部分找到最小元素，
    //       然后把它放到已排序部分的末尾。
    void selectionSort(std::vector<double>& arr);

    // 快速排序（递归实现）—— 指定排序区间 [left, right]
    // 原理（分治法）：
    //   1. 选取一个"基准值"（pivot）
    //   2. 将数组分成两部分：左边 ≤ 基准值，右边 ≥ 基准值
    //   3. 递归地对左右两部分排序
    void quickSort(std::vector<double>& arr, int left, int right);

    // 快速排序的便捷入口，对整个数组排序
    void quickSort(std::vector<double>& arr);

    // 带索引的快速排序（降序）—— 同时对成绩和对应的原始索引排序
    // 用途：排名时需要知道某个成绩来自第几个学生，
    //       通过 indices 数组记录原始位置（学号/序号）。
    void quickSortWithIndex(std::vector<double>& scores, std::vector<int>& indices, int left, int right);
    void quickSortWithIndex(std::vector<double>& scores, std::vector<int>& indices);

}
