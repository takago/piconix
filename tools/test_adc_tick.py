#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
k=(r/'kernel/kernel.c').read_text()
v=k
h=k
a=(r/'kernel/adc.c').read_text()
assert 'ticks += 1u;' in k
assert 'tick_sample == 1000u' in k
assert 'slice_ticks == 10u' in k
assert 'clock_get_hz(clk_sys)/1000u' in k
assert '(unsigned long)(t/10u)' not in k
assert 'adc_select_input' in a and 'adc_read()' in a
assert 'adc_gpio_init(26u+ch)' in a
assert 'DEV_ADC_BASE' in h and 'DEV_ADC_END' in h
assert 'adc_set_temp_sensor_enabled(true)' in a
assert '"/dev/adc/temp"' in v
assert 'temp_01c' in a
assert '"/dev/adc/"' in k
assert 'hardware_adc' in (r/'CMakeLists.txt').read_text()
assert 'TICKx1K' in (r/'userland/trace.c').read_text()
print('PASS: ADC device and 1 ms SysTick integration')
