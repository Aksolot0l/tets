"""Add one loader import to the exact legacy GUI, without rewriting its code.

The original sections/import thunks are left intact. A new writable PE section
contains a copied import-descriptor array plus a normal named import of our
companion DLL. This is specific to the checked-in x64 client, not a generic PE
editor. No credentials or process data are embedded in the result.
"""
from pathlib import Path
import hashlib
import struct

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'client.exe'
DESTINATION = ROOT / 'repair/client.exe'
EXPECTED = 'e0f90e9aa78e417f0337acbb8f84162e27b9fb9396cf2c22af5950a16db11b19'
DLL_NAME = b'GAYClientLauncher.dll\0'
IMPORT_NAME = b'GAYClientAutoloadImport\0'


def align(value, alignment):
    return (value + alignment - 1) // alignment * alignment


def patch(original):
    if hashlib.sha256(original).hexdigest() != EXPECTED:
        raise ValueError('unsupported client executable')
    image = bytearray(original)
    pe, = struct.unpack_from('<I', image, 0x3c)
    if image[pe:pe+4] != b'PE\0\0':
        raise ValueError('not a PE file')
    machine, sections = struct.unpack_from('<HH', image, pe + 4)
    optsize, = struct.unpack_from('<H', image, pe + 20)
    opt = pe + 24
    magic, = struct.unpack_from('<H', image, opt)
    if machine != 0x8664 or magic != 0x20b or optsize < 0xf0:
        raise ValueError('expected PE32+ x64')
    section_alignment, file_alignment = struct.unpack_from('<II', image, opt + 32)
    headers_size, = struct.unpack_from('<I', image, opt + 60)
    section_table = opt + optsize
    new_header = section_table + sections * 40
    if new_header + 40 > headers_size or any(image[new_header:new_header+40]):
        raise ValueError('no clean section header space')
    section_rows = []
    for i in range(sections):
        off = section_table + 40 * i
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from('<IIII', image, off + 8)
        section_rows.append((rva, virtual_size, raw_offset, raw_size))
    def file_offset(rva, size):
        for base, virtual_size, raw, raw_size in section_rows:
            if base <= rva and rva + size <= base + min(virtual_size, raw_size):
                return raw + rva - base
        raise ValueError('RVA outside a file-backed section')
    imp_rva, imp_size = struct.unpack_from('<II', image, opt + 112 + 8)
    descriptors = bytearray()
    for i in range(128):
        offset = file_offset(imp_rva + i * 20, 20)
        descriptor = image[offset:offset+20]
        if descriptor == bytes(20):
            break
        descriptors += descriptor
    else:
        raise ValueError('unterminated import descriptors')
    if not descriptors or len(descriptors) + 20 > imp_size:
        raise ValueError('invalid original import directory')
    section_rva = align(max(rva + max(vsize, rawsize) for rva, vsize, _, rawsize in section_rows), section_alignment)
    raw_offset = align(len(image), file_alignment)
    if raw_offset != len(image):
        image.extend(bytes(raw_offset - len(image)))

    # descriptor array, null descriptor, ILT, IAT, IMAGE_IMPORT_BY_NAME, DLL name
    prefix_size = len(descriptors) + 40
    ilt_off = align(prefix_size, 8)
    iat_off = ilt_off + 16
    name_off = iat_off + 16
    dll_off = name_off + 2 + len(IMPORT_NAME)
    content = bytearray(dll_off + len(DLL_NAME))
    content[:len(descriptors)] = descriptors
    struct.pack_into('<IIIII', content, len(descriptors), section_rva + ilt_off, 0, 0,
                     section_rva + dll_off, section_rva + iat_off)
    struct.pack_into('<Q', content, ilt_off, section_rva + name_off)
    struct.pack_into('<Q', content, iat_off, section_rva + name_off)
    content[name_off + 2:dll_off] = IMPORT_NAME
    content[dll_off:] = DLL_NAME
    raw_size = align(len(content), file_alignment)
    content.extend(bytes(raw_size - len(content)))
    image.extend(content)
    # IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE
    struct.pack_into('<8sIIIIIIHHI', image, new_header, b'.agimp\0\0', dll_off + len(DLL_NAME),
                     section_rva, raw_size, raw_offset, 0, 0, 0, 0, 0xc0000040)
    struct.pack_into('<H', image, pe + 6, sections + 1)
    initialized, = struct.unpack_from('<I', image, opt + 8)
    struct.pack_into('<I', image, opt + 8, initialized + raw_size)
    struct.pack_into('<I', image, opt + 56, align(section_rva + dll_off + len(DLL_NAME), section_alignment))
    struct.pack_into('<I', image, opt + 64, 0)  # No stale PE checksum.
    struct.pack_into('<II', image, opt + 112 + 8, section_rva, prefix_size)
    return bytes(image)


if __name__ == '__main__':
    result = patch(SOURCE.read_bytes())
    DESTINATION.write_bytes(result)
    print(f'{DESTINATION}: {len(result)} bytes, SHA-256 {hashlib.sha256(result).hexdigest()}')
