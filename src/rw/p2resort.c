#include "rw/rwplcore.h"

typedef struct RxSortPartition {
    RwUInt8* first;
    RwUInt8* last;
    RwUInt32 bit;
} RxSortPartition;

#define SWAPVIATYPE(type)                                                     \
    {                                                                         \
        while (remaining >= sizeof(type)) {                                   \
            type firstValue = *(type *)firstWord,                             \
                 secondValue = *(type *)secondWord;                           \
            *(type *)secondWord = firstValue;                                 \
            *(type *)firstWord = secondValue;                                 \
            firstWord += sizeof(type);                                        \
            secondWord += sizeof(type);                                       \
            remaining -= sizeof(type);                                        \
        }                                                                     \
    }

#define SWAP(left, right, size)                                                \
    {                                                                         \
        RwUInt8 *firstWord = (RwUInt8 *)(left);                               \
        RwUInt8 *secondWord = (RwUInt8 *)(right);                             \
        RwUInt32 remaining = (size);                                           \
        SWAPVIATYPE(RwUInt32);                                                 \
    }

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

/* Algorithm recovered: the 32-entry work stack, partition scans, swaps, and
 * five-entry cutoff match retail. Retail also spills unused copies of the
 * stack base, bit, byte count, and pushed-entry address, expanding its frame
 * and save range; clean source does not reproduce that macro scaffolding. */
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
                    SWAP(first, last, entrySize);
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

/*
 * The canonical RenderWare swap macro and loop structure recover the complete
 * body. The remaining residue is MWCC's unsigned-comparison materialization
 * (subfc/subfe/neg in retail versus an equivalent clean-C sequence), one dead
 * macro cursor copy, and the resulting nonvolatile coloring.
 */
static void _insertionsort(RwUInt8* base, RwUInt32 numEntries,
                           RwUInt32 entrySize, RwUInt32 keyOffset) {
    while (base += entrySize, --numEntries) {
        RwUInt32 currentKey = *(RwUInt32*)(base + keyOffset);
        RwUInt8* previous = base;

        while (previous -= entrySize,
               *(RwUInt32*)(previous + keyOffset) > currentKey) {
            SWAP(previous, previous + entrySize, entrySize);
        }
    }
}

/* Near match: guards, radix partition, sentinel scan/swap, and insertion pass
 * are exact. Retail retains a dead copy of the swap byte counter, shifting the
 * nonvolatile save range and keyUpperBound register. */
void _rx_rxRadixExchangeSort(RwUInt8* base, RwUInt32 numEntries,
                             RwUInt32 entrySize, RwUInt32 keyOffset,
                             RwUInt32 keyLowerBound,
                             RwUInt32 keyUpperBound) {
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
            SWAP(base, base + minimumIndex * entrySize, entrySize);
        }
        _insertionsort(base, numEntries, entrySize, keyOffset);
    }
}
