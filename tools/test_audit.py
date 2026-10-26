#!/usr/bin/env python3
"""Host-only unit checks for the two supported embedded image placements."""
from audit_firmware import audit


def check(image, section, expected_ok):
    symbols = {'task_ram': 0x20010000, 'tasks': 0x20001000,
               'idle_stack': 0x20002000, '__vectors': 0x10000100,
               '__VECTOR_TABLE': 0x10000100, 'ram_vector_table': 0x20000000,
               'piconix_shell_image_start': image,
               'piconix_shell_image_end': image + 144,
               'piconix_blink_image_start': image,
               'piconix_blink_image_end': image + 144}
    sections = {'.boot2': (256, 0x10000000, 0x10000000),
                '.flashtext': (0x190, 0x10000100, 0x10000100),
                '.ram_vector_table': (192, 0x20000000, 0x20000000),
                section[0]: section[1]}
    problems = audit(symbols, sections)
    assert (not problems) == expected_ok, problems


check(0x10001000, ('.rodata', (1024, 0x10001000, 0x10001000)), True)
check(0x20003000, ('.rodata', (1024, 0x20003000, 0x10003000)), True)
check(0x20003000, ('.rodata', (1024, 0x20003000, 0x20003000)), False)
check(0x20020000, ('.rodata', (1024, 0x20020000, 0x10003000)), False)
print('PASS: XIP, RAM copy, missing LMA, and slot overlap tests')

# Shell image absent must fail (Blink alone is not a bootable shell system).
base = {'task_ram': 0x20010000, 'tasks': 0x20001000, 'idle_stack': 0x20002000, '__vectors': 0x10000100,
               '__VECTOR_TABLE': 0x10000100, 'ram_vector_table': 0x20000000,
        'piconix_blink_image_start': 0x10001000, 'piconix_blink_image_end': 0x10001090}
sections = {'.boot2': (256, 0x10000000, 0x10000000),
            '.flashtext': (0x190, 0x10000100, 0x10000100),
            '.ram_vector_table': (192, 0x20000000, 0x20000000)}
assert any('shell' in issue for issue in audit(base, sections))
# Kernel RAM section must never cross into Shell slot.
base.update(piconix_shell_image_start=0x10002000, piconix_shell_image_end=0x10002090)
sections['.bss'] = (0x200, 0x2000ff00, 0x10003000)
assert any('kernel 64-KiB' in issue for issue in audit(base, sections))
print('PASS: shell presence and kernel boundary negative tests')

# Vector table regression: reject missing flash backing, bad address, and short RAM table.
valid = {'task_ram': 0x20010000, 'tasks': 0x20001000, 'idle_stack': 0x20002000,
         '__vectors': 0x10000100, '__VECTOR_TABLE': 0x10000100,
         'ram_vector_table': 0x20000000,
         'piconix_shell_image_start': 0x10001000, 'piconix_shell_image_end': 0x10001090,
         'piconix_blink_image_start': 0x10002000, 'piconix_blink_image_end': 0x10002090}
layout = {'.boot2': (256, 0x10000000, 0x10000000),
          '.flashtext': (0x190, 0x10000100, 0x10000100),
          '.ram_vector_table': (192, 0x20000000, 0x20000000)}
assert not audit(valid, layout), audit(valid, layout)
assert any('flash vector' in x for x in audit({**valid, '__vectors': 0x10000200}, layout))
assert any('192 flash-backed' in x for x in audit(valid, {**layout, '.flashtext': (0x80, 0x10000100, 0x10000100)}))
assert any('RAM vector' in x for x in audit(valid, {**layout, '.ram_vector_table': (128, 0x20000000, 0x20000000)}))
print('PASS: SDK 2.2.0 flash/RAM vector layout and negative tests')
