#!/usr/bin/env python3
"""Derive PicoNIX RP2040 XIP linker layout from legacy Pico SDK default script.

Only writable kernel allocations use the first 56 KiB of main SRAM.
Task slots and the Pico W networking reservation remain outside this region.
"""
import pathlib
import re
import sys


# XIP実行用リンク設定に必要なSDKリンカスクリプトの変更を行う．
def transform(source):
    ram = re.compile(r'(?m)^(?P<pre>\s*RAM\s*\(\s*rwx\s*\)\s*:\s*ORIGIN\s*=\s*0x20000000\s*,\s*LENGTH\s*=\s*)(?:256\s*[kK]|0x40000|262144)(?P<post>\s*)$')
    if len(list(ram.finditer(source))) != 1:
        raise ValueError('expected one 256-KiB RAM declaration in SDK XIP script')
    if not re.search(r'(?m)^\s*(?:INCLUDE\s+["\']?pico_flash_region\.ld|FLASH\s*\()', source):
        raise ValueError('SDK script lacks FLASH region')
    source = ram.sub(lambda m: m['pre'] + '56k' + m['post'], source, count=1)
    # SDK側のXIPレイアウトが変わったら，誤った置換をせず失敗させる．
    text = re.search(r'(?s)\n\s*\.text\s*:\s*\{.*?\}\s*>\s*FLASH', source)
    rodata = re.search(r'(?s)\n\s*\.rodata\s*:\s*\{.*?\}\s*>\s*FLASH', source)
    if not text or not rodata:
        raise ValueError('SDK default linker script must place .text and .rodata in FLASH')
    if re.search(r'(?s)\n\s*\.data\s*:\s*\{.*?\}\s*>\s*RAM', source) is None:
        raise ValueError('SDK default linker script must place .data in RAM')
    if '(.piconix_app_image' in source:
        raise ValueError('application image section already present')
    # 埋込みイメージはFlash上のコード／読み取り専用データの後に置く．
    # RAM初期値より前に配置し，boot2とベクタのオフセットは維持する．
    match = re.search(r'(?m)^[ \t]*\.data\s*:', source)
    if not match:
        raise ValueError('missing .data section')
    app = '''    /* PicoNIX application binaries and symbol tables stay in XIP FLASH. */
    .piconix_app_images :
    {
        . = ALIGN(4);
        __piconix_app_images_start = .;
        KEEP(*(.piconix_app_image*))
        KEEP(*(.rodata.piconix_symbols))
        KEEP(*(.rodata.piconix_kernel_symbols))
        . = ALIGN(4);
        __piconix_app_images_end = .;
    } > FLASH

'''
    app += '''    /* Packed manual in a dedicated Flash section; no SRAM copy. */
    .manual :
    {
        . = ALIGN(4);
        __manual_start = .;
        KEEP(*(.manual))
        . = ALIGN(4);
        __manual_end = .;
    } > FLASH

'''
    # このセクションは.dataのFlashロードイメージより前に置く必要がある．
    return '/* PicoNIX XIP: flash code/const, 56-KiB kernel RAM. */\n' + source[:match.start()] + app + source[match.start():]

# 対象リンカスクリプトを変換し，Flash実行のメモリ配置を整える．
def main():
    if len(sys.argv) != 3:
        sys.exit('usage: limit_sdk_xip.py sdk_memmap_default.ld output.ld')
    try:
        src = pathlib.Path(sys.argv[1]).read_text()
        pathlib.Path(sys.argv[2]).write_text(transform(src))
    except (OSError, ValueError) as exc:
        sys.exit(f'ERROR: {exc}')

if __name__ == '__main__':
    main()
