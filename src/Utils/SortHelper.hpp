#include <vector>

class ArraySortHelper
{
public:
    template<typename T, typename P>
    static void Sort(std::vector<T> &keys, int index, int length, P comparer)
    {
        if (length >= 2) {
            int count = length;
            int ideal = 0;
            while (count >= 1) {
                ideal += 1;
                count /= 2;
            }
            IntroSort(keys, index, length + index - 1, 2 * ideal, comparer);
        }
    }

private:
    template<typename T, typename P>
    static void SwapIfGreater(std::vector<T> &keys, P &comparer, int a, int b)
    {
        if (a != b && comparer(keys[b], keys[a])) {
            T val = keys[a];
            keys[a] = keys[b];
            keys[b] = val;
        }
    }

    template<typename T>
    static void Swap(std::vector<T> &a, int i, int j)
    {
        if (i != j) {
            T val = a[i];
            a[i] = a[j];
            a[j] = val;
        }
    }

    template<typename T, typename P>
    static void IntroSort(std::vector<T> &keys, int lo, int hi, int depthLimit,
                          P &comparer)
    {
        while (hi > lo) {
            int num = hi - lo + 1;
            if (num <= 16) {
                switch (num) {
                case 1:
                    break;
                case 2:
                    SwapIfGreater(keys, comparer, lo, hi);
                    break;
                case 3:
                    SwapIfGreater(keys, comparer, lo, hi - 1);
                    SwapIfGreater(keys, comparer, lo, hi);
                    SwapIfGreater(keys, comparer, hi - 1, hi);
                    break;
                default:
                    InsertionSort(keys, lo, hi, comparer);
                    break;
                }
                break;
            }
            if (depthLimit == 0) {
                Heapsort(keys, lo, hi, comparer);
                break;
            }
            depthLimit--;
            int num2 = PickPivotAndPartition(keys, lo, hi, comparer);
            IntroSort(keys, num2 + 1, hi, depthLimit, comparer);
            hi = num2 - 1;
        }
    }

    template<typename T, typename P>
    static int PickPivotAndPartition(std::vector<T> &keys, int lo, int hi,
                                     P &comparer)
    {
        int num = lo + (hi - lo) / 2;
        SwapIfGreater(keys, comparer, lo, num);
        SwapIfGreater(keys, comparer, lo, hi);
        SwapIfGreater(keys, comparer, num, hi);
        T val = keys[num];
        Swap(keys, num, hi - 1);
        int num2 = lo;
        int num3 = hi - 1;
        while (num2 < num3) {
            while (comparer(keys[++num2], val)) {
            }
            while (comparer(val, keys[--num3])) {
            }
            if (num2 >= num3) {
                break;
            }
            Swap(keys, num2, num3);
        }
        Swap(keys, num2, hi - 1);
        return num2;
    }

    template<typename T, typename P>
    static void Heapsort(std::vector<T> &keys, int lo, int hi, P &comparer)
    {
        int num = hi - lo + 1;
        for (int num2 = num / 2; num2 >= 1; num2--) {
            DownHeap(keys, num2, num, lo, comparer);
        }
        for (int num3 = num; num3 > 1; num3--) {
            Swap(keys, lo, lo + num3 - 1);
            DownHeap(keys, 1, num3 - 1, lo, comparer);
        }
    }

    template<typename T, typename P>
    static void DownHeap(std::vector<T> &keys, int i, int n, int lo,
                         P &comparer)
    {
        T val = keys[lo + i - 1];
        while (i <= n / 2) {
            int num = 2 * i;
            if (num < n && comparer(keys[lo + num - 1], keys[lo + num])) {
                num++;
            }
            if (!comparer(val, keys[lo + num - 1])) {
                break;
            }
            keys[lo + i - 1] = keys[lo + num - 1];
            i = num;
        }
        keys[lo + i - 1] = val;
    }

    template<typename T, typename P>
    static void InsertionSort(std::vector<T> &keys, int lo, int hi, P &comparer)
    {
        for (int i = lo; i < hi; i++) {
            int num = i;
            T val = keys[i + 1];
            while (num >= lo && comparer(val, keys[num])) {
                keys[num + 1] = keys[num];
                num--;
            }
            keys[num + 1] = val;
        }
    }
};
