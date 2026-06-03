#pragma once

#include <vector>
#include <string>

namespace sort {

    void bubbleSort(std::vector<double>& arr);

    void quickSort(std::vector<double>& arr, int left, int right);
    void quickSort(std::vector<double>& arr);

    void quickSortWithIndex(std::vector<double>& scores, std::vector<int>& indices, int left, int right);
    void quickSortWithIndex(std::vector<double>& scores, std::vector<int>& indices);

}
