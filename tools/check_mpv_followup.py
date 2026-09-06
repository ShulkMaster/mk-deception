"""Execute the follow-up Sofdec candidates and retail PPC with identical inputs.

Uses check_mpvabdec's PPC750 loader (unicorn==2.1.4). Stream interfaces,
block callbacks and final DCT are controlled test doubles; this isolates table
construction, block orchestration, header parsing, and macroblock parsing.
It is not a full-frame decoding test.
"""
import argparse
import json
import pathlib
import random
import struct

from unicorn import UC_HOOK_CODE
from unicorn.ppc_const import UC_PPC_REG_0, UC_PPC_REG_3, UC_PPC_REG_LR, UC_PPC_REG_PC
from check_mpvabdec import Oracle, pack

PREFIX = 'build/GQNE5D/src/libmwsfdg.a/crimw/dev/sofdec/src/sfdcore/mpv/'
CTX, DATA, SJ, VT = 0x82001000, 0x82010000, 0x82008000, 0x82008100
GET, UNGET, PUT, DECODE, MOTION, TICK = [0x82300000 + 4 * i for i in range(6)]


class Harness(Oracle):
    def __init__(self, unit):
        super().__init__(PREFIX + unit + '.o')
        self.call(self.retail['MPVVLC_Init'], 0, 0)
        self.call(self.retail['MPVBDEC_Init'], 0x82030000)
        self.events = []
        self.queue = (DATA, 8192)
        self.decode_count = 0
        self.mode = 'macroblock'
        self.hooks = {GET, UNGET, PUT, DECODE, MOTION, TICK,
                      self.retail['MPV_GoNextDelimSj'],
                      self.retail['DCT_FsriTrans6Blk']}
        self.uc.hook_add(UC_HOOK_CODE, self.hook)

    def word(self, addr):
        return struct.unpack('>I', self.uc.mem_read(addr, 4))[0]

    def hook(self, uc, address, size, user):
        if address not in self.hooks:
            return
        args = [uc.reg_read(UC_PPC_REG_3 + i) for i in range(4)]
        result = 0
        if address == GET:
            uc.mem_write(args[3], pack(*self.queue))
            self.events.append(('get', args[1], *self.queue))
        elif address in (UNGET, PUT):
            chunk = tuple(struct.unpack('>2I', uc.mem_read(args[2], 8)))
            self.events.append(('unget' if address == UNGET else 'put', args[1], *chunk))
            if address == UNGET:
                self.queue = chunk
        elif address == DECODE:
            self.decode_count += 1
            if self.mode == 'blocks':
                block = args[1]
                coeff = self.word(block + 28)
                self.events.append(('block', args[0], block,
                                    bytes(uc.mem_read(block, 48)).hex(),
                                    bytes(uc.mem_read(coeff, 256)).hex()))
                uc.mem_write(coeff, pack(0x3F800000 + self.decode_count * 0x800000))
                predictor = self.word(block + 40)
                uc.mem_write(predictor, pack(self.word(predictor) + self.decode_count))
                result = self.decode_count * 17
            else:
                self.events.append(('decode', bytes(uc.mem_read(CTX, 16)).hex(),
                                    bytes(uc.mem_read(CTX + 0x2E8, 0x70)).hex()))
        elif address == self.retail['DCT_FsriTrans6Blk']:
            self.events.append(('dct', args[0], bytes(uc.mem_read(args[0], 6)).hex()))
        elif address == self.retail['MPV_GoNextDelimSj']:
            self.events.append(('delimiter', args[0]))
        else:
            self.events.append(('motion' if address == MOTION else 'tick', args[0]))
        # Enforce caller-saved GPR rules instead of accidentally preserving them.
        for reg in (0, *range(4, 13)):
            uc.reg_write(UC_PPC_REG_0 + reg, 0xCA110000 + reg)
        uc.reg_write(UC_PPC_REG_3, result)
        uc.reg_write(UC_PPC_REG_PC, uc.reg_read(UC_PPC_REG_LR))

    def reset(self, payload='', alignment=0, extra=0, chunk_length=8192):
        uc = self.uc
        uc.mem_write(CTX - 32, b'\xA5' * (0x1400 + 64))
        uc.mem_write(CTX, b'\0' * 0x1400)
        bits = '0' * (alignment * 8 + extra) + payload
        bits = bits.ljust(65536, '0')
        uc.mem_write(DATA, int(bits, 2).to_bytes(len(bits) // 8, 'big'))
        uc.mem_write(SJ, pack(VT))
        uc.mem_write(VT, pack(0, 0, 0, 0, 0, 0, GET, UNGET, PUT, 0, 0, 0))
        self.queue = (DATA + alignment, chunk_length)
        self.events = []
        self.decode_count = 0
        uc.mem_write(CTX + 0x1310, pack(extra))
        uc.mem_write(CTX + 0x2C4, pack(MOTION, DECODE, DECODE, MOTION,
                                     MOTION, MOTION, MOTION, MOTION))
        uc.mem_write(CTX + 0x334, pack(0xFFFFFFFF, 0, 0, 128))
        uc.mem_write(CTX + 0x1D8, pack(8, 8))
        uc.mem_write(CTX + 0x1324, pack(1000))

    def execute(self, symbol, local, returns=True):
        value = self.call((self.local if local else self.retail)[symbol], CTX, SJ)
        return {'return': value if returns else None, 'events': self.events.copy(),
                'decode_callbacks': self.decode_count,
                'state': bytes(self.uc.mem_read(CTX - 32, 0x1400 + 64)).hex(),
                'stream': bytes(self.uc.mem_read(DATA, 8192)).hex()}


def compare(results, symbol, label, retail, local):
    entry = results.setdefault(symbol, {'cases': 0, 'failures': []})
    entry['cases'] += 1
    if retail != local:
        fields = [k for k in retail if retail[k] != local[k]]
        entry['failures'].append({'case': label, 'fields': fields})


def table_tests(results, inputs):
    h = Harness('mpv_vlc')
    inputs['mpv_vlc'] = h.inputs
    for symbol, table in [('mpvvlc2_InitDcSizY', 'mpvvlt2_y_dcsiz'),
                          ('mpvvlc2_InitDcSizC', 'mpvvlt2_c_dcsiz')]:
        for poison in (0, 0x55, 0xAA, 0xFF):
            sides = []
            for local in (False, True):
                addr = (h.local if local else h.retail)[table]
                h.uc.mem_write(addr - 16, bytes([poison]) * 1056)
                h.call((h.local if local else h.retail)[symbol])
                sides.append({'table_and_guards': bytes(h.uc.mem_read(addr - 16, 1056)).hex()})
            compare(results, symbol, poison, *sides)
    for offset in range(0, 32, 2):
        for poison in (0, 0x55, 0xAA, 0xFF):
            sides = []
            for local in (False, True):
                addr = 0x82020020 + offset
                h.uc.mem_write(addr - 16, bytes([poison]) * 928)
                ret = h.call((h.local if local else h.retail)['mpvvlc_InitCbpSub2'], addr)
                sides.append({'return': ret, 'table_and_guards': bytes(h.uc.mem_read(addr - 16, 928)).hex()})
            compare(results, 'mpvvlc_InitCbpSub2', [offset, poison], *sides)


def block_tests(results, inputs):
    h = Harness('mpv_cdec')
    inputs['mpv_cdec'] = h.inputs
    h.mode = 'blocks'
    rng = random.Random(0xCDEC)
    for case in range(64):
        initial = rng.randbytes(1536)
        sides = []
        for local in (False, True):
            h.reset()
            h.uc.mem_write(CTX + 0x680, initial)
            h.uc.mem_write(CTX + 0x1318, pack(DECODE, DECODE))
            h.uc.mem_write(CTX + 0x1328, pack(0x82009000, 0x82009400))
            h.uc.mem_write(CTX + 0x2E8, pack(case % 32))
            h.uc.mem_write(CTX + 0x34C, pack(case, -case, case * 2))
            sides.append(h.execute('MPVCDEC_IntraBlocks', local))
        compare(results, 'MPVCDEC_IntraBlocks', case, *sides)


def header_tests(results, inputs):
    h = Harness('mpv_hdec')
    inputs['mpv_hdec'] = h.inputs
    rng = random.Random(0x5FD)
    for case in range(96):
        width, height = rng.randrange(1, 4096), rng.randrange(1, 4096)
        payload = format(0x1B3, '032b') + format(width, '012b') + format(height, '012b')
        payload += format(case % 16, '04b') + format((case + 1) % 16, '04b')
        payload += format(rng.randrange(1 << 18), '018b') + str(case & 1)
        payload += format(rng.randrange(1024), '010b') + str((case >> 1) & 1)
        for present in (case & 1, (case >> 1) & 1):
            payload += str(present)
            if present:
                payload += ''.join(format(rng.randrange(256), '08b') for _ in range(64))
        sides = []
        for local in (False, True):
            h.reset(payload, alignment=case % 4)
            sides.append(h.execute('mpvhdec_DecShcSj', local))
        compare(results, 'mpvhdec_DecShcSj', case, *sides)


def address_codes(h, picture):
    family = 'i' if picture in 'ID' else picture.lower()
    width, split = (12, 8) if family == 'i' else (11, 7)
    lo = h.word(h.retail['mpvvlc_mbai_' + family + '_0'])
    hi = h.word(h.retail['mpvvlc_mbai_' + family + '_1'])
    found = {}
    for peek in range(1 << width):
        addr = lo + peek * 2 if peek >> split == 0 else hi + (peek >> 6) * 2
        desc = struct.unpack('>h', h.uc.mem_read(addr, 2))[0]
        length, inc, flags = desc & 15, (desc >> 4) & 63, (desc & 0xFFFFFFFF) >> 10
        if not 0 < length <= width or not 1 <= inc <= 5:
            continue
        if family != 'i' and flags & 32 and flags & 12:
            continue  # Motion-code paths are outside these controlled fixtures.
        code = format(peek, '0' + str(width) + 'b')[:length]
        if family != 'i' and not flags & 32:
            table = h.word(h.retail['mpvvlc_' + family + '_mbtype'])
            type_width = 6 if family == 'b' else 5
            for index in range(1 << type_width):
                entry = struct.unpack('>h', h.uc.mem_read(table + index * 2, 2))[0]
                n, type_flags = entry & 255, (entry & 0xFFFFFFFF) >> 8
                if 0 < n <= type_width and type_flags & 1 and not type_flags & 14:
                    code += format(index, '0' + str(type_width) + 'b')[:n]
                    flags = type_flags
                    break
            else:
                continue
        if family != 'i' and flags & 14:
            continue
        key = (inc, bool(flags & 16))
        found.setdefault(key, (code, flags))
    return list(found.values())


def macroblock_tests(results, inputs):
    h = Harness('mpv_dec')
    inputs['mpv_dec'] = h.inputs
    for picture in 'BPID':
        symbol = 'MPVDEC_Dec' + picture + 'picMb'
        codes = address_codes(h, picture)
        if not codes:
            raise RuntimeError('No valid address fixtures for ' + picture)
        for alignment in range(4):
            for extra in range(8):
                for index, (code, flags) in enumerate(codes):
                    for refill in (False, True):
                        block = code + (format((index * 7) % 32, '05b') if flags & 16 else '')
                        if picture == 'D':
                            block += '1'
                        payload = block * 3 + '0' * 64
                        sides = []
                        for local in (False, True):
                            h.reset(payload, alignment, extra, 64 if refill else 8192)
                            sides.append(h.execute(symbol, local, returns=False))
                        compare(results, symbol, [alignment, extra, index, refill], *sides)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    results, inputs = {}, {}
    for test in (table_tests, block_tests, header_tests, macroblock_tests):
        test(results, inputs)
        print(test.__name__, {k: (v['cases'], len(v['failures'])) for k, v in results.items()}, flush=True)
    report = {'results': results, 'inputs': inputs,
              'scope': 'Retail PPC versus candidate PPC; controlled transport, block callbacks, and final DCT.'}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    return int(any(v['failures'] for v in results.values()))


if __name__ == '__main__':
    raise SystemExit(main())
