#include "rw/rwplcore.h"

typedef struct RxSortPartition {
    RwUInt8* first;
    RwUInt8* last;
    RwUInt32 bit;
} RxSortPartition;

static RwInt32 _msbitpos(RwUInt32 value) {
    RwInt32 position;

    if (value != 0) {
        position = 0;
        while ((value >>= 1) != 0) {
            position += 1;
        }
        return position;
    }
    return -1;
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
        RwUInt8* originalFirst;
        RwUInt8* originalLast;
        RwUInt8* rightFirst;

        stackTop--;
        first = stackTop->first;
        last = stackTop->last;
        bit = stackTop->bit;

        for (;;) {
            originalFirst = first;
            originalLast = last;

            while (first <= last) {
                while (first <= last &&
                       (bit & *(RwUInt32*)(first + keyOffset)) == 0) {
                    first += entrySize;
                }
                while (first <= last &&
                       (bit & *(RwUInt32*)(last + keyOffset)) != 0) {
                    last -= entrySize;
                }
                if (first <= last) {
                    RwUInt8* firstWord = first;
                    RwUInt8* secondWord = last;
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

static void _insertionsort(RwUInt8* base, RwUInt32 numEntries,
                           RwUInt32 entrySize, RwUInt32 keyOffset) {
    RwUInt8* current = base;

    for (;;) {
        current += entrySize;
        numEntries -= 1;
        if (numEntries == 0) {
            break;
        }
        {
            RwUInt32 currentKey = *(RwUInt32*)(current + keyOffset);
            RwUInt8* previous = current;

            for (;;) {
                RwBool moveRecord;
                previous -= entrySize;
                moveRecord =
                    *(RwUInt32*)(previous + keyOffset) > currentKey;
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
