"""Read-only disassembly of the recognized retail executable for wrapper work."""
import argparse
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools' / 'python'))
import capstone
import pefile

parser = argparse.ArgumentParser()
parser.add_argument('--executable', type=pathlib.Path, default=ROOT / 'game' / 'fforce.exe')
parser.add_argument('mode', choices=['refs', 'range'])
parser.add_argument('values', nargs='+', type=lambda value: int(value, 0))
args = parser.parse_args()
pe = pefile.PE(str(args.executable))
base = pe.OPTIONAL_HEADER.ImageBase
decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
decoder.skipdata = True
for section in pe.sections:
    if not section.Characteristics & 0x20000000:
        continue
    code = list(decoder.disasm(section.get_data(), base + section.VirtualAddress))
    selected = set()
    for index, ins in enumerate(code):
        if args.mode == 'range':
            match = args.values[0] <= ins.address < args.values[1]
        else:
            match = any(hex(value) in ins.op_str for value in args.values)
        if match:
            selected.update(range(max(0, index - (8 if args.mode == 'refs' else 0)),
                                  min(len(code), index + (9 if args.mode == 'refs' else 1))))
    previous = -2
    for index in sorted(selected):
        if index != previous + 1:
            print('...')
        ins = code[index]
        print(f'{ins.address:08x}: {ins.mnemonic:8} {ins.op_str}')
        previous = index
