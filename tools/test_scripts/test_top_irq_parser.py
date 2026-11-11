#!/usr/bin/env python3
"""Compile and verify top's real cumulative IRQ parser on the host."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = root / 'userland/apps/top.c'
with tempfile.TemporaryDirectory() as tmp:
    f = Path(tmp) / 'test.c'
    f.write_text(f'''#define main piconix_top_main
#include "{source}"
#undef main
int main(void){{
    irq_sample_t a={{0}},b={{0}};
    if(!parse_irqs("SysTick 12345\\nPendSV 256\\nSVC 111\\nuart1 9\\n",&a))return 1;
    if(a.systick!=12345 || a.pendsv!=256 || a.svc!=111 || a.uart1!=9)return 2;
    if(parse_irqs("SysTick 12345\\nPendSV 256\\n",&b))return 3;
    if(!parse_irqs("SysTick 14845\\nPendSV 276\\nSVC 211\\nuart1 19\\n",&b))return 4;
    if(b.systick!=14845 || b.pendsv!=276 || b.svc!=211 || b.uart1!=19)return 5;
    return 0;
}}
''')
    binary = Path(tmp) / 'test'
    subprocess.run(['gcc', '-std=c11', '-O2', '-ffunction-sections', '-fdata-sections',
                    '-Wl,--gc-sections', '-I', str(root/'include'),
                    '-I', str(root/'userland/lib'), str(f), '-o', str(binary)], check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: top cumulative interrupt parser')
