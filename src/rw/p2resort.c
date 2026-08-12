#include "rw/rwplcore.h"

typedef struct RxSortPartition {
    unsigned char* first;
    unsigned char* last;
    unsigned int bit;
} RxSortPartition;

static int _msbitpos(unsigned int value) {
    int position;

    if (value != 0) {
        position = 0;
        while ((value >>= 1) != 0) {
            position += 1;
        }
        return position;
    }
    return -1;
}

static void _repartition(unsigned char* first, unsigned char* last,
                         unsigned int entrySize, unsigned int keyOffset,
                         unsigned int bit) {
    RxSortPartition stack[32];
    RxSortPartition* stackTop = stack;

    stackTop->first = first;
    stackTop->last = last;
    stackTop->bit = bit;
    stackTop++;

    while (stackTop != stack) {
        unsigned char* originalFirst;
        unsigned char* originalLast;
        unsigned char* rightFirst;

        stackTop--;
        first = stackTop->first;
        last = stackTop->last;
        bit = stackTop->bit;

        for (;;) {
            originalFirst = first;
            originalLast = last;

                while (first <= last) {
                while (first <= last &&
                       (bit & *(unsigned int*)(first + keyOffset)) == 0) {
                    first += entrySize;
                }
                while (first <= last &&
                       (bit & *(unsigned int*)(last + keyOffset)) != 0) {
                    last -= entrySize;
                }
                if (first <= last) {
                    unsigned char* leftWord = first;
                    unsigned char* rightWord = last;
                    unsigned int remaining = entrySize;

                    while (remaining >= sizeof(unsigned int)) {
                        unsigned int value = *(unsigned int*)leftWord;
                        *(unsigned int*)leftWord = *(unsigned int*)rightWord;
                        *(unsigned int*)rightWord = value;
                        leftWord += sizeof(unsigned int);
                        rightWord += sizeof(unsigned int);
                        remaining -= sizeof(unsigned int);
                    }
                    first += entrySize;
                    last -= entrySize;
                }
            }

            bit >>= 1;
            if (bit == 0) {
                break;
            }

            rightFirst = last + entrySize;
            if (originalLast >= rightFirst + entrySize * 5) {
                stackTop->first = rightFirst;
                stackTop->last = originalLast;
                stackTop->bit = bit;
                stackTop++;
            }

            last = first - entrySize;
            first = originalFirst;
            if (last < first + entrySize * 5) {
                break;
            }
        }
    }
}

static void _insertionsort(unsigned char* base, unsigned int numEntries,
                           unsigned int entrySize, unsigned int keyOffset) {
    while (base += entrySize, --numEntries) {
        unsigned int currentKey = *(unsigned int*)(base + keyOffset);
        unsigned char* previous = base;

        while (previous -= entrySize,
               *(unsigned int*)(previous + keyOffset) > currentKey) {
            unsigned char* leftWord = previous;
            unsigned char* rightWord = previous + entrySize;
            unsigned int remaining = entrySize;

            while (remaining >= sizeof(unsigned int)) {
                unsigned int value = *(unsigned int*)leftWord;
                *(unsigned int*)leftWord = *(unsigned int*)rightWord;
                *(unsigned int*)rightWord = value;
                leftWord += sizeof(unsigned int);
                rightWord += sizeof(unsigned int);
                remaining -= sizeof(unsigned int);
            }
        }
    }
}

void _rx_rxRadixExchangeSort(unsigned char* base, unsigned int numEntries,
                             unsigned int entrySize, unsigned int keyOffset,
                             unsigned int keyLowerBound,
                             unsigned int keyUpperBound) {
    if (base == 0) {
        return;
    }
    if (keyOffset + 4 > entrySize) {
        return;
    }
    if (keyLowerBound >= keyUpperBound) {
        return;
    }

    if (numEntries > 5) {
        _repartition(base, base + (numEntries - 1) * entrySize, entrySize,
                     keyOffset, 1U << _msbitpos(keyUpperBound));
    }

    if (numEntries > 1) {
        unsigned int index = 4;
        unsigned int minimumIndex;
        unsigned int minimumKey;

        if (index > numEntries - 1) {
            index = numEntries - 1;
        }
        minimumKey = *(unsigned int*)(base + index * entrySize + keyOffset);
        minimumIndex = index;
        index--;
        do {
            unsigned int key = *(unsigned int*)(base + index * entrySize + keyOffset);
            if (key < minimumKey) {
                minimumKey = key;
                minimumIndex = index;
            }
        } while (index-- != 0);

        if (minimumIndex != 0) {
            unsigned char* leftWord = base;
            unsigned char* rightWord = base + minimumIndex * entrySize;
            unsigned int remaining = entrySize;

            while (remaining >= sizeof(unsigned int)) {
                unsigned int value = *(unsigned int*)leftWord;
                *(unsigned int*)leftWord = *(unsigned int*)rightWord;
                *(unsigned int*)rightWord = value;
                leftWord += sizeof(unsigned int);
                rightWord += sizeof(unsigned int);
                remaining -= sizeof(unsigned int);
            }
        }
        _insertionsort(base, numEntries, entrySize, keyOffset);
    }
}
