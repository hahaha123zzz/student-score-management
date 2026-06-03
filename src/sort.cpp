#include "sort.h"

namespace sort {

    void bubbleSort(std::vector<double>& arr) {
        int n = (int)arr.size();
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - 1 - i; j++) {
                if (arr[j] > arr[j + 1]) {
                    double tmp = arr[j];
                    arr[j] = arr[j + 1];
                    arr[j + 1] = tmp;
                }
            }
        }
    }

    void quickSort(std::vector<double>& arr, int left, int right) {
        if (left >= right) return;
        double pivot = arr[left];
        int i = left, j = right;
        while (i < j) {
            while (i < j && arr[j] >= pivot) j--;
            if (i < j) arr[i++] = arr[j];
            while (i < j && arr[i] <= pivot) i++;
            if (i < j) arr[j--] = arr[i];
        }
        arr[i] = pivot;
        quickSort(arr, left, i - 1);
        quickSort(arr, i + 1, right);
    }

    void quickSort(std::vector<double>& arr) {
        if (arr.size() > 1)
            quickSort(arr, 0, (int)arr.size() - 1);
    }

    void quickSortWithIndex(std::vector<double>& scores, std::vector<int>& indices, int left, int right) {
        if (left >= right) return;
        double pivotScore = scores[left];
        int pivotIdx = indices[left];
        int i = left, j = right;
        while (i < j) {
            while (i < j && scores[j] <= pivotScore) j--;
            if (i < j) {
                scores[i] = scores[j];
                indices[i++] = indices[j];
            }
            while (i < j && scores[i] >= pivotScore) i++;
            if (i < j) {
                scores[j] = scores[i];
                indices[j--] = indices[i];
            }
        }
        scores[i] = pivotScore;
        indices[i] = pivotIdx;
        quickSortWithIndex(scores, indices, left, i - 1);
        quickSortWithIndex(scores, indices, i + 1, right);
    }

    void quickSortWithIndex(std::vector<double>& scores, std::vector<int>& indices) {
        if (scores.size() > 1)
            quickSortWithIndex(scores, indices, 0, (int)scores.size() - 1);
    }

}
