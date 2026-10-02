from pathlib import Path
p=Path(__file__).resolve().parent
s=(p/'import_rigged.py').read_text();s=s[:s.index("print('BLEND_FACTORIES'")]
exec(s)
exec((p/'import_motion.py').read_text())
