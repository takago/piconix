# cut — delimiter-separated field extraction

`cut` extracts selected fields from each input line, reading a file or standard input. It does not parse quoted CSV or regular expressions.

```text
cut [-d CHAR] -f LIST [FILE]
```

- `-d CHAR`: single-byte delimiter (default: TAB).
- `-f LIST`: required, 1-based field numbers or inclusive ranges, e.g. `1,3-5`; fields 1 through 64.
- `[FILE]`: optional input filename; omitted means standard input.

```sh
cut -d, -f1,4 /tmp/data.csv
cat /tmp/data.csv | cut -d, -f2-3
```

Selected fields retain input order and are joined with the original delimiter. Missing fields are skipped; a line without the delimiter passes through unchanged. Lines longer than 512 bytes fail with an error. Quoted CSV and multibyte delimiters are unsupported. Partial writes are retried.

`cut` works on delimiter-separated text. `/proc/cpu` is a fixed-width table, so CSV field extraction does not apply to it.
