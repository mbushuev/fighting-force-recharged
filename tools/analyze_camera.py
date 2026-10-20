"""Check the static camera map for the supported retail image; never write it."""
import argparse
import hashlib
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools' / 'python'))
import capstone
import pefile

EXPECTED_SHA256 = '92ab6179bf6497d471c6ab30d6df6f4f333d5bc3f641a154d6966b47fdba90a7'


def analyze(executable):
    data = executable.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != EXPECTED_SHA256:
        raise ValueError('Unsupported executable hash; camera addresses were not checked')
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True

    def instruction(address):
        return next(decoder.disasm(pe.get_data(address - base, 16), address, count=1))

    def memory_reference(name, address, target, mnemonic='mov'):
        item = instruction(address)
        if item.mnemonic != mnemonic or not any(
            operand.type == capstone.x86.X86_OP_MEM and operand.mem.disp == target
            for operand in item.operands
        ):
            raise ValueError('Camera reference differs: ' + name)
        return {'field': name, 'rva': hex(target - base), 'evidence_rva': hex(address - base)}

    fields = [
        ('position_x', 0x404206, 0xC66220),
        ('position_y', 0x404231, 0xC66224),
        ('position_z', 0x404270, 0xC66228),
        ('yaw', 0x40428D, 0xC7888C),
        ('pitch', 0x4042A4, 0xC7888E),
        ('desired_x', 0x404210, 0xC6D31C),
        ('desired_y', 0x404216, 0xC6D324),
        ('desired_z', 0x40421E, 0xC6D328),
        ('desired_yaw', 0x404226, 0xC6D318),
        ('desired_pitch', 0x40423D, 0xC74CF2),
        ('velocity_x', 0x404200, 0xC744A8),
        ('velocity_y', 0x404248, 0xC651A4),
        ('velocity_z', 0x40425D, 0xC689F8),
        ('render_x', 0x440C18, 0xC677B0),
        ('render_y', 0x440BC7, 0xC677B4),
        ('render_z', 0x440C23, 0xC677B8),
        ('render_yaw', 0x440C3A, 0xC73CE0),
        ('render_pitch', 0x440BD5, 0xC7459C),
        ('input_yaw_snapshot', 0x441182, 0xC5A68C),
        ('input_yaw_consumer', 0x44F197, 0xC5A68C),
        ('geometry_yaw_consumer', 0x409609, 0xC7888C),
    ]
    references = [memory_reference(*entry) for entry in fields]
    references.append(memory_reference('stage_camera_table', 0x44120F, 0xC736E0, 'call'))
    handlers = []
    for stage, address, target in [(0, 0x41B91E, 0x407AA0), (1, 0x41B973, 0x407BF0)]:
        memory_reference('stage_camera_initializer', address, 0xC736E0 + stage * 4)
        item = instruction(address)
        if item.operands[1].type != capstone.x86.X86_OP_IMM or item.operands[1].imm != target:
            raise ValueError('Stage camera initializer differs')
        handlers.append({'stage': stage, 'handler_rva': hex(target - base)})
    return {
        'image_sha256': digest,
        'method': 'Static instruction references; no runtime writes or timing measurements',
        'camera_update_rva': '0x4200',
        'focus_selection_rva': '0x42f0',
        'framing_policy_rva': '0x43d0',
        'recorded_shot_reader_rva': '0x45a0',
        'stage_handlers': handlers,
        'references': references,
        'not_established': ['general camera collision solver', 'arbitrary-orbit geometry coverage',
                            'live camera update cadence', 'authored-shot ownership boundaries'],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=pathlib.Path, default=ROOT / 'game' / 'fforce.exe')
    args = parser.parse_args()
    try:
        result = analyze(args.executable)
    except ValueError as error:
        parser.exit(1, str(error) + '\n')
    except OSError:
        parser.exit(1, 'Cannot read the selected executable\n')
    except StopIteration:
        parser.exit(1, 'Cannot decode a camera reference\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
