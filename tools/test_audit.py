#!/usr/bin/env python3
"""Host-only unit checks for the two supported embedded image placements."""
from audit_firmware import audit

APPS = ('pins', 'fbctl', 'kitesys', 'shell', 'blink', 'omikuji', 'python', 'hello', 'hexdump', 'trace', 'ps', 'cat', 'watch', 'pwd', 'ls', 'rm', 'echo', 'kill', 'tty')

def all_images(address):
    return {key: value for app in APPS for key, value in ((f'piconix_{app}_image_start', address), (f'piconix_{app}_image_end', address + 144))}



def check(image, section, expected_ok):
    symbols = {'task_ram': 0x20010000, 'tasks': 0x20001000,
               'idle_stack': 0x20002000, '__vectors': 0x10000100,
               '__VECTOR_TABLE': 0x10000100, 'ram_vector_table': 0x20000000,
               **all_images(image)}
    sections = {'.boot2': (256, 0x10000000, 0x10000000),
                '.flashtext': (0x190, 0x10000100, 0x10000100),
                '.ram_vector_table': (192, 0x20000000, 0x20000000),
                '.piconix_app_images': (1024, 0x10001000, 0x10001000)}
    problems = audit(symbols, sections)
    assert (not problems) == expected_ok, problems


check(0x10001000, None, True)
check(0x20003000, None, False)
print('PASS: all embedded application images require XIP flash')

valid = {'task_ram': 0x20010000, 'tasks': 0x20001000, 'idle_stack': 0x20002000,
         '__vectors': 0x10000100, '__VECTOR_TABLE': 0x10000100,
         'ram_vector_table': 0x20000000, **all_images(0x10001000)}
layout = {'.boot2': (256, 0x10000000, 0x10000000),
          '.flashtext': (0x190, 0x10000100, 0x10000100),
          '.ram_vector_table': (192, 0x20000000, 0x20000000),
          '.piconix_app_images': (1024, 0x10001000, 0x10001000)}
assert not audit(valid, layout), audit(valid, layout)
assert any('flash vector' in x for x in audit({**valid, '__vectors': 0x10000200}, layout))
assert any('RAM vector' in x for x in audit(valid, {**layout, '.ram_vector_table': (128, 0x20000000, 0x20000000)}))
assert any('kernel 64-KiB' in x for x in audit(valid, {**layout, '.bss': (0x200, 0x2000ff00, 0x10003000)}))
print('PASS: vector and kernel RAM layout negative tests')
