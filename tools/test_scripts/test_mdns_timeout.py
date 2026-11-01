from pathlib import Path
import re


def test_mdns_timeout_capacity():
    root = Path(__file__).resolve().parents[2]
    opts = (root / "kernel/lwipopts.h").read_text()
    assert "#define LWIP_MDNS_RESPONDER 1" in opts
    assert re.search(r"#define\s+MEMP_NUM_SYS_TIMEOUT\s+\(LWIP_NUM_SYS_TIMEOUT_INTERNAL\s*\+\s*4\)", opts)
    assert "#define LWIP_IGMP 1" in opts


if __name__ == "__main__":
    test_mdns_timeout_capacity()
    print("PASS: extra lwIP mDNS system timeout capacity")
