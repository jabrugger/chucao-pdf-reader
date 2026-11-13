"""Generate synthetic encrypted fixtures, never use this on user documents.

Requires pypdf 6.1.1 and cryptography 46.0.3 only for regeneration.
The bundled PDFs let the C++ tests run without Python or these packages.
"""
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(root / ".tools" / "pdf-test-python"))
from pypdf import PdfReader, PdfWriter

source = root / "test-results" / "sample.pdf"
if not source.exists():
    raise SystemExit("Run scripts/smoke-test.ps1 to create the synthetic source first.")

cases = [
    ("aes256-ascii.pdf", "Chucao-test", "Chucao-owner", "AES-256", None),
    ("aes256-unicode.pdf", "Chucao-café-漢字", "Chucao-owner", "AES-256", None),
    ("aes128.pdf", "Chucao-test", "Chucao-owner", "AES-128", None),
    ("rc4.pdf", "Chucao-test", "Chucao-owner", "RC4-128", None),
    ("restricted.pdf", "reader", "owner", "AES-256", 0),
]
for filename, user, owner, algorithm, permissions in cases:
    writer = PdfWriter(clone_from=PdfReader(source))
    options = {} if permissions is None else {"permissions_flag": permissions}
    writer.encrypt(user_password=user, owner_password=owner, algorithm=algorithm, **options)
    writer.write(Path(__file__).parent / filename)
print("Generated 5 synthetic password fixtures.")
