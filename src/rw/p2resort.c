#include "rw/rwplcore.h"

typedef struct RxSortPartition {
    RwUInt8* first;
    RwUInt8* last;
    RwUInt32 bit;
} RxSortPartition;

static RwInt32 _msbitpos(RwUInt32 value) {
    RwInt32 position;

    if (value == 0) {
        return -1;
    }
    position = 0;
    for (;;) {
        value >>= 1;
        if (value == 0) {
            break;
        }
        position += 1;
    }
    return position;
}

static void _repartition(RwUInt8* first, RwUInt8* last,
                         RwUInt32 entrySize, RwUInt32 keyOffset,
                         RwUInt32 bit) {
    RxSortPartition stack[32];
    RxSortPartition* stackTop = stack;

    stackTop->first = first;
    stackTop->last = last;
    stackTop->bit = bit;
    stackTop++;

    while (stackTop != stack) {
        RwUInt8* left;
        RwUInt8* right;
        RwUInt8* originalFirst;
        RwUInt8* originalLast;

        stackTop--;
        first = stackTop->first;
        last = stackTop->last;
        bit = stackTop->bit;

        for (;;) {
            originalFirst = first;
            originalLast = last;
            left = first;
            right = last;

            while (left <= right) {
                while (left <= right &&
                       (bit & *(RwUInt32*)(left + keyOffset)) == 0) {
                    left += entrySize;
                }
                while (left <= right &&
                       (bit & *(RwUInt32*)(right + keyOffset)) != 0) {
                    right -= entrySize;
                }
                if (left <= right) {
                    RwUInt8* firstWord = left;
                    RwUInt8* secondWord = right;
                    RwUInt32 remaining = entrySize;
                    while (remaining >= 4) {
                        RwUInt32 firstValue = *(RwUInt32*)firstWord;
                        RwUInt32 secondValue = *(RwUInt32*)secondWord;
                        *(RwUInt32*)secondWord = firstValue;
                        *(RwUInt32*)firstWord = secondValue;
                        firstWord += 4;
                        secondWord += 4;
                        remaining -= 4;
                    }
                    left += entrySize;
                    right -= entrySize;
                }
            }

            bit >>= 1;
            if (bit == 0) {
                break;
            }

            first = right + entrySize;
            if (originalLast >= first + entrySize * 5) {
                stackTop->first = first;
                stackTop->last = originalLast;
                stackTop->bit = bit;
                stackTop++;
            }

            last = left - entrySize;
            first = originalFirst;
            if (last < first + entrySize * 5) {
                break;
            }
        }
    }
}

static void _insertionsort(RwUInt8* base, RwUInt32 numEntries,
                           RwUInt32 entrySize, RwUInt32 keyOffset) {
    RwUInt8* current = base;

    while (--numEntries != 0) {
        current += entrySize;
        {
            RwUInt32 currentKey = *(RwUInt32*)(current + keyOffset);
            RwUInt8* previous = current;

            for (;;) {
                RwBool moveRecord;
                previous -= entrySize;
                moveRecord =
                    currentKey < *(RwUInt32*)(previous + keyOffset);
                if (moveRecord == FALSE) {
                    break;
                }
                {
                    RwUInt8* firstWord = previous;
                    RwUInt8* secondWord = previous + entrySize;
                    RwUInt32 remaining = entrySize;
                    while (remaining >= 4) {
                        RwUInt32 firstValue = *(RwUInt32*)firstWord;
                        RwUInt32 secondValue = *(RwUInt32*)secondWord;
                        *(RwUInt32*)secondWord = firstValue;
                        *(RwUInt32*)firstWord = secondValue;
                        firstWord += 4;
                        secondWord += 4;
                        remaining -= 4;
                    }
                }
            }
        }
    }
}

void _rx_rxRadixExchangeSort(RwUInt8* base, RwUInt32 numEntries,
                             RwUInt32 entrySize, RwUInt32 keyOffset,
                             RwUInt32 keyLowerBound,
                             RwUInt32 keyUpperBound) {
    if (base == 0 || keyOffset + 4 > entrySize ||
        keyLowerBound >= keyUpperBound) {
        return;
    }

    if (numEntries > 5) {
        RwUInt32 bit = 1U << _msbitpos(keyUpperBound);
        _repartition(base, base + (numEntries - 1) * entrySize, entrySize,
                     keyOffset, bit);
    }

    if (numEntries > 1) {
        RwUInt32 index = 4;
        RwUInt32 minimumIndex;
        RwUInt32 minimumKey;

        if (index > numEntries - 1) {
            index = numEntries - 1;
        }
        minimumKey = *(RwUInt32*)(base + index * entrySize + keyOffset);
        minimumIndex = index;
        index--;
        do {
            RwUInt32 key = *(RwUInt32*)(base + index * entrySize + keyOffset);
            if (key < minimumKey) {
                minimumKey = key;
                minimumIndex = index;
            }
        } while (index-- != 0);

        if (minimumIndex != 0) {
            RwUInt8* firstWord = base;
            RwUInt8* secondWord = base + minimumIndex * entrySize;
            RwUInt32 remaining = entrySize;
            while (remaining >= 4) {
                RwUInt32 firstValue = *(RwUInt32*)firstWord;
                RwUInt32 secondValue = *(RwUInt32*)secondWord;
                *(RwUInt32*)secondWord = firstValue;
                *(RwUInt32*)firstWord = secondValue;
                firstWord += 4;
                secondWord += 4;
                remaining -= 4;
            }
        }
        _insertionsort(base, numEntries, entrySize, keyOffset);
    }
}
