#!/usr/bin/env python3
"""Host-side checks for Stage 13's compact DHCP reply layout."""
from pathlib import Path
import re

source = (Path(__file__).resolve().parents[1] / 'kernel' / 'net.c').read_text()
match = re.search(r'dhcp_reply_options\[\]\s*=\s*\{([^}]+)\}', source)
assert match, 'missing DHCP reply options'
options = [int(x.strip()) for x in match.group(1).split(',') if x.strip()]
assert options[-1] == 255
assert options[:3] == [53, 1, 0]
assert 240 + len(options) < 300
assert 'unsigned char reply[DHCP_REPLY_LEN]' in source
assert 'if (message_type) goto done;' in source
print(f'PASS: DHCP reply {240 + len(options)} bytes, duplicate type rejected')
