"""Compare Sofdec block decoding against executed retail PowerPC instructions.

Requires the initialized GQNE5D tree, a compiled mpvabdec.o, and unicorn==2.1.4.
Runs 10,296 non-intra blocks plus intra AC-sign and luma/chroma DC cases.
DC tests cover recovered sizes, boundary amplitudes, predictors and alignments.
No decoder implementation is translated into Python: the reference is main.dol.
"""
import argparse
import hashlib
import json
import os
import pathlib
import random
import struct

from unicorn import Uc, UC_ARCH_PPC, UC_MODE_PPC32, UC_MODE_BIG_ENDIAN
from unicorn.ppc_const import (
    UC_CPU_PPC32_750CXE_V3_1, UC_PPC_REG_0, UC_PPC_REG_1, UC_PPC_REG_3,
    UC_PPC_REG_MSR, UC_PPC_REG_LR, UC_PPC_REG_PC,
)

class Elf:

    def __init__(self, path):
        self.data = pathlib.Path(path).read_bytes()
        if self.data[:6] != b'\x7fELF\x01\x02':
            raise ValueError('Expected a 32-bit big-endian PowerPC ELF')
        self.end = '>' if self.data[5] == 2 else '<'
        h = struct.unpack_from(self.end + 'HHIIIIIHHHHHH', self.data, 16)
        if h[1] != 20:
            raise ValueError('Expected EM_PPC')
        off = h[5]
        size = h[10]
        n = h[11]
        self.sections = [struct.unpack_from(self.end + 'IIIIIIIIII', self.data, off + i * size) for i in range(n)]
        self.symbols = []
        for s in self.sections:
            if s[1] != 2:
                continue
            strings = self.section(s[6])
            for o in range(s[4], s[4] + s[5], s[9]):
                name, value, size, info, other, idx = struct.unpack_from(self.end + 'IIIBBH', self.data, o)
                self.symbols.append(dict(name=self.cstr(strings, name), value=value, size=size, info=info, section=idx))

    def cstr(self, b, o):
        return b[o:b.index(0, o)].decode('latin1')

    def section(self, i):
        s = self.sections[i]
        return self.data[s[4]:s[4] + s[5]]

OBJ = 'build/GQNE5D/src/libmwsfdg.a/crimw/dev/sofdec/src/sfdcore/mpv/mpvabdec.o'
CTX = 0x82001000
BLOCK = 0x82002000
COEFF = 0x82003000
SCAN = 0x82004000
QUANT = 0x82005000
SCALE = 0x82006000
STREAM = 0x82010000
STOP = 0x823ff000
def pack(*values):
    return struct.pack('>' + str(len(values)) + 'I', *(x & 0xffffffff for x in values))

class Oracle:

    def __init__(self, obj=OBJ):
        self.uc = Uc(UC_ARCH_PPC, UC_MODE_PPC32 | UC_MODE_BIG_ENDIAN)
        u = self.uc
        u.ctl_set_cpu_model(UC_CPU_PPC32_750CXE_V3_1)
        u.mem_map(0x80000000, 0x2000000)
        u.mem_map(0x82000000, 0x400000)
        d = pathlib.Path('orig/GQNE5D/sys/main.dol').read_bytes()
        hdr = struct.unpack_from('>54I', d)
        for off, addr, size in zip(hdr[:18], hdr[18:36], hdr[36:]):
            if size:
                u.mem_write(addr, d[off:off + size])
        e = Elf('orig/GQNE5D/files/mk6gc_release.elf')
        self.inputs = {
            'retail_dol_sha1': hashlib.sha1(d).hexdigest(),
            'retail_elf_sha256': hashlib.sha256(e.data).hexdigest(),
        }
        self.retail = {s['name']: s['value'] for s in e.symbols}
        self.call(self.retail['mpvvlc_InitIntRunLevel'])
        self.call(self.retail['mpvvlc_InitDcSizY'])
        self.call(self.retail['mpvvlc2_InitDcSizY'])
        self.call(self.retail['mpvvlc_InitDcSizC'])
        self.call(self.retail['mpvvlc2_InitDcSizC'])
        self.tables = [self.retail['mpvvlt_run_level_' + s] for s in ['8', '4', '2', '1', '0a', '0b', '0c']]
        # mpvlib_InitHn biases these tables before decoder lookup.
        self.tables[1] -= 16
        self.tables[2] -= 32
        self.tables[3] -= 32
        self.local = self.load_object(obj)

    def call(self, addr, *args):
        u = self.uc
        for i in range(32):
            u.reg_write(UC_PPC_REG_0 + i, 0)
        u.reg_write(UC_PPC_REG_1, 0x823f0000)
        u.reg_write(UC_PPC_REG_MSR, 8192)
        u.reg_write(UC_PPC_REG_LR, STOP)
        for i, v in enumerate(args):
            u.reg_write(UC_PPC_REG_3 + i, v)
        u.emu_start(addr, STOP, count=100000)
        if u.reg_read(UC_PPC_REG_PC) != STOP:
            raise RuntimeError('instruction limit at ' + hex(u.reg_read(UC_PPC_REG_PC)))
        return u.reg_read(UC_PPC_REG_3)

    def load_object(self, path):
        e = Elf(path)
        self.inputs['candidate_object'] = str(pathlib.Path(path).resolve())
        self.inputs['candidate_sha256'] = hashlib.sha256(e.data).hexdigest()
        base = {}
        u = self.uc
        for i, s in enumerate(e.sections):
            if s[2] & 2 and s[5]:
                if s[5] > 0x10000 or i >= 128:
                    raise ValueError("Object section exceeds isolated mapping")
                base[i] = 0x81800000 + i * 0x10000
                if s[1] != 8:
                    u.mem_write(base[i], e.section(i))

        def symval(s):
            if s['section'] == 0xfff1:
                return s['value']
            if s['section'] == 0:
                return self.retail[s['name']]
            return base[s['section']] + s['value']
        for i, s in enumerate(e.sections):
            if s[1] != 4 or s[7] not in base:
                continue
            data = e.section(i)
            for o in range(0, len(data), 12):
                off, info, add = struct.unpack_from('>IIi', data, o)
                kind = info & 255
                val = symval(e.symbols[info >> 8]) + add & 0xffffffff
                dst = base[s[7]] + off
                # PPC ADDR32, ADDR16_LO/HI/HA, REL24, and REL32.
                if kind == 1:
                    u.mem_write(dst, pack(val))
                elif kind in [4, 5, 6]:
                    part = val if kind == 4 else val >> 16 if kind == 5 else val + 0x8000 >> 16
                    u.mem_write(dst, struct.pack('>H', part & 0xffff))
                elif kind == 10:
                    inst = struct.unpack('>I', u.mem_read(dst, 4))[0]
                    u.mem_write(dst, pack(inst & 0xfc000003 | val - dst & 0x3fffffc))
                elif kind == 26:
                    u.mem_write(dst, pack(val - dst))
                elif kind != 0:
                    raise RuntimeError('unsupported relocation ' + str(kind))
        return {s['name']: symval(s) for s in e.symbols if s['section'] in base}

    def run(self, bits, alignment=0, qscale=8, scan=None, quant=None, scales=None, local=False, function='MPVABDEC_NintraBlock', dc_initial=0, dc_chroma=False, decode_mode=0):
        u = self.uc
        bits = '0' * alignment + bits + '10' * 512
        bits = bits[:8192].ljust(8192, '0')
        words = [int(bits[i:i + 32], 2) for i in range(0, len(bits), 32)]
        u.mem_write(STREAM, pack(*words[2:]))
        u.mem_write(CTX, b'\x00' * 512)
        u.mem_write(BLOCK, b'\x00' * 64)
        u.mem_write(COEFF, b'\x7f' * 272)
        u.mem_write(SCAN, bytes(scan or list(range(64))) + b'\x00' * 192)
        u.mem_write(QUANT, bytes(quant or [16] * 64))
        u.mem_write(SCALE, struct.pack('>64f', *(scales or [1.0] * 64)))
        u.mem_write(CTX, pack(words[0] << alignment & 0xffffffff, words[1], alignment, STREAM, *self.tables, SCAN, self.retail['mpvbdec_bitmsk'], SCALE))
        u.mem_write(0x82007000, pack(dc_initial))
        u.mem_write(CTX + 0x1e8, pack(decode_mode))
        dc_table = self.retail[('mpvvlt2_' if function.endswith('Dc11') else 'mpvvlt_') + ('c' if dc_chroma else 'y') + '_dcsiz']
        u.mem_write(BLOCK + 28, pack(COEFF, QUANT, qscale, 0x82007000, dc_table))
        addr = (self.local if local else self.retail)[function]
        ret = self.call(addr, CTX, BLOCK)
        return {'return': ret, 'dc_predictor': bytes(u.mem_read(0x82007000, 4)).hex(), 'context': bytes(u.mem_read(CTX, 16)).hex(), 'block': bytes(u.mem_read(BLOCK, 24)).hex(), 'coefficients': bytes(u.mem_read(COEFF, 256)).hex(), 'guard': bytes(u.mem_read(COEFF + 256, 16)).hex()}

def read_ac_codes(o):
    codes = {(0, 1): '11'}
    long = struct.unpack('>128I', o.uc.mem_read(o.tables[0], 512))
    for i, entry in enumerate(long):
        run = entry & 255
        level = entry >> 8 & 255
        length = entry >> 16
        if length and run != 64:
            codes[run, level] = f'{i:08b}'[:length - 1]
    for ti, length, count, bias, base in [(1, 11, 8, 8, 0), (2, 13, 16, 16, 0), (3, 14, 16, 16, 0), (4, 15, 16, 0, 32), (5, 16, 16, 0, 32), (6, 17, 16, 0, 32)]:
        vals = struct.unpack('>' + str(count) + 'H', o.uc.mem_read(o.tables[ti] + bias * 2, count * 2))
        for i, entry in enumerate(vals):
            codes[entry & 255, entry >> 8] = f'{(i + bias) * 2 + base:0{length}b}'[:-1]

    return codes

def make_cases(o):
    rng = random.Random(0x4d5056)
    codes = read_ac_codes(o)

    def encode(run, level, first=False, escape=False):
        negative = level < 0
        mag = abs(level)
        if first and run == 0 and (mag == 1) and (not escape):
            return '1' + str(int(negative))
        if (run, mag) in codes and (not escape):
            return codes[run, mag] + str(int(negative))
        if -127 <= level <= 127 and level != 0:
            payload = f'{level & 255:08b}'
        elif level > 0:
            payload = '00000000' + f'{level:08b}'
        else:
            payload = '10000000' + f'{level + 256 & 255:08b}'
        return '000001' + f'{run:06b}' + payload
    cases = []
    for run, level in sorted(codes):
        for sign in [1, -1]:
            for align in range(32):
                cases.append(dict(bits=encode(run, level * sign, True) + '10', alignment=align, qscale=1 + align % 31))
    for level in [-255, -128, -127, -1, 1, 127, 128, 255]:
        for run in [0, 1, 31, 62]:
            for align in [0, 1, 7, 15, 23, 31]:
                cases.append(dict(bits=encode(run, level, True, True) + '10', alignment=align, qscale=31))
    keys = list(codes)
    for i in range(3000):
        bits = ''
        pos = -1
        for j in range(rng.randrange(1, 20)):
            options = [k for k in keys if k[0] + pos + 1 < 64]
            if not options:
                break
            run, level = rng.choice(options)
            level *= rng.choice([-1, 1])
            bits += encode(run, level, j == 0, rng.randrange(10) == 0)
            pos += run + 1
            if pos >= 60:
                break
        scan = list(range(64))
        rng.shuffle(scan)
        cases.append(dict(bits=bits + '10', alignment=rng.randrange(32), qscale=rng.randrange(1, 32), scan=scan, quant=[rng.randrange(1, 256) for _ in range(64)], scales=[rng.choice([0.125, 0.5, 1.0, 1.375, 2.0]) for _ in range(64)]))
    return (cases, len(codes))

def make_dc_cases(oracle, function):
    """Exercise recovered luma/chroma DC sizes, signs, predictors and alignments."""
    cases = []
    width = 10 if function.endswith('Dc11') else 7
    for chroma in (False, True):
        name = ('mpvvlt2_' if width == 10 else 'mpvvlt_') + ('c' if chroma else 'y') + '_dcsiz'
        table = oracle.uc.mem_read(oracle.retail[name], 1 << width)
        prefixes = {}
        for index, entry in enumerate(table):
            size, length = entry >> 4, entry & 15
            if length:
                prefixes[size] = f'{index:0{width}b}'[:length]
        for size, prefix in sorted(prefixes.items()):
            values = [0] if size == 0 else sorted({1 << (size - 1), (1 << size) - 1,
                                                  -(1 << (size - 1)), -((1 << size) - 1)})
            for value in values:
                amplitude = '' if size == 0 else f'{value if value > 0 else (1 << size) - 1 + value:0{size}b}'
                for alignment in range(32):
                    for predictor in (-1024, 1024):
                        cases.append(dict(bits=prefix + amplitude + '11011110', alignment=alignment,
                                          function=function, dc_initial=predictor, dc_chroma=chroma))
        if function == 'MPVABDEC_IntraBlock':
            for alignment in range(32):
                cases.append(dict(bits=prefixes[0] + '10', alignment=alignment, function=function,
                                  dc_chroma=chroma, decode_mode=4))
    return cases


def make_ac_cases(oracle, function):
    """Check every recovered AC entry after an intra DC coefficient."""
    cases = []
    for (run, level), code in sorted(read_ac_codes(oracle).items()):
        if run >= 63:
            continue
        for sign in (0, 1):
            for alignment in range(32):
                cases.append(dict(bits='100' + code + str(sign) + '10', alignment=alignment,
                                  function=function, qscale=1 + alignment % 31,
                                  dc_initial=1024, quant=[1 + (i * 17 + alignment) % 255 for i in range(64)],
                                  scales=[0.125, 0.5, 1.0, 2.0] * 16))
    return cases


def compare_cases(oracle, cases):
    failures = []
    errors = []
    for index, case in enumerate(cases):
        try:
            retail = oracle.run(**case)
            local = oracle.run(**case, local=True)
        except Exception as error:
            errors.append(dict(index=index, case=case, error=str(error)))
            continue
        if retail != local:
            failures.append(dict(index=index, case=case, fields=[key for key in retail if retail[key] != local[key]], retail=retail, local=local))
    return dict(cases=len(cases), failures=len(failures), errors=len(errors), examples=failures[:20], error_examples=errors[:5])

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=pathlib.Path, default=pathlib.Path(OBJ), help='compiled MWCC decoder object, relative to the current directory')
    parser.add_argument('--output', type=pathlib.Path, help='write detailed JSON evidence')
    parser.add_argument('--input-manifest', type=pathlib.Path,
                        help='write hashes of the exact inputs loaded by the oracle')
    args = parser.parse_args()
    obj = args.object.resolve()
    output = args.output.resolve() if args.output else None
    manifest = args.input_manifest.resolve() if args.input_manifest else None
    os.chdir(pathlib.Path(__file__).resolve().parents[1])
    oracle = Oracle(obj)
    cases, code_count = make_cases(oracle)
    results = {'MPVABDEC_NintraBlock': compare_cases(oracle, cases)}
    results['MPVABDEC_NintraBlock']['codes'] = code_count
    for function in ('MPVABDEC_IntraBlock', 'MPVABDEC_IntraBlockDc11'):
        cases = [dict(bits='100' + code + str(sign) + '10', alignment=alignment, function=function) for code in ('0000001000', '000000011111') for sign in (0, 1) for alignment in range(32)]
        cases.extend(make_dc_cases(oracle, function))
        cases.extend(make_ac_cases(oracle, function))
        results[function] = compare_cases(oracle, cases)
    if output:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(results, indent=2) + '\n')
    if manifest:
        manifest.parent.mkdir(parents=True, exist_ok=True)
        manifest.write_text(json.dumps(oracle.inputs, indent=2) + '\n')
    print(json.dumps({name: {key: value for key, value in result.items() if not isinstance(value, list)} for name, result in results.items()}, indent=2))
    return int(any((result['failures'] or result['errors'] for result in results.values())))
if __name__ == '__main__':
    raise SystemExit(main())
